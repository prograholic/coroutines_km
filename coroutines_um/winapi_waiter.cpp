#include "common.h"


#include <string>

#include <iostream>


struct WinapiAsyncWriter : public AsyncWriterBase {
    explicit WinapiAsyncWriter(HANDLE handle)
        : AsyncWriterBase(handle)
        , m_promise(::CreateEvent(nullptr, true, true, nullptr))
        , m_message() {
    }

    HANDLE WriteAsync(const std::string& message) {
        m_message = message;
        std::cout << "starting ..." << std::endl;
        StartAsyncWrite(m_message.data(), m_message.size());
        return m_promise;
    }

    virtual void WriteFinished(DWORD bytesWritten) {
        std::cout << "done, bytes written: " << bytesWritten << std::endl;

        ::SetEvent(m_promise);
    }

    HANDLE m_promise;
    std::string m_message;
};

void TryWinApiWaiter() {
    auto fileHandle = ::CreateFile(TEXT("1.txt"), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, nullptr);
    WinapiAsyncWriter asyncWriter{fileHandle};
    
    auto future = asyncWriter.WriteAsync("hello world");
    ::WaitForSingleObjectEx(future, INFINITE, true);
}
