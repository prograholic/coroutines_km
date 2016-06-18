#include "common.h"


#include <experimental/resumable>

#include <system_error>

#include <future>
#if 0
namespace no_exception
{

struct nx_promise;

struct nx_future;



////////////////////////////////////////////////////////////////////////////////


struct nx_promise
{
    typedef std::error_code return_value;

    nx_promise()
    {
        CALL_SPY();
    }

    nx_promise(const nx_promise& ) = delete;
    nx_promise& operator=(const nx_promise& ) = delete;

    ~nx_promise()
    {
        CALL_SPY();
    }

    std::error_code Initialize();

public:
    std::experimental::suspend_never initial_suspend()
    {
        CALL_SPY();
        return {};
    }

    nx_future get_return_object();

    std::experimental::suspend_never final_suspend()
    {
        CALL_SPY();
        return {};
    }


public:
    void SetValue(const std::error_code& ec)
    {
        CALL_SPY();
        m_error = ec;
    }


private:
    HANDLE m_event;
    std::error_code m_error;
};


////////////////////////////////////////////////////////////////////////////////

struct nx_future
{
    typedef nx_promise promise_type;

    nx_future()
        : m_event(nullptr)
        , m_error()
    {
    }

    explicit nx_future(const std::error_code& ec)
        : m_event(nullptr)
        , m_error(ec)
    {
    }

    nx_future(HANDLE event, const std::error_code& ec)
        : m_event(event)
        , m_error(ec)
    {
        CALL_SPY();
    }

    ~nx_future()
    {
        CALL_SPY();
        if (m_event)
        {
            ::CloseHandle(m_event);
        }
    }

    nx_future(nx_future&& other) = default;
    nx_future& operator=(nx_future&& other) = default;

    std::error_code GetValue()
    {
        CALL_SPY();
        if (m_event)
        {
            DWORD waitStatus = ::WaitForSingleObjectEx(m_event, INFINITE, true);
            if (waitStatus != WAIT_IO_COMPLETION)
            {
                return GetLastErrorCode();
            }
        }
        CALL_SPY();

        return m_error;
    }
private:
    HANDLE m_event;
    std::error_code m_error;
};

////////////////////////////////////////////////////////////////////////////////


nx_future nx_promise::get_return_object()
{
    CALL_SPY();
    return nx_future{m_event, m_error};
}


////////////////////////////////////////////////////////////////////////////////


class AwaiterBase : private OVERLAPPED
{
public:

    AwaiterBase()
        : OVERLAPPED()
        , m_coroutineHandle()
    {
        CALL_SPY();
    }

protected:
    std::experimental::coroutine_handle<nx_promise> m_coroutineHandle;

    OVERLAPPED* GetOverlapped()
    {
        return static_cast<OVERLAPPED*>(this);
    }

    static AwaiterBase* GetAwaiterBase(OVERLAPPED* overlapped)
    {
        return reinterpret_cast<AwaiterBase*>(overlapped);
    }

    static void WINAPI OnWrite(DWORD error, DWORD bytesReaded, LPOVERLAPPED overlapped)
    {
        CALL_SPY();
        AwaiterBase* me = GetAwaiterBase(overlapped);

        
        CALL_SPY();
        auto res = GetErrorCodeFromWindowsResult(error);

        me->m_coroutineHandle.promise().SetValue(res);       
        CALL_SPY();

        me->m_coroutineHandle();

        CALL_SPY();
    }
};

struct WriteAwaiter : AwaiterBase
{
    explicit WriteAwaiter(HANDLE handle)
        : AwaiterBase()
        , m_handle(handle)
        , m_data(DataSize, 'b')
    {
        CALL_SPY();
    }

    bool await_ready()
    {
        CALL_SPY();
        return false;
    }

    void await_suspend(std::experimental::coroutine_handle<nx_promise> coroutineHandle)
    {
        CALL_SPY();
        m_coroutineHandle = coroutineHandle;

        auto res = m_coroutineHandle.promise().Initialize();
        if (!res)
        {
            m_error = res;
            m_coroutineHandle.destroy();
            return;
        }

        CALL_SPY();
        auto writeFileStatus = ::WriteFileEx(m_handle, m_data.data(), m_data.size(), GetOverlapped(), OnWrite);
        if (!writeFileStatus)
        {
            m_error = GetLastErrorCode();
            m_coroutineHandle.destroy();
            return;
        }

        CALL_SPY();
    }

    std::error_code await_resume()
    {
        CALL_SPY();
        return m_error;
    }

private:
    HANDLE m_handle;
    blob_t m_data;
    std::error_code m_error;
};


WriteAwaiter WriteAsync()
{
    CALL_SPY();
    HANDLE file = ::CreateFile(TEXT("2.txt"),
                               GENERIC_WRITE,
                               FILE_SHARE_READ,
                               nullptr,
                               CREATE_ALWAYS,
                               FILE_FLAG_OVERLAPPED,
                               nullptr);
    EXPECT(file != INVALID_HANDLE_VALUE);


    return WriteAwaiter{file};
}



#if 0
//std::future<void>
nx_future
TryCoroWaiter2()
{
    CALL_SPY();
    auto err = await WriteAsync();
    if (err)
    {
        TRACE() << "WriteAsync failed: " << err.message() << std::endl;
    //    return err;
    }
    CALL_SPY();

    //return std::error_code();
}
#endif //0
} // namespace no_exception

#endif //0
void TryCoroWaiterNoexcept()
{
#if 0
    using namespace no_exception;

    CALL_SPY();
    auto&& res = TryCoroWaiter2();
    CALL_SPY();
    
    TRACE() << "TryCoroWaiter2 finished with: " << res.GetValue().message() << std::endl;
#endif //0
}