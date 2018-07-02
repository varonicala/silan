/*
 *  linux/arch/csky/lib/memset.c
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file "COPYING" in the main directory of this archive
 *  for more details.
 *
 *  Copyright (C) 2009  Hangzhou C-SKY Microsystems.
 *
 */

#include <linux/types.h>

/*
 * memory set function.
 */
void *memset(void *dest, int c, size_t l)
{
	char   *d = dest;
	int    ch = c;

	if ((long)d & 0x3)
	{
		while (l--) *d++ = ch;
	}
	else
	{
		ch &= 0xff; 
		int tmp = (ch | ch << 8 | ch << 16 | ch << 24);
        
		while (l >= 16)
		{
			*(((long *)d)) = tmp;
			*(((long *)d)+1) = tmp;
			*(((long *)d)+2) = tmp;
			*(((long *)d)+3) = tmp;
			l -= 16;
			d += 16;
		}
		while (l > 3)
		{
			*(((long *)d)) = tmp;
			d = d + 4;
			l -= 4;
		}
		while (l)
		{
			*d++ = ch;
			l--;
		}
	}
	return dest;
}
