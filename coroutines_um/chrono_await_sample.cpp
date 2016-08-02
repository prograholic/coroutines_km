#include <windows.h>
#include <future>
#include <iostream>
 
auto operator await(std::chrono::system_clock::duration duration)
{
    class awaiter
    {
        static void CALLBACK TimerCallback(PTP_CALLBACK_INSTANCE, void *Context, PTP_TIMER)
        {
            std::experimental::coroutine_handle<>::from_address(Context)();
        }
        
        PTP_TIMER timer = nullptr;
        std::chrono::system_clock::duration duration;
    public:
        
        explicit awaiter(std::chrono::system_clock::duration d)
            : duration(d)
        {
        }

        bool await_ready() const
        {
            return duration.count() <= 0;
        }
        
        bool await_suspend(std::experimental::coroutine_handle<> resume_cb)
        {
            int64_t relative_count = -duration.count();
            timer = CreateThreadpoolTimer(TimerCallback, resume_cb.address(), nullptr);
            SetThreadpoolTimer(timer, (PFILETIME)&relative_count, 0, 0);
            return timer != 0;
        }
        
        void await_resume()
        {
        }
        
        ~awaiter()
        {
            if (timer)
            {
                CloseThreadpoolTimer(timer);
            }
        }
    };
    
    return awaiter{ duration };
}
 
using namespace std;
using namespace std::chrono;
 
future<void> test()
{
    cout << this_thread::get_id() << ": sleeping...\n";
    await 1ms;
    cout << this_thread::get_id() << ": woke up\n";
}


void TestChronoAwait()
{
    test().get();
    cout << this_thread::get_id() << ": back in main\n";
}
