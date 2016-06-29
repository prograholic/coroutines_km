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

        me->m_value = bytesWritten;

        me->m_coroutineHandle.promise().set_error(std_emu::GetErrorCodeFromWindowsResult(error));

        me->m_coroutineHandle.resume();
    }
};

struct WriteAwaiter : AwaiterBase
{
    explicit WriteAwaiter(HANDLE handle, size_t size, char c)
        : AwaiterBase()
        , m_handle(handle)
        , m_data(size, c)
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

        auto success = ::WriteFileEx(m_handle.get(), m_data.data(), m_data.size(), GetOverlapped(), OnWrite);
        if (!success)
        {
            m_coroutineHandle.promise().set_error(std_emu::GetLastErrorCode());
            m_coroutineHandle.destroy();
            return;
        }
    }

    value_type await_resume()
    {
        return m_value;
    }

private:
    std_emu::HandleGuard m_handle;
    blob_t m_data;
};


coro::future<void>
WriteAndTrace(WriteAwaiter& awaiter2, WriteAwaiter& awaiter3)
{
    auto bytesWritten2 = await awaiter2;
    TRACE() << "WOW!!! we have result2 (bytes written): " << bytesWritten2 << std::endl;

    auto bytesWritten3 = await awaiter3;
    TRACE() << "WOW!!! we have result3 (bytes written): " << bytesWritten3 << std::endl;
}

} // namespace no_exceptions

void TryCoroWaiter()
{
    HANDLE file2 = ::CreateFile(TEXT("2.txt"),
                               GENERIC_WRITE,
                               FILE_SHARE_READ,
                               nullptr,
                               CREATE_ALWAYS,
                               FILE_FLAG_OVERLAPPED,
                               nullptr);
    if (file2 == INVALID_HANDLE_VALUE)
    {
        std_emu::error_code err = std_emu::GetLastErrorCode();
        TRACE() << "CreateFile failed:: " << err.value() << std::endl;
        return;
    }

    no_exceptions::WriteAwaiter awaiter2{file2, 1000, 'b'};

    HANDLE file3 = ::CreateFile(TEXT("3.txt"),
                               GENERIC_WRITE,
                               FILE_SHARE_READ,
                               nullptr,
                               CREATE_ALWAYS,
                               FILE_FLAG_OVERLAPPED,
                               nullptr);
    if (file3 == INVALID_HANDLE_VALUE)
    {
        std_emu::error_code err = std_emu::GetLastErrorCode();
        TRACE() << "CreateFile failed:: " << err.value() << std::endl;
        return;
    }

    no_exceptions::WriteAwaiter awaiter3{file3, 2000, 'c'};



    auto&& res = no_exceptions::WriteAndTrace(awaiter2, awaiter3);
    auto err = res.get_value();

    if (err)
    {
        TRACE() << "fail with error: " << err.value() << std::endl;
    }
    else
    {
        TRACE() << "finished" << std::endl;
    }
}