#pragma once

#include "Globals.h"

class UniformGenerator
{
private:
	int seed;
	double low_level;
	double high_level;
	static const int max = 2147483647;
	static const int a = 16807;
	static const int q = 127773;
	static const int r = 2836;
public:
	UniformGenerator( int _seed, double low , double high ): seed( _seed ), low_level( low ), high_level( high ) {}
	double NextValue()
	{
		int h = seed/q;
		seed = a * ( seed - q * h ) - r * h;
		if(seed < 0) seed+=max;
		return ( (double)seed/(double)max*( high_level-low_level ) + low_level );
	}
};