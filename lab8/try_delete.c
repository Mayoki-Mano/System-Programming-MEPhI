#include <windows.h>
#include <stdio.h>
#include <string.h>

// Function to create a file
void createFile(const char* filename) {
    HANDLE hFile = CreateFile(
            filename,               // File name
            GENERIC_WRITE,          // Write access
            0,                      // No sharing
            NULL,                   // Default security attributes
            CREATE_NEW,             // Create a new file (fails if the file already exists)
            FILE_ATTRIBUTE_NORMAL,  // Normal file attributes
            NULL                    // No template file
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Error creating file: %d\n", GetLastError());
    } else {
        printf("File successfully created: %s\n", filename);
        CloseHandle(hFile);
    }
}

// Function to delete a file
void deleteFile(const char* filename) {
    if (DeleteFile(filename)) {
        printf("File successfully deleted: %s\n", filename);
    } else {
        printf("Error deleting file: %d\n", GetLastError());
    }
}

// Function to rename a file
void renameFile(const char* oldFilename, const char* newFilename) {
    if (MoveFile(oldFilename, newFilename)) {
        printf("File successfully renamed: %s -> %s\n", oldFilename, newFilename);
    } else {
        printf("Error renaming file: %d\n", GetLastError());
    }
}

// Function to display the menu
void printMenu() {
    printf("\nMenu:\n");
    printf("1. Create a file\n");
    printf("2. Delete a file\n");
    printf("3. Rename a file\n");
    printf("4. Exit\n");
    printf("Choose an option: ");
}

int main() {
    int choice;
    char filename[256];
    char newFilename[256];

    while (1) {
        printMenu();
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                printf("Enter the file name to create: ");
                scanf("%s", filename);
                createFile(filename);
                break;

            case 2:
                printf("Enter the file name to delete: ");
                scanf("%s", filename);
                deleteFile(filename);
                break;

            case 3:
                printf("Enter the current file name: ");
                scanf("%s", filename);
                printf("Enter the new file name: ");
                scanf("%s", newFilename);
                renameFile(filename, newFilename);
                break;

            case 4:
                printf("Exiting the program.\n");
                return 0;

            default:
                printf("Invalid choice. Please try again.\n");
                break;
        }
    }

    return 0;
}