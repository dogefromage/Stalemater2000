#include "BitMath.h"

#define U64 unsigned long long

//https://stackoverflow.com/questions/45221914/number-of-trailing-zeroes
int trailingZeros(U64 x)
{
	int n;
	if (x == 0) return 64;
	n = 1;
	if ((x & 0x00000000FFFFFFFF) == 0) { n += 32; x >>= 32; }
	if ((x & 0x000000000000FFFF) == 0) { n += 16; x >>= 16; }
	if ((x & 0x000000000000FFFF) == 0) { n += 16; x >>= 16; }
	if ((x & 0x00000000000000FF) == 0) { n += 8; x >>= 8; }
	if ((x & 0x000000000000000F) == 0) { n += 4; x >>= 4; }
	if ((x & 0x0000000000000003) == 0) { n += 2; x >>= 2; }
	return n - (x & 1);
}

void popLSB(U64 &bb)
{
	bb ^= 1ULL << trailingZeros(bb);
}

// use lookup to reverse u64
U64 reverse(U64 n)
{
	U64 rev = 0;
	for (int i = 0; i < 8; i++)
	{
		rev <<= 8;
		rev |= REVERSED_BYTES[n & 0xFF];
		n >>= 8;
	}
	return rev;
}

unsigned int countBits(U64 n)
{
	if (n == 0) return 0;

	int i = 0;
	for (int j = 0; j < 8; j++)
	{
		i += NUMBER_OF_BITS[n & 0xFF];
		n >>= 8;
	}
	return i;
}

#undef U64