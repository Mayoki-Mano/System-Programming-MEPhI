#ifndef LAB7_H
#define LAB7_H
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#define SERVICE_NAME "USBMonitorService"
#define SERVICE_DISPLAY_NAME "USB Monitoring Service"
#define PROGRAM_PATH "C:\\Program Files\\USB.exe"


void InstallService();

void RemoveService();

void StartServiceManually();

void StopServiceManually();

void WINAPI USBMonitoringService(DWORD argc, LPSTR *argv);

void WINAPI ServiceCtrlHandler(DWORD dwCtrlCode);
#endif //LAB7_H
