#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 12345
#define BUFFER_SIZE 1024

// Thread function to handle receiving messages
DWORD WINAPI receiveMessages(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE+1];
    buffer[BUFFER_SIZE] = '\0';
    FILE *file;
    int bytesReceived;

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) {
            printf("Client disconnected.\n");
            break;
        }

        // Check for file transfer initiation
        if (strncmp(buffer, "FILE_START",strlen("FILE_START")) == 0) {
            printf("Receiving a file...\n");
            file = fopen("received_file_from_client", "wb");
            if (!file) {
                printf("Error creating file.\n");
                continue;
            }

            while (1) {
                memset(buffer, 0, BUFFER_SIZE);
                bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
                if (bytesReceived <= 0 || strstr(buffer, "FILE_END") != NULL) {
                    fwrite(buffer, 1, bytesReceived-strlen("FILE_END"), file);
                    printf("File received successfully.\n");
                    break;
                }
                fwrite(buffer, 1, bytesReceived, file);
            }
            fclose(file);
        } else {
            printf("Client: %s\n", buffer);
        }
    }
    return 0;
}

// Thread function to handle sending messages
DWORD WINAPI sendMessages(SOCKET clientSocket) {
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

            // Notify client about file transfer
            send(clientSocket, "FILE_START", strlen("FILE_START"), 0);

            // Send file content
            while (!feof(file)) {
                int bytesRead = fread(buffer, 1, BUFFER_SIZE, file);
                send(clientSocket, buffer, bytesRead, 0);
            }
            fclose(file);

            // Notify client transfer is complete
            send(clientSocket, "FILE_END", strlen("FILE_END"), 0);
            printf("File sent: %s\n", filename);
        } else {
            send(clientSocket, buffer, strlen(buffer), 0);
        }
    }
}

int main() {
    WSADATA wsaData;
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    int addrLen = sizeof(clientAddr);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Winsock initialization failed.\n");
        return 1;
    }

    // Create server socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Configure server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);

    // Bind socket
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Socket binding failed: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Listen for connections
    if (listen(serverSocket, 1) == SOCKET_ERROR) {
        printf("Socket listening failed: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    printf("Waiting for a client to connect...\n");

    // Accept client connection
    clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrLen);
    if (clientSocket == INVALID_SOCKET) {
        printf("Client connection failed: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    printf("Client connected.\n");

    // Create threads for receiving and sending
    HANDLE recvThread = CreateThread(NULL, 0, receiveMessages, (LPVOID)clientSocket, 0, NULL);
    HANDLE sendThread = CreateThread(NULL, 0, sendMessages, (LPVOID)clientSocket, 0, NULL);

    // Wait for threads to finish
    WaitForSingleObject(recvThread, INFINITE);
    WaitForSingleObject(sendThread, INFINITE);

    // Cleanup
    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
