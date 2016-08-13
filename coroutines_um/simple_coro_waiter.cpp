#include "common.h"

#include "future_promise.h"

namespace simple {

struct WinApiAwaitable : AsyncWriterBase {
    WinApiAwaitable(HANDLE handle, const char* data, size_t size);

    bool await_ready();

    void await_suspend(std::experimental::coroutine_handle<coro::promise<void>> coroutineHandle);

    DWORD await_resume();

private:
    std::experimental::coroutine_handle<coro::promise<void>> m_coroutineHandle;
    const void* m_data;
    size_t m_size;
    DWORD m_bytesWritten;

    virtual void WriteFinished(DWORD bytesWritten);
};


WinApiAwaitable::WinApiAwaitable(HANDLE handle, const char* data, size_t size)
    : AsyncWriterBase(handle)
    , m_coroutineHandle()
    , m_data(data)
    , m_size(size) {
    }

bool WinApiAwaitable::await_ready() {
    return false;
}

void WinApiAwaitable::await_suspend(std::experimental::coroutine_handle<coro::promise<void>> coroutineHandle) {
    m_coroutineHandle = coroutineHandle;
    StartAsyncWrite(m_data, m_size);
}

DWORD WinApiAwaitable::await_resume() {
    return m_bytesWritten;
}

void WinApiAwaitable::WriteFinished(DWORD bytesWritten) {
    m_bytesWritten = bytesWritten;
    m_coroutineHandle.resume();
}


coro::future<void> WriteAsync(HANDLE handle, std::string message) {
    std::cout << "starting ..." << std::endl;
    auto bytesWritten = co_await WinApiAwaitable{handle, message.data(), message.size()};
    std::cout << "done, bytes written: " << bytesWritten << std::endl;
}

#if defined(CORO_PRESENTATION_MODE)

typedef void(__cdecl *CoroResumeFunction)(void *);

template <typename PromiseType>
struct coro_frame_header
{
    CoroResumeFunction resumeAddr;
    PromiseType promise;

    explicit coro_frame_header(CoroResumeFunction fn)
        : resumeAddr(fn)
        , promise() {
    }
};

template <typename PromiseType>
struct coroutine_handle {
    coro_frame_header<PromiseType>* m_frame

    explicit coroutine_handle(coro_frame_header<PromiseType> frame)
        : m_frame(frame) {
    }

    void resume() {
        m_frame->resumeAddr(m_frame);
    }

    void destroy();
};

struct FunctionContext {
    coro_frame_header<coro::promise<void>> frame_header;

    HANDLE handle;
    std::string message;

    WinApiAwaitable awaitable;

    FunctionContext(HANDLE h, std::string m, CoroResumeFunction fn)
        : frame_header(fn), handle(h), message(std::move(m)), awaitable(handle, message.data(), message.size()) {
    }
};

void WriteAsync_ResumeCoro2(FunctionContext* ctx) {
    DWORD bytesWritten = ctx->awaitable.await_resume();
    std::cout << "done, bytes written: " << bytesWritten << std::endl;
    delete ctx;
}

coro::future<void> WriteAsync2(HANDLE handle, std::string message) {
    FunctionContext* ctx = new FunctionContext{handle, std::move(message), WriteAsync_ResumeCoro2};

    auto res = ctx->coroutineHandle.promise.get_future();
    
    std::cout << "starting ..." << std::endl;
    ctx->awaitable.await_suspend(ctx->coroutineHandle);

    return res;
}


coro::future<void> WriteAsync3(HANDLE handle, std::string message) {
    auto* ctx = new (std::nothrow) FunctionContext{handle, std::move(message), WriteAsync_ResumeCoro2};
    typedef std::experimental::coroutine_traits<coro::future<void>, HANDLE, std::string> coro_trait;
    if (!ctx) {
        return coro_trait::get_return_object_on_allocation_failure();
    }

    auto res = ctx->coroutineHandle.promise.get_future();
    
    std::cout << "starting ..." << std::endl;
    ctx->awaitable.await_suspend(ctx->coroutineHandle);

    return res;
}

#endif /* CORO_PRESENTATION_MODE */

} // namespace simple

using namespace simple;

void TrySimpleCoroWaiter() {
    auto file = ::CreateFile(TEXT("2.txt"), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, nullptr);

    auto future = WriteAsync(file, "hello coroutines!");
    future.get_value();
}
