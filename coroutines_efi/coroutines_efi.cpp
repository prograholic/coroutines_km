#include <future_promise.h>


class TimerAwaiterBase
{
public:
    typedef coro::promise<void> promise_type;

    TimerAwaiterBase()
        : m_coroutineHandle()
        , m_event(nullptr)
    {
        CORO_TRACE_SIMPLE("TimerAwaiterBase::ctor");
    }

    ~TimerAwaiterBase()
    {
        CORO_TRACE_SIMPLE("TimerAwaiterBase::dtor");
        if (m_event)
        {
            gBS->CloseEvent(m_event);
        }
    }

protected:
    std::experimental::coroutine_handle<promise_type> m_coroutineHandle;
    EFI_EVENT m_event;

    static TimerAwaiterBase* GetAwaiterBase(void* ctx)
    {
        return static_cast<TimerAwaiterBase*>(ctx);
    }

    static void EFIAPI OnTimer(EFI_EVENT event, void* ctx)
    {
        CORO_TRACE_SIMPLE("TimerAwaiterBase::OnTimer(1)");

        TimerAwaiterBase* me = GetAwaiterBase(ctx);

        me->m_coroutineHandle.promise().set_error(std_emu::errc::success);
        me->m_coroutineHandle.resume();

        CORO_TRACE_SIMPLE("TimerAwaiterBase::OnTimer(2)");
    }
};


struct TimerAwaiter : public TimerAwaiterBase
{
    bool await_ready()
    {
        CORO_TRACE_SIMPLE("TimerAwaiter::await_ready");
        return false;
    }

    void await_suspend(std::experimental::coroutine_handle<promise_type> coroutineHandle)
    {
        CORO_TRACE_SIMPLE("TimerAwaiter::await_suspend(0)");
        m_coroutineHandle = coroutineHandle;

        CORO_TRACE_SIMPLE("TimerAwaiter::await_suspend(1)");

        if (!m_coroutineHandle.promise().valid())
        {
            CORO_TRACE_SIMPLE("TimerAwaiter::await_suspend: promise invalid");
            m_coroutineHandle.destroy();
            return;
        }

        EFI_STATUS status = EFI_SUCCESS;
        EFI_EVENT event = nullptr;
        do
        {
            CORO_TRACE_SIMPLE("TimerAwaiter::await_suspend: creating event...");
            status = gBS->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, TimerAwaiterBase::OnTimer, this, &event);
            if (EFI_ERROR(status))
            {
                CORO_TRACE_SIMPLE("TimerAwaiter::await_suspend: gBS->CreateEvent failed");
                break;
            }

            CORO_TRACE_SIMPLE("TimerAwaiter::await_suspend: setting timer...");
            status = gBS->SetTimer(event, TimerRelative, EFI_TIMER_PERIOD_MILLISECONDS(10000));
            if (EFI_ERROR(status))
            {
                CORO_TRACE_SIMPLE("TimerAwaiter::await_suspend: gBS->SetTimer failed");
                gBS->CloseEvent(event);
                break;
            }

        } while(0);
        
        
        if (EFI_ERROR(status))
        {
            CORO_TRACE_SIMPLE("TimerAwaiter::await_suspend: some error");
            m_coroutineHandle.promise().set_error(std_emu::GetErrorCodeFromEfiStatus(status));
            m_coroutineHandle.destroy();
            return;
        }
        m_event = event;

        CORO_TRACE_SIMPLE("TimerAwaiter::await_suspend(2)");
    }

    void await_resume()
    {
        CORO_TRACE_SIMPLE("TimerAwaiter::await_resume");
    }
};


coro::future<void>
AsyncOperation()
{
    CORO_TRACE_SIMPLE("AsyncOperation(1)");
    co_await TimerAwaiter{};
    CORO_TRACE_SIMPLE("AsyncOperation(2)");
}


std_emu::error_code Call()
{
    CORO_TRACE_SIMPLE("Call(1)");
    auto future = AsyncOperation();
    CORO_TRACE_SIMPLE("Call(2)");

    auto err = future.get_value();
    CORO_TRACE_SIMPLE("Call(3)");
    return err;
}


extern "C" EFI_STATUS EFIAPI EfiEntry(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE* systemTable)
{
    CORO_TRACE_SIMPLE("EfiEntry");
    return Call().value();
}
