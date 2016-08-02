#pragma once


#include <libefi.h>

#define CORO_NO_EXCEPTIONS

extern "C" void __cdecl __debugbreak(void);
#pragma intrinsic(__debugbreak)


void Print(const wchar_t* msg);

#define CORO_TRACE_SIMPLE(x) Print(L ## x "\r\n")

#define assert(x) do {if (!x){CORO_TRACE_SIMPLE(#x); __debugbreak();}} while(0)

namespace std_emu
{

enum class errc : UINTN
{
    // **** Available SYSTEM error codes ****
    coro_invalid_shared_state = EFIERR(100),


    success = EFI_SUCCESS,

    not_enough_memory = EFI_OUT_OF_RESOURCES,

    invalid_parameter = EFI_INVALID_PARAMETER,
};

inline
errc GetErrorCodeFromEfiStatus(EFI_STATUS status)
{
    return static_cast<errc>(status);
}


struct DefaultErrorCodeTraits
{
    typedef EFI_STATUS value_type;

    static bool IsError(value_type status)
    {
        return EFI_ERROR(status);
    }
};


struct DefaultPlatformEvent
{
    DefaultPlatformEvent()
        : m_event(nullptr)
    {
    }

    DefaultPlatformEvent(const DefaultPlatformEvent& ) = delete;
    DefaultPlatformEvent& operator=(const DefaultPlatformEvent& ) = delete;
    
    DefaultPlatformEvent(DefaultPlatformEvent&& other)
        : m_event(other.m_event)
    {
        other.m_event = nullptr;
    }
    DefaultPlatformEvent& operator=(DefaultPlatformEvent&& other)
    {
        if (&other != this)
        {
            m_event = other.m_event;
            other.m_event = nullptr;
        }

        return *this;
    }

    ~DefaultPlatformEvent()
    {
        if (m_event)
        {
            gBS->CloseEvent(m_event);
        }
    }


    bool IsValid() const
    {
        return m_event != nullptr;
    }

    errc Initialize()
    {
        CORO_TRACE_SIMPLE("DefaultPlatformEvent::Initialize(0)");
        auto status = gBS->CreateEvent(0, 0, nullptr, nullptr, &m_event);
        if (EFI_ERROR(status))
        {
            CORO_TRACE_SIMPLE("DefaultPlatformEvent::Initialize failed");
            return GetErrorCodeFromEfiStatus(status);
        }

        CORO_TRACE_SIMPLE("DefaultPlatformEvent::Initialize(1)");
        return std_emu::errc::success;
    }

    errc Wait()
    {
        CORO_TRACE_SIMPLE("DefaultPlatformEvent::Wait(0)");
        UINTN eventId = 0;
        auto status = gBS->WaitForEvent(1, &m_event, &eventId);
        if (EFI_ERROR(status))
        {
            CORO_TRACE_SIMPLE("DefaultPlatformEvent::Wait failed");
            return GetErrorCodeFromEfiStatus(status);
        }

        CORO_TRACE_SIMPLE("DefaultPlatformEvent::Wait(1)");
        return std_emu::errc::success;
    }

    errc Notify()
    {
        CORO_TRACE_SIMPLE("DefaultPlatformEvent::Notify(0)");
        auto status = gBS->SignalEvent(m_event);
        if (EFI_ERROR(status))
        {
            CORO_TRACE_SIMPLE("DefaultPlatformEvent::Notify failed");
            return GetErrorCodeFromEfiStatus(status);
        }

        CORO_TRACE_SIMPLE("DefaultPlatformEvent::Notify(1)");
        return std_emu::errc::success;
    }

private:
    EFI_EVENT m_event;
};

} // namespace std_emu