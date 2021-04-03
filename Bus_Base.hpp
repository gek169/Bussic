#include <cstdlib>
#include <cstdint>
#include <memory>
#include <cassert>
#include <cmath>
#include <ctgmath>
#include <iostream>
#include <cstring>

#define BUSSIC_FAST_FLOAT_MATH 1
#define BUSSIC_USE_FLOAT 1
#define BUSSIC_USE_DOUBLE 1
#define BUSSIC_USE_LONG_DOUBLE 1
#define BUSSIC_IMPLEMENTATION_BITS 64
#define BUSSIC_MAX_ALIGNMENT 32
//Implementation block size- How much memory are you willing to store on the stack?
#define BUSSICBLOCKLEVEL	18
#define BUSSICBLOCKBYTES	getwidth(BUSSICBLOCKLEVEL)


#if BUSSIC_USE_FLOAT
typedef float f32;
static_assert(sizeof (f32) == 4);
#endif
typedef uint8_t u8;
typedef int8_t i8;
typedef u8 BYTE;
typedef uint16_t u16;
typedef int16_t i16;
static_assert(sizeof(u8) == 1);
static_assert(sizeof(u16) == 2);
static_assert(sizeof(i8) == 1);
static_assert(sizeof(i16) == 2);

#if BUSSIC_IMPLEMENTATION_BITS >= 32
typedef uint32_t u32;
typedef int32_t i32;
static_assert(sizeof(u32) == 4);
static_assert(sizeof(i32) == 4);
#endif
typedef size_t umax;
typedef ssize_t imax;


#if BUSSIC_IMPLEMENTATION_BITS >= 64

typedef uint64_t u64;
typedef int64_t i64;
static_assert(sizeof(u64) == 8);
static_assert(sizeof(i64) == 8);

#endif

#if BUSSIC_USE_DOUBLE
typedef double f64;
static_assert(sizeof(f64) == 8);
#endif

#if BUSSIC_USE_LONG_DOUBLE
typedef long double f128;
static_assert(sizeof(f128) == 16);
#endif



constexpr static inline umax getwidth(umax size)            {return (umax)1<<((umax)(size-1));}
constexpr static inline umax getbytemask(umax size)         {return getwidth(size) - 1;}
constexpr static inline umax getrelwidth(umax os, umax size){return getwidth(size) / getwidth(os);}
constexpr static inline umax getmask(umax os, umax size)    {return getrelwidth(os,size)?
																(getrelwidth(os, size)-1):
																0;}
constexpr static inline umax fixindex(umax ind, umax os, umax size) {return ind & getmask(os,size);}
constexpr static inline umax byteoff(umax ind, umax os, umax size) {return fixindex(ind, os, size) * getwidth(os);}



//Bus Base Class
class Bus{
	public:
	//Load and store a byte.
	//Uses byte-adressing
	inline virtual BYTE ld(umax addr)				{(void)addr;return 0;} //Null Bus.
	inline virtual void st(BYTE value, umax addr)	{(void)value; (void)addr;/*Do nothing.*/}
	//Load or store an arbitrary type.
	//Addressed by the size of the type.
	template <typename T>
	inline T load(T& retval, umax addr){
		addr *= sizeof(T);//Convert to addressing by the size of the type.
		for(umax i = 0; i < sizeof(T); i++){
			((BYTE*)&retval)[i] = ld(addr + i);
		}
		return retval;
	}

	//The compiler is smart
	template <typename T>
	inline void store(T& value, umax addr){
		addr *= sizeof(T);
		for(umax i = 0; i < sizeof(T); i++){
			st( ((BYTE*)&value)[i], addr + i);
		}
	}
};



template <typename  bustype>
void myfunc(bustype& value){
	u32 a;
	value.load(a, 0);
	a *= 47;
	value.store(a, 0);
}


void otherfunc(){
	Bus a;
	myfunc<Bus>(a);
}
