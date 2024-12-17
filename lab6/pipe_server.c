#include <windows.h>
#include <stdio.h>

#define PIPE_NAME "\\\\.\\pipe\\EchoPipe"
#define BUFFER_SIZE 1024

// Функция обработки сообщений клиента
DWORD WINAPI handleClient(LPVOID lpParam) {
    HANDLE pipe = (HANDLE)lpParam;
    char buffer[BUFFER_SIZE];
    DWORD bytesRead, bytesWritten;

    // Чтение и отправка сообщений от клиента
    while (ReadFile(pipe, buffer, BUFFER_SIZE, &bytesRead, NULL)) {
        if (bytesRead == 0) {
            break; // Если клиент отключился
        }

        // Отправка "эхо"-сообщения обратно клиенту
        WriteFile(pipe, buffer, bytesRead, &bytesWritten, NULL);
        printf("Echoed message: %s\n", buffer);
    }

    // Закрытие канала после завершения работы с клиентом
    CloseHandle(pipe);
    return 0;
}

int main() {
    HANDLE hPipe;
    BOOL connected;

    // Создание именованного канала
    hPipe = CreateNamedPipe(
        PIPE_NAME,             // имя канала
        PIPE_ACCESS_DUPLEX,    // доступ на чтение и запись
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, // тип канала
        PIPE_UNLIMITED_INSTANCES, // максимальное количество подключений
        BUFFER_SIZE,           // размер выходного буфера
        BUFFER_SIZE,           // размер входного буфера
        0,                      // таймаут ожидания
        NULL                    // атрибуты безопасности
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        printf("Error creating named pipe: %d\n", GetLastError());
        return 1;
    }

    printf("Server started, waiting for clients...\n");

    // Основной цикл сервера для обработки нескольких клиентов
    while (1) {
        // Ожидание подключения клиента
        connected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (connected) {
            printf("Client connected.\n");

            // Создание нового потока для обработки клиента
            DWORD threadId;
            HANDLE threadHandle = CreateThread(
                NULL,               // Атрибуты потока
                0,                  // Размер стека
                handleClient,       // Функция потока
                (LPVOID)hPipe,      // Параметры функции
                0,                  // Флаги
                &threadId           // Идентификатор потока
            );

            if (threadHandle == NULL) {
                printf("Error creating thread: %d\n", GetLastError());
                continue;
            }

            // Мы не можем закрывать hPipe сразу, потому что он еще нужен в потоке.
            // Поток сам закроет hPipe, когда завершится.
            CloseHandle(threadHandle);
        } else {
            printf("Failed to connect to client: %d\n", GetLastError());
        }

        // Создание нового канала для следующего клиента
        hPipe = CreateNamedPipe(
            PIPE_NAME,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            BUFFER_SIZE,
            BUFFER_SIZE,
            0,
            NULL
        );
    }
}
