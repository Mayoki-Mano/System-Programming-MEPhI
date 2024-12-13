#include <windows.h>
#include <stdio.h>
#include <tlhelp32.h>

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

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING;

typedef struct _LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY InLoadOrderLinks;
    PVOID DllBase;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG Flags;
    SHORT LoadCount;
    SHORT TlsIndex;
    LIST_ENTRY HashLinks;
    ULONG TimeDateStamp;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef struct _PEB_LDR_DATA {
    ULONG Length;
    BOOLEAN Initialized;
    HANDLE SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _PEB {
    BOOLEAN InheritedAddressSpace;
    BOOLEAN ReadImageFileExecOptions;
    BOOLEAN BeingDebugged;
    BOOLEAN Spare;
    HANDLE Mutant;
    PVOID ImageBaseAddress;
    PPEB_LDR_DATA Ldr;
} PEB, *PPEB;

typedef struct _PROCESS_BASIC_INFORMATION {
    PVOID Reserved1;
    PPEB PebBaseAddress;
    PVOID Reserved2[2];
    ULONG_PTR UniqueProcessId;
    PVOID Reserved3;
} PROCESS_BASIC_INFORMATION;

typedef NTSTATUS(WINAPI *NtQueryInformationProcess_t)(
    HANDLE, ULONG, PVOID, ULONG, PULONG);

void displayProcessModules() {
    HMODULE hNtdll = LoadLibrary("ntdll.dll");
    if (!hNtdll) {
        printf("Error: failed to upload ntdll.dll.\n");
        return;
    }

    NtQueryInformationProcess_t NtQueryInformationProcess =
        (NtQueryInformationProcess_t)GetProcAddress(hNtdll, "NtQueryInformationProcess");
    if (!NtQueryInformationProcess) {
        printf("Error: couldn't get the address NtQueryInformationProcess.\n");
        FreeLibrary(hNtdll);
        return;
    }

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());
    if (!hProcess) {
        printf("Error: the process could not be opened.\n");
        FreeLibrary(hNtdll);
        return;
    }

    PROCESS_BASIC_INFORMATION pbi;
    ULONG returnLength;
    NTSTATUS status = NtQueryInformationProcess(hProcess, 0, &pbi, sizeof(pbi), &returnLength);
    if (status != 0) {
        printf("Error: NtQueryInformationProcess returned 0x%X\n", status);
        CloseHandle(hProcess);
        FreeLibrary(hNtdll);
        return;
    }

    PEB peb;
    if (!ReadProcessMemory(hProcess, pbi.PebBaseAddress, &peb, sizeof(peb), NULL)) {
        printf("Error: couldn't read PEB.\n");
        CloseHandle(hProcess);
        FreeLibrary(hNtdll);
        return;
    }

    PEB_LDR_DATA ldrData;
    if (!ReadProcessMemory(hProcess, peb.Ldr, &ldrData, sizeof(ldrData), NULL)) {
        printf("Error: couldn't read PEB_LDR_DATA.\n");
        CloseHandle(hProcess);
        FreeLibrary(hNtdll);
        return;
    }

    LIST_ENTRY *head = ldrData.InLoadOrderModuleList.Flink;
    LIST_ENTRY *current = head;

    do {
        LDR_DATA_TABLE_ENTRY moduleEntry;
        if (!ReadProcessMemory(hProcess, current, &moduleEntry, sizeof(moduleEntry), NULL)) {
            printf("Error: the module could not be read.\n");
            break;
        }

        wchar_t moduleName[256] = L"<Unknown>";
        if (moduleEntry.BaseDllName.Buffer && ReadProcessMemory(hProcess, moduleEntry.BaseDllName.Buffer, moduleName, moduleEntry.BaseDllName.Length, NULL)) {
            moduleName[moduleEntry.BaseDllName.Length / sizeof(wchar_t)] = L'\0';
            wprintf(L"Module: %s\n", moduleName);
        } else {
            printf("Error: the module name could not be read. Error code: %lu\n", GetLastError());
        }
        current = moduleEntry.InLoadOrderLinks.Flink;
    } while (current != head);

    CloseHandle(hProcess);
    FreeLibrary(hNtdll);
}

int main() {
    int choice;
    printf("Select a task:\n");
    printf("1. Creating a process:\n");
    printf("2. Iterating through existing processes\n");
    printf("3. Creating streams\n");
    printf("4. Displaying information about processes\n");
    printf("Enter the task number: ");
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
            displayProcessModules();
        break;
        default:
            printf("Wrong choice.\n");
        break;
    }
    return 0;
}