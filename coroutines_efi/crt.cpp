#include <libefi.h>

EFI_SYSTEM_TABLE* gST = NULL;
EFI_BOOT_SERVICES* gBS = NULL;
EFI_RUNTIME_SERVICES* gRT = NULL;
EFI_HANDLE gImageHandle = NULL;

// Image entry point to set all global vars
EFI_STATUS EFIAPI LibEfiEntry(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE* systemTable)
{
    EFI_STATUS status = EFI_SUCCESS;

	if (!imageHandle || !systemTable)
	{
		return EFI_INVALID_PARAMETER;
	}

	gImageHandle = imageHandle;
	gST = systemTable;
	gBS = systemTable->BootServices;
	gRT = systemTable->RuntimeServices;

	status = EfiEntry(imageHandle, systemTable);
	return status;
}