#pragma once

#include <wdm.h>

#define CORO_PRETTY_PRINT(firstArg, ...) "coro_km(%s:%d): " firstArg "\n", __FUNCTION__, __LINE__, __VA_ARGS__

#define CORO_TRACE(...) KdPrintEx (( DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, CORO_PRETTY_PRINT(__VA_ARGS__)))

_Dispatch_type_(IRP_MJ_CREATE)
NTSTATUS CreateDevice(_In_ DEVICE_OBJECT* deviceObject, _Inout_ IRP* irp);

_Dispatch_type_(IRP_MJ_CLOSE)
NTSTATUS CloseDevice(_In_ DEVICE_OBJECT* deviceObject, _Inout_ IRP* irp);
