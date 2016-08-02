#pragma once

#include <std_emu.h>


// intrinsics used in implementation of coroutine_handle
extern "C" size_t _coro_resume(void *);
extern "C" void _coro_destroy(void *);
extern "C" size_t _coro_done(void *);
#pragma intrinsic(_coro_resume)
#pragma intrinsic(_coro_destroy)
#pragma intrinsic(_coro_done)

namespace std
{

using nullptr_t = decltype(nullptr);

namespace experimental
{


// TEMPLATE CLASS coroutine_traits
template <typename _Ret, typename... _Ts>
struct coroutine_traits
{
	using promise_type = typename _Ret::promise_type;
};

	// TEMPLATE CLASS coroutine_handle
	template <typename _PromiseT = void>
	struct coroutine_handle;

	// TEMPLATE CLASS coroutine_handle<void> - no promise access
	template <>
	struct coroutine_handle<void>
	{
		coroutine_handle() noexcept = default;

		coroutine_handle(std::nullptr_t) noexcept : _Ptr(nullptr)
		{
		}

		coroutine_handle &operator=(nullptr_t) noexcept
		{
			_Ptr = nullptr;
			return *this;
		}

		static coroutine_handle from_address(void *_Addr) noexcept
		{
			coroutine_handle _Result;
			_Result._Ptr = reinterpret_cast<_Resumable_frame_prefix *>(_Addr);
			return _Result;
		}

		void *address() const noexcept
		{
			return _Ptr;
		}

		void operator()() const noexcept
		{
			resume();
		}

		explicit operator bool() const noexcept
		{
			return _Ptr != nullptr;
		}

		void resume() const
		{
			_coro_resume(_Ptr);
		}

		void destroy()
		{
			_coro_destroy(_Ptr);
		}

		bool done() const
		{
			// REVISIT: should return _coro_done() == 0; when intrinsic is
			// hooked up
			return (_Ptr->_Index == 0);
		}

		struct _Resumable_frame_prefix
		{
			typedef void(__cdecl *_Resume_fn)(void *);
			_Resume_fn _Fn;
			std_emu::uint16_t _Index;
			std_emu::uint16_t _Flags;
		};

	  protected:
		_Resumable_frame_prefix *_Ptr;
	};

	// TEMPLATE CLASS coroutine_handle<_PromiseT> - general form
	template <typename _PromiseT>
	struct coroutine_handle : coroutine_handle<>
	{
		coroutine_handle() noexcept = default;

		using coroutine_handle<>::coroutine_handle;

		static coroutine_handle from_promise(_PromiseT& _Prom) noexcept
		{
			auto _FramePtr = reinterpret_cast<char *>(&_Prom) + _ALIGNED_SIZE;
			coroutine_handle<_PromiseT> _Result;
			_Result._Ptr =
				reinterpret_cast<_Resumable_frame_prefix *>(_FramePtr);
			return _Result;
		}

		coroutine_handle &operator=(nullptr_t) noexcept
		{
			_Ptr = nullptr;
			return *this;
		}

		static const size_t _ALIGN_REQ = sizeof(void *) * 2;

		static const size_t _ALIGNED_SIZE =
			std_emu::is_empty<_PromiseT>::value
				? 0
				: ((sizeof(_PromiseT) + _ALIGN_REQ - 1) & ~(_ALIGN_REQ - 1));

		_PromiseT &promise() noexcept
		{
			return *reinterpret_cast<_PromiseT *>(
				reinterpret_cast<char *>(_Ptr) - _ALIGNED_SIZE);
		}

		_PromiseT const &promise() const noexcept
		{
			return *reinterpret_cast<_PromiseT const *>(
				reinterpret_cast<char const *>(_Ptr) - _ALIGNED_SIZE);
		}
	};

	template <typename _PromiseT>
	bool operator==(coroutine_handle<_PromiseT> const &_Left,
					coroutine_handle<_PromiseT> const &_Right) noexcept
	{
		return _Left.address() == _Right.address();
	}

	template <typename _PromiseT>
	bool operator!=(coroutine_handle<_PromiseT> const &_Left,
					coroutine_handle<_PromiseT> const &_Right) noexcept
	{
		return !(_Left == _Right);
	}

	// trivial awaitables

	struct suspend_if
	{
		bool _Ready;

		explicit suspend_if(bool _Condition) noexcept : _Ready(!_Condition)
		{
		}

		bool await_ready() noexcept
		{
			return _Ready;
		}

		void await_suspend(coroutine_handle<>) noexcept
		{
		}

		void await_resume() noexcept
		{
		}
	};

	struct suspend_always
	{
		bool await_ready() noexcept
		{
			return false;
		}

		void await_suspend(coroutine_handle<>) noexcept
		{
		}

		void await_resume() noexcept
		{
		}
	};

	struct suspend_never
	{
		bool await_ready() noexcept
		{
			return true;
		}

		void await_suspend(coroutine_handle<>) noexcept
		{
		}

		void await_resume() noexcept
		{
		}
	};

	// _Resumable_helper_traits class isolates front-end from public surface
	// naming changes

	template <typename _Ret, typename... _Ts>
	struct _Resumable_helper_traits
	{
		using _Traits = coroutine_traits<_Ret, _Ts...>;
		using _PromiseT = typename _Traits::promise_type;
		using _Handle_type = coroutine_handle<_PromiseT>;

		static _PromiseT *_Promise_from_frame(void *_Addr) noexcept
		{
			return reinterpret_cast<_PromiseT *>(
				reinterpret_cast<char *>(_Addr) - _Handle_type::_ALIGNED_SIZE);
		}

		static _Handle_type _Handle_from_frame(void *_Addr) noexcept
		{
			return _Handle_type::from_promise(*_Promise_from_frame(_Addr));
		}

		static void _Set_exception(void *_Addr)
		{
			_Promise_from_frame(_Addr)->set_exception(_STD current_exception());
		}

		static void _ConstructPromise(void *_Addr, void *_Resume_addr, int _HeapElision)
		{
			*reinterpret_cast<void **>(_Addr) = _Resume_addr;
			*reinterpret_cast<std_emu::uint32_t *>(reinterpret_cast<std_emu::uintptr_t>(_Addr) +
										   sizeof(void *)) = 2 + (_HeapElision ? 0 : 0x10000);
			auto _Prom = _Promise_from_frame(_Addr);
			::new (static_cast<void *>(_Prom)) _PromiseT();
		}

		static void _DestructPromise(void *_Addr)
		{
			_Promise_from_frame(_Addr)->~_PromiseT();
		}
    };


} // namespace experimental

} // namespace std
