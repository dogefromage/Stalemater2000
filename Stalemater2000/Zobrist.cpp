#include "Zobrist.h"
#include <stdlib.h>     /* srand, rand */

#define RANDOM_8 ((unsigned long long)(rand() % 0xFF))
#define RANDOM_16 ((RANDOM_8  <<  8) | RANDOM_8)
#define RANDOM_32 ((RANDOM_16 << 16) | RANDOM_16)
#define RANDOM_64 ((RANDOM_32 << 32) | RANDOM_32)

unsigned long long ZobristValues[zobristTableSize];

void InitZobrist()
{
	for (int i = 0; i < zobristTableSize; i++)
	{
		ZobristValues[i] = RANDOM_64;
	}
}

#undef RANDOM_8
#undef RANDOM_16
#undef RANDOM_32
#undef RANDOM_64