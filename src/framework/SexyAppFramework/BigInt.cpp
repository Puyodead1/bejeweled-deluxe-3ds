#include "BigInt.h"
#include <string.h>
#include <ctype.h>

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

///////////////////////////////////////////////////////////////////////////////
// MD5 (Colin Plumb's public domain implementation)

typedef unsigned long uint32;

struct MD5Context
{
	uint32 buf[4];
	uint32 bits[2];
	unsigned char in[64];
};

void MD5Transform(uint32 buf[4], uint32 const in[16]);

// FUNCTION: GAME187 0x49a050
void MD5Init(struct MD5Context *ctx)
{
	ctx->buf[0] = 0x67452301;
	ctx->buf[1] = 0xefcdab89;
	ctx->buf[2] = 0x98badcfe;
	ctx->buf[3] = 0x10325476;

	ctx->bits[0] = 0;
	ctx->bits[1] = 0;
}

// FUNCTION: GAME187 0x49a080
void MD5Update(struct MD5Context *ctx, unsigned char const *buf, unsigned len)
{
	uint32 t;

	t = ctx->bits[0];
	if ((ctx->bits[0] = t + ((uint32) len << 3)) < t)
		ctx->bits[1]++;
	ctx->bits[1] += len >> 29;

	t = (t >> 3) & 0x3f;

	if (t)
	{
		unsigned char *p = (unsigned char *) ctx->in + t;

		t = 64 - t;
		if (len < t)
		{
			memcpy(p, buf, len);
			return;
		}
		memcpy(p, buf, t);
		MD5Transform(ctx->buf, (uint32 *) ctx->in);
		buf += t;
		len -= t;
	}

	while (len >= 64)
	{
		memcpy(ctx->in, buf, 64);
		MD5Transform(ctx->buf, (uint32 *) ctx->in);
		buf += 64;
		len -= 64;
	}

	memcpy(ctx->in, buf, len);
}

// FUNCTION: GAME187 0x49a140
void MD5Final(unsigned char digest[16], struct MD5Context *ctx)
{
	unsigned count;
	unsigned char *p;

	count = (ctx->bits[0] >> 3) & 0x3F;

	p = ctx->in + count;
	*p++ = 0x80;

	count = 64 - 1 - count;

	if (count < 8)
	{
		memset(p, 0, count);
		MD5Transform(ctx->buf, (uint32 *) ctx->in);
		memset(ctx->in, 0, 56);
	}
	else
	{
		memset(p, 0, count - 8);
	}

	((uint32 *) ctx->in)[14] = ctx->bits[0];
	((uint32 *) ctx->in)[15] = ctx->bits[1];

	MD5Transform(ctx->buf, (uint32 *) ctx->in);
	memcpy(digest, ctx->buf, 16);
	memset(ctx, 0, sizeof(ctx)); /* In case it's sensitive */
}

#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

#define MD5STEP(f, w, x, y, z, data, s) \
	( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )

// FUNCTION: GAME187 0x49a1d0
void MD5Transform(uint32 buf[4], uint32 const in[16])
{
	register uint32 a, b, c, d;

	a = buf[0];
	b = buf[1];
	c = buf[2];
	d = buf[3];

	MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
	MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
	MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
	MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
	MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
	MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
	MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
	MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
	MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
	MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
	MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
	MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
	MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
	MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
	MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
	MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

	MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
	MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
	MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
	MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
	MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
	MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
	MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
	MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
	MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
	MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
	MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
	MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
	MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
	MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
	MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
	MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

	MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
	MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
	MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
	MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
	MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
	MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
	MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
	MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
	MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
	MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
	MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
	MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
	MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
	MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
	MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
	MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

	MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
	MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
	MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
	MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
	MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
	MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
	MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
	MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
	MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
	MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
	MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
	MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
	MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
	MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
	MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
	MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);
	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}

///////////////////////////////////////////////////////////////////////////////

// FUNCTION: GAME187 0x4988c0
BigInt::BigInt()
{
	mNegative = false;
	mData = NULL;
	mLength = 0;
	mCapacity = 0;
}

// FUNCTION: GAME187 0x498a60
BigInt::~BigInt()
{
	delete [] mData;
}

// FUNCTION: GAME187 0x498a80
void BigInt::Reserve(int theCapacity)
{
	mCapacity = theCapacity;
	unsigned short* aNewData = new unsigned short[theCapacity];
	if (mLength > 0)
		memcpy(aNewData, mData, mLength * sizeof(unsigned short));
	delete [] mData;
	mData = aNewData;
}

// FUNCTION: GAME187 0x498ad0
void BigInt::Grow()
{
	if (mCapacity == 0)
		Reserve(8);
	else
		Reserve(mCapacity * 2);
}

// FUNCTION: GAME187 0x498af0
void BigInt::Normalize()
{
	while ((mLength >= 1) && (mData[mLength - 1] == 0))
		mLength--;
}

// FUNCTION: GAME187 0x498b20
void BigInt::Clear()
{
	mNegative = false;
	mLength = 0;
}

// FUNCTION: GAME187 0x498b30
bool BigInt::IsOdd() const
{
	if (mLength == 0)
		return false;
	return (mData[0] & 1) != 0;
}

// FUNCTION: GAME187 0x498b50
bool BigInt::IsNegative() const
{
	return mNegative;
}

// FUNCTION: GAME187 0x498ba0
unsigned short BigInt::GetTopWord() const
{
	if (mLength == 0)
		return 0;
	return mData[mLength - 1];
}

// FUNCTION: GAME187 0x498bc0
unsigned short BigInt::GetWord(int theIndex) const
{
	if (theIndex >= mLength)
		return 0;
	return mData[theIndex];
}

// FUNCTION: GAME187 0x498be0
void BigInt::SetWord(int theIndex, unsigned short theWord)
{
	while (theIndex >= mLength + 1)
	{
		if (mLength >= mCapacity)
			Grow();
		mData[mLength++] = 0;
	}

	if (theIndex >= mLength)
	{
		if (mLength >= mCapacity)
			Grow();
		mData[mLength++] = theWord;
	}
	else
	{
		mData[theIndex] = theWord;
	}
}

int BigInt::GetLength() const
{
	return mLength;
}

// FUNCTION: GAME187 0x498e90
BigInt& BigInt::ShiftLeft(int theBits)
{
	int aLength = GetLength();
	int aWordShift = theBits / 16;

	if (aWordShift > 0)
	{
		while (mLength + aWordShift - 1 >= mCapacity)
			Grow();

		memcpy(mData + aWordShift, mData, mLength * sizeof(unsigned short));
		for (int i = 0; i < aWordShift; i++)
			mData[i] = 0;
		mLength += aWordShift;
	}

	int aCarry = 0;
	for (int i = 0; i < aLength; i++)
	{
		int aVal = aCarry + (GetWord(i + aWordShift) << (theBits % 16));
		aCarry = aVal >> 16;
		SetWord(i + aWordShift, aVal);
	}

	if (aCarry != 0)
		SetWord(aLength + aWordShift, aCarry);

	return *this;
}

// FUNCTION: GAME187 0x498f80
BigInt& BigInt::ShiftRight(int theBits)
{
	int aWordShift = theBits / 16;

	if (aWordShift > 0)
	{
		if (aWordShift >= mLength)
		{
			mLength = 0;
			mNegative = false;
			return *this;
		}

		mLength -= aWordShift;
		memcpy(mData, mData + aWordShift, mLength * sizeof(unsigned short));
	}

	int aCarry = 0;
	for (int i = GetLength() - 1; i >= 0; i--)
	{
		int aWord = GetWord(i);
		int aVal = aCarry + (aWord >> (theBits % 16));
		aCarry = (aWord << (16 - theBits % 16)) & 0xFFFF;
		SetWord(i, aVal);
	}

	Normalize();
	return *this;
}

// FUNCTION: GAME187 0x499030
bool BigInt::operator==(const BigInt& theBigInt) const
{
	if (mLength != theBigInt.mLength)
		return false;

	for (int i = 0; i < mLength; i++)
	{
		if (mData[i] != theBigInt.mData[i])
			return false;
	}

	return true;
}

// FUNCTION: GAME187 0x499070
bool BigInt::operator<(const BigInt& theBigInt) const
{
	if (IsNegative())
	{
		if (!theBigInt.IsNegative())
			return true;

		if (mLength > theBigInt.mLength)
			return true;
		if (mLength < theBigInt.mLength)
			return false;

		for (int i = mLength - 1; i >= 0; i--)
		{
			if (mData[i] < theBigInt.mData[i])
				return false;
			if (mData[i] > theBigInt.mData[i])
				return true;
		}
	}
	else
	{
		if (theBigInt.IsNegative())
			return false;

		if (mLength > theBigInt.mLength)
			return false;
		if (mLength < theBigInt.mLength)
			return true;

		for (int i = mLength - 1; i >= 0; i--)
		{
			if (mData[i] < theBigInt.mData[i])
				return true;
			if (mData[i] > theBigInt.mData[i])
				return false;
		}
	}

	return false;
}

// FUNCTION: GAME187 0x499120
bool BigInt::operator>(const BigInt& theBigInt) const
{
	return !(*this == theBigInt) && !(*this < theBigInt);
}

// FUNCTION: GAME187 0x499150
bool BigInt::operator>=(const BigInt& theBigInt) const
{
	return !(*this < theBigInt);
}

// FUNCTION: GAME187 0x499180
BigInt& BigInt::operator=(const BigInt& theBigInt)
{
	mLength = theBigInt.mLength;
	mCapacity = theBigInt.mCapacity;
	delete [] mData;
	mData = new unsigned short[mCapacity];
	if (mLength > 0)
		memcpy(mData, theBigInt.mData, mLength * sizeof(unsigned short));
	mNegative = theBigInt.mNegative;
	return *this;
}

// FUNCTION: GAME187 0x498900
BigInt::BigInt(int theValue)
{
	mData = new unsigned short[8];
	mCapacity = 8;
	mLength = 2;

	if (theValue < 0)
	{
		mNegative = true;
		theValue = -theValue;
	}
	else
		mNegative = false;

	mData[0] = (unsigned short) theValue;
	mData[1] = (unsigned short) (theValue >> 16);
	Normalize();
}

// FUNCTION: GAME187 0x498950
BigInt::BigInt(const BigInt& theBigInt) :
	mData(NULL),
	mLength(0),
	mCapacity(0)
{
	*this = theBigInt;
}

// FUNCTION: GAME187 0x498b60
int BigInt::GetBitLength() const
{
	unsigned short aTopWord = GetTopWord();
	for (int i = 15; i >= 0; i--)
	{
		if (aTopWord & (1 << i))
			return (mLength - 1) * 16 + i + 1;
	}
	return 0;
}

// FUNCTION: GAME187 0x498c50
void BigInt::DivMod(const BigInt& theDivisor, BigInt& theQuotient, BigInt& theRemainder) const
{
	theQuotient.Clear();

	if (theDivisor > *this)
	{
		theRemainder = *this;
		return;
	}

	int aShift = GetBitLength() - theDivisor.GetBitLength() + 1;
	theRemainder = *this;
	if (theRemainder.mCapacity == 0)
		theRemainder.Grow();
	theRemainder.ShiftRight(aShift);

	for (int aBit = aShift - 1; aBit >= 0; aBit--)
	{
		// theRemainder <<= 1
		int aCarry = 0;
		int i;
		for (i = 0; i < theRemainder.mLength; i++)
		{
			int aVal = aCarry + theRemainder.mData[i] * 2;
			aCarry = aVal >> 16;
			theRemainder.mData[i] = aVal;
		}
		if (aCarry != 0)
		{
			if (theRemainder.mLength == theRemainder.mCapacity)
				theRemainder.Grow();
			theRemainder.mData[theRemainder.mLength++] = aCarry;
		}

		int aWordIdx = aBit / 16;
		int aBitIdx = aBit % 16;

		if (mData[aWordIdx] & (1 << aBitIdx))
		{
			if (theRemainder.mLength == 0)
			{
				theRemainder.mLength = 1;
				theRemainder.mData[0] = 1;
			}
			else
				theRemainder.mData[0] |= 1;
		}

		if (theRemainder >= theDivisor)
		{
			if (aWordIdx >= theQuotient.mLength)
			{
				while (aWordIdx >= theQuotient.mCapacity)
					theQuotient.Grow();
				while (aWordIdx > theQuotient.mLength)
					theQuotient.mData[theQuotient.mLength++] = 0;
				theQuotient.mData[theQuotient.mLength++] = 1 << aBitIdx;
			}
			else
			{
				theQuotient.mData[aWordIdx] |= 1 << aBitIdx;
			}

			// theRemainder -= theDivisor
			int aBorrow = 0;
			for (i = 0; i < theDivisor.mLength; i++)
			{
				int aVal = theRemainder.mData[i] - theDivisor.mData[i] - aBorrow;
				if (aVal < 0)
				{
					aBorrow = 1;
					aVal += 0x10000;
				}
				else
					aBorrow = 0;
				theRemainder.mData[i] = aVal;
			}
			while (aBorrow > 0)
			{
				int aVal = theRemainder.mData[i] - aBorrow;
				if (aVal < 0)
				{
					aBorrow = 1;
					aVal += 0x10000;
				}
				else
					aBorrow = 0;
				theRemainder.mData[i] = aVal;
			}

			while ((theRemainder.mLength > 0) && (theRemainder.mData[theRemainder.mLength - 1] == 0))
				theRemainder.mLength--;
		}
	}

	theQuotient.mNegative = IsNegative() ^ theDivisor.IsNegative();
}

// FUNCTION: GAME187 0x499160
BigInt BigInt::operator-() const
{
	BigInt aResult(*this);
	aResult.mNegative = !aResult.mNegative;
	return aResult;
}

// FUNCTION: GAME187 0x499550
BigInt BigInt::operator-(const BigInt& theBigInt) const
{
	if (theBigInt.IsNegative())
		return *this + (-theBigInt);
	else if (IsNegative())
		return -((-*this) + theBigInt);
	else if (theBigInt > *this)
		return -(theBigInt - *this);

	BigInt aResult;
	int aLength = max(GetLength(), theBigInt.GetLength());
	int aBorrow = 0;
	for (int i = 0; i < aLength; i++)
	{
		int aVal = GetWord(i) - theBigInt.GetWord(i) - aBorrow;
		if (aVal < 0)
		{
			aBorrow = 1;
			aVal += 0x10000;
		}
		else
			aBorrow = 0;
		aResult.SetWord(i, aVal);
	}
	aResult.Normalize();
	return aResult;
}

// FUNCTION: GAME187 0x499840
BigInt BigInt::operator%(const BigInt& theBigInt) const
{
	BigInt aQuotient;
	BigInt aRemainder;
	DivMod(theBigInt, aQuotient, aRemainder);
	return aRemainder;
}

// FUNCTION: GAME187 0x4998c0
BigInt BigInt::operator<<(int theBits) const
{
	BigInt aResult(*this);
	aResult.ShiftLeft(theBits);
	return aResult;
}

// FUNCTION: GAME187 0x499930
BigInt BigInt::operator>>(int theBits) const
{
	BigInt aResult(*this);
	aResult.ShiftRight(theBits);
	return aResult;
}

// FUNCTION: GAME187 0x4999a0
BigInt BigInt::operator|(const BigInt& theBigInt) const
{
	BigInt aResult;
	int aLength = max(GetLength(), theBigInt.GetLength());
	for (int i = 0; i < aLength; i++)
		aResult.SetWord(i, GetWord(i) | theBigInt.GetWord(i));
	return aResult;
}

// FUNCTION: GAME187 0x4992c0
BigInt& BigInt::operator<<=(int theBits)
{
	*this = *this << theBits;
	return *this;
}

// FUNCTION: GAME187 0x499330
BigInt& BigInt::operator>>=(int theBits)
{
	*this = *this >> theBits;
	return *this;
}

// FUNCTION: GAME187 0x499390
BigInt& BigInt::operator|=(const BigInt& theBigInt)
{
	*this = *this | theBigInt;
	return *this;
}

// FUNCTION: GAME187 0x499400
BigInt BigInt::operator+(const BigInt& theBigInt) const
{
	if (theBigInt.IsNegative())
		return *this - (-theBigInt);
	else if (IsNegative())
		return theBigInt - (-*this);

	BigInt aResult;
	int aLength = max(GetLength(), theBigInt.GetLength());
	int aCarry = 0;
	for (int i = 0; i < aLength; i++)
	{
		int aVal = GetWord(i) + theBigInt.GetWord(i) + aCarry;
		aResult.SetWord(i, aVal);
		aCarry = aVal >> 16;
	}
	if (aCarry > 0)
		aResult.SetWord(aLength, aCarry);
	return aResult;
}

// FUNCTION: GAME187 0x498970
BigInt::BigInt(const std::string& theHexString) :
	mData(NULL),
	mLength(0),
	mCapacity(0)
{
	for (int i = 0; i < (int) theHexString.length(); i++)
	{
		char c = theHexString[i];
		int aVal = 0;
		if ((c >= '0') && (c <= '9'))
			aVal = c - '0';
		else if ((c >= 'a') && (c <= 'f'))
			aVal = c - 'a' + 10;
		if ((c >= 'A') && (c <= 'F'))
			aVal = c - 'A' + 10;

		*this <<= 4;
		BigInt aDigit(aVal);
		*this |= aDigit;
	}
}

// FUNCTION: GAME187 0x499250
BigInt& BigInt::operator+=(const BigInt& theBigInt)
{
	*this = *this + theBigInt;
	return *this;
}

// FUNCTION: GAME187 0x4996f0
BigInt BigInt::operator*(const BigInt& theBigInt) const
{
	BigInt aResult;
	for (int i = 0; i < theBigInt.GetLength(); i++)
	{
		int aMul = theBigInt.GetWord(i);
		BigInt aPartial;
		int aCarry = 0;
		for (int j = 0; j < GetLength(); j++)
		{
			unsigned int aVal = aMul * GetWord(j) + aCarry;
			aPartial.SetWord(i + j, aVal);
			aCarry = aVal >> 16;
		}
		if (aCarry != 0)
			aPartial.SetWord(i + GetLength(), aCarry);
		aResult += aPartial;
	}
	aResult.mNegative = IsNegative() ^ theBigInt.IsNegative();
	return aResult;
}

// FUNCTION: GAME187 0x499a60
BigInt BigInt::ModPow(const BigInt& theExponent, const BigInt& theModulus) const
{
	BigInt aResult(1);
	BigInt aBase(*this);
	BigInt anExponent(theExponent);

	while (anExponent > 0)
	{
		if (anExponent.IsOdd())
			aResult = (aResult * aBase) % theModulus;
		anExponent >>= 1;
		aBase = (aBase * aBase) % theModulus;
	}

	return aResult;
}

// FUNCTION: GAME187 0x499c10
BigInt Sexy::HashData(const char* theData, int theLen, int theBits)
{
	MD5Context aContext;
	unsigned char aDigest[16];

	MD5Init(&aContext);
	MD5Update(&aContext, (unsigned char const*) theData, theLen);
	MD5Final(aDigest, &aContext);

	BigInt aResult;
	int i;
	for (i = 0; i < 16; i++)
	{
		aResult <<= 8;
		BigInt aByte(aDigest[i]);
		aResult += aByte;
	}

	for (i = 0; i < (theBits + 3) / 4 - 32; i++)
	{
		aResult <<= 4;
		BigInt aNibble(i % 16);
		aResult += aNibble;
	}

	aResult.ShiftRight(aResult.GetBitLength() - theBits);
	return aResult;
}

// FUNCTION: GAME187 0x499d20
BigInt Sexy::HashData(const std::string& theData, int theBits)
{
	return HashData(theData.c_str(), theData.length(), theBits);
}

char* gKeyChars = "234679ACDEFGHJKLMNPQRTUVWXYZ";

// FUNCTION: GAME187 0x499d70
BigInt Sexy::KeyToInt(const std::string& theKey)
{
	std::string aKey;
	int i;
	for (i = 0; i < (int) theKey.length(); i++)
	{
		if (theKey[i] != ' ')
		{
			if (theKey[i] == '1')
				aKey += "L";
			else if ((theKey[i] == '0') || (theKey[i] == 'o') || (theKey[i] == 'O'))
				aKey += 'Q';
			else
				aKey += (char) toupper(theKey[i]);
		}
	}

	BigInt aResult;
	for (i = 0; i < (int) aKey.length(); i++)
	{
		char c = toupper(theKey[i]);
		if ((aKey.length() - i) % 6 == 0)
		{
			if (c != '-')
				return 0;
		}
		else
		{
			int aVal = -1;
			for (int j = 0; j < 28; j++)
			{
				if (gKeyChars[j] == c)
					aVal = j;
			}

			if (aVal == -1)
				return 0;

			aResult *= 28;
			aResult += aVal;
		}
	}

	return aResult;
}

// FUNCTION: GAME187 0x4991e0
BigInt& BigInt::operator*=(const BigInt& theBigInt)
{
	*this = *this * theBigInt;
	return *this;
}

// BigInt::`scalar deleting destructor'