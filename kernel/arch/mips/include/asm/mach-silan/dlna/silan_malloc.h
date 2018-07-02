/***************************************************************
*file:sie_malloc.h
*auther:sie
*date:2007-11-27
***************************************************************/

#ifndef __SIE_MALLOC__
#define __SIE_MALLOC__
#define SILAN_MALLOC_SIZE 0x4000000 //64m
#define TOTAL_RAM_FRAG			128
//#define TOTAL_RAM_FRAG			64
typedef struct ram_frag{
	unsigned char * ram_start;
	unsigned int ram_size;
	struct ram_frag * next;
	struct ram_frag * pre;
} ram_frag_t;

typedef struct sie_heap_ctrl{
	unsigned char * ram_address;
	unsigned int ram_size;
	ram_frag_t * ram_fragment;
	ram_frag_t * used;
	ram_frag_t * free;
} sie_heap_ctrl_t;

extern int init_sie_heap(unsigned char * start_addr, unsigned int size);
extern void free_sie_heap(void);
extern unsigned char * sie_malloc(unsigned int size);
extern int sie_free(unsigned char * addr);

#endif
