#include <wdm.h>

#include "irp_handlers.h"

#define CORO_NT_DEVICE_NAME      L"\\Device\\CoroTest"
#define CORO_DOS_DEVICE_NAME     L"\\DosDevices\\CoroTest"



void CoroDriverUnload(PDRIVER_OBJECT driverObject)
{
    UNICODE_STRING uniWin32NameString;
    RtlInitUnicodeString(&uniWin32NameString, CORO_DOS_DEVICE_NAME);

    IoDeleteSymbolicLink( &uniWin32NameString );

    if (driverObject->DeviceObject)
    {
        IoDeleteDevice(driverObject->DeviceObject);
    }
}


extern "C" DRIVER_INITIALIZE DriverEntry;

extern "C"
NTSTATUS DriverEntry(IN PDRIVER_OBJECT driverObject,
                     IN PUNICODE_STRING /* registryPath */)
{
    PDEVICE_OBJECT  deviceObject = nullptr;
    
    UNICODE_STRING  ntUnicodeString;
    RtlInitUnicodeString(&ntUnicodeString, CORO_NT_DEVICE_NAME);

    auto status = IoCreateDevice(driverObject,
                                 0,
                                 &ntUnicodeString,
                                 FILE_DEVICE_UNKNOWN,
                                 FILE_DEVICE_SECURE_OPEN,
                                 FALSE,
                                 &deviceObject);
    if (!NT_SUCCESS(status))
    {
        DbgPrint("Couldn't create the device object: %x \n", status);
        return status;
    }

    driverObject->MajorFunction[IRP_MJ_CREATE] = CreateDevice;
    driverObject->MajorFunction[IRP_MJ_CLOSE] = CloseDevice;
    //driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SDmaDeviceControl;
    driverObject->DriverUnload = CoroDriverUnload;

    UNICODE_STRING ntWin32NameString;
    RtlInitUnicodeString(&ntWin32NameString, CORO_DOS_DEVICE_NAME);

    status = IoCreateSymbolicLink(&ntWin32NameString, &ntUnicodeString);
    if (!NT_SUCCESS(status))
    {
        //
        // Delete everything that this routine has allocated.
        //
        DbgPrint("Couldn't create symbolic link: %X\n", status);
        IoDeleteDevice(deviceObject);
    }


    return status;
}
