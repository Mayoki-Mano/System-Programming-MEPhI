#include <windows.h>
#include <stdio.h>
#include <tlhelp32.h>
#include <winternl.h>

void create_process(char *cmdLine) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );
    if (CreateProcess(
            NULL,       // Имя исполняемого файла (NULL для использования cmdLine)
            cmdLine,    // Командная строка
            NULL,       // Дескриптор безопасности для процесса
            NULL,       // Дескриптор безопасности для потока
            FALSE,      // Наследование дескрипторов
            0,          // Флаги создания
            NULL,       // Указатель на переменные окружения
            NULL,       // Текущая директория
            &si,        // Указатель на структуру STARTUPINFO
            &pi         // Указатель на структуру PROCESS_INFORMATION
        )) {
        printf("The process has been successfully created.\n");
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode;
        if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
            printf("The process is completed with the code: %lu\n", exitCode);
        } else {
            printf("The process completion code could not be received.\n");
        }
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        } else {
            printf("Error creating the process. Error code: %lu\n", GetLastError());
        }
}

void process_enumeration() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        printf("Error: failed to create a snapshot of the processes.\n");
        return;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe)) {
        do {
            printf("PID: %lu, Name: %s\n", pe.th32ProcessID, pe.szExeFile);
        } while (Process32Next(hSnapshot, &pe));
    } else {
        printf("Error: failed to get information about processes.\n");
    }
    CloseHandle(hSnapshot);
}

DWORD WINAPI ThreadFunction(LPVOID lpParam) {
    for (int i = 0; i < 5; i++) {
        printf("Thread %d: meow\n", (int)(size_t)lpParam);
        Sleep(1000);
    }
    return 0;
}

void createThreads(int threads_num) {
    HANDLE threads[threads_num];

    for (int i = 0; i < threads_num; i++) {
        threads[i] = CreateThread(
            NULL,
            0,
            ThreadFunction,
            (LPVOID)(size_t)i,
            0,
            NULL
        );

        if (!threads[i]) {
            printf("Error: failed to create a stream %d\n", i);
            return;
        }
    }

    WaitForMultipleObjects(threads_num, threads, TRUE, INFINITE);

    for (int i = 0; i < threads_num; i++) {
        CloseHandle(threads[i]);
    }

    printf("All threads are completed.\n");
}

typedef NTSTATUS(WINAPI *NtQueryInformationProcess_t)(
    HANDLE, ULONG, PVOID, ULONG, PULONG);

void displayCurrentProcessModules() {
    HANDLE hProcess = GetCurrentProcess();
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    NtQueryInformationProcess_t NtQueryInformationProcess =
        (NtQueryInformationProcess_t)GetProcAddress(hNtdll, "NtQueryInformationProcess");
    if (!NtQueryInformationProcess) {
        printf("Error: couldn't get the address of NtQueryInformationProcess.\n");
        FreeLibrary(hNtdll);
        CloseHandle(hProcess);
        return;
    }
    PROCESS_BASIC_INFORMATION pbi;
    NTSTATUS status = NtQueryInformationProcess(hProcess, 0, &pbi, sizeof(pbi), NULL);
    if (!NT_SUCCESS(status)) {
        printf("Error: NtQueryInformationProcess failed with status 0x%X\n", status);
        FreeLibrary(hNtdll);
        CloseHandle(hProcess);
        return;
    }

    PEB peb;
    if (!ReadProcessMemory(hProcess, pbi.PebBaseAddress, &peb, sizeof(peb), NULL)) {
        printf("Error: couldn't read PEB. Error code: %lu\n", GetLastError());
        FreeLibrary(hNtdll);
        CloseHandle(hProcess);
        return;
    }
    PEB_LDR_DATA ldrData;
    if (!ReadProcessMemory(hProcess, peb.Ldr, &ldrData, sizeof(ldrData), NULL)) {
        printf("Error: couldn't read PEB_LDR_DATA. Error code: %lu\n", GetLastError());
        FreeLibrary(hNtdll);
        CloseHandle(hProcess);
        return;
    }

    LIST_ENTRY *head = ldrData.InMemoryOrderModuleList.Flink;
    if (!head) {
        printf("Error: Module list is empty.\n");
        FreeLibrary(hNtdll);
        CloseHandle(hProcess);
        return;
    }

    LIST_ENTRY *current = head;
    CloseHandle(hProcess);
    FreeLibrary(hNtdll);
    do {
        LDR_DATA_TABLE_ENTRY moduleEntry;
        if (!ReadProcessMemory(hProcess, current, &moduleEntry, sizeof(moduleEntry), NULL)) {
            printf("Error: couldn't read module entry. Error code: %lu\n", GetLastError());
            break;
        }
        UNICODE_STRING fullDllName;
        if (!ReadProcessMemory(hProcess, moduleEntry.FullDllName.Buffer, &fullDllName, moduleEntry.FullDllName.Length, NULL)) {
            printf("Error: couldn't read UNICODE_STRING. Error code: %lu\n", GetLastError());
            break;
        }
        printf("Module Name: %S\n",moduleEntry.FullDllName.Buffer);

        current = current->Flink;
        if (current == NULL) {
            printf("Error: Flink is NULL. Stopping iteration.\n");
            break;
        }

    } while (current != head);
    CloseHandle(hProcess);
    FreeLibrary(hNtdll);
}


int main() {
    int choice;
    printf("Select a task:\n");
    printf("1. Creating a process:\n");
    printf("2. Iterating through existing processes\n");
    printf("3. Creating threads\n");
    printf("4. Displaying information about processes\n");
    printf("Enter the task number: \n");
    scanf("%d", &choice);
    switch (choice) {
        case 1:
            char cmdLine[] = "notepad.exe";
            create_process(cmdLine);
        break;
        case 2:
            process_enumeration();
        break;
        case 3:
            createThreads(3);
        break;
        case 4:
            displayCurrentProcessModules();
        break;
        default:
            printf("Wrong choice.\n");
        break;
    }
    return 0;
}