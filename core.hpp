#include <stdfloat>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <array>
#include <bit>

static constexpr int NUM_FPREGS		= 32;

using std::uint32_t, std::uint64_t, std::float32_t, std::float64_t;

static_assert(std::endian::native == std::endian::little);
union widereg {
	std::uint32_t low;
	std::uint64_t wide;
};

union fpregpair {
	std::float32_t sgl;
	std::float64_t dbl;
	std::uint32_t word;
	std::uint64_t lng;
};

class r4300_core {
public:
	enum gpr : unsigned {
		zero, at, v0, v1, a0, a1, a2, a3,
		  t0, t1, t2, t3, t4, t5, t6, t7,
		  s0, s1, s2, s3, s4, s5, s6, s7,
		  t8, t9, k0, k1, gp, sp, s8, ra,
		NUM_GPREGS,
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
		NUM_COP0REGS	= 32,
	};

protected:
	widereg gpregs[NUM_GPREGS];
	uint64_t cop0regs[NUM_COP0REGS];
	fpregpair fpregs[NUM_FPREGS];
	widereg pc;
	uint64_t mult[2];
	bool LLBit;

public:
	typedef void (r4300_core::*execute_opcode_t)(uint32_t opcode);

	enum status : unsigned {
		IE = 0, EXL, EXR, KSU, UX = 5, SX, KX, IM,
		DS = 16, RE = 25, FR, RP, CU
	};

	static constexpr const char *const gpr_names[NUM_GPREGS] = {
		"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
		  "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
		  "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
		  "t8", "t9", "k0", "k1", "gp", "sp", "s8", "ra",
	};

	static constexpr const char *const control_names[NUM_COP0REGS] = {
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

	void write_fpr64(int index, float64_t value) {
		assert(index >= 0 && index < 32);
#if 0 // funky
		if(cop0regs[Status] & (1 << FR)) {
			fpregs[index >> 1].reg[index & 1].value = value;
		} else {
			unsigned char repr[8];
			memcpy(repr, &value, 8);
			std::memcpy(fpregs[index >> 1].reg[0].lo, repr, 4);
			std::memcpy(fpregs[index >> 1].reg[1].lo, repr + 4, 4);
		}
#endif
		fpregs[index].dbl = value;
	}

	float64_t read_fpr64(int index) {
		assert(index >= 0 && index < 32);
#if 0 // funky
		if(cop0regs[Status] & (1 << FR)) {
			return fpregs[index >> 1].reg[index & 1].value;
		} else {
			unsigned char repr[8];
			std::memcpy(repr, fpregs[index >> 1].reg[0].lo, 4);
			std::memcpy(repr + 4, fpregs[index >> 1].reg[1].lo, 4);
			return std::bit_cast<float64_t>(repr);
		}
#endif
		return fpregs[index].dbl;
	}

	execute_opcode_t decode(uint32_t opcode);
};
