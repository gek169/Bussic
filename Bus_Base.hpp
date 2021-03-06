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
static_assert(sizeof (f32) == 4, "Type size test failed.");
#endif
typedef uint8_t u8;
typedef int8_t i8;
typedef u8 BYTE;
typedef uint16_t u16;
typedef int16_t i16;
static_assert(sizeof(u8) == 1, "Type size test failed.");
static_assert(sizeof(u16) == 2, "Type size test failed.");
static_assert(sizeof(i8) == 1, "Type size test failed.");
static_assert(sizeof(i16) == 2, "Type size test failed.");

#if BUSSIC_IMPLEMENTATION_BITS >= 32
typedef uint32_t u32;
typedef int32_t i32;
static_assert(sizeof(u32) == 4, "Type size test failed.");
static_assert(sizeof(i32) == 4, "Type size test failed.");
#endif
typedef size_t umax;
typedef ssize_t imax;


#if BUSSIC_IMPLEMENTATION_BITS >= 64

typedef uint64_t u64;
typedef int64_t i64;
static_assert(sizeof(u64) == 8, "Type size test failed.");
static_assert(sizeof(i64) == 8, "Type size test failed.");

#endif

#if BUSSIC_USE_DOUBLE
typedef double f64;
static_assert(sizeof(f64) == 8, "Type size test failed.");
#endif

#if BUSSIC_USE_LONG_DOUBLE
typedef long double f128;
static_assert(sizeof(f128) == 16, "Type size test failed.");
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
inline T load(T& retval, umax off, umax addr){\
	addr *= sizeof(T);\
	for(imax i = 0; i < (imax)sizeof(T); i++){\
		((BYTE*)&retval)[i] = ld(off + addr + i);\
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
template <typename T>\
inline void store(T& value, umax off, umax addr){\
	addr *= sizeof(T);\
	for(imax i = 0; i < (imax)sizeof(T); i++){\
		st( ((BYTE*)&value)[i], off + addr + i);\
	}\
}\
template <typename T> inline void storev(T value, umax addr){store(value, addr);}\
template <typename T> inline void storev(T value, umax off, umax addr){store(value, off, addr);}

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

template <umax triggeraddr, umax triggeroff, typename what, typename T2, void (*F)(T2&)>
class FuncBus{
	public:
		FuncBus<triggeraddr, triggeroff, what, T2, F>(T2& _bus): bus(_bus) {}
		inline BYTE ld(umax addr) {return bus.ld(addr);}
		inline void st(BYTE value, umax addr){
			bus.st(value, addr);
			if(addr == triggeroff + ((triggeraddr) * sizeof(what)) + (sizeof(what)-1)) //The last byte of it is being written.
				F(bus);
		}
		BUS_FUNCTIONS()
	private:
		T2& bus;
};

//Lazily-evaluated Funcbus. Values are not evaluated until they are loaded.
//This usually causes a reversed order of execution.
template <umax triggeraddr, umax triggeroff, typename what, typename T2, void (*F)(T2&)>
class LazyFuncBus{
	public:
		LazyFuncBus<triggeraddr, triggeroff, what, T2, F>(T2& _bus): bus(_bus) {}
		inline BYTE ld(umax addr) {
			if(addr == triggeroff + ((triggeraddr) * sizeof(what))) //The first byte of a triggering address.
				F(bus);
			return bus.ld(addr);
		}
		inline void st(BYTE value, umax addr){bus.st(value, addr);}
		BUS_FUNCTIONS()
	private:
		T2& bus;
};

#define PROCDEF(name) template <typename  bustype> inline void name(bustype& BUS)
#define typeof(name) decltype(name)


//Lazily-evaluated functions.
PROCDEF(myfunc3){
	f32 a;
	BUS.load(a, 0x100, 0); //Intercept myfunc2's argument.
	printf("Multiplying %f by 7...\n", a);
	a *= 7;
	BUS.store(a, 0x100, 0); //write back.
}

PROCDEF(myfunc2){
	f32 a;
	BUS.load(a, 0x100, 0); //Retrieve. This will cause myfunc3 to be invoked, the argument is intercepted.
	printf("Adding 3 to %f...\n", a);
	a += 3;
	BUS.store(a, 0x100, 0); //Write back. this does *not* cause an invocation.
}

//Non lazy function which writes to a different location than its arguments.
PROCDEF(myfunc){
	//Our argument is in 2, and is expected to be written to zero.
	f32 a;
	BUS.load(a,  2); //retrieve our argument
	printf("Multiplying %f by 3...\n", a);
	a *= 3;
	BUS.store(a, 0x100, 0); //Store at 0x100
	puts("MARK");
	BUS.load(a, 0x100, 0); //Retrieve from 0x100, it has been processed from over the bus. 
							//(This actually causes myfunc2 to be invoked, which is intercepted by myfunc3...)
	puts("Storing result...");
	BUS.store(a, 0x00, 0); //store at the return spot.
}




void otherfunc(f32 value){
	MemBus<10> a;
	//Define a custom memory layout!
	//Here we chain two functions together to produce a combined function.
	LazyFuncBus<0, 0x100, f32, typeof(a), myfunc3> b(a); //Myfunc3 will never invoke myfunc2 or itself.
	LazyFuncBus<0, 0x100, f32, typeof(b), myfunc2> c(b); //When myfunc2 loads 0x100, it invokes myfunc3.
	//The order of evaluation is as follows:
	//myfunc->myfunc3->myfunc2->myfunc
	c.store(value, 0x00, 2);
	myfunc(c); //we expect the returned value to be (value * 3 + 3) * 7
	fgetc(stdin);
	c.load(value, 0x00, 0);
	printf("Value is %f\n", value);
}
