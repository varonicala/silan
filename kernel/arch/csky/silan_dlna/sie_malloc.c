/***************************************************************
*file:sie_malloc.c
*auther:sie
*date:2007-11-27
***************************************************************/

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/highmem.h>
#include <linux/firmware.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <asm/io.h>
#include <silan_malloc.h>
#include <silan_def.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/mips-boards/prom.h>
#ifndef null
#define null 0
#endif

#ifndef NULL
#define NULL	0
#endif

#define ERR_OCCUR	(-1)
#define NONE_ERR	(0)

#define TRUE (1)
#define FALSE (0)


#ifdef __USE_OS_SCHED__
#define RESTRICT_IN
#define RESTRICT_OUT
#else
#define RESTRICT_IN
#define RESTRICT_OUT
#endif



#define sw_endian_dw(a) (a)
#define sw_endian_w(a) (a)

//#define DEBUG
#define __USE_STATIC_FRAG_DATA__
#ifdef DEBUG
#define INFO_PRINT(a)	printk(a)
#define ERR_PRINT(a)		printk(a)
#define PRIVATE
#else
#define INFO_PRINT(a)
#define ERR_PRINT(a)
#define PRIVATE static
#endif


#define ASSIGNED_FOR_32BIT(size, sz)		do{if(sz == 0)	size = 4;\
											else		size = (sz+3)&(~3);}while(0)

//add by bly
#define ASSIGNED_FOR_8BYTE(size, sz)		do{if(sz == 0)	size = 8;\
											else		size = (sz+7)&(~7);}while(0)


//1024 byte ALIGN
#define ASSIGNED_FOR_1024BYTE(size, sz)     do{if(sz == 0)  size = 1024;\
	                                        else        size = (sz+1023)&(~1023);}while(0)


extern int prom_putchar(char c);

PRIVATE sie_heap_ctrl_t sie_heap_ctrl;
PRIVATE ram_frag_t * mem_frag_free;
#ifdef __USE_STATIC_FRAG_DATA__
PRIVATE ram_frag_t mem_frag_pool[TOTAL_RAM_FRAG];
#endif


PRIVATE void init_mem_frag_pool(void);
PRIVATE ram_frag_t * alloc_mem_frag(void);

PRIVATE void init_mem_frag_pool(void)
{
	int i;
	ram_frag_t * tmp, * tmp2;
	mem_frag_free = &(sie_heap_ctrl.ram_fragment[0]);
	tmp = mem_frag_free;
	tmp->pre = null;
	for(i=1; i<TOTAL_RAM_FRAG; i++)
	{
		tmp2 = &(sie_heap_ctrl.ram_fragment[i]);
		tmp->next = tmp2;
		tmp2->pre = tmp;
		tmp2->next = null;
		tmp = tmp2;
	}
}

PRIVATE ram_frag_t * alloc_mem_frag(void)
{
	ram_frag_t * tmp;
	if(mem_frag_free == null)
	{
		INFO_PRINT(("info@alloc_mem_frag:there is not free frag!\n"));
		return null;
	}
	tmp = mem_frag_free;
	mem_frag_free = mem_frag_free->next;
	if(mem_frag_free != null)
		mem_frag_free->pre = null;
	return tmp;
}

PRIVATE int if_frag_valid(ram_frag_t * frag, char * func_name)
{
	if(frag == null)
	{
		ERR_PRINT(("err@%s:frag==null!\n", func_name));
		return ERR_OCCUR;
	}
	if(frag < sie_heap_ctrl.ram_fragment || frag > &(sie_heap_ctrl.ram_fragment[TOTAL_RAM_FRAG - 1]))
	{
		ERR_PRINT(("err@%s:frag is out of range!\n", func_name));
		return ERR_OCCUR;
	}
	return NONE_ERR;
}

PRIVATE int free_mem_frag(ram_frag_t * frag)
{
	if(ERR_OCCUR == if_frag_valid(frag, "free_mem_frag"))
		return ERR_OCCUR;
	frag->pre = null;
	frag->next = mem_frag_free;
	if(mem_frag_free != null)
		mem_frag_free->pre = frag;
	mem_frag_free = frag;
	return NONE_ERR;
}


/****************

addr:the phys addr of memory,must 8-byte align now
sz: the size of the phys memory


*******************/
int init_sie_heap(unsigned char * addr, unsigned int sz)
{
	ram_frag_t * tmp;
	unsigned char * start_addr;
#ifndef __USE_STATIC_FRAG_DATA__
	unsigned int ctrl_sz,ctrl_sz_tmp;
#endif
	unsigned int size;
	RESTRICT_IN;
#ifdef __USE_STATIC_FRAG_DATA__
	
	sie_heap_ctrl.ram_fragment = mem_frag_pool;
	start_addr = addr;
	size = sz;
#else
	ctrl_sz_tmp = sizeof(ram_frag_t) * TOTAL_RAM_FRAG;

	ASSIGNED_FOR_8BYTE(ctrl_sz, ctrl_sz_tmp);	


	if(sz <= ctrl_sz)
	{
		ERR_PRINT(("err@init_sie_heap:sz<=ctrl_sz!\n"));
		goto failed;
	}
	//sie_heap_ctrl.ram_fragment = (sie_heap_ctrl_t *)addr;
	sie_heap_ctrl.ram_fragment = (sie_heap_ctrl_t *)ioremap(addr,ctrl_sz);  //todo
	start_addr = addr + ctrl_sz;
	size = sz - ctrl_sz;
#endif

	sie_heap_ctrl.free=kzalloc(sizeof(ram_frag_t),GFP_KERNEL);
	sie_heap_ctrl.used=kzalloc(sizeof(ram_frag_t),GFP_KERNEL);


	init_mem_frag_pool();
	tmp = alloc_mem_frag();
	if(tmp == null)
	{
		ERR_PRINT(("err@init_file_buffer:tmp==null!\n"));
		goto failed;
	}
	tmp->ram_start = start_addr;
	tmp->ram_size = size;
	tmp->next = null;
	tmp->pre = null;
	sie_heap_ctrl.ram_address = start_addr;
	sie_heap_ctrl.ram_size = size;
	sie_heap_ctrl.free = tmp;
	sie_heap_ctrl.used = null;
	RESTRICT_OUT;
	return NONE_ERR;
failed:
	RESTRICT_OUT;
#ifndef __USE_STATIC_FRAG_DATA__
	if(sie_heap_ctrl.ram_fragment)
	{
		iounmap(sie_heap_ctrl.ram_fragment);
	}
#endif
	return ERR_OCCUR;
}

void free_sie_heap(void)

{

	kfree(sie_heap_ctrl.free);
	kfree(sie_heap_ctrl.used);
#ifndef __USE_STATIC_FRAG_DATA__
	if(sie_heap_ctrl.ram_fragment)
	{
		iounmap(sie_heap_ctrl.ram_fragment);
	}
#endif
}

PRIVATE ram_frag_t * find_fit_frag(unsigned int size)
{
	ram_frag_t * tmp = sie_heap_ctrl.free;

	if(tmp == null)
	{
		INFO_PRINT(("info@find_fit_frag:mem is over!\n"));
		return null;
	}
	while(tmp != null)
	{
		if(size <= tmp->ram_size)
			break;
		tmp = tmp->next;
	}
	return tmp;
}

PRIVATE int get_out_of_free_list(ram_frag_t * frag)
{
	if(ERR_OCCUR == if_frag_valid(frag, "get_out_of_free_list"))
		return ERR_OCCUR;
	if(frag->pre == null)	/*frag is @ list head*/
	{
		sie_heap_ctrl.free = frag->next;
		if(frag->next != null)	/*frag is not @ list tail*/
			frag->next->pre = null;
	}
	else		/*frag is not @ list head*/
	{
		frag->pre->next = frag->next;
		if(frag->next != null)	/*frag is not @ list tail*/
			frag->next->pre = frag->pre;
	}
	return NONE_ERR;
}


PRIVATE int add_into_free_list(ram_frag_t * frag)
{
	ram_frag_t * tmp;
	if(ERR_OCCUR == if_frag_valid(frag, "add_into_free_list"))
		return ERR_OCCUR;
	tmp = sie_heap_ctrl.free;
	if(tmp == null)
	{
		frag->next = null;
		frag->pre = null;
		sie_heap_ctrl.free = frag;
		return NONE_ERR;
	}
	while(tmp != null)
	{
		if(frag->ram_size <= tmp->ram_size)
		{
			frag->pre = tmp->pre;
			frag->next = tmp;
			tmp->pre = frag;
			if(frag->pre == null)
				sie_heap_ctrl.free = frag;
			else
				frag->pre->next = frag;
			return NONE_ERR;
		}
		if(tmp->next == null)
		{
			tmp->next = frag;
			frag->pre = tmp;
			frag->next = null;
			return NONE_ERR;
		}
		tmp = tmp->next;
	}
	return NONE_ERR;
}

#define AT_THIS_ADDR 		0
#define BEFORE_THIS_ADDR	1

PRIVATE ram_frag_t * find_frag_by_addr(void * addr, 
										ram_frag_t * head, 
										int mode,
										char * func_name)
{
	int is_true;
	ram_frag_t * tmp = head;
	
	if(addr == null)
	{
		ERR_PRINT(("err@%s:addr==null!\n", func_name));
		return null;
	}
	while(tmp != null)
	{
		switch(mode)
		{
			case AT_THIS_ADDR:
				is_true = (tmp->ram_start == addr);
				break;
			case BEFORE_THIS_ADDR:
				is_true = (tmp->ram_start + tmp->ram_size == addr);
				break;
			default:
				is_true = 0;
				break;
		}
		if(is_true)
			break;
		tmp = tmp->next;
	}
	return tmp;
}


PRIVATE int get_out_of_used_list(ram_frag_t * frag)
{
	if(ERR_OCCUR == if_frag_valid(frag, "get_out_of_used_list"))
		return ERR_OCCUR;
	if(frag->pre == null)	/*frag is @ list head*/
	{
		sie_heap_ctrl.used = frag->next;
		if(frag->next != null)	/*frag is not @ list tail*/
			frag->next->pre = null;
	}
	else		/*frag is not @ list head*/
	{
		frag->pre->next = frag->next;
		if(frag->next != null)	/*frag is not @ list tail*/
			frag->next->pre = frag->pre;
	}
	return NONE_ERR;
}

PRIVATE int add_into_used_list(ram_frag_t * frag)
{
	if(ERR_OCCUR == if_frag_valid(frag, "add_into_used_list"))
		return ERR_OCCUR;
	frag->next = sie_heap_ctrl.used;
	frag->pre = null;
	if(sie_heap_ctrl.used != null)
		sie_heap_ctrl.used->pre = frag;
	sie_heap_ctrl.used = frag;
	return NONE_ERR;
}

unsigned char * sie_malloc(unsigned int sz)
{
	ram_frag_t * tmp;
	ram_frag_t * new_tmp;
	unsigned int size;

	RESTRICT_IN;
    ASSIGNED_FOR_1024BYTE(size, sz);
/*find a fit fragment in free list*/	
	tmp = find_fit_frag(size);
	if(tmp == null)
	{
		INFO_PRINT(("info@alloc_file_buffer:file fit frag failed!\n"));
		goto failed;
	}
/*alloc a frag blk from frag pool*/
	new_tmp = alloc_mem_frag();
	if(new_tmp == null)
	{
		INFO_PRINT(("info@alloc_file_buffer:alloc frag failed!\n"));
		goto failed;
	}
/*get the fit frag out of free list*/
	if(ERR_OCCUR == get_out_of_free_list(tmp))
	{
		ERR_PRINT(("err@alloc_file_buffer:get frag out of free list failed!\n"));
		goto failed;
	}
/*creat new frag blk*/
	new_tmp->ram_start = tmp->ram_start;
	new_tmp->ram_size = size;
/*add new frag into used list*/
	if(ERR_OCCUR == add_into_used_list(new_tmp))
	{
		ERR_PRINT(("err@alloc_file_buffer:add frag into used list failed!\n"));
		goto failed;
	}
/*deal the old frag blk*/
	tmp->ram_start += size;
	tmp->ram_size -= size;
	if(tmp->ram_size == 0)
	{
		if(ERR_OCCUR == free_mem_frag(tmp))
		{
			ERR_PRINT(("err@alloc_file_buffer:free mem frag failed!\n"));
			goto failed;
		}
	}
	else
	{
		if(ERR_OCCUR == add_into_free_list(tmp))
		{
			ERR_PRINT(("err@alloc_file_buffer:add frag into free list failed!\n"));
			goto failed;
		}
	}
	RESTRICT_OUT;
	return (new_tmp->ram_start);
failed:
	RESTRICT_OUT;
	return null;
}

int sie_free(unsigned char * addr)
{
	ram_frag_t * tmp, * tmp1, * tmp2;
	RESTRICT_IN;
/*find used frag from used list*/
	tmp = find_frag_by_addr(addr, sie_heap_ctrl.used, AT_THIS_ADDR, "free_file_buffer");
	if(ERR_OCCUR == if_frag_valid(tmp, "free_file_buffer"))
	{
		ERR_PRINT(("err@free_file_buffer:find used frag failed!\n"));
		goto right_end;
	}
/*get frag out of used list*/
	if(ERR_OCCUR == get_out_of_used_list(tmp))
	{
		ERR_PRINT(("err@free_file_buffer:get frag out of used list failed!\n"));
		goto right_end;
	}
/*find frag that the mem addr is right after the mem addr of tmp frag*/
	tmp1 = find_frag_by_addr(addr + tmp->ram_size , sie_heap_ctrl.free, AT_THIS_ADDR, "free_file_buffer");
/*find frag that the mem addr is right before the mem addr of tmp frag*/
	tmp2 = find_frag_by_addr(addr, sie_heap_ctrl.free, BEFORE_THIS_ADDR, "free_file_buffer");
	if(tmp1 == null && tmp2 == null)
		goto right_end;
	if(tmp1 != null && tmp2 == null)
	{
		if(ERR_OCCUR != if_frag_valid(tmp1, "free_file_buffer"))
		{
			if(ERR_OCCUR == get_out_of_free_list(tmp1))
				goto failed;
			if(ERR_OCCUR == free_mem_frag(tmp1))
				goto failed;
			tmp->ram_size += tmp1->ram_size;
		}
		else
			goto failed;
		goto right_end;
	}
	if(tmp1 == null && tmp2 != null)
	{
		if(ERR_OCCUR != if_frag_valid(tmp2, "free_file_buffer"))
		{
			if(ERR_OCCUR == get_out_of_free_list(tmp2))
				goto failed;
			if(ERR_OCCUR == free_mem_frag(tmp))
				goto failed;
			tmp2->ram_size += tmp->ram_size;
			tmp = tmp2;
		}
		else
			goto failed;
		goto right_end;
	}
	if(tmp1 != null && tmp2 != null)
	{
		
		if((ERR_OCCUR != if_frag_valid(tmp1, "free_file_buffer")) &&
			(ERR_OCCUR != if_frag_valid(tmp2, "free_file_buffer")))
		{
			if(ERR_OCCUR == get_out_of_free_list(tmp1))
				goto failed;
			if(ERR_OCCUR == free_mem_frag(tmp1))
				goto failed;
			if(ERR_OCCUR == get_out_of_free_list(tmp2))
				goto failed;
			if(ERR_OCCUR == free_mem_frag(tmp))
				goto failed;
			tmp2->ram_size += tmp->ram_size + tmp1->ram_size;
			tmp = tmp2;
		}
		else
			goto failed;
		goto right_end;
	}

right_end:
	if(ERR_OCCUR == add_into_free_list(tmp))
		goto failed;
	RESTRICT_OUT;
	return NONE_ERR;
failed:
	ERR_PRINT(("err@free_file_buffer!\n"));
	RESTRICT_OUT;
	return ERR_OCCUR;
}

#ifdef DEBUG

void printf_frag_pool()
{
	ram_frag_t * frag1, * frag2;
	INFO_PRINT(("frag_pool:\n"));
	INFO_PRINT(("free--->"));
	frag1 = mem_frag_free;
	frag2 = frag1;
	while(frag1 != null)
	{
		frag2 = frag1;
		INFO_PRINT(("0x%08x->", frag1));
		frag1 = frag1->next;
	}
	INFO_PRINT(("null\n"));
	INFO_PRINT(("=======>"));
	while(frag2 != null)
	{
		INFO_PRINT(("0x%08x=>", frag2));
		frag2 = frag2->pre;
	}
	INFO_PRINT(("null\n"));
}

void printf_file_buf_ctrl()
{
	int i;
	int count;
	ram_frag_t * free=null, * used=null, * pool=null;
	ram_frag_t * free0=null, * used0=null, * pool0=null;
	INFO_PRINT(("sie_heap_ctrl:\n"));
	INFO_PRINT(("    "));
	INFO_PRINT(("ram_addr: 0x%08x\n", sie_heap_ctrl.ram_address));
	INFO_PRINT(("    "));
	INFO_PRINT(("ram_size:  %d\n", sie_heap_ctrl.ram_size));
	INFO_PRINT(("    "));
	INFO_PRINT(("ram_frags:\n"));
	for(i=0; i<TOTAL_RAM_FRAG; i++)
	{
		INFO_PRINT(("    "));INFO_PRINT(("    "));
		INFO_PRINT(("start: 0x%08x,  size: %08d, next: 0x%08x, pre: 0x%08x\n", 
				sie_heap_ctrl.ram_fragment[i].ram_start, 
				sie_heap_ctrl.ram_fragment[i].ram_size, 
				sie_heap_ctrl.ram_fragment[i].next,
				sie_heap_ctrl.ram_fragment[i].pre));
	}
	free = sie_heap_ctrl.free;
	used = sie_heap_ctrl.used;
	pool = mem_frag_free;
	INFO_PRINT(("    "));
	INFO_PRINT(("pool--->"));
	count = 0;
	while(pool != null)
	{
		if(count > TOTAL_RAM_FRAG)	{printf("OVERFLOW!!\n");break;}
		count++;
		pool0 = pool;
		INFO_PRINT(("0x%08x->", pool));
		pool = pool->next;
	}
	INFO_PRINT(("null\n"));
	INFO_PRINT(("    "));
	INFO_PRINT(("pool===>"));
	count = 0;
	while(pool0 != null)
	{
		if(count > TOTAL_RAM_FRAG)	{printf("OVERFLOW!!\n");break;}
		count++;
		INFO_PRINT(("0x%08x=>", pool0));
		pool0 = pool0->pre;
	}
	INFO_PRINT(("null\n"));
	INFO_PRINT(("    "));
	INFO_PRINT(("free--->"));
	count = 0;
	while(free != null)
	{
		if(count > TOTAL_RAM_FRAG)	{printf("OVERFLOW!!\n");break;}
		count++;
		free0 = free;
		INFO_PRINT(("0x%08x->", free));
		free = free->next;
	}
	INFO_PRINT(("null\n"));
	INFO_PRINT(("    "));
	INFO_PRINT(("free===>"));
	count = 0;
	while(free0 != null)
	{
		if(count > TOTAL_RAM_FRAG)	{printf("OVERFLOW!!\n");break;}
		count++;
		INFO_PRINT(("0x%08x=>", free0));
		free0 = free0->pre;
	}
	INFO_PRINT(("null\n"));
	INFO_PRINT(("    "));
	INFO_PRINT(("used--->"));
	count = 0;
	while(used != null)
	{
		if(count > TOTAL_RAM_FRAG)	{printf("OVERFLOW!!\n");break;}
		count++;
		used0 = used;
		INFO_PRINT(("0x%08x->", used));
		used = used->next;
	}
	INFO_PRINT(("null\n"));
	INFO_PRINT(("    "));
	INFO_PRINT(("used===>"));
	count = 0;
	while(used0 != null)
	{
		if(count > TOTAL_RAM_FRAG)	{printf("OVERFLOW!!\n");break;}
		count++;
		INFO_PRINT(("0x%08x=>", used0));
		used0 = used0->pre;
	}
	INFO_PRINT(("null\n"));
}

void printf_ram_use_status()
{
	int count;
	ram_frag_t * free, * used;
	free = sie_heap_ctrl.free;
	used = sie_heap_ctrl.used;
	INFO_PRINT(("FREE:>\n"));
	count = 0;
	while(free != null)
	{
		if(count > TOTAL_RAM_FRAG)	{printf("OVERFLOW!!\n");break;}
		count++;
		INFO_PRINT(("----0x%08x----\n", free->ram_start));
		INFO_PRINT(("|                 |\n"));
		INFO_PRINT(("====0x%08x====\n\n", free->ram_start + free->ram_size));
		free = free->next;
	}
	INFO_PRINT(("USED:>\n"));
	count = 0;
	while(used != null)
	{
		if(count > TOTAL_RAM_FRAG)	{printf("OVERFLOW!!\n");break;}
		count++;
		INFO_PRINT(("----0x%08x----\n", used->ram_start));
		INFO_PRINT(("|                 |\n"));
		INFO_PRINT(("====0x%08x====\n\n", used->ram_start + used->ram_size));
		used = used->next;
	}
}

#endif



static int __init silan_malloc_init(void)
{
	int ret;
	unsigned int phys_addr_malloc;
	phys_addr_malloc=prom_phy_mem_malloc(SILAN_MALLOC_SIZE, SILAN_DEV_DUMMY);
	printk("\n\n*****phys_addr_sie_malloc:%p\n\n",(unsigned int *)phys_addr_malloc);

	ret = init_sie_heap((unsigned char *)phys_addr_malloc,SILAN_MALLOC_SIZE);
	if(ret<0)
	{
		printk("\n*****silan_malloc:init_sie_heap failed****\n");
	}
	return ret;
}

static void __exit silan_malloc_exit(void)
{
	
}
module_init(silan_malloc_init);
module_exit(silan_malloc_exit);

