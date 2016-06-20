#pragma once

#if defined (_KERNEL_MODE)

#include <wdm.h>

#define assert(x) ASSERT(x)

#define CORO_DECLARE_NTSTATUS_VALUE(severity, facility, code) ((severity << 30) | (facility << 16) | code)

namespace std_emu
{

enum class errc
{
    // **** Available SYSTEM error codes ****
    coro_invalid_shared_state = CORO_DECLARE_NTSTATUS_VALUE(STATUS_SEVERITY_ERROR, FACILITY_MAXIMUM_VALUE, 344),


    success = STATUS_SUCCESS,

    not_enough_memory = STATUS_NO_MEMORY
};

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

private:
    KEVENT m_event;
};

} // namespace std_emu

#elif defined(WIN32) || defined(_WIN32)

#include <Windows.h>

#include <cassert>

namespace std_emu
{

enum class errc
{
    // **** Available SYSTEM error codes ****
    coro_invalid_shared_state = 344,


    success = ERROR_SUCCESS,

    not_enough_memory = ERROR_NOT_ENOUGH_MEMORY
};



struct DefaultErrorCodeTraits
{
    typedef DWORD value_type;

    static bool IsError(value_type val)
    {
        return val != ERROR_SUCCESS;
    }
};

errc GetErrorCodeFromWindowsResult(DWORD result)
{
    return static_cast<errc>(result);
}

errc GetLastErrorCode()
{
    return GetErrorCodeFromWindowsResult(::GetLastError());
}



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
            ::CloseHandle(m_event);
        }
    }


    bool IsValid() const
    {
        return m_event != nullptr;
    }

    errc Initialize()
    {
        m_event = ::CreateEvent(nullptr, true, true, nullptr);
        
        return std_emu::GetLastErrorCode();
    }

    errc Wait()
    {
        auto status = WaitForSingleObjectEx(m_event, INFINITE, true);
        if (status != WAIT_IO_COMPLETION)
        {
            return std_emu::GetLastErrorCode();
        }

        return errc::success;
    }

private:
    HANDLE m_event;
};

} // namespace std_emu

#else

#error Unsupported platform

#endif

