#pragma once

#include <vector>
#include <string>
#include <iostream>

#include <Windows.h>

typedef std::vector<unsigned char> blob_t;

#define STR(x) STR2(#x)
#define STR2(x) #x

#define TRACE() std::cerr << __FUNCTION__ << ": "

constexpr size_t DataSize = 16 * 1024 * 1024;



void TryWinApiWaiter();
void TryCoroWaiter();
void TestChronoAwait();
void ConnectToDriver();
void TrySimpleCoroWaiter();
void CStyleWinapiWaiter();


struct AsyncIoBase : private OVERLAPPED {
    AsyncIoBase()
        : OVERLAPPED() {
    }

    OVERLAPPED* GetOverlapped() {
        return static_cast<OVERLAPPED*>(this);
    }

    template <typename AwaiterType>
    static AwaiterType* GetAwaiter(OVERLAPPED* overlapped) {
        return static_cast<AwaiterType*>(overlapped);
    }
};


struct AsyncWriterBase : public AsyncIoBase {
    explicit AsyncWriterBase(HANDLE handle)
        : AsyncIoBase()
        , m_handle(handle) {
    }

    void StartAsyncWrite(const void* data, size_t size) {
        ::WriteFileEx(m_handle, data, size, GetOverlapped(), OnWrite);
    }

    virtual void WriteFinished(DWORD bytesWritten) = 0;

    static void WINAPI OnWrite(DWORD error, DWORD bytesWritten, LPOVERLAPPED overlapped) {
        AsyncWriterBase* me = GetAwaiter<AsyncWriterBase>(overlapped);

        me->WriteFinished(bytesWritten);
    }

    HANDLE m_handle;
};
