#include <wdm.h>

#include <wdmsec.h>

#include <driver_connection.h>

#include "irp_handlers.h"


void CoroDriverUnload(PDRIVER_OBJECT driverObject)
{
    UNICODE_STRING uniWin32NameString;
    RtlInitUnicodeString(&uniWin32NameString, CORO_DOS_DEVICE_NAME);

    IoDeleteSymbolicLink(&uniWin32NameString);

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

    auto status = IoCreateDeviceSecure(driverObject,
                                       0,
                                       &ntUnicodeString,
                                       FILE_DEVICE_UNKNOWN,
                                       FILE_DEVICE_SECURE_OPEN,
                                       FALSE,
                                       &SDDL_DEVOBJ_SYS_ALL_ADM_ALL,
                                       nullptr,
                                       &deviceObject);
    if (!NT_SUCCESS(status))
    {
        CORO_TRACE("Couldn't create the device object: %x", status);
        return status;
    }

    driverObject->MajorFunction[IRP_MJ_CREATE] = CreateDevice;
    driverObject->MajorFunction[IRP_MJ_CLOSE] = CloseDevice;
    driverObject->DriverUnload = CoroDriverUnload;

    UNICODE_STRING ntWin32NameString;
    RtlInitUnicodeString(&ntWin32NameString, CORO_DOS_DEVICE_NAME);

    status = IoCreateSymbolicLink(&ntWin32NameString, &ntUnicodeString);
    if (!NT_SUCCESS(status))
    {
        //
        // Delete everything that this routine has allocated.
        //
        CORO_TRACE("Couldn't create symbolic link: %X", status);
        IoDeleteDevice(deviceObject);
    }


    return status;
}
