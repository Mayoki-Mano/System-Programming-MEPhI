#include <ntddk.h>

VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
    // Этот код выполняется при выгрузке драйвера
    DbgPrint("Goodbye, world!\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    // Этот код выполняется при загрузке драйвера
    DbgPrint("Hello, world!\n");

    // Устанавливаем функцию выгрузки
    DriverObject->DriverUnload = DriverUnload;

    return STATUS_SUCCESS;
}