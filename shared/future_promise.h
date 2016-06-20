#pragma once

#include <std_emu.h>

#include <experimental/coro.h>

namespace coro
{

namespace detail
{

template <typename PlatformEventType = std_emu::DefaultPlatformEvent>
struct shared_state_base
{
public:

    shared_state_base() = default;

    shared_state_base(const shared_state_base& ) = delete;
    shared_state_base& operator=(const shared_state_base& ) = delete;

    shared_state_base(shared_state_base&&) = default;
    shared_state_base& operator=(shared_state_base&& other) = default;

    std_emu::error_code Initialize()
    {
        return m_event.Initialize();
    }

    std_emu::error_code Wait()
    {
        if (!IsValid())
        {
            return std_emu::errc::coro_invalid_shared_state;
        }

        return m_event.Wait();
    }

    bool IsValid() const
    {
        return m_event.IsValid();
    }

    void SetError(const std_emu::error_code& ec)
    {
        m_error = ec;
    }

    std_emu::error_code GetError() const
    {
        return m_error;
    }

private:
    PlatformEventType m_event;
    std_emu::error_code m_error;
};



template <typename Type>
struct shared_state : private shared_state_base<>
{
public:

    using shared_state_base<>::Initialize;
    using shared_state_base<>::IsValid;
    using shared_state_base<>::SetError;
    using shared_state_base<>::Wait;

    void SetValue(const Type& value)
    {
        m_value = value;
    }

    std_emu::error_code GetValue(Type& value)
    {
        auto err = GetError();
        if (err)
        {
            return err;
        }

        value = std_emu::move(m_value);
        return std_emu::error_code();
    }

private:
    Type m_value;
};

template <>
struct shared_state<void> : private shared_state_base<>
{
public:

    using shared_state_base<>::Initialize;
    using shared_state_base<>::IsValid;
    using shared_state_base<>::SetError;
    using shared_state_base<>::Wait;

    void SetValue()
    {
    }

    std_emu::error_code GetValue()
    {
        return GetError();
    }
};


template <typename Type>
using shared_state_ptr = std_emu::shared_ptr<shared_state<Type>>;

template <typename Type>
bool IsValid(const shared_state_ptr<Type>& state)
{
    if (!state)
    {
        return false;
    }

    return state->IsValid();
}

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
    std_emu::error_code get_value(Value&&... val)
    {
        if (!detail::IsValid(m_state))
        {
            return std_emu::errc::coro_invalid_shared_state;
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

    bool valid() const
    {
        return detail::IsValid(m_state);
    }

    // communication with future

    future<Type> get_future()
    {
        return future<Type>{m_state};
    }


    template <typename ...Args>
    void set_value(Args&&... args)
    {
        assert(detail::IsValid(m_state));

        m_state->SetValue(std::forward<Args>(args)...);
    }

    void set_error(const std_emu::error_code& ec)
    {
        assert(detail::IsValid(m_state));

        m_state->SetError(ec);
    }

    // promise_type implementation

    /// @note This type is used for early error detection
    /// If we fail to create shared state for future/promise
    /// we stop further processing as soon as possible
	struct destroy_coro_if
	{
        explicit destroy_coro_if(bool shouldDestroy)
            : m_shouldDestroy(shouldDestroy)
        {
        }

		bool await_ready() noexcept
		{
			return !m_shouldDestroy;
		}

		void await_suspend(std::experimental::coroutine_handle<> coroutine) noexcept
		{
            assert(m_shouldDestroy);

            coroutine.destroy();
		}

		void await_resume() noexcept
		{
            assert(!m_shouldDestroy);
		}

    private:
        bool m_shouldDestroy;
	};

    destroy_coro_if initial_suspend()
    {
        return destroy_coro_if(!detail::IsValid(m_state));
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

    std_emu::error_code init()
    {
        detail::shared_state_ptr<Type> tmp;

        auto err = std_emu::make_shared(tmp);
        if (err)
        {
            return err;
        }

        err = tmp->Initialize();
        if (err)
        {
            return err;
        }
        
        m_state = std_emu::move(tmp);

        return std_emu::error_code();
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
