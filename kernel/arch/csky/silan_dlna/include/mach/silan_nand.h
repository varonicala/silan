#ifndef __SILAN_NAND_H__
#define __SILAN_NAND_H__

/*linux/drivers/mtd/nand/slnand.c
 *Copyright(c)2011 Silan
 * lifei <lifei@silan.com.cn>
 *
 * Silan NAND device controller platfrom_device info
 *
 *Changelog:
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* struct sl_nand_set
 *
 * define an set of one or more nand chips registered with an unique mtd
 *
 * nr_chips	 = number of chips in this set
 * nr_partitions = number of partitions pointed to be partitoons (or zero)
 * name		 = name of set (optional)
 * nr_map	 = map for low-layer logical to physical chip numbers (option)
 * partitions	 = mtd partition list
*/
struct sl_nand_set 
{
	int			nr_chips;
	int			nr_partitions;
	char			*name;
	int			*nr_map;
	struct mtd_partition	*partitions;
};

struct sl_platform_nand {
	/* timing information for controller, all times in nanoseconds */

	int	tacls;	/* time for active CLE/ALE to nWE/nOE */
	int	twrph0;	/* active time for nWE/nOE */
	int	twrph1;	/* time for release CLE/ALE from nWE/nOE inactive */

	int			nr_sets;
	struct sl_nand_set *sets;

	void			(*select_chip)(struct sl_nand_set *,
					       int chip);
};

#define FLASH_EVENT_OPERATION_FINISH  					0x01
#define FLASH_STATUS_ERROR 								1

#define __REG(x) x
#define FLASH_FIFO_ADDR              					__REG(0x0000)
#define FLASH_ENABLE_REG								__REG(0x0004)  
#define FLASH_SYS_CLK_REG								__REG(0x0008) 

#define FLASH_REG_COMMAND1_0                            __REG(0xB000)
#define FLASH_REG_COMMAND1_1                            __REG(0xB100)
#define FLASH_REG_COMMAND1_2                            __REG(0xB200)
#define FLASH_REG_COMMAND2_0                            __REG(0xB300)
#define FLASH_REG_COMMAND2_1                            __REG(0xB400)
#define FLASH_REG_COMMAND2_2                            __REG(0xB500)
#define FLASH_REG_BASE_ADDR0                            __REG(0xB600)
#define FLASH_REG_BASE_ADDR1                            __REG(0xB700)
#define FLASH_REG_BASE_ADDR2                            __REG(0xB800)
#define FLASH_REG_BASE_ADDR3                            __REG(0xB900)
#define FLASH_REG_BASE_ADDR4                            __REG(0xBA00)
#define FLASH_REG_COPYBACK_ADDR0                    	__REG(0xBB00)
#define FLASH_REG_COPYBACK_ADDR1                    	__REG(0xBC00)
#define FLASH_REG_COPYBACK_ADDR2                    	__REG(0xBD00)
#define FLASH_REG_COPYBACK_ADDR3                    	__REG(0xBE00)
#define FLASH_REG_COPYBACK_ADDR4                    	__REG(0xBF00)
#define FLASH_REG_START_OP_0                            __REG(0xC000)
#define FLASH_REG_START_OP_1                            __REG(0xC100)   
#define FLASH_REG_RESET                                 __REG(0xC200)
#define FLASH_REG_INT                                   __REG(0xC300)
#define FLASH_REG_EOTR                                  __REG(0xC400)
#define FLASH_REG_STATUS                                __REG(0xC500)   
#define FLASH_REG_DATA_CHECK                            __REG(0xC600)  
#define FLASH_REG_DEBUG  			                  	__REG(0xC700)
#define FLASH_REG_CURRENT_ADDR0                    		__REG(0x0800)
#define FLASH_REG_CURRENT_ADDR1                     	__REG(0xC900)
#define FLASH_REG_CURRENT_ADDR2                     	__REG(0xCA00)
#define FLASH_REG_CURRENT_ADDR3                     	__REG(0xCB00)
#define FLASH_REG_CURRENT_ADDR4                     	__REG(0xCC00)
#define FLASH_REG_BUF_ADDR_PTR0			    			__REG(0xCD00) 
#define FLASH_REG_BUF_ADDR_PTR1			    			__REG(0xCE00)
#define FLASH_REG_INTMASK			                  	__REG(0xCF00)

#define FLASH_REG_COPYBACK_NUM		           			__REG(0xAF00)
#define FLASH_REG_CHIP_SELECT			           		__REG(0xAE00)
#define FLASH_REG_MODE_SELECT		                  	__REG(0xAD00)  
#define FLASH_REG_TIMING_SELECT		           			__REG(0xAC00)    
#define FLASH_REG_BASE_ADDR5                           	__REG(0xAB00)
#define FLASH_REG_COPYBACK_ADDR5                   		__REG(0xAA00)
#define FLASH_REG_CURRENT_ADDR5                     	__REG(0xA900)
#define FLASH_REG_EVENT				           			__REG(0xA800)
#define FLASH_REG_ONE_ERRS			           			__REG(0xA700)
#define FLASH_REG_TWO_ERRS			           			__REG(0xA600)
#define FLASH_REG_THREE_ERRS			           		__REG(0xA500)
#define FLASH_REG_FOUR_ERRS			           			__REG(0xA400) 	
#define FLASH_REG_START_OP_2				    		__REG(0xA300)                                                       
/**                                         		    
 * RESERVERD    0X0D0 ~ 0X0FF                            		
 */                      
#endif
