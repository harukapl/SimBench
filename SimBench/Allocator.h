#pragma once

#include "Globals.h"

template <typename T>
class Allocator
{
private:
	vector<T*> memory_list;
	int block_size;
	T* memory;
	int count;
public:
	Allocator(int blockSize):memory(NULL),count(0),block_size(blockSize)
	{
		memory = (T*)calloc(block_size,sizeof(T));
		memory_list.push_back(memory);
	}
	~Allocator()
	{
		for(int x = 0; x < memory_list.size(); x++)
			free(memory_list[x]);
	}
	inline T* Add()
	{
		if(count <block_size)
		{
			++count;
			return (memory+count-1);
		}
		else
		{
			count = 1;
			memory = (T*)calloc(1000,sizeof(T));
			memory_list.push_back(memory);
			return memory;
		}
	}
	void GetMemoryBlockCount()
	{
		int block = memory_list.size();
		printf("Memory block in use: %i. Element use: %i\n",block,block_size*(block-1)+count);
	}
};