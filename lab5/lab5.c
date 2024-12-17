#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#define MAX_ITEMS 10 // Maximum size of the list
#define NUMBER_OF_THREADS 3 // Number of threads for task 3

typedef struct {
    int buffer[MAX_ITEMS];
    int head;
    int tail;
    int count;
} Queue;

void queue_add(Queue *q, int item) {
    q->buffer[q->tail] = item;
    q->tail = (q->tail + 1) % MAX_ITEMS;
    q->count++;
}

int queue_remove(Queue *q) {
    int item = q->buffer[q->head];
    q->head = (q->head + 1) % MAX_ITEMS;
    q->count--;
    return item;
}

volatile BOOL exitFlag = FALSE; // Global flag to signal threads to exit
Queue globalQueue;
int counter1 = 0, counter2 = 0, counter3 = 0;
HANDLE mutex; // Mutex for synchronization
// Task 1: Producer-Consumer
DWORD WINAPI producer(LPVOID param) {
    int id = (int) (size_t) param;
    while (!exitFlag) {
        WaitForSingleObject(mutex, INFINITE); // Wait for mutex lock
        if (exitFlag)
            return 0;
        if (globalQueue.count < MAX_ITEMS) {
            int elem = rand() % 100;
            queue_add(&globalQueue, elem); // Add element
            printf("Producer %d added element %d to the list.\n", id, elem);
        } else {
            printf("Producer %d: The list is full.\n", id);
        }
        ReleaseMutex(mutex); // Release mutex
        Sleep(500); // Delay
    }
    return 0;
}

DWORD WINAPI consumer(LPVOID param) {
    int id = (int) (size_t) param;
    while (!exitFlag) {
        WaitForSingleObject(mutex, INFINITE); // Wait for mutex lock
        if (exitFlag)
            return 0;
        if (globalQueue.count > 0) {
            int elem = queue_remove(&globalQueue); // Remove element
            printf("Consumer %d removed element %d from the list.\n", id, elem);
        } else {
            printf("Consumer %d: The list is empty.\n", id);
        }
        ReleaseMutex(mutex); // Release mutex
        Sleep(500); // Delay
    }
    return 0;
}

// Task 2: Readers-Writers
HANDLE mutex_write; // Mutex for writing to file
HANDLE RC; // Mutex for reading from file
int read_count = 0; // Number of readers reading
HANDLE file_write; // File handle for writing
HANDLE file_read; // File handle for reading
HANDLE semaphore;
BOOL semaphoreFree = TRUE;

DWORD WINAPI writer(LPVOID param) {
    int id = (int) (size_t) param;
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Data from writer %d\n", id); // Формируем строку

    while (!exitFlag) {
        WaitForSingleObject(mutex_write, INFINITE); // Захват мьютекса для записи
        if (exitFlag)
            return 0;
        DWORD bytesWritten;
        BOOL writeResult = WriteFile(file_write, buffer, (DWORD) strlen(buffer), &bytesWritten, NULL);
        if (writeResult && bytesWritten == strlen(buffer)) {
            printf("Writer %d wrote data.\n", id);
        } else {
            printf("Writer %d failed to write data. Error code: %d\n", id, GetLastError());
        }

        ReleaseMutex(mutex_write); // Освобождение мьютекса
        Sleep(500); // Задержка
    }
    return 0;
}

DWORD WINAPI reader(LPVOID param) {
    int id = (int) (size_t) param;
    char buffer[256];
    DWORD bytesRead;

    while (!exitFlag) {
        WaitForSingleObject(RC, INFINITE); // Захват мьютекса для чтения
        if (exitFlag)
            return 0;
        read_count++;
        if (read_count == 1)
            WaitForSingleObject(mutex_write, INFINITE); // Захват мьютекса для записи, если первый читатель
        ReleaseMutex(RC); // Освобождение мьютекса для счётчика
        // Чтение данных из файла
        BOOL readResult = ReadFile(file_read, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
        if (readResult && bytesRead > 0) {
            buffer[bytesRead] = '\0'; // Завершаем строку нулевым символом
            printf("Reader %d read:\n%s\n", id, buffer);
        } else {
            if (!readResult) {
                printf("Reader %d failed to read data. Error code: %d\n", id, GetLastError());
            } else {
                printf("Reader %d: End of file reached.\n", id);
            }
        }

        WaitForSingleObject(RC, INFINITE); // Захват мьютекса для чтения
        read_count--;
        if (read_count == 0)
            ReleaseMutex(mutex_write); // Освобождение мьютекса для записи
        ReleaseMutex(RC); // Освобождение мьютекса для счётчика
        Sleep(500); // Задержка
    }
    return 0;
}

DWORD WINAPI thread_counter(LPVOID param) {
    size_t *params = (size_t *) param;
    int counter = (int) (size_t) params[0]; // Ссылка на счетчик
    int speed = (int) (size_t) params[1]; // Скорость потока
    int event_trigger = (int) (size_t) params[2]; // Порог события
    int thread_id = (int) (size_t) params[3]; // Идентификатор потока
    while (!exitFlag) {
        if (thread_id == 1) {
            WaitForSingleObject(RC, INFINITE);
            if (semaphoreFree) {
                ReleaseMutex(RC);
                ++counter;
                printf("Thread [%d]: counter = %d\n", thread_id, counter);
                Sleep(speed); // Задержка зависит от скорости потока
                continue;
            } else {
                ReleaseMutex(RC);
                WaitForSingleObject(semaphore, INFINITE); // Ожидаем, пока не возобновят поток
                continue;
            }
        }
        counter++;
        printf("Thread [%d]: counter = %d\n", thread_id, counter);
        if (counter == event_trigger) {
            WaitForSingleObject(RC, INFINITE); // Захват мьютекса записи semaphoreFree
            semaphoreFree = !semaphoreFree;
            ReleaseSemaphore(semaphore, semaphoreFree, NULL);
            ReleaseMutex(RC); // Освобождение мьютекса записи semaphoreFree
        }
        Sleep(speed); // Задержка зависит от скорости потока
    }
    return 0;
}

int num_producers = 0, num_consumers = 0;
int num_writers = 0, num_readers = 0;
HANDLE *threads;

void destructor() {
    exitFlag = TRUE; // Signal threads to exit
    if (threads != NULL) {
        for (int i = 0; i < num_writers + num_readers; i++) {
            if (threads[i]) {
                WaitForSingleObject(threads[i], INFINITE);
                CloseHandle(threads[i]);
            }
        }

        for (int i = 0; i < num_producers + num_consumers; i++) {
            if (threads[i]) {
                WaitForSingleObject(threads[i], INFINITE);
                CloseHandle(threads[i]);
            }
        }
        for (int i = 0; i < NUMBER_OF_THREADS; i++) {
            if (threads[i]) {
                WaitForSingleObject(threads[i], INFINITE);
                CloseHandle(threads[i]);
            }
        }
        free(threads);
    }
    if (mutex)
        CloseHandle(mutex);
    if (RC)
        CloseHandle(RC);
    if (mutex_write)
        CloseHandle(mutex_write);
    if (file_write != INVALID_HANDLE_VALUE)
        CloseHandle(file_write);
    if (file_read != INVALID_HANDLE_VALUE)
        CloseHandle(file_read);
    if (num_readers)
        num_readers = 0;
    if (num_writers)
        num_writers = 0;
    if (num_producers)
        num_producers = 0;
    if (num_consumers)
        num_consumers = 0;
    if (semaphore)
        CloseHandle(semaphore);
}

int main() {
    while (1) {
        int task;
        printf("Select a task (1 - Producer-Consumer, 2 - Readers-Writers, 3 - Thread-Counter 0 - Exit): \n");
        scanf("%d", &task);
        scanf("%*c");
        exitFlag = FALSE;
        switch (task) {
            case 1: {
                // Task 1: Producer-Consumer
                mutex = CreateMutex(NULL, FALSE, NULL);
                if (mutex == NULL) {
                    printf("Failed to create mutex.\n");
                    return 1;
                }
                printf("Enter number of producers: \n");
                scanf("%d", &num_producers);
                if (num_producers <= 0) {
                    printf("Invalid number of producers.\n");
                    break;
                }
                printf("Enter number of consumers: \n");
                scanf("%d", &num_consumers);
                if (num_consumers <= 0) {
                    printf("Invalid number of consumers.\n");
                    break;
                }
                scanf("%*c");
                int ids[num_producers + num_consumers];
                threads = (HANDLE *) malloc((num_producers + num_consumers) * sizeof(HANDLE));
                if (threads == NULL) {
                    printf("Failed to allocate memory for threads.\n");
                    destructor();
                    break;
                }
                for (int i = 0; i < num_producers; i++) {
                    ids[i] = i + 1;
                    threads[i] = CreateThread(NULL, 0, producer, (LPVOID) (size_t) ids[i], 0, NULL);
                }

                for (int i = 0; i < num_consumers; i++) {
                    ids[num_producers + i] = i + 1;
                    threads[num_producers + i] = CreateThread(NULL, 0, consumer,
                                                              (LPVOID) (size_t) ids[num_producers + i], 0, NULL);
                }

                printf("Press any key to stop...\n");
                getchar(); // Wait for user input
                destructor();
                break;
            }
            case 2: {
                // Task 2: Readers-Writers
                int num_writers, num_readers;
                printf("Enter number of writers: \n");
                scanf("%d", &num_writers);
                if (num_writers <= 0) {
                    printf("Invalid number of writers.\n");
                    break;
                }
                printf("Enter number of readers: \n");
                scanf("%d", &num_readers);
                if (num_readers <= 0) {
                    printf("Invalid number of readers.\n");
                    break;
                }
                scanf("%*c");
                threads = (HANDLE *) malloc((num_writers + num_readers) * sizeof(HANDLE));
                if (threads == NULL) {
                    printf("Failed to allocate memory for threads.\n");
                    destructor();
                    break;
                }
                mutex_write = CreateMutex(NULL, FALSE, NULL);
                RC = CreateMutex(NULL, FALSE, NULL);
                file_write = CreateFile(
                    "data.txt", // File name
                    FILE_APPEND_DATA, // Desired access
                    FILE_SHARE_READ | FILE_SHARE_WRITE, // Share mode
                    NULL, // Security attributes
                    OPEN_ALWAYS, // Create new if doesn't exist
                    FILE_ATTRIBUTE_NORMAL, // File attributes
                    NULL // Template file handle
                );
                file_read = CreateFile(
                    "data.txt",
                    GENERIC_READ, // Режим доступа для чтения
                    FILE_SHARE_READ | FILE_SHARE_WRITE, // Разрешить другим потокам чтение и запись
                    NULL,
                    OPEN_ALWAYS, // Создать файл, если он не существует
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                );
                if (file_write == INVALID_HANDLE_VALUE || file_read == INVALID_HANDLE_VALUE) {
                    printf("Error creating/opening file: %d\n", GetLastError());
                    destructor();
                    break;
                }
                if (mutex_write == NULL || RC == NULL) {
                    printf("Failed to create mutexes for reading and writing.\n");
                    destructor();
                    break;
                }
                int ids[num_writers + num_readers];
                for (int i = 0; i < num_writers; i++) {
                    ids[i] = i + 1;
                    threads[i] = CreateThread(NULL, 0, writer, (LPVOID) (size_t) ids[i], 0, NULL);
                    if (threads[i] == NULL) {
                        printf("Failed to create writer thread %d.\n", i);
                        destructor();
                        return 1;
                    }
                }
                for (int i = 0; i < num_readers; i++) {
                    ids[num_writers + i] = i + 1;
                    threads[num_writers + i] = CreateThread(NULL, 0, reader, (LPVOID) (size_t) ids[num_writers + i], 0,
                                                            NULL);
                    if (threads[num_writers + i] == NULL) {
                        printf("Failed to create reader thread %d.\n", i);
                        destructor();
                        return 1;
                    }
                }
                printf("Press any key to stop...\n");
                getchar(); // Wait for user input
                destructor();
                break;
            }
            case 3: {
                // Написать программу, запускающую три дочерних потока. Каждый поток увеличивает
                // значение счётчика (начиная с нуля) с разной скоростью. Приостановить выполнение
                // первого потока, при достижении счётчиком второго потока значения 50 и
                // возобновить выполнение при достижении счётчиком третьего потока значения 250
                semaphore = CreateSemaphore(NULL, 1, 1, NULL); // Создаем семафор
                RC = CreateMutex(NULL, FALSE, NULL);
                if (RC == NULL) {
                    printf("Failed to create mutex.\n");
                    destructor();
                    break;
                }
                if (semaphore == NULL) {
                    printf("Failed to create semaphore.\n");
                    destructor();
                    break;
                }
                threads = (HANDLE *) malloc(NUMBER_OF_THREADS * sizeof(HANDLE));
                if (threads == NULL) {
                    printf("Failed to allocate memory for threads.\n");
                    destructor();
                    break;
                }
                size_t params1[4] = {(size_t) counter1, (size_t) 100, (size_t) -1, (size_t) 1};
                size_t params2[4] = {(size_t) counter2, (size_t) 200, (size_t) 50, (size_t) 2};
                size_t params3[4] = {(size_t) counter3, (size_t) 150, (size_t) 250, (size_t) 3};
                threads[0] = CreateThread(NULL, 0, thread_counter, (LPVOID) params1, 0, NULL);
                threads[1] = CreateThread(NULL, 0, thread_counter, (LPVOID) params2, 0, NULL);
                threads[2] = CreateThread(NULL, 0, thread_counter, (LPVOID) params3, 0, NULL);
                printf("Press any key to stop...\n");
                getchar(); // Wait for user input
                destructor();
                break;
            }
            case 0:
                return 0;
            default: {
                printf("Invalid task selection.\n");
                break;
            }
        }
    }
}
