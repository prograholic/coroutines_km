#pragma once


#include <libefi.h>

#define CORO_NO_EXCEPTIONS

#define assert(x) ASSERT(x)

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
        //::KeInitializeEvent(&m_event, NotificationEvent, FALSE);
    }

    DefaultPlatformEvent(const DefaultPlatformEvent& ) = delete;
    DefaultPlatformEvent& operator=(const DefaultPlatformEvent& ) = delete;
    DefaultPlatformEvent(DefaultPlatformEvent&& other) = delete;
    DefaultPlatformEvent& operator=(DefaultPlatformEvent&& other) = delete;

    ~DefaultPlatformEvent()
    {
        gBS->CloseEvent(m_event);
    }


    bool IsValid() const
    {
        return m_event != nullptr;
    }

    errc Initialize()
    {
        return GetErrorCodeFromEfiStatus(gBS->CreateEvent(EVT_NOTIFY_WAIT, TPL_CALLBACK, EventHandler, nullptr, &m_event));
    }

    errc Wait()
    {
        UINTN eventId = 0;
        return GetErrorCodeFromEfiStatus(gBS->WaitForEvent(1, &m_event, &eventId));
    }

    errc Notify()
    {
        return GetErrorCodeFromEfiStatus(gBS->SignalEvent(m_event));
    }

private:
    EFI_EVENT m_event;


    static void EFIAPI EventHandler(EFI_EVENT event, void* context)
    {
    }
};

} // namespace std_emu