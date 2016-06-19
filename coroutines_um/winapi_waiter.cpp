#include "common.h"



typedef std::pair<std::string, DWORD> error_info_t;

#define EXPECT_EX(status, error) \
do { if (!(status)) {auto le = (error); throw error_info_t(std::make_pair(STR(status), le));}} while(0)

#define TRACE() std::cerr << __FUNCTION__ << ": "

#define CALL_SPY() TRACE() << __LINE__ << std::endl


#define EXPECT(status) EXPECT_EX(status, ::GetLastError())



struct Waiter
{
    HANDLE m_event;

    void Wait()
    {
        TRACE() << "Wait start" << std::endl;
        EXPECT(::WaitForSingleObjectEx(m_event, INFINITE, true) == WAIT_OBJECT_0);

        TRACE() << "Wait end" << std::endl;
    }
};

class AsyncIo : private OVERLAPPED
{
public:

    explicit AsyncIo(HANDLE handle)
        : OVERLAPPED()
        , m_handle(handle)
        , m_data(DataSize, 'a')
    {
        EXPECT(handle != INVALID_HANDLE_VALUE);

        hEvent = ::CreateEvent(nullptr, true, true, nullptr);
        EXPECT(hEvent != nullptr);
    }


    Waiter WriteAsync()
    {
        TRACE() << "write async end" << std::endl;
        EXPECT(::WriteFileEx(m_handle, m_data.data(), m_data.size(), GetOverlapped(), AsyncIo::OnWrite));

        TRACE() << "write async end" << std::endl;

        return {hEvent};
    }

    ~AsyncIo()
    {
        ::CloseHandle(hEvent);
        ::CloseHandle(m_handle);
    }

private:
    HANDLE m_handle;

    blob_t m_data;

    AsyncIo(const AsyncIo&) = delete;
    AsyncIo& operator=(const AsyncIo&) = delete;

    OVERLAPPED* GetOverlapped()
    {
        return static_cast<OVERLAPPED*>(this);
    }

    static AsyncIo* GetAsyncIo(OVERLAPPED* overlapped)
    {
        return reinterpret_cast<AsyncIo*>(overlapped);
    }

    static void WINAPI OnWrite(DWORD error, DWORD bytesReaded, LPOVERLAPPED overlapped)
    {
        if (error == ERROR_HANDLE_EOF)
        {
            // does nothing, everything is readed
            return;
        }

        EXPECT_EX(error == ERROR_SUCCESS, error);

        // do not know what to do
        EXPECT(false);
    }
};



void TryWinApiWaiter()
{
    try
    {
        AsyncIo asyncIo(::CreateFile(TEXT("1.txt"),
                                     GENERIC_WRITE,
                                     FILE_SHARE_READ,
                                     nullptr,
                                     CREATE_ALWAYS,
                                     FILE_FLAG_OVERLAPPED,
                                     nullptr));

        auto waiter = asyncIo.WriteAsync();

        waiter.Wait();
    }
    catch (const error_info_t& e)
    {
        TRACE() << e.first << ": " << e.second << std::endl;
    }
}
