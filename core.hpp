#include <type_traits>
#include <stdfloat>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <array>
#include <bit>

#ifndef TOO_ACCURATE
# define TOO_ACCURATE 0
#endif

using std::uint32_t, std::uint64_t, std::float32_t, std::float64_t;

static_assert(std::endian::native == std::endian::little);
union widereg {
	std::uint32_t low;
	std::uint64_t wide;
};

class r4300_core {
public:
	enum gpr : unsigned {
		zero, at, v0, v1, a0, a1, a2, a3,
		  t0, t1, t2, t3, t4, t5, t6, t7,
		  s0, s1, s2, s3, s4, s5, s6, s7,
		  t8, t9, k0, k1, gp, sp, s8, ra,
	};

	enum control : unsigned {
		Index		= 0,
		Random		= 1,
		EntryLo0	= 2,
		EntryLo1	= 3,
		Context		= 4,
		PageMask	= 5,
		Wired		= 6,
		BadVAddr	= 8,
		Count		= 9,
		EntryHi		= 10,
		Compare		= 11,
		Status		= 12,
		Cause		= 13,
		EPC		= 14,
		PRId		= 15,
		Config		= 16,
		LLAddr		= 17,
		WatchLo		= 18,
		WatchHi		= 19,
		XContext	= 20,
		ParityError	= 26,
		CacheError	= 27,
		TagLo		= 28,
		TagHi		= 29,
		ErrorEPC	= 30,
	};

protected:
	widereg gpregs[32];
	uint64_t cop0regs[32];
	std::byte fpregs[sizeof(float64_t)][32];
	widereg pc;
	uint64_t mult[2];
	bool LLBit;

public:
	enum status : unsigned {
		IE = 0,		EXL = 1,	EXR = 2,	KSU = 3,
		UX = 5,		SX = 6,		KX = 7,		IM = 8,
		DS = 16,	RE = 25,	FR = 26,	RP = 27,
		CU = 28,
	};

	static const char *const gpr_names[] = {
		"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
		  "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
		  "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
		  "t8", "t9", "k0", "k1", "gp", "sp", "s8", "ra",
	};

	static const char *const control_names[] = {
		"Index",	"Random",	"EntryLo0",	"EntryLo1",
		"Context",	"PageMask",	"Wired",	"CP0R7",
		"BadVAddr",	"Count",	"EntryHi",	"Compare",
		"Status",	"Cause",	"EPC",		"PRId",
		"Config",	"LLAddr",	"WatchLo",	"WatchHi",
		"XContext",	"CP0R21",	"CP0R22",	"CP0R23",
		"CP0R24",	"CP0R25",	"ParityError",	"CacheError",
		"TagLo",	"TagHi",	"ErrorEPC",	"CP0R31",
	};

	void write_gpr32(int index, std::uint32_t value) {
		assert(index >= 0 && index < 32);
		if(index) {
			gpregs[index].low = value;
		}
	}
	std::uint32_t read_gpr32(int index) {
		assert(index >= 0 && index < 32);
		return gpregs[index].low;
	}

	void write_gpr64(int index, std::uint64_t value) {
		assert(index >= 0 && index < 32);
		if(index) {
			gpregs[index].wide = value;
		}
	}
	std::uint64_t read_gpr64(int index) {
		assert(index >= 0 && index < 32);
		return gpregs[index].wide;
	}

	template<typename T>
	requires((std::is_integral<T> || std::is_floating_point<T>) &&
		 (sizeof(T) == 4))
	void write_fpr32(int index, T value) {
		assert(index >= 0 && index < 32);
		std::memcpy(fpregs[index], &value, 4);
	}

	template<typename T>
	requires((std::is_integral<T> || std::is_floating_point<T>) &&
		 (sizeof(T) == 4))
	T read_fpr32(int index) {
		assert(index >= 0 && index < 32);
		return *reinterpret_cast<T *>(fpregs[index]);
	}

	template<typename T>
	requires((std::is_integral<T> || std::is_floating_point<T>) &&
		 (sizeof(T) == 8))
	void write_fpr64(int index, T value) {
		assert(index >= 0 && index < 32);
		if((cop0regs[Status] & (1 << FR)) || !TOO_ACCURATE) {
			std::memcpy(fpregs[index], &value, 8);
		} else {
			std::byte *repr = reinterpret_cast<std::byte *>(&value);
			std::memcpy(fpregs[index], repr, 4);
			std::memcpy(fpregs[index + 1], repr + 4, 4);
		}
	}

	template<typename T>
	requires((std::is_integral<T> || std::is_floating_point<T>) &&
		 (sizeof(T) == 8))
	T read_fpr64(int index) {
		assert(index >= 0 && index < 32);
		if((cop0regs[Status] & (1 << FR)) || !TOO_ACCURATE) {
			return *reinterpret_cast<T *>(fpregs[index]);
		} else {
			std::byte repr[8];
			std::memcpy(repr, fpregs[index], 4);
			std::memcpy(repr + 4, fpregs[index + 1], 4);
			return *reinterpret_cast<T *>(repr);
		}
	}

	void decode(uint32_t opcode);
};
