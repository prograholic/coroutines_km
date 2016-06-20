#pragma once

#include <coro_platform.h>


#ifndef __NOTHROW_T_DEFINED
#define __NOTHROW_T_DEFINED
    namespace std
    {
        struct nothrow_t { };

        extern nothrow_t const nothrow;
    }
#endif


    _Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(_Size)
void* __cdecl operator new(
    size_t                _Size,
    std::nothrow_t const&
    ) throw();


#ifndef __PLACEMENT_NEW_INLINE
    #define __PLACEMENT_NEW_INLINE
    _Ret_notnull_ _Post_writable_byte_size_(_Size)
    inline void* operator new(size_t _Size, _Writable_bytes_(_Size) void* _Where) throw()
    {
        (void)_Size;
        return _Where;
    }

    inline void operator delete(void*, void*) throw()
    {
        return;
    }
#endif

#ifndef __PLACEMENT_VEC_NEW_INLINE
    #define __PLACEMENT_VEC_NEW_INLINE
    _Ret_notnull_ _Post_writable_byte_size_(_Size)
    inline void* operator new[](size_t _Size, _Writable_bytes_(_Size) void* _Where) throw()
    {
        (void)_Size;
        return _Where;
    }

    inline void operator delete[](void*, void*) throw()
    {
    }
#endif



namespace std_emu
{

    typedef unsigned short uint16_t;
    typedef unsigned int uint32_t;

    typedef size_t uintptr_t;


	// TEMPLATE CLASS remove_reference
template<class _Ty>
	struct remove_reference
	{	// remove reference
	typedef _Ty type;
	};

template<class _Ty>
	struct remove_reference<_Ty&>
	{	// remove reference
	typedef _Ty type;
	};

template<class _Ty>
	struct remove_reference<_Ty&&>
	{	// remove rvalue reference
	typedef _Ty type;
	};


	// TEMPLATE CLASS integral_constant
template<class _Ty,
	_Ty _Val>
	struct integral_constant
	{	// convenient template for integral constant types
	static constexpr _Ty value = _Val;

	typedef _Ty value_type;
	typedef integral_constant<_Ty, _Val> type;

	constexpr operator value_type() const noexcept
		{	// return stored value
		return (value);
		}

	constexpr value_type operator()() const noexcept
		{	// return stored value
		return (value);
		}
	};

typedef integral_constant<bool, true> true_type;
typedef integral_constant<bool, false> false_type;


	// TEMPLATE CLASS _Cat_base
template<bool _Val>
	struct _Cat_base
		: integral_constant<bool, _Val>
	{	// base class for type predicates
	};

template<class _Ty>
	struct is_empty
		: _Cat_base<__is_empty(_Ty)>
	{	// determine whether _Ty is an empty class
	};



	// TEMPLATE CLASS is_lvalue_reference
template<class _Ty>
	struct is_lvalue_reference
		: false_type
	{	// determine whether _Ty is an lvalue reference
	};

template<class _Ty>
	struct is_lvalue_reference<_Ty&>
		: true_type
	{	// determine whether _Ty is an lvalue reference
	};




template<class _Ty> inline
constexpr typename remove_reference<_Ty>::type&&
move(_Ty&& _Arg) noexcept
{	// forward _Arg as movable
    return (static_cast<typename remove_reference<_Ty>::type&&>(_Arg));
}



template<class _Ty> inline
	constexpr _Ty&& forward(
		typename remove_reference<_Ty>::type&& _Arg) noexcept
	{	// forward an rvalue as an rvalue
	static_assert(!is_lvalue_reference<_Ty>::value, "bad forward call");
	return (static_cast<_Ty&&>(_Arg));
	}




template <typename ErrorCodeTraits>
struct error_code_base
{
    typedef typename ErrorCodeTraits::value_type value_type;

    error_code_base()
        : m_value(static_cast<value_type>(errc::success))
    {
    }

    error_code_base(errc value)
        : m_value(static_cast<value_type>(value))
    {
    }

    error_code_base& operator=(errc value)
    {
        m_value = value;
        return *this;
    }

    value_type value() const
    {
        return m_value;
    }

    error_code_base(const error_code_base& ) = default;
    error_code_base(error_code_base&& ) = default;

    error_code_base& operator=(const error_code_base& ) = default;
    error_code_base& operator=(error_code_base&& ) = default;

    explicit operator bool() const
    {
        return ErrorCodeTraits::IsError(m_value);
    }

private:
     value_type m_value;
};


using error_code = error_code_base<DefaultErrorCodeTraits>;


// very s
template <typename Type>
class shared_ptr
{
public:

    shared_ptr()
        : m_sharedCount(nullptr)
    {
    }

    ~shared_ptr()
    {
        if (m_sharedCount)
        {
            m_sharedCount->Release();
        }
    }

    shared_ptr(const shared_ptr& other)
        : m_sharedCount(other.m_sharedCount)
    {
        if (m_sharedCount)
        {
            m_sharedCount->AddRef();
        }
    }

    shared_ptr(shared_ptr&& other)
        : m_sharedCount(other.m_sharedCount)
    {
        other.m_sharedCount = nullptr
    }

    shared_ptr& operator=(const shared_ptr& other)
    {
        if (&other != this)
        {
            shared_ptr tmp = move(*this);

            m_sharedCount = other.m_sharedCount;
            if (m_sharedCount)
            {
                m_sharedCount->AddRef();
            }
        }
        return *this;
    }


    shared_ptr& operator=(shared_ptr&& other)
    {
        if (&other != this)
        {
            m_sharedCount = other.m_sharedCount;

            other.m_sharedCount = nullptr;
        }
        return *this;
    }


    Type* operator->() const
    {
        assert(m_sharedCount);

        return &(m_sharedCount->m_value);
    }

    explicit operator bool() const
    {
        return m_sharedCount != nullptr;
    }

private:

    template <typename Type, typename ...Args>
    friend
    error_code make_shared(shared_ptr<Type>& ptr, Args&&... args);

    struct SharedCount // no type erasure
    {
        template <typename ...Args>
        SharedCount(Args&&... args)
            : m_value(forward<Args>(args)...)
            , m_counter(1)
        {
        }


        void AddRef()
        {
            ++m_counter;
        }

        void Release()
        {
            --m_counter;
            if (m_counter == 0)
            {
                delete this;
            }
        }

        Type m_value;
        long m_counter;
    };

    SharedCount* m_sharedCount;


    shared_ptr(SharedCount* sharedCount)
        : m_sharedCount(sharedCount)
    {
    }
};

template <typename Type, typename ...Args>
error_code make_shared(shared_ptr<Type>& ptr, Args&&... args)
{
    typedef shared_ptr<Type>::SharedCount shared_count_t;

    typename shared_count_t* sharedCount = new(std::nothrow) shared_count_t(forward<Args>(args)...);
    if (!sharedCount)
    {
        return error_code{errc::not_enough_memory};
    }

    ptr = move(shared_ptr<Type>{sharedCount});

    return error_code{};
}

}
