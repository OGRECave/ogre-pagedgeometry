/*-------------------------------------------------------------------------------------
Copyright (c) 2006 John Judnich

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
-------------------------------------------------------------------------------------*/

#ifndef __RandomTable_H__
#define __RandomTable_H__

#include <OgrePrerequisites.h>
#include <OgreMath.h>

#include <random>
#ifdef _WIN32
#define _WIN32_WINNT 0x0600
#include <windows.h>
#endif

// random table class that speeds up PG a bit
class RandomTable
{
public:
	RandomTable(unsigned long size=0x8000);
	~RandomTable();

	void resetRandomIndex();
	float getUnitRandom();
	float getRangeRandom(float start, float end);

protected:
	unsigned long tableSize;
	float *table;
	unsigned long customRandomIndex;

	void generateRandomNumbers();	
};


// implementation below
inline RandomTable::RandomTable(unsigned long size) : tableSize(size), table(0), customRandomIndex(0)
{
	table = (float *)malloc(sizeof(float) * tableSize);
	generateRandomNumbers();
}

inline RandomTable::~RandomTable()
{
	if(table)
	{
		free(table);
		table=0;
	}
}

inline void RandomTable::resetRandomIndex()
{
	customRandomIndex = 0;
}

inline float RandomTable::getUnitRandom()
{
	// prevent against overflow
	if(customRandomIndex > tableSize - 1)
		customRandomIndex = 0;
	return table[customRandomIndex++];
}

inline float RandomTable::getRangeRandom(float start, float end)
{
	return (start + ((end - start) * getUnitRandom()));
}

inline void RandomTable::generateRandomNumbers()
{
	// using our Mersenne Twister (preferred way)
    std::mt19937 rng;
	for(unsigned long i = 0; i < tableSize; i++)
		table[i] = float(rng())/rng.max();
}

#endif // __RandomTable_H__

