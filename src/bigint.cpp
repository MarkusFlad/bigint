#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <stdlib.h>

template<class WordType, class HalfWordType>
class BigInt {
public:
	using Size = std::size_t;
	using ValueVector = std::vector<WordType>;
	BigInt(WordType value) {
		_vv.push_back(value);
	}
	BigInt(const ValueVector vv) {
		_vv = vv;
	}
	BigInt(const std::string& valueString) {
//		for (Size i=0; i<valueString.size(); i++) {
//			char digitCharacter = valueString[i];
//			WordType digit = digitCharacter - '0';
//		}
	}
	bool operator==(const BigInt& other) const {
		return _vv == other._vv;
	}
	friend BigInt operator+(const BigInt& a, const BigInt& b) {
		constexpr WordType ZERO = zeroValue();
		ValueVectorPair vvp = vvSortedBySize(a, b);
		const ValueVector& s1 = vvp.vv1;
		const ValueVector& s2 = vvp.vv2;
		Size s1Size = s1.size();
		Size s2Size = s2.size();
		ValueVector result (s2Size + 1, ZERO);

		for (Size i=0; i<s1Size; i++) {
			result[i] += lowerSum(s1[i], s2[i], result[i+1]);
		}
		for (Size j=s1Size; j<s2Size; j++) {
			result[j] = lowerSum(result[j], s2[j], result[j+1]);
		}
		if (result.back() == ZERO) {
			result.resize(s2Size);
		}
		return BigInt(result);
	}
	friend BigInt operator*(const BigInt& a, const BigInt& b) {
		BigInt result(0L);
		constexpr HalfWordType ZERO = zeroHalfValue();
		const ValueVector& avv = a._vv;
		const ValueVector& bvv = b._vv;
		Size avvSize = avv.size();
		Size bvvSize = bvv.size();
		ValueVector u;
		for (Size i=0; i<bvvSize; i++) {
			HalfWords hwB = halfWords(bvv[i]);
			ValueVector v(u);
			for (Size j=0; j<avvSize; j++) {
				HalfWords hwA = halfWords(avv[i]);
				WordType rALower = hwA.lowerHalf * hwB.lowerHalf;
				WordType rAUpper = hwA.upperHalf * hwB.lowerHalf;
				if (rALower != ZERO) {
					ValueVector rVLower(v);
					rVLower.push_back(rALower);
					result = result + BigInt(rVLower);
				}
				if (rAUpper != ZERO) {
					ValueVector rVUpper(v);
					// TODO: Refactor to be a generic implementation ...
				    rVUpper.push_back(rAUpper << 32);
				    rVUpper.push_back(rAUpper >> 32);
					result = result + BigInt(rVUpper);
				}
				v.push_back(ZERO);
			}
			v = u;
			for (Size j=0; j<avvSize; j++) {
				HalfWords hwA = halfWords(avv[i]);
				WordType rALower = hwA.lowerHalf * hwB.upperHalf;
				WordType rAUpper = hwA.upperHalf * hwB.upperHalf;
				if (rALower != ZERO) {
					ValueVector rVLower(v);
					// TODO: Refactor to be a generic implementation ...
				    rVLower.push_back(rALower << 32);
				    rVLower.push_back(rALower >> 32);
					result = result + BigInt(rVLower);
				}
				v.push_back(ZERO);
				if (rAUpper != ZERO) {
					ValueVector rVUpper(v);
				    rVUpper.push_back(rAUpper);
					result = result + BigInt(rVUpper);
				}
			}
			u.push_back(ZERO);
		}
		return result;
	}
private:
	struct ValueVectorPair {
		const ValueVector& vv1;
		const ValueVector& vv2;
	};
	static ValueVectorPair vvSortedBySize (const BigInt& a, const BigInt& b) {
		Size aSize = a._vv.size();
		Size bSize = b._vv.size();
		const ValueVector* pSummand1 = &(a._vv);
		const ValueVector* pSummand2 = &(b._vv);
		if (aSize > bSize) {
			std::swap(pSummand1, pSummand2);
		}
		return {*pSummand1, *pSummand2};
	}
	struct HalfWords {
		WordType upperHalf;
		WordType lowerHalf;
	};
	static HalfWords halfWords(WordType word);
	static constexpr WordType zeroValue();
	static constexpr HalfWordType zeroHalfValue();
	static WordType lowerSum(WordType a, WordType b, WordType& carryWord);
private:
	ValueVector _vv;
};
template<>
constexpr int64_t BigInt<int64_t, int32_t>::zeroValue() {
	return 0L;
}
template<>
constexpr uint64_t BigInt<uint64_t, uint32_t>::zeroValue() {
	return 0UL;
}

template<>
constexpr int32_t BigInt<int64_t, int32_t>::zeroHalfValue() {
	return 0;
}
template<>
constexpr uint32_t BigInt<uint64_t, uint32_t>::zeroHalfValue() {
	return 0U;
}

template<>
int64_t BigInt<int64_t, int32_t>::lowerSum(int64_t a, int64_t b,
		int64_t& carryWord) {
    long sumNext;
	if (__builtin_saddl_overflow(a, b, &sumNext)) {
		carryWord = 1L;
		return sumNext & std::numeric_limits<int64_t>::max();
	} else {
		return sumNext;
	}
}
template<>
uint64_t BigInt<uint64_t, uint32_t>::lowerSum(uint64_t a, uint64_t b,
		uint64_t& carryWord) {
    uint64_t sumNext;
	if (__builtin_uaddl_overflow(a, b, &sumNext)) {
		carryWord = 1L;
	}
	return sumNext;
}

template<>
BigInt<int64_t, int32_t>::HalfWords BigInt<int64_t, int32_t>::halfWords(
		int64_t word) {
	HalfWords hw;
	constexpr uint64_t lowerMask = std::numeric_limits<uint32_t>::max();
	hw.lowerHalf = word & lowerMask;
	hw.upperHalf = word >> 32;
	return hw;
}
template<>
BigInt<uint64_t, uint32_t>::HalfWords BigInt<uint64_t, uint32_t>::halfWords(
		uint64_t word) {
	HalfWords hw;
	constexpr uint64_t lowerMask = std::numeric_limits<uint32_t>::max();
	hw.lowerHalf = word & lowerMask;
	hw.upperHalf = word >> 32;
	return hw;
}

template<class T>
void assertEqual(T expected, T actual) {
	if (expected == actual) {
		return;
	} else {
		throw std::runtime_error("assertEqual does not match.");
	}
}

int main(int argc, char** argv) {
	using WordType = uint64_t;
	using HalfWordType = uint32_t;
	WordType maxWordVal = std::numeric_limits<WordType>::max();
	HalfWordType maxHalfWordVal = std::numeric_limits<HalfWordType>::max();
	using BigUL = BigInt<WordType, HalfWordType>;
	using Vec = std::vector<WordType>;
	if (argc == 3) {
		BigUL a (atol(argv[1]));
		BigUL b (atol(argv[2]));
		BigUL c = a + b;
		BigUL d (Vec({0, 1}));
		return c == d;
	} else {
		try {
			BigUL a (Vec({maxWordVal}));
			BigUL b = a + a;
			assertEqual(b, BigUL(Vec({maxWordVal-1, 1})));
			BigUL d (Vec({maxWordVal, maxWordVal}));
			BigUL e = d + d;
			assertEqual(e, BigUL(Vec({maxWordVal-1, maxWordVal, 1})));
			BigUL f = e + b;
			assertEqual(f, BigUL(Vec({maxWordVal-3, 1, 2})));
			BigUL g (Vec({2}));
			BigUL h = g * g;
			assertEqual(h, BigUL(Vec({4})));
			BigUL i (Vec({maxHalfWordVal}));
			BigUL j = i * i;
			assertEqual(j, BigUL(Vec({0xFFFFFFFE00000001UL})));
			BigUL k (Vec({maxWordVal}));
			BigUL l = k * k;
			assertEqual(l, BigUL(Vec({0x0000000000000001, 0xFFFFFFFFFFFFFFFEUL})));
		} catch (std::runtime_error&) {
			return 0;
		}
		return 1;
	}
}
