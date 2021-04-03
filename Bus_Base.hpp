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
#define BUSSIC_USE_LONG_DOUBLE 0
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

constexpr umax __bussic_log2_ceil(umax n)
{
  return ( (n<2) ? 1 : 1+log2(n/2));
}

constexpr umax __bussic_log2_floor(umax n)
{
  return ( (n<2) ? 0 : 1+log2(n/2));
}

constexpr umax __bussic_ceil_p2n(umax n){
	return (umax)1<<__bussic_log2_ceil(n);
}

constexpr umax __bussic_floor_p2n(umax n){
	return (umax)1<<__bussic_log2_floor(n);
}


#define BUS_FUNCTIONS()\
template <typename T>\
inline T load(T& retval, umax addr){\
	addr *= sizeof(T);\
	for(imax i = 0; i < (imax)sizeof(T); i++){\
		((BYTE*)&retval)[i] = ld(addr + i);\
	}\
	return retval;\
}\
template <typename T>\
inline void store(T& value, umax addr){\
	addr *= sizeof(T);\
	for(imax i = 0; i < (imax)sizeof(T); i++){\
		st( ((BYTE*)&value)[i], addr + i);\
	}\
}\
template <typename T> inline void storev(T value, umax addr){store(value, addr);}

//Bus Base Class
class NullBus{
	public:
	//Load and store a byte.
	//Uses byte-adressing
	inline virtual BYTE ld(umax addr) {(void)addr;return 0;}; //Null Bus.
	inline virtual void st(BYTE value, umax addr){(void)value; (void)addr;};/*Do nothing.*/
	//Optional but good for performance- specify load and store for arbitrary data.
	BUS_FUNCTIONS()
};

//Contiguous Memory-backed bus.
template <umax pow2size>
class MemBus{
	public:
		MemBus() = default;
		~MemBus() = default;
		inline BYTE ld(umax addr) 				{return memory[addr & getbytemask(pow2size)];}
		inline void st(BYTE value, umax addr) 	{memory[addr & getbytemask(pow2size)] = value;}
		BUS_FUNCTIONS()
	private:
		alignas(BUSSIC_MAX_ALIGNMENT) BYTE memory[1<<(pow2size-1)];
};

template <umax triggeraddr, typename what, typename T2, void (*F)(T2&)>
class FuncBus{
	public:
		FuncBus<triggeraddr,what, T2, F>(T2& _bus): bus(_bus) {}
		inline BYTE ld(umax addr) {return bus.ld(addr);}
		inline void st(BYTE value, umax addr){
			bus.st(value, addr);
			if(addr == (triggeraddr) * sizeof(what) + (sizeof(what)-1)) //The last byte of it is being written.
				F(bus);
		}
		BUS_FUNCTIONS()
	private:
		T2& bus;
};

#define PROCDEF(name) template <typename  bustype> inline void name(bustype& BUS)
#define FBUSDEF(name)


PROCDEF(myfunc){
	f32 a;
	BUS.load(a, 2);
	a *= 3;
	BUS.store(a, 0);
    BUS.store(a, 3);
}

PROCDEF(myfunc2){
	f32 a;
	BUS.load(a, 2);
	a += 3;
	BUS.store(a, 0);
}


void otherfunc(f32 value){
	MemBus<4> a;
	FuncBus<3, f32, MemBus<4>, myfunc2> b(a);
	a.store(value, 2);
	myfunc(b);
	a.load(value, 0);
	printf("Value is %f", value);
}
