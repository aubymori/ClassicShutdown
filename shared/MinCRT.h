#pragma once

#pragma intrinsic(memset)

#pragma function(memset)
inline void *memset(void *ptr, int value, size_t num)
{
	while (num--)
	{
		((unsigned char *)ptr)[num] = value;
	}
	return ptr;
}