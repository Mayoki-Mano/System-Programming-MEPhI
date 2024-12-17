#include <windows.h>
#include <stdio.h>

#define PIPE_NAME "\\\\.\\pipe\\EchoPipe"
#define BUFFER_SIZE 1024

int main() {
    HANDLE hPipe;
    DWORD bytesRead, bytesWritten;
    char buffer[BUFFER_SIZE];

    // Открытие канала для чтения и записи
    hPipe = CreateFile(
        PIPE_NAME,               // имя канала
        GENERIC_READ | GENERIC_WRITE, // доступ на чтение и запись
        0,                        // тип доступа (неконкурентное подключение)
        NULL,                     // атрибуты безопасности
        OPEN_EXISTING,            // открыть существующий канал
        0,                        // дополнительные атрибуты
        NULL                      // шаблон безопасности
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        printf("Failed to connect to the pipe. Error: %d\n", GetLastError());
        return 1;
    }

    printf("Connected to server.\n");

    // Бесконечный цикл для отправки сообщений серверу
    while (1) {
        printf("Enter a message: \n");
        printf(">");
        fgets(buffer, sizeof(buffer), stdin);

        // Убираем символ новой строки в конце строки
        buffer[strcspn(buffer, "\n")] = '\0';

        if (strlen(buffer) == 0) {
            continue;
        }

        // Отправка сообщения серверу
        WriteFile(hPipe, buffer, strlen(buffer), &bytesWritten, NULL);
        if (bytesWritten == 0) {
            printf("Error sending message.\n");
            break;
        }

        // Получение эхо-ответа от сервера
        if (ReadFile(hPipe, buffer, BUFFER_SIZE, &bytesRead, NULL)) {
            buffer[bytesRead] = '\0'; // добавление завершающего нуля
            printf("Received from server: %s\n", buffer);
        } else {
            printf("Error receiving message.\n");
            break;
        }
    }

    // Закрытие канала
    CloseHandle(hPipe);

    return 0;
}
