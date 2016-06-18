#include <wdm.h>

namespace std
{
namespace experimental
{

template <typename Ret, typename... Ts>
struct coroutine_traits
{
    using promise_type = typename Ret::promise_type;
};

template <typename PromiseType = void>
struct coroutine_handle;

template <>
struct coroutine_handle<void>
{
};

template <typename PromiseType>
struct coroutine_handle : public coroutine_handle<void>
{
};

template <typename Ret, typename... Ts>
struct _Resumable_helper_traits
{
    using traits_type = coroutine_traits<Ret, Ts...>;
    using promise_type = typename traits_type::promise_type;
    using handle_type = coroutine_handle<promise_type>;

    static promise_type* _Promise_from_frame(void* addr) noexcept
    {
        return reinterpret_cast<promise_type*>(reinterpret_cast<char*>(addr) - handle_type::AlignedSize);
    }

    static void _ConstructPromise(void* _Addr, void* _Resume_addr)
    {
        *reinterpret_cast<void**>(_Addr) = _Resume_addr;
        *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(_Addr) + sizeof(void*)) = 2;
        auto _Prom = _Promise_from_frame(_Addr);
        ::new (static_cast<void*>(_Prom)) promise_type();
    }


    static void _DestructPromise(void* _Addr)
    {
        _Promise_from_frame(_Addr)->~promise_type();
    }
};

}
}

template <typename Type>
struct simple_async_result
{
    simple_async_result(const Type& /* val */)
    {
    }

    int await_resume()
    {
    }
};

template <typename Type>
struct simple_generator
{
    struct promise_type
    {
        const Type* m_current_value;

        promise_type& get_return_object()
        {
            return *this;
        }

        void yield_value(const Type& val)
        {
            m_current_value = &val;
        }

        bool initial_suspend()
        {
            return true;
        }

        bool final_suspend()
        {
            return true;
        }

        //typedef Type return_value;

#if 0
        int return_value()
        {
        }
#endif //9
    };

    struct iterator
    {
        iterator& operator++()
        {
            return *this;
        }

        bool operator==(const iterator& /* right */) const
        {
            return false;
            //return _Coro == _Right._Coro;
        }

        bool operator!=(const iterator& right) const
        {
            return !(*this == right);
        }

        Type const& operator*() const
        {
            //return *_Coro.promise()._CurrentValue;
        }

        Type const* operator->() const
        {
            //
        }
    };

    iterator begin()
    {
    }

    iterator end()
    {
    }
};



simple_async_result<int> Bar()
{
    return 10;
}


simple_generator<int> Foo()
{
    int res = 0;
    for ( ; ; )
    {
        //auto res = __await Bar();
        __yield_value res;
        ++res;
    }
}



int Baz(size_t count)
{
    int res = 0;

    for (auto val: Foo())
    {
        if (count == 0)
        {
            break;
        }

        --count;
        res += val;
    }

    return res;
}


extern "C"
NTSTATUS DriverEntry(IN PDRIVER_OBJECT /* theDriverObject */,
                     IN PUNICODE_STRING /* theRegistryPath */)
{

    //__await

    //__yield_value

    return STATUS_SUCCESS;
}
