#include "irp_handlers.h"

#include <future_promise.h>

namespace nx
{


class SimpleThread
{
public:

    template <typename Function, typename Context>
    static
    std_emu::error_code Start(Function function, Context context)
    {
        struct ThreadContext : public ThreadContextBase
        {
            Function m_function;
            Context m_context;

            ThreadContext(Function function, Context context)
                : ThreadContextBase()
                , m_function(function)
                , m_context(context)
            {
            }

            virtual NTSTATUS Invoke()
            {
                m_function(m_context);
                CORO_TRACE("done");

                return STATUS_SUCCESS;
            }
        };

        ThreadContextBase* threadContext = new(std::nothrow) ThreadContext{function, context};
        if (!threadContext)
        {
            CORO_TRACE("not enough memory");
            return std_emu::errc::not_enough_memory;
        }

        auto err = threadContext->Start();
        if (err)
        {
            CORO_TRACE("start failed");
            delete threadContext;
            return err;
        }

        return std_emu::errc::success;
    }

private:
    
    struct ThreadContextBase
    {
        ThreadContextBase()
            : m_threadHandle(nullptr)
        {
        }

        virtual ~ThreadContextBase()
        {
            if (m_threadHandle)
            {
                ::ZwClose(m_threadHandle);
            }
        }

        virtual NTSTATUS Invoke() = 0;

        std_emu::error_code Start()
        {
            auto status = ::PsCreateSystemThread(&m_threadHandle,
                                                 STANDARD_RIGHTS_ALL,
                                                 nullptr,
                                                 nullptr,
                                                 nullptr,
                                                 ThreadFunction,
                                                 this);

            CORO_TRACE("PsCreateSystemThread: %d", status);
                                                 
            return std_emu::GetErrorCodeFromNtStatus(status);
        }

    private:
        HANDLE m_threadHandle;

        static void ThreadFunction(void* context)
        {
            ThreadContextBase* threadContext = static_cast<ThreadContextBase*>(context);
            assert(threadContext);

            auto status = threadContext->Invoke();
            delete threadContext;

            CORO_TRACE("done");

            ::PsTerminateSystemThread(status);
        }
    };
};



class AwaiterBase
{
public:
    typedef coro::promise<void> promise_type;

    explicit AwaiterBase()
        : m_coroutineHandle()
    {
    }

protected:
    std::experimental::coroutine_handle<promise_type> m_coroutineHandle;

    static AwaiterBase* GetAwaiterBase(void* context)
    {
        return static_cast<AwaiterBase*>(context);
    }

    static void ThreadRoutine(AwaiterBase* me)
    {
        CORO_TRACE("thread routine invoked");

        me->m_coroutineHandle.promise().set_error(std_emu::GetErrorCodeFromNtStatus(STATUS_SUCCESS));
        me->m_coroutineHandle.resume();

        CORO_TRACE("resumed");
    }
};

struct WriteAwaiter : AwaiterBase
{
    explicit WriteAwaiter()
        : AwaiterBase()
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
            CORO_TRACE("promise invalid");
            m_coroutineHandle.destroy();
            return;
        }

        auto err = SimpleThread::Start(ThreadRoutine, this);
        if (err)
        {
            CORO_TRACE("thread start failed: %d", err.value());
            m_coroutineHandle.promise().set_error(err);
            m_coroutineHandle.destroy();
            return;
        }
    }

    void await_resume()
    {
    }
};


coro::future<void>
AsyncOperation()
{
    CORO_TRACE("Starting...\n");

    co_await WriteAwaiter{};
    
    CORO_TRACE("Wow, we`ve finished\n");
}


std_emu::error_code Call()
{
    auto future = AsyncOperation();
    CORO_TRACE("got future");

    auto err = future.get_value();
    CORO_TRACE("got status: %d", err.value());
    return err;
}


}


_Dispatch_type_(IRP_MJ_CREATE)
NTSTATUS CreateDevice(_In_ DEVICE_OBJECT* /* deviceObject */, _Inout_ IRP* irp)
{
    auto err = nx::Call();

    auto status = err.value();

    CORO_TRACE("got status: %d", status);

    irp->IoStatus.Status = status;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);

    return status;
}

_Dispatch_type_(IRP_MJ_CLOSE)
NTSTATUS CloseDevice(_In_ DEVICE_OBJECT* /* deviceObject */, _Inout_ IRP* irp)
{
    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}
