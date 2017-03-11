#pragma once

/*
inline unsigned int fast_hash(char* s)
{
	char* p;
	unsigned int g, h=0;
	for (p=s; (*p)!='\0'; p++)
	{
		h = (h<<4) + (*p);
		if (g=h&0xF0000000) h = h^(g>>24); h=h^g;
	}
	return h;
}
*/

inline unsigned int fast_hash(char* key)
{
	unsigned int hash = 2166136261;
	for (const char *s = key; *s; s++)
		hash = (16777619 * hash) ^ (*s);
	return hash;
}