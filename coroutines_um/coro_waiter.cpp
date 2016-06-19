#include "common.h"


#include "future_promise.h"

namespace no_exceptions
{


class AwaiterBase : private OVERLAPPED
{
public:

    typedef DWORD value_type;

    typedef coro::promise<void> promise_type;

    AwaiterBase()
        : OVERLAPPED()
        , m_coroutineHandle()
        , m_value(0)
    {
    }

protected:
    std::experimental::coroutine_handle<promise_type> m_coroutineHandle;
    value_type m_value;

    OVERLAPPED* GetOverlapped()
    {
        return static_cast<OVERLAPPED*>(this);
    }

    static AwaiterBase* GetAwaiterBase(OVERLAPPED* overlapped)
    {
        return reinterpret_cast<AwaiterBase*>(overlapped);
    }

    static void WINAPI OnWrite(DWORD error, DWORD bytesWritten, LPOVERLAPPED overlapped)
    {
        AwaiterBase* me = GetAwaiterBase(overlapped);

        me->m_coroutineHandle.promise().set_error(coro::detail::GetErrorCodeFromWindowsResult(error));
        me->m_value = bytesWritten;

        me->m_coroutineHandle.resume();
    }
};

struct WriteAwaiter : AwaiterBase
{
    explicit WriteAwaiter(HANDLE handle)
        : AwaiterBase()
        , m_handle(handle)
        , m_data(DataSize, 'b')
    {
    }

    bool await_ready()
    {
        return false;
    }

    void await_suspend(std::experimental::coroutine_handle<promise_type> coroutineHandle)
    {
        m_coroutineHandle = coroutineHandle;

        if (!m_coroutineHandle.promise().valid())
        {
            m_coroutineHandle.destroy();
            return;
        }

        auto success = ::WriteFileEx(m_handle, m_data.data(), m_data.size(), GetOverlapped(), OnWrite);
        if (!success)
        {
            m_coroutineHandle.promise().set_error(coro::detail::GetLastErrorCode());
            m_coroutineHandle.destroy();
            return;
        }
    }

    value_type await_resume()
    {
        return m_value;
    }

private:
    HANDLE m_handle;
    blob_t m_data;
};


coro::future<void>
WriteAndTrace(HANDLE file)
{
    auto bytesWritten = await WriteAwaiter{file};

    TRACE() << "WOW!!! we have result (bytes written): " << bytesWritten << std::endl;
}

} // namespace no_exceptions

void TryCoroWaiter()
{
    struct HandleDeleter
    {
        void operator()(HANDLE handle)
        {
            if (handle != INVALID_HANDLE_VALUE)
            {
                ::CloseHandle(handle);
            }
        }
    };
    

    HANDLE file = ::CreateFile(TEXT("2.txt"),
                               GENERIC_WRITE,
                               FILE_SHARE_READ,
                               nullptr,
                               CREATE_ALWAYS,
                               FILE_FLAG_OVERLAPPED,
                               nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        std::error_code err = coro::detail::GetLastErrorCode();
        TRACE() << "CreateFile failed:: " << err.message() << std::endl;
        return;
    }

    auto&& res = no_exceptions::WriteAndTrace(file);
    auto err = res.get_value();

    ::CloseHandle(file);

    if (err)
    {
        TRACE() << "fail with error: " << err.message() << std::endl;
    }
    else
    {
        TRACE() << "finished" << std::endl;
    }
}