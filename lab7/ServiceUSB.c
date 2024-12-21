#include <windows.h>
#include <stdio.h>
#include "ServiceManager.h"
#include <Cfgmgr32.h>
#include <initguid.h>
#include <Usbiodef.h>
SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE hStatus;
CRITICAL_SECTION cs;
#define USBSTOR_KEY_PATH "SYSTEM\\CurrentControlSet\\Enum\\USBSTOR"  // Ключ реестра для устройств USB


void LogMessage(const char* message) {
    FILE *logFile;
    fopen_s(&logFile, "C:\\ServiceLog.txt", "a");
    if (logFile != NULL) {
        fprintf(logFile, "%s\n", message);
        fclose(logFile);
    }
}

// Функция для блокировки USB устройства по идентификатору
void BlockUSBDeviceByKey(const LPCWSTR deviceKey) {
    HKEY hKey;
    DWORD dwDisposition;
    char errorMessage[256];
    sprintf_s(errorMessage, sizeof(errorMessage), "try RegOpenKeyEx\n");
    LogMessage(errorMessage);
    // Открытие ключа реестра
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, USBSTOR_KEY_PATH, REG_OPTION_OPEN_LINK, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS) {
        sprintf_s(errorMessage, sizeof(errorMessage), "Couldn't open registr key.\n");
        LogMessage(errorMessage);
        return;
    }

    // Открываем ключ устройства, используя его идентификатор
    HKEY deviceKeyHandle;
    if (RegOpenKeyEx(hKey, deviceKey, REG_OPTION_OPEN_LINK, KEY_ALL_ACCESS, &deviceKeyHandle) == ERROR_SUCCESS) {
        // Удаляем ключ устройства, что фактически "блокирует" его
        if (RegDeleteKey(hKey, deviceKey) == ERROR_SUCCESS) {
            sprintf_s(errorMessage, sizeof(errorMessage), "Device blocked: %S\n", deviceKey);
            LogMessage(errorMessage);
        } else {
            sprintf_s(errorMessage, sizeof(errorMessage), "Could not delete device key: %S\n", deviceKey);
            LogMessage(errorMessage);
        }
        RegCloseKey(deviceKeyHandle);
    } else {
        sprintf_s(errorMessage, sizeof(errorMessage), "Could not open device key: %S\n", deviceKey);
        LogMessage(errorMessage);
    }

    RegCloseKey(hKey);
}
void RegisterEventSourceManually() {
    HKEY hKey;
    DWORD typesSupported = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
    char *subkey = "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\USBMonitorService";
    char *eventMessageFile = "advapi32.dll";
    char *logFilePath = "C:\\ServiceLog.txt";
    char * prohibitedSerialNumbers = "Disk&Rev_5.00#8&1f7b8582&0&_&0 Disk&Rev_5.00#6&1d017224&1&_&0 #5&22b8dd10";
    char errorMessage[256];
    // Создание раздела
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, subkey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        // Установка пути к файлу сообщений
        RegSetValueEx(hKey, "EventMessageFile", 0, REG_EXPAND_SZ,
                      (const BYTE*)eventMessageFile,
                      (strlen(eventMessageFile) + 1) * sizeof(char));
        RegSetValueEx(hKey, "LogFilePath", 0, REG_EXPAND_SZ, (const BYTE*)logFilePath, (strlen(logFilePath) + 1) * sizeof(char));
        // Установка типов поддерживаемых событий
        RegSetValueEx(hKey, "ProhibitedSerialNumbers",0, REG_EXPAND_SZ, (const BYTE*)prohibitedSerialNumbers, 256+strlen(prohibitedSerialNumbers));
        RegSetValueEx(hKey, "TypesSupported", 0, REG_DWORD,
                      (const BYTE*)&typesSupported, sizeof(DWORD));

        RegCloseKey(hKey);
        sprintf_s(errorMessage, sizeof(errorMessage), "Event source registered successfully.\n");
        LogMessage(errorMessage);
    } else {
        sprintf_s(errorMessage, sizeof(errorMessage), "Failed to register event source. Error: %lu\n", GetLastError());
        LogMessage(errorMessage);
    }
}


HANDLE hEventLog=NULL;
void LogEvent(LPCWSTR message, WORD eventType) {
    HANDLE hEventLog = RegisterEventSource(NULL, SERVICE_NAME);

    if (hEventLog == NULL) {
        char errorMessage[256];
        sprintf_s(errorMessage, sizeof(errorMessage), "Failed to register event source. Error: %lu\n", GetLastError());
        LogMessage(errorMessage);
        return;
    }

    LPCWSTR messages[2];
    messages[0] = message;
    messages[1] = NULL;
    DWORD eventID = 1000; // Default for informational events

    if (!ReportEvent(
            hEventLog,              // Handle to event log
            eventType,              // Event type: EVENTLOG_SUCCESS, EVENTLOG_ERROR_TYPE, etc.
            0,                      // Category (0 if not used)
            eventID,                      // Event ID
            NULL,                   // Security identifier (NULL if not used)
            1,                      // Number of strings to log
            0,                      // Size of raw data (0 if not used)
            messages,               // Array of strings to log
            NULL                    // Raw data (NULL if not used)
        )) {
        char errorMessage[256];
        sprintf_s(errorMessage, sizeof(errorMessage), "Failed to report event. Error: %lu\n", GetLastError());
        LogMessage(errorMessage);
        }
    LogMessage((char *)message);
    DeregisterEventSource(hEventLog);
}

void LogDeviceConnection(LPCWSTR serialNumber) {
    char message[256];
    char serialNumberAnsi[256];
    WideCharToMultiByte(CP_ACP, 0, serialNumber, -1, serialNumberAnsi, sizeof(serialNumberAnsi), NULL, NULL);
    snprintf(message, 256, "USB device connected: Serial Number: %s", serialNumberAnsi);
    LogEvent(message, EVENTLOG_INFORMATION_TYPE);
}

void LogDeviceDisconnection(LPCWSTR serialNumber) {
    char message[256];
    char serialNumberAnsi[256];
    WideCharToMultiByte(CP_ACP, 0, serialNumber, -1, serialNumberAnsi, sizeof(serialNumberAnsi), NULL, NULL);
    snprintf(message, 256, "USB device disconnected: Serial Number: %s", serialNumberAnsi);
    LogEvent(message, EVENTLOG_INFORMATION_TYPE);
}
CONFIGRET CALLBACK NotificationCallback(HCMNOTIFICATION hNotify, PVOID Context, CM_NOTIFY_ACTION Action, PCM_NOTIFY_EVENT_DATA EventData, DWORD EventDataSize) {
    switch (Action) {
        case CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL: {
            LPCWSTR serialNumber = EventData->u.DeviceInterface.SymbolicLink;
            char message[256];
            char serialNumberAnsi[256];
            WideCharToMultiByte(CP_ACP, 0, serialNumber, -1, serialNumberAnsi, sizeof(serialNumberAnsi), NULL, NULL);
            snprintf(message, 256, "BIBA1: %s", serialNumberAnsi);
            LogMessage(message);
            HKEY hKey;
            char errorMessage[256];
            char *subkey = "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\USBMonitorService";
            sprintf_s(errorMessage, sizeof(errorMessage), "PIP0: \n");
            LogMessage(errorMessage);
            RegCreateKeyEx(HKEY_LOCAL_MACHINE, subkey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hKey, NULL);
            char ProhibitedSerialNumbers[1024];
            sprintf_s(errorMessage, sizeof(errorMessage), "PIP1: \n");
            LogMessage(errorMessage);
            DWORD dwSize = sizeof(ProhibitedSerialNumbers);
            sprintf_s(errorMessage, sizeof(errorMessage), "PIP2: \n");
            LogMessage(errorMessage);
            LSTATUS S = RegQueryValueExA(hKey,
                                "ProhibitedSerialNumbers",
                                0,
                                NULL,
                                (BYTE *)ProhibitedSerialNumbers,
                                &dwSize);
            sprintf_s(errorMessage, sizeof(errorMessage), "ProhibitedSerialNumbers: %s\n", ProhibitedSerialNumbers);
            LogMessage(errorMessage);
            char buffer[1024];
            strncpy(buffer,ProhibitedSerialNumbers,sizeof(buffer)-1);
            buffer[sizeof(buffer)-1]='\0';
            char * token = strtok(ProhibitedSerialNumbers, " ");
            while (token!=NULL){
                if (strstr(serialNumberAnsi, token) != NULL) {
                    snprintf(message, 256, "BIBA2: %s", serialNumber);
                    LogMessage(message);
                    BlockUSBDeviceByKey(serialNumber);
                    return CR_SUCCESS;
                }else
                    token= strtok(NULL," ");
            }
            LogDeviceConnection(serialNumber);
            break;
        }
        case CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL: {
            LPCWSTR serialNumber = EventData->u.DeviceInterface.SymbolicLink;
            LogDeviceDisconnection(serialNumber);
            break;
        }
        default: {
            printf("Other event occurred.\n");
            break;
        }
    }
    return CR_SUCCESS;
}


void WINAPI USBMonitoringService(DWORD argc, LPSTR *argv) {
    InitializeCriticalSection(&cs);
    hStatus = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);
    if (!hStatus) {
        printf("Failed to register service control handler. Error code: %lu\n", GetLastError());
        DeleteCriticalSection(&cs);
        return;
    }

    ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    ServiceStatus.dwWin32ExitCode = 0;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint = 0;
    ServiceStatus.dwWaitHint = 0;

    SetServiceStatus(hStatus, &ServiceStatus);

    // Initialize the service
    HCMNOTIFICATION hNotification = NULL;

    // Указываем GUID для USB-устройств
    CM_NOTIFY_FILTER filter = { 0 };
    filter.cbSize = sizeof(filter);
    filter.Flags = CM_NOTIFY_FILTER_FLAG_ALL_INTERFACE_CLASSES;
    filter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
    ZeroMemory(&filter.u.DeviceInterface.ClassGuid, sizeof(GUID));
    CONFIGRET cr = CM_Register_Notification(&filter, NULL, NotificationCallback, &hNotification);
    if (cr != CR_SUCCESS) {
        char errorMessage[256];
        sprintf_s(errorMessage, sizeof(errorMessage), "Failed to register notification: %d, %lu", cr, GetLastError());
        LogMessage(errorMessage);
        return;
    }
    ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(hStatus, &ServiceStatus);



    // Main service loop
    while (TRUE){
        EnterCriticalSection(&cs);
        if (ServiceStatus.dwCurrentState != SERVICE_RUNNING) {
            LeaveCriticalSection(&cs);
            break;
        }
        LeaveCriticalSection(&cs);
        printf("service is running\n");
        // Do something
        Sleep(1000);

    }

    ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(hStatus, &ServiceStatus);
    if (hNotification) {
        CM_Unregister_Notification(hNotification);
    }
    DeleteCriticalSection(&cs);
}

void WINAPI ServiceCtrlHandler(DWORD dwCtrlCode) {
    switch (dwCtrlCode) {
        case SERVICE_CONTROL_STOP: {
            EnterCriticalSection(&cs);
            ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
            SetServiceStatus(hStatus, &ServiceStatus);
            ServiceStatus.dwCurrentState = SERVICE_STOPPED;
            SetServiceStatus(hStatus, &ServiceStatus);
            LeaveCriticalSection(&cs);
            break;
        }
        case SERVICE_CONTROL_SHUTDOWN: {
            EnterCriticalSection(&cs);
            ServiceStatus.dwCurrentState = SERVICE_CONTROL_SHUTDOWN;
            SetServiceStatus(hStatus, &ServiceStatus);
            LeaveCriticalSection(&cs);
            break;
        }

        default:
            break;
    }
}


int main() {

    SERVICE_TABLE_ENTRY ServiceTable[] = {
        {SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)USBMonitoringService},
        {NULL, NULL}
    };
    RegisterEventSourceManually();
    // Регистрируем службы с диспетчером SCM
    if (!StartServiceCtrlDispatcher(ServiceTable)) {
        printf("Server registration error: %d\n", GetLastError());
        return 1;
    }
    return 0;
}
