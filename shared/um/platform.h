#pragma once


#include <Windows.h>

#include <cassert>

#if !defined(_CPPUNWIND)
#define CORO_NO_EXCEPTIONS
#endif /* _CPPUNWIND */

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

inline
errc GetErrorCodeFromWindowsResult(DWORD result)
{
    return static_cast<errc>(result);
}

inline
errc GetLastErrorCode()
{
    return GetErrorCodeFromWindowsResult(::GetLastError());
}


struct HandleGuard
{
    HandleGuard()
        : m_handle(nullptr)
    {
    }

    explicit HandleGuard(HANDLE handle)
        : m_handle(handle)
    {
    }

    HandleGuard(const HandleGuard&) = delete;
    HandleGuard& operator=(const HandleGuard&) = delete;

    HandleGuard(HandleGuard&& other)
        : m_handle(other.m_handle)
    {
        other.m_handle = nullptr;
    }

    HandleGuard& operator=(HandleGuard&& other)
    {
        if (&other != this)
        {
            m_handle = other.m_handle;
            other.m_handle = nullptr;
        }
        
        return *this;
    }

    HandleGuard& operator=(HANDLE handle)
    {
        if (handle != m_handle)
        {
            Destroy();
            m_handle = handle;
        }

        return *this;
    }


    ~HandleGuard()
    {
        Destroy();
    }


    HANDLE get() const
    {
        return m_handle;
    }

    bool valid() const
    {
        return m_handle && (m_handle != INVALID_HANDLE_VALUE);
    }

private:
    HANDLE m_handle;

    void Destroy()
    {
        if (valid())
        {
            ::CloseHandle(m_handle);
        }
    }
};



struct DefaultPlatformEvent
{
    DefaultPlatformEvent()
        : m_event()
    {
    }

    bool IsValid() const
    {
        return m_event.valid();
    }

    errc Initialize()
    {
        m_event = ::CreateEvent(nullptr, true, true, nullptr);
        
        return GetLastErrorCode();
    }

    errc Wait()
    {
        for ( ; ; )
        {
            auto status = WaitForSingleObjectEx(m_event.get(), INFINITE, true);
            switch (status)
            {
            case WAIT_IO_COMPLETION:
                // go to next iteration
                continue;
                break;

            case WAIT_ABANDONED:
                return GetErrorCodeFromWindowsResult(ERROR_ABANDONED_WAIT_0);

            case WAIT_OBJECT_0:
                return errc::success;

            case WAIT_TIMEOUT:
                return GetErrorCodeFromWindowsResult(ERROR_WAIT_1);

            default:
                return GetLastErrorCode();
            }
        }
    }

    errc Notify()
    {
        auto success = ::SetEvent(m_event.get());
        if (!success)
        {
            return GetLastErrorCode();
        }

        return errc::success;
    }

private:
    HandleGuard m_event;
};

} // namespace std_emu
