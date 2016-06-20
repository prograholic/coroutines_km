#include "irp_handlers.h"

_Dispatch_type_(IRP_MJ_CREATE)
NTSTATUS CreateDevice(_In_ DEVICE_OBJECT* /* deviceObject */, _Inout_ IRP* /* irp */)
{
    return STATUS_SUCCESS;
}

_Dispatch_type_(IRP_MJ_CLOSE)
NTSTATUS CloseDevice(_In_ DEVICE_OBJECT* /* deviceObject */, _Inout_ IRP* /* irp */)
{
    return STATUS_SUCCESS;
}
