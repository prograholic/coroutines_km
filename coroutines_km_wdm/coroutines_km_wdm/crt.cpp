#include <wdm.h>

#include <std_emu.h>

namespace
{
constexpr ULONG CrtPoolTag = 'CRTP';
} // namespace

void __cdecl operator delete(void* location, size_t)
{
    if (location)
    {
        ::ExFreePoolWithTag(location, CrtPoolTag);
    }
}

void * __cdecl operator new(size_t _Size, struct std::nothrow_t const &)
{
    return ::ExAllocatePoolWithTag(PagedPool, _Size, CrtPoolTag);
}

namespace std
{
nothrow_t const nothrow;
} // namespace std
