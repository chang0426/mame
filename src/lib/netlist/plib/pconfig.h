// license:GPL-2.0+
// copyright-holders:Couriersud
/*
 * pconfig.h
 *
 */

#ifndef PCONFIG_H_
#define PCONFIG_H_

#include <cstdint>
#include <thread>
#include <chrono>

#ifndef PSTANDALONE
	#define PSTANDALONE (0)
#endif

#define PHAS_INT128 (0)

#ifndef PHAS_INT128
#define PHAS_INT128 (0)
#endif

#if (PHAS_INT128)
typedef __uint128_t UINT128;
typedef __int128_t INT128;
#endif

#define PLIB_NAMESPACE_START() namespace plib {
#define PLIB_NAMESPACE_END() }

#if !(PSTANDALONE)
#include "osdcomm.h"
//#include "eminline.h"

#ifndef assert
#define assert(x) do {} while (0)
#endif

#include "delegate.h"

#else
#include <stdint.h>
#endif
#include <cstddef>


//============================================================
//  Standard defines
//============================================================

// prevent implicit copying
#define P_PREVENT_COPYING(_name)                \
	private:                                    \
		_name(const _name &);                   \
		_name &operator=(const _name &);

//============================================================
//  Compiling standalone
//============================================================

#if !(PSTANDALONE)

/* use MAME */
#if (USE_DELEGATE_TYPE == DELEGATE_TYPE_INTERNAL)
#define PHAS_PMF_INTERNAL 1
#else
#define PHAS_PMF_INTERNAL 0
#endif

#else

/* determine PMF approach */

#if defined(__GNUC__)
	/* does not work in versions over 4.7.x of 32bit MINGW  */
	#if defined(__MINGW32__) && !defined(__x86_64) && defined(__i386__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 7)))
		#define PHAS_PMF_INTERNAL 1
		#define MEMBER_ABI __thiscall
	#elif defined(EMSCRIPTEN)
		#define PHAS_PMF_INTERNAL 0
	#elif defined(__arm__) || defined(__ARMEL__)
		#define PHAS_PMF_INTERNAL 0
	#else
		#define PHAS_PMF_INTERNAL 1
	#endif
#else
#define PHAS_PMF_INTERNAL 0
#endif

#ifndef MEMBER_ABI
	#define MEMBER_ABI
#endif

/* not supported in GCC prior to 4.4.x */
#define ATTR_HOT               __attribute__((hot))
#define ATTR_COLD              __attribute__((cold))

#define RESTRICT                __restrict__
#define EXPECTED(x)     (x)
#define UNEXPECTED(x)   (x)
#define ATTR_PRINTF(x,y)        __attribute__((format(printf, x, y)))
#define ATTR_UNUSED             __attribute__((__unused__))

/* 8-bit values */
typedef unsigned char                       UINT8;
typedef signed char                         INT8;

/* 16-bit values */
typedef unsigned short                      UINT16;
typedef signed short                        INT16;

/* 32-bit values */
#ifndef _WINDOWS_H
typedef unsigned int                        UINT32;
typedef signed int                          INT32;
#endif

/* 64-bit values */
#ifndef _WINDOWS_H
#ifdef _MSC_VER
typedef signed __int64                      INT64;
typedef unsigned __int64                    UINT64;
#else
typedef uint64_t    UINT64;
typedef int64_t      INT64;
#endif
#endif

/* U64 and S64 are used to wrap long integer constants. */
#if defined(__GNUC__) || defined(_MSC_VER)
#define U64(val) val##ULL
#define S64(val) val##LL
#else
#define U64(val) val
#define S64(val) val
#endif

#endif

using pticks_t = INT64;

inline pticks_t pticks()
{
	return std::chrono::high_resolution_clock::now().time_since_epoch().count();
}
inline pticks_t pticks_per_second()
{
	return std::chrono::high_resolution_clock::period::den / std::chrono::high_resolution_clock::period::num;
}

#if 1 && defined(__x86_64__)

static inline pticks_t pprofile_ticks()
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}
#else
inline pticks_t pprofile_ticks() { return pticks(); }
#endif

/*
 * The following class was derived from the MAME delegate.h code.
 * It derives a pointer to a member function.
 */

#if (PHAS_PMF_INTERNAL)
class pmfp
{
public:
	// construct from any member function pointer
	class generic_class;
	using generic_function = void (*)();

	template<typename _MemberFunctionType>
	pmfp(_MemberFunctionType mfp)
	: m_function(0), m_this_delta(0)
	{
		*reinterpret_cast<_MemberFunctionType *>(this) = mfp;
	}

	// binding helper
	template<typename _FunctionType, typename _ObjectType>
	_FunctionType update_after_bind(_ObjectType *object)
	{
		return reinterpret_cast<_FunctionType>(
				convert_to_generic(reinterpret_cast<generic_class *>(object)));
	}
	template<typename _FunctionType, typename _MemberFunctionType, typename _ObjectType>
	static _FunctionType get_mfp(_MemberFunctionType mfp, _ObjectType *object)
	{
		pmfp mfpo(mfp);
		return mfpo.update_after_bind<_FunctionType>(object);
	}

private:
	// extract the generic function and adjust the object pointer
	generic_function convert_to_generic(generic_class * object) const
	{
		// apply the "this" delta to the object first
		generic_class * o_p_delta = reinterpret_cast<generic_class *>(reinterpret_cast<std::uint8_t *>(object) + m_this_delta);

		// if the low bit of the vtable index is clear, then it is just a raw function pointer
		if (!(m_function & 1))
			return reinterpret_cast<generic_function>(m_function);

		// otherwise, it is the byte index into the vtable where the actual function lives
		std::uint8_t *vtable_base = *reinterpret_cast<std::uint8_t **>(o_p_delta);
		return *reinterpret_cast<generic_function *>(vtable_base + m_function - 1);
	}

	// actual state
	uintptr_t               m_function;         // first item can be one of two things:
												//    if even, it's a pointer to the function
												//    if odd, it's the byte offset into the vtable
	int                     m_this_delta;       // delta to apply to the 'this' pointer
};
#endif

#endif /* PCONFIG_H_ */
