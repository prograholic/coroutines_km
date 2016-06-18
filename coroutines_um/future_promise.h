#pragma once

#include <Windows.h>

#include <system_error>
#include <memory>

namespace coro
{

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
    using shared_state_base::GetError;

    void SetValue(const Type& value)
    {
        m_value = value;
    }

    Type GetValue() const
    {
        return m_value;
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
    using shared_state_base::GetError;

    void SetValue()
    {
    }

    void GetValue() const
    {
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

private:
    detail::shared_state_ptr<Type> m_state;
};



template <typename Type>
struct promise
{
    promise()
        : m_state()
    {
    }

    promise(const promise&) = delete;
    promise& operator=(const promise& ) = delete;

    promise(promise&& ) = default;
    promise& operator=(promise&& ) = default;

    std::error_code init()
    {
        detail::shared_state_ptr<Type> tmp(new (std::nothrow)detail::shared_state<Type>())
        if (!tmp)
        {
            return std::make_error_code(std::not_enough_memory);
        }

        auto err = tmp->Initialize();
        if (err)
        {
            return err;
        }
        
        m_state = std::move(tmp);

        return std::error_code();
    }

    ///

    future<Type> get_future()
    {
        return {m_state};
    }


    void set_value(const Type& value)
    {
        assert(m_state && m_state->IsValid());

        m_state->SetValue(value);
    }

    void set_error(const std::error_code& ec)
    {
        assert(m_state && m_state->IsValid());

        m_state->SetError(ec);
    }

private:
    detail::shared_state_ptr<Type> m_state;
};

} // namespace coro

