
enum class CoroState {
    Init,
    Resume
};

typedef void(__cdecl *CoroResumeFunction)(void *);

template <typename PromiseType>
struct CoroutineHandle {
    CoroResumeFunction resumeAddr;
    CoroState state;
    PromiseType promise;

    explicit CoroutineHandle(CoroResumeFunction fn)
        : resumeAddr(fn)
        , state(CoroState::Init)
        , promise() {
    }
};

struct CoroutineContext {
    CoroutineHandle<coro::promise<void>> coroutineHandle;

    HANDLE handle;
    std::string message;

    WinApiAwaitable awaitable;
};

void WriteAsync_ResumeCoro2(CoroutineContext* ctx) {
    switch (ctx->coroutineHandle.state) {
    case CoroState::Init:
        new(&ctx->awaitable) WinApiAwaitable(ctx->handle, ctx->message.data(), ctx->message.size());
        std::cout << "starting ..." << std::endl;
        ctx->coroutineHandle.state = CoroState::Resume;
        ctx->awaitable.await_suspend(ctx->coroutineHandle);
        break;

    case CoroState::Resume: {
        DWORD bytesWritten = ctx->awaitable.await_resume();
        std::cout << "done, bytes written: " << bytesWritten << std::endl;
        delete ctx;
    }
    break;
    };
}

coro::future<void> WriteAsync_InitCoro1(CoroutineContext* ctx, HANDLE handle, std::string message) {
    new(&ctx->coroutineHandle) CoroutineHandle<coro::promise<void>>((CoroResumeFunction)WriteAsync_ResumeCoro2);
    new(&ctx->handle) HANDLE(handle);
    new(&ctx->message) std::string(std::move(message));

    auto res = ctx->coroutineHandle.promise.get_future();
    WriteAsync_ResumeCoro2(ctx);
    return res;
}

coro::future<void> WriteAsync(HANDLE handle, std::string message) {
    CoroutineContext* ctx = std::allocator<CoroutineContext>{}.allocate(1);

    return WriteAsync_InitCoro1(ctx, handle, message);
}
