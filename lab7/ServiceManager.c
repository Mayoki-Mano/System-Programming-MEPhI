#include "ServiceManager.h"

SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE hStatus;
int main() {
    int choice;

    while (1) {
        printf("\nChoose an action:\n");
        printf("1. Install service\n");
        printf("2. Remove service\n");
        printf("3. Start service\n");
        printf("4. Stop service\n");
        printf("5. Exit\n");
        printf("Enter your choice: \n");
        scanf("%d", &choice);
        scanf("%*c");
        switch (choice) {
            case 1:
                InstallService();
                break;
            case 2:
                RemoveService();
                break;
            case 3:
                StartServiceManually();
                break;
            case 4:
                StopServiceManually();
                break;
            case 5:
                printf("Exiting the program.\n");
                return 0;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    }
}



void InstallService() {
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!hSCManager) {
        printf("Failed to open service manager: %d\n", GetLastError());
        return;
    }

    SC_HANDLE hService = CreateService(
        hSCManager,
        SERVICE_NAME,
        SERVICE_DISPLAY_NAME,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        PROGRAM_PATH,
        NULL, NULL, NULL, NULL, NULL
    );

    if (!hService) {
        printf("Failed to create service: %d\n", GetLastError());
    } else {
        printf("Service installed successfully.\n");
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
}

void RemoveService() {
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!hSCManager) {
        printf("Failed to open service manager: %d\n", GetLastError());
        return;
    }

    SC_HANDLE hService = OpenService(hSCManager, SERVICE_NAME, DELETE);
    if (!hService) {
        printf("Failed to open service: %d\n", GetLastError());
    } else if (!DeleteService(hService)) {
        printf("Failed to delete service: %d\n", GetLastError());
    } else {
        printf("Service removed successfully.\n");
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
}

void StartServiceManually() {
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCManager) {
        printf("Failed to open service manager: %d\n", GetLastError());
        return;
    }

    SC_HANDLE hService = OpenService(hSCManager, SERVICE_NAME, SERVICE_START);
    if (!hService) {
        printf("Failed to open service: %d\n", GetLastError());
    } else if (!StartService(hService, 0, NULL)) {
        printf("Failed to start service: %d\n", GetLastError());
    } else {
        printf("Service started successfully.\n");
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
}

void StopServiceManually() {
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSCManager) {
        printf("Failed to open service manager: %d\n", GetLastError());
        return;
    }

    SC_HANDLE hService = OpenService(hSCManager, SERVICE_NAME, SERVICE_STOP);
    if (!hService) {
        printf("Failed to open service: %d\n", GetLastError());
    } else {
        SERVICE_STATUS status;
        if (!ControlService(hService, SERVICE_CONTROL_STOP, &status)) {
            printf("Failed to stop service: %d\n", GetLastError());
        } else {
            printf("Service stopped successfully.\n");
        }
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
}


