#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 12345
#define BUFFER_SIZE 1024

// Thread function to handle receiving messages from the server
DWORD WINAPI receiveMessages(SOCKET serverSocket) {
    char buffer[BUFFER_SIZE+1];
    buffer[BUFFER_SIZE] = '\0';
    FILE *file;
    int bytesReceived;

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        bytesReceived = recv(serverSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) {
            printf("Connection closed by server.\n");
            break;
        }

        // Check for file transfer initiation
        if (strcmp(buffer, "FILE_START") == 0) {
            printf("Receiving a file from server...\n");
            file = fopen("received_file_from_server", "wb");
            if (!file) {
                printf("Error creating file.\n");
                continue;
            }

            while (1) {
                memset(buffer, 0, BUFFER_SIZE);
                bytesReceived = recv(serverSocket, buffer, BUFFER_SIZE, 0);
                if (bytesReceived <= 0 || strstr(buffer, "FILE_END") != NULL) {
                    fwrite(buffer, 1, bytesReceived-strlen("FILE_END"), file);
                    printf("File received successfully.\n");
                    break;
                }
                fwrite(buffer, 1, bytesReceived, file);
            }
            fclose(file);
        } else {
            printf("Server: %s\n", buffer);
        }
    }
    return 0;
}

// Thread function to handle sending messages to the server
DWORD WINAPI sendMessages(SOCKET serverSocket) {
    char buffer[BUFFER_SIZE];
    FILE *file;
    char filename[256];
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        printf("Enter message (or 'file:<filename>' to send a file): \n");
        printf(">");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        // Check if sending a file
        if (strncmp(buffer, "file:", 5) == 0) {
            memcpy(filename, buffer+5, strlen(buffer+5));
            file = fopen(filename, "rb");
            if (!file) {
                printf("Error opening file: %s\n", filename);
                continue;
            }

            // Notify server about file transfer
            send(serverSocket, "FILE_START", strlen("FILE_START"), 0);

            // Send file content
            while (!feof(file)) {
                int bytesRead = fread(buffer, 1, BUFFER_SIZE, file);
                send(serverSocket, buffer, bytesRead, 0);
            }
            fclose(file);

            // Notify server transfer is complete
            send(serverSocket, "FILE_END", strlen("FILE_END"), 0);
            printf("File sent: %s\n", filename);
        } else {
            send(serverSocket, buffer, strlen(buffer), 0);
        }
    }
    return 0;
}

int main() {
    WSADATA wsaData;
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Winsock initialization failed.\n");
        return 1;
    }

    // Create client socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Configure server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Server IP address (localhost)
    serverAddr.sin_port = htons(SERVER_PORT);

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Connection to server failed: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    printf("Connected to server.\n");

    // Create threads for receiving and sending
    HANDLE recvThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)receiveMessages, (void *)clientSocket, 0, NULL);
    HANDLE sendThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)sendMessages, (void *)clientSocket, 0, NULL);

    // Wait for threads to finish
    WaitForSingleObject(recvThread, INFINITE);
    WaitForSingleObject(sendThread, INFINITE);

    // Cleanup
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
