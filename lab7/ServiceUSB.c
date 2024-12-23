#include <windows.h>
#include <stdio.h>
#include "ServiceManager.h"
#include <Cfgmgr32.h>
#include <initguid.h>
#include <Usbiodef.h>
#include <setupapi.h>

SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE hStatus;
CRITICAL_SECTION cs;
#define USB_KEY_PATH "SYSTEM\\CurrentControlSet\\Enum\\"  // Ключ реестра для устройств USB


void LogMessage(const char* message) {
    FILE *logFile;
    fopen_s(&logFile, "C:\\ServiceLog.txt", "a");
    if (logFile != NULL) {
        fprintf(logFile, "%s\n", message);
        fclose(logFile);
    }
}
void replace_hash_with_slash(char *str) {
    if (!str) {
        return;
    }

    while (*str) {
        if (*str == '#') {
            *str = '\\';
        }
        str++;
    }
}

char* find_last_occurrence(const char *str, const char *substr) {
    char *last_occurrence = NULL;
    char *current_occurrence = strstr(str, substr);

    while (current_occurrence) {
        last_occurrence = current_occurrence;
        current_occurrence = strstr(current_occurrence + 1, substr);
    }

    return last_occurrence;
}

char * ParseDeviceKey(const char * deviceSerialNumber) {
    if (!deviceSerialNumber) {
        return NULL;
    }
    const char* result_start = deviceSerialNumber + 4;
    char * result_end = find_last_occurrence(result_start, "#{");
    result_end[0] = '\0';
    size_t result_length = result_end - result_start+1;
    char* result = (char*)malloc(result_length);
    if (!result) {
        return NULL;
    }
    strncpy(result, result_start, result_length-1);
    result[result_length-1] = '\\';
    result[result_length] = '\0';
    char errorMessage[512];
    sprintf_s(errorMessage, sizeof(errorMessage), "Parse key: %s\n", result);
    LogMessage(errorMessage);
    replace_hash_with_slash(result);
    return result;
}

BOOL DisableUsbDevice(LPCTSTR aDeviceInterfaceDbccName) {
    /**
     * @brief Disable device by dbcc_name with SetupAPI
     *
     * @details Similar to Device Manager's "disable" option. To enable again, use Device Manager
     * Warning: some devices can't be blocked (such as certain input devices)
     */
    HDEVINFO deviceInfoSet;
    SP_DEVINFO_DATA deviceInfoData;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;

    // Create a device information set for the specified device interface
    deviceInfoSet = SetupDiCreateDeviceInfoList(NULL, 0);
    if (deviceInfoSet == INVALID_HANDLE_VALUE)
        return FALSE;  // Failed to get the device information set

    // Add "aDeviceInterfaceDbccName" to the device info set
    ZeroMemory(&deviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    if (!SetupDiOpenDeviceInterface(deviceInfoSet, aDeviceInterfaceDbccName, 0, &deviceInterfaceData)) {
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return FALSE;
    }

    // Retrieve the device interface data
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if (!SetupDiEnumDeviceInfo(deviceInfoSet, 0, &deviceInfoData)) {
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return FALSE;  // Failed to enumerate device info
    }

    // Set the device installation parameters
    SP_CLASSINSTALL_HEADER classInstallHeader;
    classInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    classInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;

    SP_PROPCHANGE_PARAMS propChangeParams;
    propChangeParams.ClassInstallHeader = classInstallHeader;
    propChangeParams.StateChange = DICS_DISABLE;
    propChangeParams.Scope = DICS_FLAG_CONFIGSPECIFIC;
    propChangeParams.HwProfile = 0;

    if (!SetupDiSetClassInstallParams(deviceInfoSet, &deviceInfoData, &propChangeParams.ClassInstallHeader, sizeof(SP_PROPCHANGE_PARAMS))) {
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return FALSE;  // Failed to set class install parameters
    }

    // Call the class installer to disable the device
    if (!SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, deviceInfoSet, &deviceInfoData)) {
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return FALSE;  // Failed to call class installer
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return TRUE;
}


void RegisterEventSourceManually() {
    HKEY hKey;
    DWORD typesSupported = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
    char *subkey = "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\USBMonitorService";
    char *eventMessageFile = "advapi32.dll";
    char *logFilePath = "C:\\ServiceLog.txt";
    char * prohibitedSerialNumbers = "8&1f7b8582&15&_&0 ";
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
        sprintf_s(errorMessage, sizeof(errorMessage), "Event source registered successfully.");
        LogMessage(errorMessage);
    } else {
        sprintf_s(errorMessage, sizeof(errorMessage), "Failed to register event source. Error: %lu", GetLastError());
        LogMessage(errorMessage);
    }
}


HANDLE hEventLog=NULL;
void LogEvent(LPCWSTR message, WORD eventType) {
    HANDLE hEventLog = RegisterEventSource(NULL, SERVICE_NAME);

    if (hEventLog == NULL) {
        char errorMessage[256];
        sprintf_s(errorMessage, sizeof(errorMessage), "Failed to register event source. Error: %lu", GetLastError());
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
            ZeroMemory(message, 256);
            char serialNumberAnsi[256];
            WideCharToMultiByte(CP_ACP, 0, serialNumber, -1, serialNumberAnsi, sizeof(serialNumberAnsi), NULL, NULL);
            char * serialNumberEnd = find_last_occurrence(serialNumberAnsi, "}");
            long long serialNumberLen = serialNumberEnd - serialNumberAnsi + 1;
            serialNumberAnsi[serialNumberLen] = '\0';
            ZeroMemory(serialNumberEnd+1, serialNumberLen-1);
            snprintf(message, 256, "NEW DEVICE: %s", serialNumberAnsi);
            LogMessage(message);
            HKEY hKey;
            char errorMessage[256];
            char *subkey = "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\USBMonitorService";
            RegCreateKeyEx(HKEY_LOCAL_MACHINE, subkey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hKey, NULL);
            char ProhibitedSerialNumbers[1024];
            DWORD dwSize = sizeof(ProhibitedSerialNumbers);
            RegQueryValueExA(hKey,
                                "ProhibitedSerialNumbers",
                                0,
                                NULL,
                                (BYTE *)ProhibitedSerialNumbers,
                                &dwSize);
            sprintf_s(errorMessage, sizeof(errorMessage), "ProhibitedSerialNumbers: %s", ProhibitedSerialNumbers);
            LogMessage(errorMessage);
            char buffer[1024];
            strncpy(buffer,ProhibitedSerialNumbers,sizeof(buffer)-1);
            buffer[sizeof(buffer)-1]='\0';
            char * token = strtok(buffer, " ");
            char * path_key = ParseDeviceKey(serialNumberAnsi);
            if (path_key == NULL) {
                LogMessage("Could not parse device key.");
                return CR_FAILURE;
            }
            while (token!=NULL){

                snprintf(message, 256, "token: %s, path_key: %s", token,path_key);
                LogMessage(message);
                if (strstr(path_key, token) != NULL) {
                    snprintf(message, 256, "TRY TO BLOCK: %s, path_key: %s", serialNumber,path_key);
                    LogMessage(message);
                    WideCharToMultiByte(CP_ACP, 0, serialNumber, -1, serialNumberAnsi, sizeof(serialNumberAnsi), NULL, NULL);
                    serialNumberEnd = find_last_occurrence(serialNumberAnsi, "}");
                    serialNumberLen = serialNumberEnd - serialNumberAnsi + 1;
                    serialNumberAnsi[serialNumberLen] = '\0';
                    ZeroMemory(serialNumberEnd+1, serialNumberLen-1);
                    snprintf(message, 256, "Blocking device: %s", serialNumberAnsi);
                    LogMessage(message);
                    if (DisableUsbDevice(serialNumberAnsi)) {
                        LogMessage("Device successfully disabled.");
                    } else {
                        LogMessage("Failed to disable device.");
                    }
                    free(path_key);
                    return CR_SUCCESS;
                }else {
                    token= strtok(NULL," ");
                }
            }
            free(path_key);
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
