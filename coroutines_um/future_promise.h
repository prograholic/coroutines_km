#pragma once

#include <Windows.h>

#include <system_error>
#include <memory>
#include <cassert>

namespace coro
{

enum class error_codes
{
    invalid_shared_state = 1000,

}; // enum class coro_error_codes

} // namespace coro


namespace std
{

template <>
struct is_error_code_enum<coro::error_codes> : public true_type
{
};

} // namespace std

namespace coro
{

struct coro_error_category : public std::error_category
{
    virtual const char *name() const _NOEXCEPT
    {
        return "coro";
    }

	virtual std::string message(int err) const
    {
        switch (err)
        {
        case error_codes::invalid_shared_state:
            return "invalid shared state";
        }

        return "<UNKNOWN>";
    }
};


inline
const std::error_category& coro_category()
{
    return (std::_Immortalize<coro_error_category>());
}

inline
std::error_code make_error_code(error_codes errorCode)
{
    return std::error_code(static_cast<int>(errorCode), coro_category());
}


namespace detail
{

std::error_code GetErrorCodeFromWindowsResult(DWORD result)
{
    return std::error_code(result, std::system_category());
}

std::error_code GetLastErrorCode()
{
    return GetErrorCodeFromWindowsResult(::GetLastError());
}


struct shared_state_base
{
public:

    shared_state_base()
        : m_event(nullptr)
        , m_error()
    {
    }

    shared_state_base(const shared_state_base& ) = delete;
    shared_state_base& operator=(const shared_state_base& ) = delete;

    shared_state_base(shared_state_base&& other)
        : m_event(other.m_event)
        , m_error(std::move(other.m_error))
    {
        other.m_event = nullptr;
    }

    shared_state_base& operator=(shared_state_base&& other)
    {
        if (&other != this)
        {
            m_event = other.m_event;
            other.m_event = nullptr;

            m_error = std::move(other.m_error);
        }

        return *this;
    }

    ~shared_state_base()
    {
        if (m_event)
        {
            ::CloseHandle(m_event);
        }
    }


    std::error_code Initialize()
    {
        m_event = ::CreateEvent(nullptr, true, true, nullptr);
        
        return GetLastErrorCode();
    }

    std::error_code Wait()
    {
        if (!IsValid())
        {
            return error_codes::invalid_shared_state;
        }

        auto status = WaitForSingleObjectEx(m_event, INFINITE, true);
        if (status != WAIT_IO_COMPLETION)
        {
            return GetLastErrorCode();
        }

        return std::error_code();
    }

    bool IsValid() const
    {
        return m_event != nullptr;
    }

    void SetError(const std::error_code& ec)
    {
        m_error = ec;
    }

    std::error_code GetError() const
    {
        return m_error;
    }

private:
    HANDLE m_event;
    std::error_code m_error;
};



template <typename Type>
struct shared_state : private shared_state_base
{
public:

    using shared_state_base::Initialize;
    using shared_state_base::IsValid;
    using shared_state_base::SetError;
    using shared_state_base::Wait;

    void SetValue(const Type& value)
    {
        m_value = value;
    }

    std::error_code GetValue(Type& value)
    {
        auto err = GetError();
        if (err)
        {
            return err;
        }

        value = std::move(m_value);
        return std::error_code();
    }

private:
    Type m_value;
};

template <>
struct shared_state<void> : private shared_state_base
{
public:

    using shared_state_base::Initialize;
    using shared_state_base::IsValid;
    using shared_state_base::SetError;
    using shared_state_base::Wait;

    void SetValue()
    {
    }

    std::error_code GetValue()
    {
        return GetError();
    }
};


template <typename Type>
using shared_state_ptr = std::shared_ptr<shared_state<Type>>;

} // namespace detail


template <typename Type>
struct future
{
    future()
        : m_state()
    {
    }

    explicit future(const detail::shared_state_ptr<Type>& state)
        : m_state(state)
    {
    }

    future(const future&) = delete;
    future& operator=(const future& ) = delete;

    future(future&& ) = default;
    future& operator=(future&& ) = default;

    // obtain result

    template <typename ...Value>
    std::error_code get_value(Value&&... val)
    {
        if (!m_state || !m_state->IsValid())
        {
            return make_error_code(error_codes::invalid_shared_state);
        }

        auto err = m_state->Wait();
        if (err)
        {
            return err;
        }

        return m_state->GetValue(std::forward<Value>(val)...);
    }

private:
    detail::shared_state_ptr<Type> m_state;
};



template <typename Type>
struct promise
{
    promise()
        : m_state()
    {
        init();
    }

    promise(const promise&) = delete;
    promise& operator=(const promise& ) = delete;

    promise(promise&& ) = default;
    promise& operator=(promise&& ) = default;



    // communication with future

    future<Type> get_future()
    {
        return future<Type>{m_state};
    }


    template <typename ...Args>
    void set_value(Args&&... args)
    {
        assert(m_state && m_state->IsValid());

        m_state->SetValue(std::forward<Args>(args)...);
    }

    void set_error(const std::error_code& ec)
    {
        assert(m_state && m_state->IsValid());

        m_state->SetError(ec);
    }

    // promise_type implementation


    std::experimental::suspend_never initial_suspend()
    {
        // todo: prograholic: Maybe we may use it for early error detection
        return {};
    }

    future<Type> get_return_object()
    {
        return get_future();
    }

    std::experimental::suspend_never final_suspend()
    {
        return {};
    }

private:
    detail::shared_state_ptr<Type> m_state;

    std::error_code init()
    {
        detail::shared_state_ptr<Type> tmp(new (std::nothrow)detail::shared_state<Type>());
        if (!tmp)
        {
            return std::make_error_code(std::errc::not_enough_memory);
        }

        auto err = tmp->Initialize();
        if (err)
        {
            return err;
        }
        
        m_state = std::move(tmp);

        return std::error_code();
    }
};

} // namespace coro


namespace std
{
namespace experimental
{

template <typename Type, typename... Args>
struct coroutine_traits<coro::future<Type>, Args...>
{
    using promise_type = coro::promise<Type>;
};


} // namespace experimental
} // namespace std
