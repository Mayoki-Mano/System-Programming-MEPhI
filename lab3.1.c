#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>

char *readline() {
    char buf[81] = {0};
    char *res = NULL;
    int len = 0;
    int n;
    do {
        n = scanf("%80[^\n]", buf);
        if (n < 0) {
            if (!res) {
                return NULL;
            }

        } else if (n > 0) {
            unsigned buf_len = strlen(buf);
            unsigned str_len = len + buf_len;
            char * tmp = res;
            res = (char *) realloc(res, sizeof(char) * (str_len + 1));
            if (!res){
                if (!tmp)
                    free(tmp);
                return NULL;
            }
            for (int i = 0; i < buf_len; i++) {
                res[len] = buf[i];
                len++;
            }
        } else {
            scanf("%*c");
        }
    } while (n > 0);
    if (len > 0) {
        if (!res)
            return NULL;
        res[len] = '\0';
    } else {
        res = (char *) calloc(1, sizeof(char));
    }
    return res;
}

void print_help() {
    printf("Usage: file_tool.exe <command> [parameters]\n");
    printf("Commands:\n");
    printf("  create <filename> <content>      - Create file with content\n");
    printf("  read <filename>                 - Read content of file\n");
    printf("  delete <filename>               - File removing\n");
    printf("  rename <filename> <new_name>    - Rename file\n");
    printf("  copy <filename> <destination>   - Copy file\n");
    printf("  size <filename>                 - Get size of file\n");
    printf("  attributes <filename>           - Get attributes of file\n");
    printf("  set_readonly <filename>         - Set readonly mode for file\n");
    printf("  set_hidden <filename>           - Set hidden mode for file\n");
    printf("  list <directory>                - List content of directory\n");
    printf("  help                            - Get main commands (help)\n");
}

void create_file(const char *filename, const char *content) {
    HANDLE hFile = CreateFile(
            filename,
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
    );
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Error: couldn't create file %s.\n", filename);
        return;
    }
    DWORD bytesWritten;
    WriteFile(hFile, content, strlen(content), &bytesWritten, NULL);
    printf("File %s created successfully.\n", filename);
    CloseHandle(hFile);
}

void read_file(const char *filename) {
    HANDLE hFile = CreateFile(
            filename,
            GENERIC_READ,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
    );
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Error: couldn't open file %s for reading.\n", filename);
        return;
    }
    char buffer[1024];
    DWORD bytesRead;
    if (ReadFile(hFile, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
        buffer[bytesRead] = '\0';
        printf("File content:\n%s\n", buffer);
    } else {
        printf("Reading file error %s.\n", filename);
    }
    CloseHandle(hFile);
}

void delete_file(const char *filename) {
    if (DeleteFile(filename)) {
        printf("File %s deleted successfully.\n", filename);
    } else {
        printf("Error: couldn't delete file %s.\n", filename);
    }
}

void rename_file(const char *old_name, const char *new_name) {
    if (MoveFile(old_name, new_name)) {
        printf("File %s renamed to %s.\n", old_name, new_name);
    } else {
        printf("Error: couldn't rename file %s.\n", old_name);
    }
}

void copy_file(const char *filename, const char *destination) {
    if (CopyFile(filename, destination, FALSE)) {
        printf("File %s copied to %s successfully.\n", filename, destination);
    } else {
        printf("Error: couldn't copy file %s.\n", filename);
    }
}

void get_file_size(const char *filename) {
    HANDLE hFile = CreateFile(
            filename,
            GENERIC_READ,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
    );
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Error: couldn't open file %s for getting size.\n", filename);
        return;
    }
    LARGE_INTEGER fileSize;
    if (GetFileSizeEx(hFile, &fileSize)) {
        printf("File size %s: %lld byte.\n", filename, fileSize.QuadPart);
    } else {
        printf("Error: couldn't get file size  %s.\n", filename);
    }
    CloseHandle(hFile);
}

void get_file_attributes(const char *filename) {
    DWORD attributes = GetFileAttributes(filename);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        printf("Error: couldn't get file attributes %s.\n", filename);
        return;
    }
    printf("File attributes %s:\n", filename);
    if (attributes & FILE_ATTRIBUTE_READONLY) printf("  Readonly\n");
    if (attributes & FILE_ATTRIBUTE_HIDDEN) printf("  Hidden\n");
    if (attributes & FILE_ATTRIBUTE_SYSTEM) printf("  System\n");
    if (attributes & FILE_ATTRIBUTE_ARCHIVE) printf("  Archived\n");
}

void set_file_attribute(const char *filename, DWORD attribute) {
    DWORD attributes = GetFileAttributes(filename);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        printf("Error: couldn't get file attributes %s.\n", filename);
        return;
    }
    if (SetFileAttributes(filename, attributes | attribute)) {
        printf("Attribute was set successfully %s.\n", filename);
    } else {
        printf("Error: couldn't set attribute %s.\n", filename);
    }
}

void list_directory(const char *directory) {
    WIN32_FIND_DATA findFileData;
    char searchPath[MAX_PATH];
    snprintf(searchPath, MAX_PATH, "%s\\*", directory);
    HANDLE hFind = FindFirstFile(searchPath, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        printf("Error: couldn't open directory %s.\n", directory);
        return;
    }
    printf("Directory content %s:\n", directory);
    do {
        printf("  %s\n", findFileData.cFileName);
    } while (FindNextFile(hFind, &findFileData));
    FindClose(hFind);
}


int menu(){
    printf("Enter your choice:\n");
    printf("[1] Create new file with content\n");
    printf("[2] Read file\n");
    printf("[3] Delete file\n");
    printf("[4] Rename file\n");
    printf("[5] Copy file\n");
    printf("[6] Get size of file\n");
    printf("[7] Get attributes of file\n");
    printf("[8] Set readonly attribute for file\n");
    printf("[9] Set hidden attribute for file\n");
    printf("[10] Get list of files for directory\n");
    printf("[11] Exit\n");
    int choice;
    scanf("%d", &choice);
    scanf("%*c");
    return choice;
}

void parse_argv(int argc, char *argv[]){
    if (argc < 2) {
        print_help();
    }
    if (strcmp(argv[1], "create") == 0 && argc == 4) {
        create_file(argv[2], argv[3]);
    } else if (strcmp(argv[1], "read") == 0 && argc == 3) {
        read_file(argv[2]);
    } else if (strcmp(argv[1], "delete") == 0 && argc == 3) {
        delete_file(argv[2]);
    } else if (strcmp(argv[1], "rename") == 0 && argc == 4) {
        rename_file(argv[2], argv[3]);
    } else if (strcmp(argv[1], "copy") == 0 && argc == 4) {
        copy_file(argv[2], argv[3]);
    } else if (strcmp(argv[1], "size") == 0 && argc == 3) {
        get_file_size(argv[2]);
    } else if (strcmp(argv[1], "attributes") == 0 && argc == 3) {
        get_file_attributes(argv[2]);
    } else if (strcmp(argv[1], "set_readonly") == 0 && argc == 3) {
        set_file_attribute(argv[2], FILE_ATTRIBUTE_READONLY);
    } else if (strcmp(argv[1], "set_hidden") == 0 && argc == 3) {
        set_file_attribute(argv[2], FILE_ATTRIBUTE_HIDDEN);
    } else if (strcmp(argv[1], "list") == 0 && argc == 3) {
        list_directory(argv[2]);
    } else {
        print_help();
    }
}


void interactive_mode(){
    int choice;
    while (1){
        choice = menu();
        switch (choice) {
            case 1: {
                printf("Enter filename:\n");
                char * filename = readline();
                printf("Enter content:\n");
                char * content = readline();
                create_file(filename,content);
                free(filename);
                free(content);
                break;
            }
            case 2: {
                printf("Enter filename:\n");
                char * filename = readline();
                read_file(filename);
                free(filename);
                break;
            }
            case 3:{
                printf("Enter filename:\n");
                char * filename = readline();
                delete_file(filename);
                free(filename);
                break;
            }
            case 4:{
                printf("Enter old filename:\n");
                char * old_filename = readline();
                printf("Enter new filename:\n");
                char * new_filename = readline();
                rename_file(old_filename,new_filename);
                free(old_filename);
                free(new_filename);
                break;
            }
            case 5:{
                printf("Enter filename for copy:\n");
                char * filename = readline();
                printf("Enter destination for new file:\n");
                char * destination = readline();
                copy_file(filename,destination);
                free(filename);
                free(destination);
                break;
            }
            case 6:{
                printf("Enter filename to get size:\n");
                char * filename = readline();
                get_file_size(filename);
                free(filename);
                break;
            }
            case 7:{
                printf("Enter filename to get attributes:\n");
                char * filename = readline();
                get_file_attributes(filename);
                free(filename);
                break;
            }
            case 8:{
                printf("Enter filename to set readonly attribute:\n");
                char * filename = readline();
                set_file_attribute(filename,FILE_ATTRIBUTE_READONLY);
                free(filename);
                break;
            }
            case 9:{
                printf("Enter filename to set hidden attribute:\n");
                char * filename = readline();
                set_file_attribute(filename,FILE_ATTRIBUTE_HIDDEN);
                free(filename);
                break;
            }
            case 10:{

                printf("Enter directory:\n");
                char * directory = readline();
                list_directory(directory);
                free(directory);
                break;
            }
            case 11:{
                return;
            }
            default: {
                printf("Enter error\n");
                break;
            }
        }
    }
}

int main(int argc, char *argv[]){
    if (argc==1){
        interactive_mode();
    }else{
        parse_argv(argc, argv);
    }
    return 0;
}