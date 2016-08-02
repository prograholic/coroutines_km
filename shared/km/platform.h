#pragma once

#include <wdm.h>

#define assert(x) ASSERT(x)

#define CORO_DECLARE_NTSTATUS_VALUE(severity, facility, code) ((severity << 30) | (facility << 16) | code)

#define CORO_NO_EXCEPTIONS

namespace std_emu
{

enum class errc
{
    // **** Available SYSTEM error codes ****
    coro_invalid_shared_state = CORO_DECLARE_NTSTATUS_VALUE(STATUS_SEVERITY_ERROR, FACILITY_MAXIMUM_VALUE, 344),


    success = STATUS_SUCCESS,

    not_enough_memory = STATUS_NO_MEMORY,

    invalid_parameter = STATUS_INVALID_PARAMETER,
};

inline
errc GetErrorCodeFromNtStatus(NTSTATUS status)
{
    return static_cast<errc>(status);
}


struct DefaultErrorCodeTraits
{
    typedef NTSTATUS value_type;

    static bool IsError(value_type status)
    {
        return !NT_SUCCESS(status);
    }
};


struct DefaultPlatformEvent
{
    DefaultPlatformEvent()
        : m_event()
    {
        ::KeInitializeEvent(&m_event, NotificationEvent, FALSE);
    }

    DefaultPlatformEvent(const DefaultPlatformEvent& ) = delete;
    DefaultPlatformEvent& operator=(const DefaultPlatformEvent& ) = delete;
    DefaultPlatformEvent(DefaultPlatformEvent&& other) = delete;
    DefaultPlatformEvent& operator=(DefaultPlatformEvent&& other) = delete;

    ~DefaultPlatformEvent()
    {
        ::KeClearEvent(&m_event);
    }


    bool IsValid() const
    {
        return true;
    }

    errc Initialize()
    {
        return errc::success;
    }

    errc Wait()
    {
        auto status = ::KeWaitForSingleObject(&m_event, UserRequest, KernelMode, FALSE, nullptr);
        if (status != STATUS_SUCCESS)
        {
            return std_emu::GetErrorCodeFromNtStatus(status);
        }

        return errc::success;
    }

    errc Notify()
    {
        ::KeSetEvent(&m_event, IO_NO_INCREMENT, FALSE);

        return errc::success;
    }

private:
    KEVENT m_event;
};

} // namespace std_emu
