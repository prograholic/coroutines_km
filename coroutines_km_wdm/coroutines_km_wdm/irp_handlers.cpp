#include "irp_handlers.h"

#include <future_promise.h>


void * __cdecl operator new(unsigned int)
{
    return nullptr;
}

void __cdecl operator delete(void *,unsigned int)
{

}

_Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(_Size)
void * __cdecl operator new(size_t _Size, struct std::nothrow_t const &)
{
    (void)_Size;
    return nullptr;
}

namespace std
{
nothrow_t const nothrow;
}






NTSTATUS DoSomeAsyncShit(void*, void*)
{
    return STATUS_SUCCESS;
}

namespace nx
{


class AwaiterBase
{
public:
    typedef coro::promise<void> promise_type;

    AwaiterBase()
        : m_coroutineHandle()
    {
    }

protected:
    std::experimental::coroutine_handle<promise_type> m_coroutineHandle;

    static AwaiterBase* GetAwaiterBase(void* context)
    {
        return static_cast<AwaiterBase*>(context);
    }

    static void OnWrite(NTSTATUS status, void* context)
    {
        AwaiterBase* me = GetAwaiterBase(context);

        me->m_coroutineHandle.promise().set_error(std_emu::GetErrorCodeFromNtStatus(status));

        me->m_coroutineHandle.resume();
    }
};

struct WriteAwaiter : AwaiterBase
{
    WriteAwaiter()
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
            m_coroutineHandle.destroy();
            return;
        }

        auto status = ::DoSomeAsyncShit(this, OnWrite);
        if (!NT_SUCCESS(status))
        {
            m_coroutineHandle.promise().set_error(std_emu::GetErrorCodeFromNtStatus(status));
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
    __await WriteAwaiter{};

    //TRACE() << "WOW!!! we have result (bytes written): " << bytesWritten << std::endl;
}


std_emu::error_code Call()
{
    auto future = AsyncOperation();

    auto err = future.get_value();
    return err;
}


}





_Dispatch_type_(IRP_MJ_CREATE)
NTSTATUS CreateDevice(_In_ DEVICE_OBJECT* /* deviceObject */, _Inout_ IRP* /* irp */)
{
    //return nx::Call().value();

    return STATUS_SUCCESS;
}

_Dispatch_type_(IRP_MJ_CLOSE)
NTSTATUS CloseDevice(_In_ DEVICE_OBJECT* /* deviceObject */, _Inout_ IRP* /* irp */)
{
    return STATUS_SUCCESS;
}
