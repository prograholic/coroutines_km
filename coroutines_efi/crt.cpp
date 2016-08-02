#include <libefi.h>

#include <std_emu.h>

EFI_SYSTEM_TABLE* gST = NULL;
EFI_BOOT_SERVICES* gBS = NULL;
EFI_RUNTIME_SERVICES* gRT = NULL;
EFI_HANDLE gImageHandle = NULL;

void Print(const wchar_t* msg)
{
    gST->ConOut->OutputString(gST->ConOut, (CHAR16*)msg);
}


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

void __cdecl operator delete(void* location, size_t)
{
    if (location)
    {
        gBS->FreePool(location);
    }
}

void * __cdecl operator new(size_t _Size, struct std::nothrow_t const &)
{
    void* memoryLocation = nullptr;
    auto status = gBS->AllocatePool(EfiBootServicesData, _Size, &memoryLocation);
    if (EFI_ERROR(status))
    {
        return nullptr;
    }

    return memoryLocation;
}

namespace std
{
nothrow_t const nothrow;
} // namespace std
