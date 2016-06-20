#pragma once

#include <wdm.h>

_Dispatch_type_(IRP_MJ_CREATE)
NTSTATUS CreateDevice(_In_ DEVICE_OBJECT* deviceObject, _Inout_ IRP* irp);

_Dispatch_type_(IRP_MJ_CLOSE)
NTSTATUS CloseDevice(_In_ DEVICE_OBJECT* deviceObject, _Inout_ IRP* irp);
