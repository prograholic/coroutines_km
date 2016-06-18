#include "common.h"


#include <experimental/resumable>

#include <future>

#include "future_promise.h"

namespace has_exceptions
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
        CALL_SPY();
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
        CALL_SPY();
        return m_value;
    }

private:
    HANDLE m_handle;
    blob_t m_data;
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




//std::future<void>
coro::future<void>
WriteAndTrace()
{
    CALL_SPY();
    await WriteAsync();
    
    auto bytesWritten = await WriteAsync();
    TRACE() << "WOW!!! we have result (bytes written): " << bytesWritten << std::endl;
}

} // namespace has_exceptions

void TryCoroWaiter()
{
    using namespace has_exceptions;

    CALL_SPY();
    try
    {
        CALL_SPY();
        auto&& res = WriteAndTrace();
        CALL_SPY();

        auto err = res.get_value();

        if (err)
        {
            TRACE() << "fail with error: " << err.message() << std::endl;
        }
        else
        {
            TRACE() << "finished" << std::endl;
        }


        CALL_SPY();
    }
    catch (const error_info_t& e)
    {
        TRACE() << e.first << ": " << e.second << std::endl;
    }

    CALL_SPY();
}