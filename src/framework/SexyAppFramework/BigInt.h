#ifndef __BIGINT_H__
#define __BIGINT_H__

#include <string>
#include "decomp.h"

// Arbitrary precision signed integer with 16-bit digits, used by the registration code check.
// RTTI: .?AVBigInt@@ (global namespace)
// VTABLE: GAME187 0x51f5d0
// SIZE 0x14
class BigInt
{
public:
	bool mNegative;
	unsigned short* mData;
	int mLength;
	int mCapacity;

protected:
	void Reserve(int theCapacity);
	void Grow();
	void Normalize();
	unsigned short GetTopWord() const;
	unsigned short GetWord(int theIndex) const;
	void SetWord(int theIndex, unsigned short theWord);
	void DivMod(const BigInt& theDivisor, BigInt& theQuotient, BigInt& theRemainder) const;

public:
	BigInt();
	BigInt(int theValue);
	BigInt(const std::string& theHexString);
	BigInt(const BigInt& theBigInt);
	virtual ~BigInt();

	void Clear();
	bool IsOdd() const;
	bool IsNegative() const;
	int GetLength() const; // folded with Sexy::Font::GetHeight (0x4155d0)
	int GetBitLength() const;

	BigInt& ShiftLeft(int theBits);
	BigInt& ShiftRight(int theBits);

	BigInt& operator=(const BigInt& theBigInt);
	bool operator==(const BigInt& theBigInt) const;
	bool operator<(const BigInt& theBigInt) const;
	bool operator>(const BigInt& theBigInt) const;
	bool operator>=(const BigInt& theBigInt) const;

	BigInt operator-() const;
	BigInt operator-(const BigInt& theBigInt) const;
	BigInt operator+(const BigInt& theBigInt) const;
	BigInt operator*(const BigInt& theBigInt) const;
	BigInt operator%(const BigInt& theBigInt) const;
	BigInt operator|(const BigInt& theBigInt) const;
	BigInt operator<<(int theBits) const;
	BigInt operator>>(int theBits) const;

	BigInt& operator<<=(int theBits);
	BigInt& operator>>=(int theBits);
	BigInt& operator|=(const BigInt& theBigInt);
	BigInt& operator+=(const BigInt& theBigInt);
	BigInt& operator*=(const BigInt& theBigInt);

	BigInt ModPow(const BigInt& theExponent, const BigInt& theModulus) const;
};

namespace Sexy
{

BigInt HashData(const char* theData, int theLen, int theBits);
BigInt HashData(const std::string& theData, int theBits);
BigInt KeyToInt(const std::string& theKey);

}

#endif // __BIGINT_H__