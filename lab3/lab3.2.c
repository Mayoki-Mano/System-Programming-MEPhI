#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

void print_help() {
    printf("Usage: file_mapping_tool.exe <command> <file>\n");
    printf("Commands:\n");
    printf("  sort_alpha <file>               - Sort letters by alphabet\n");
    printf("  count_letters <file>            - Count number of lowercase and uppercase letters\n");
    printf("  remove_a <file>                 - Delete all letters 'a', write down the number of deleted\n");
    printf("  sort_numbers <file>             - Sort the contents of a numeric file in descending order\n");
    printf("  help                            - Show help\n");
}

void sort_alphabetically(const char *filename) {
    HANDLE hFile = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Error: couldn't open file %s.\n", filename);
        return;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE || fileSize == 0) {
        printf("Error: file is empty or unavailable.\n");
        CloseHandle(hFile);
        return;
    }

    HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
    if (!hMapping) {
        printf("Error: Unable to create file mapping.\n");
        CloseHandle(hFile);
        return;
    }

    char *data = (char *)MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, fileSize);
    if (!data) {
        printf("Error: Unable to map file into memory.\n");
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return;
    }

    qsort(data, fileSize, sizeof(char), (int (*)(const void *, const void *))strcmp);

    printf("The contents of the file are sorted alphabetically..\n");

    UnmapViewOfFile(data);
    CloseHandle(hMapping);
    CloseHandle(hFile);
}

void count_letters(const char *filename) {
    HANDLE hFile = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Error: Unable to open file %s.\n", filename);
        return;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE || fileSize == 0) {
        printf("Error: file is empty or unavailable.\n");
        CloseHandle(hFile);
        return;
    }

    HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hMapping) {
        printf("Error: Unable to create file mapping.\n");
        CloseHandle(hFile);
        return;
    }

    char *data = (char *)MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, fileSize);
    if (!data) {
        printf("Error: Unable to map file into memory.\n");
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return;
    }

    int lower = 0, upper = 0;
    for (DWORD i = 0; i < fileSize; i++) {
        if (islower(data[i])) {
            lower++;
        } else if (isupper(data[i])) {
            upper++;
        }
    }

    printf("Number of lowercase letters: %d\n", lower);
    printf("Number of capital letters: %d\n", upper);

    UnmapViewOfFile(data);
    CloseHandle(hMapping);
    CloseHandle(hFile);
}

void remove_a(const char *filename) {
    HANDLE hFile = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Error: Unable to open file %s.\n", filename);
        return;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE || fileSize == 0) {
        printf("Error: file is empty or unavailable.\n");
        CloseHandle(hFile);
        return;
    }

    HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
    if (!hMapping) {
        printf("Error: Unable to create file mapping.\n");
        CloseHandle(hFile);
        return;
    }
    printf("%ld",fileSize);

    char *data = (char *)MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, fileSize);
    printf("%ld",fileSize);
    if (!data) {
        printf("Error: Unable to map file into memory.\n");
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return;
    }

    int removed = 0;
    DWORD  newSize = 0;
    printf("%ld",fileSize);

    printf("%ld",newSize);

    for (DWORD i = 0; i < fileSize; i++) {
        if (data[i] != 'a') {
            data[newSize++] = data[i];
            printf("%ld",newSize);
        } else {
            removed++;
        }
    }
    int needed_length = snprintf(NULL, 0, "%d", removed);
    if (needed_length < 0) {
        printf("Error: snprintf failed to calculate string length.\n");
        UnmapViewOfFile(data);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return;
    }
    snprintf(data + newSize, 23 + needed_length, "\nDeleted: %d letters 'a'", removed);
    newSize += 22 + needed_length;
    printf("Deleted: %d letters 'a'. %d\n", removed, needed_length);
    UnmapViewOfFile(data);

    CloseHandle(hMapping);
    SetFilePointer(hFile, (LONG) newSize, NULL, FILE_BEGIN);
    if (!SetEndOfFile(hFile)) {
        printf("Error: Unable to set new file size.\n");
    }

    CloseHandle(hFile);

}

int compare_descending(const void *a, const void *b) {
    return (*(int *)b - *(int *)a);
}

void sort_numbers(const char *filename) {
    HANDLE hFile = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Error: Unable to open file %s.\n", filename);
        return;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE || fileSize == 0) {
        printf("Error: file is empty or unavailable.\n");
        CloseHandle(hFile);
        return;
    }

    HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
    if (!hMapping) {
        printf("Error: Unable to create file mapping.\n");
        CloseHandle(hFile);
        return;
    }

    char *data = (char *)MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, fileSize);
    if (!data) {
        printf("Error: Unable to map file into memory.\n");
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return;
    }

    int numbers[1024], count = 0;
    char *token = strtok(data, " ,\n");
    while (token && count < 1024) {
        numbers[count++] = atoi(token);
        token = strtok(NULL, " ,\n");
    }

    qsort(numbers, count, sizeof(int), compare_descending);

    DWORD offset = 0;
    for (int i = 0; i < count; i++) {
        offset += sprintf(data + offset, "%d ", numbers[i]);
    }

    printf("The numbers are sorted in descending order.\n");

    UnmapViewOfFile(data);
    CloseHandle(hMapping);
    CloseHandle(hFile);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_help();
        return 0;
    }

    if (strcmp(argv[1], "sort_alpha") == 0) {
        sort_alphabetically(argv[2]);
    } else if (strcmp(argv[1], "count_letters") == 0) {
        count_letters(argv[2]);
    } else if (strcmp(argv[1], "remove_a") == 0) {
        remove_a(argv[2]);
    } else if (strcmp(argv[1], "sort_numbers") == 0) {
        sort_numbers(argv[2]);
    } else {
        print_help();
    }

}
