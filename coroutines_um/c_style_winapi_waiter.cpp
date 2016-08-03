#include "common.h"


#include <iostream>

struct AsyncContext : public OVERLAPPED
{
    HANDLE m_event;

    AsyncContext()
        : OVERLAPPED()
        , m_event(::CreateEvent(nullptr, true, true, nullptr)) {
    }

    static void WINAPI OnWrite(DWORD error, DWORD bytesWritten, LPOVERLAPPED overlapped) {
        AsyncContext* me = static_cast<AsyncContext*>(overlapped);
        std::cout << "done, bytes written: " << bytesWritten << std::endl;
        ::SetEvent(me->m_event);
    }
};


void CStyleWinapiWaiter() {
    auto file = ::CreateFile(TEXT("3.txt"), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, nullptr);
    const char data [] = "hello from good ol` C!";
    AsyncContext ctx;
    std::cout << "starting..." << std::endl;
    ::WriteFileEx(file, data, sizeof(data) - 1, &ctx, AsyncContext::OnWrite);
    ::WaitForSingleObjectEx(ctx.m_event, INFINITE, true);
}
