#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/ioport.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/device.h>
#include <linux/io.h>
#include <asm/irq.h>
#include <silan_padmux.h>
#include <silan_reset.h>

#include "i2c-silan.h"

#define I2C_DRV_NAME "silan-i2c"
#define DEV_CMD_TIMEOUT 2*HZ
#define I2C_BUS_CLK 400000

#define silan_i2c_pad(x)	silan_padmux_ctrl(SILAN_PADMUX_I2C_##x,PAD_ON);
//#define SINGLE_REG_MODE	
#define I2C_TASKLET
struct silan_i2c 
{
	spinlock_t		    lock;
	unsigned int        suspended:1;
	struct completion	*done;
	struct i2c_msg		*msg;
	unsigned int		msg_num;
	unsigned int		msg_idx;
	unsigned int		msg_ptr;
	unsigned int 		msg_cnt;	

	unsigned int		tx_setup;
	unsigned int		irq;

	enum silan_i2c_state	state;
	unsigned long		clkrate;
	unsigned long 		clk_rate;
	unsigned int 		data_flag;
	unsigned int 		data_num;

	unsigned int 		*data_regs[4];

	wait_queue_head_t	wait;
	I2CREG				*regs;
	struct clk			*clk;
	struct device		*dev;
	struct resource		*ioarea;
	struct i2c_adapter	adap;
	struct timeval      time_clk;
	struct tasklet_struct tasklet;
};

int set_buf_data(char *buf,int val,int num)
{
	int i;
	for(i = 0;i < num;i++)
	{
		buf[i] = (val>>(8*i))&0xff;
	}
	return 0;
}

int get_buf_data(char *buf,int num)
{
	int val = 0,i;
	for(i = 0;i < num;i++)
	{
		val |= (buf[i]&0xff)<<(8*i);
	}
	return val;
}

static void silan_i2c_message_start(struct silan_i2c *i2c,
				      struct i2c_msg *msg)
{
	unsigned int addr = (msg->addr)<<1;
	unsigned int reg_value =0;
#ifndef I2C_TASKLET
	DECLARE_COMPLETION_ONSTACK(complete);
	i2c->done = &complete;
#endif
	writel(MBNR_SINGLE,&i2c->regs->MBNR);
	writel(addr,&i2c->regs->MBDR0);

	reg_value = START_CONFIG;	
	writel(reg_value,&i2c->regs->MBCR);
#ifndef I2C_TASKLET
	wait_for_completion(i2c->done);	
#endif
}

static void silan_i2c_stop(struct silan_i2c *i2c)
{
	int reg_value = 0;
	reg_value = STOP_CONFIG;
	writel(reg_value,&i2c->regs->MBCR);
	writel(CLEAR_INT_BIT,&i2c->regs->MBSR);
}

int  silan_i2c_get_status(struct silan_i2c *i2c,int num)
{
	unsigned int status,ret;
	status = readl(&i2c->regs->MBSR);
	ret = (status & WRITE_DONE_BIT)&&(status & BIT_COMPLETE(num)); 
	return ret;

}

#ifndef I2C_TASKLET
static void i2c_silan_irq_nextbyte(struct silan_i2c *i2c, unsigned long iicstat)
{
	unsigned int num = 0;
	unsigned int reg_value =0;
	int val,tmp;
	#ifdef SINGLE_REG_MODE
		long delay;
		int status;
	#else
		int i;
	#endif
	switch (i2c->state) 
	{
		case STATE_IDLE:
			break;

		case STATE_STOP:
			reg_value = readl(&i2c->regs->MBCR);
			writel(reg_value & ~MBCR_MIEN, &i2c->regs->MBCR);
			break;

		case STATE_START:
			if (i2c->msg->flags & I2C_M_RD)
				i2c->state = STATE_READ;
			else
				i2c->state = STATE_WRITE;
		
			if (i2c->state == STATE_READ)
				goto prepare_read;

		case STATE_WRITE:
			if(i2c->msg->len - i2c->msg_cnt >0)
			{
			#ifdef SINGLE_REG_MODE 
				if((i2c->msg->len - i2c->msg_cnt)/FIFO_LEN > 0)
				{
					num = FIFO_LEN;
					writel(num,&i2c->regs->MBNR);			
					tmp = get_buf_data(&i2c->msg->buf[i2c->msg_cnt],FIFO_LEN);
					writel(tmp,&i2c->regs->MBDR0);
				
					i2c->msg_cnt += FIFO_LEN;
					writel(CLEAR_INT_BIT,&i2c->regs->MBSR);
				}
				else
				{
					num = (i2c->msg->len - i2c->msg_cnt) % FIFO_LEN;			
					writel(num,&i2c->regs->MBNR);			
					tmp = get_buf_data(&i2c->msg->buf[i2c->msg_cnt],num);
					writel(tmp,&i2c->regs->MBDR0);
					i2c->msg_cnt += num;
					writel(CLEAR_INT_BIT,&i2c->regs->MBSR);
				}
				delay =  jiffies + DEV_CMD_TIMEOUT;	
				while(!(readl(&i2c->regs->MBSR)& WRITE_DONE_BIT) && time_before(jiffies,delay));

			#else
				if((i2c->msg->len - i2c->msg_cnt) / MAX_NUM_SEND_ONCE > 0)
				{
					num = MAX_NUM_SEND_ONCE;	
					writel(num,&i2c->regs->MBNR);		
					for(i = 0;i < FIFO_LEN;i++)
					{
						tmp = get_buf_data(&i2c->msg->buf[i2c->msg_cnt],FIFO_LEN);
						writel(tmp,i2c->data_regs[i]);
						i2c->msg_cnt += FIFO_LEN;
					}
				}
				else
				{
					num = (i2c->msg->len - i2c->msg_cnt) % MAX_NUM_SEND_ONCE;	
					writel(num,&i2c->regs->MBNR);		
					for(i = 0;i < num / FIFO_LEN;i++)
					{
						tmp = get_buf_data(&i2c->msg->buf[i2c->msg_cnt],FIFO_LEN);
						writel(tmp,i2c->data_regs[i]);
						i2c->msg_cnt += FIFO_LEN;
					}
					if(i2c->msg->len - i2c->msg_cnt > 0)	
					{	
						tmp = get_buf_data(&i2c->msg->buf[i2c->msg_cnt],(i2c->msg->len - i2c->msg_cnt)%FIFO_LEN);
						writel(tmp,i2c->data_regs[i]);
						i2c->msg_cnt += (i2c->msg->len - i2c->msg_cnt);
					}
				}			 
				writel(CLEAR_INT_BIT,&i2c->regs->MBSR);
			#endif
			}
			else
			{
				if(i2c->msg_num == 1)
				{
					reg_value = STOP_CONFIG;
					writel(reg_value,&i2c->regs->MBCR);
					writel(CLEAR_INT_BIT,&i2c->regs->MBSR);
		
					complete(i2c->done);
				}
				else
				{
					i2c->msg_ptr =0;
					i2c->msg_idx++;
					i2c->msg_cnt = 0;
					i2c->msg++;
					
					i2c->state = STATE_READ;
				}
				break;
			}
			break;
		
		case STATE_READ:
prepare_read:
			if((i2c->msg_ptr) == 0)							//slave address
			{
				i2c->msg_ptr++;
				writel(MBNR_SINGLE,&i2c->regs->MBNR);
				writel( (i2c->msg->addr<<1) |1,&i2c->regs->MBDR0);

				reg_value = RESTART_CONFIG;
				writel(reg_value,&i2c->regs->MBCR);
				writel(CLEAR_INT_BIT,&i2c->regs->MBSR);

				break;
			}

			if((i2c->msg->len - i2c->msg_cnt) == 0)							
			{
				reg_value = STOP_CONFIG;
				writel(reg_value,&i2c->regs->MBCR);
				writel(CLEAR_INT_BIT,&i2c->regs->MBSR);
				complete(i2c->done);
				break;
			}
	#ifdef SINGLE_REG_MODE	
			if((i2c->msg->len - i2c->msg_cnt) > FIFO_LEN)
				writel(FIFO_LEN,&i2c->regs->MBNR);
			else
				writel(0x104,&i2c->regs->MBNR);
			
			writel(CLEAR_INT_BIT,&i2c->regs->MBSR);
			delay =  jiffies + DEV_CMD_TIMEOUT;	
			while(!(status & READ_FIFO_FULL) && time_before(jiffies,delay))
			{
				status = readl(&i2c->regs->MBSR);
			} 
		
			val  = readl(&i2c->regs->MBDR0); 
			if((i2c->msg->len - i2c->msg_cnt) > FIFO_LEN || (i2c->msg->len - i2c->msg_cnt) == FIFO_LEN)
			{	
				set_buf_data(&i2c->msg->buf[i2c->msg_cnt],val,FIFO_LEN);
				i2c->msg_cnt += FIFO_LEN;
			}
			else
			{
				num = (i2c->msg->len - i2c->msg_cnt) % FIFO_LEN; 
				set_buf_data(&i2c->msg->buf[i2c->msg_cnt],val,num);
				i2c->msg_cnt += num;
			}
	#else
			if(i2c->data_flag == 0)
			{
				i2c->data_flag = 1;
				if((i2c->msg->len - i2c->msg_cnt) > MAX_NUM_SEND_ONCE)
				{
					i2c->data_num = FIFO_LEN;
					writel(MAX_NUM_SEND_ONCE,&i2c->regs->MBNR);
				}
				else
				{
					i2c->data_num = (i2c->msg->len-i2c->msg_cnt)%FIFO_LEN == 0?(i2c->msg->len-i2c->msg_cnt)/FIFO_LEN:(i2c->msg->len-i2c->msg_cnt)/FIFO_LEN + 1;
					writel(RECV_LAST_BYTES(i2c->msg->len%MAX_NUM_SEND_ONCE),&i2c->regs->MBNR);
				}
				writel(CLEAR_INT_BIT,&i2c->regs->MBSR);
			}
			else
			{
				i2c->data_flag = 0;
				for(i = 0;i < i2c->data_num ;i++)
				{
					val = readl(i2c->data_regs[i]);
					num = FIFO_LEN;
					if((i2c->msg->len - i2c->msg_cnt)/FIFO_LEN == 0 && (i2c->msg->len -i2c->msg_cnt)% FIFO_LEN != 0 )
					{
						num = (i2c->msg->len - i2c->msg_cnt)% FIFO_LEN;
					}
					set_buf_data(&i2c->msg->buf[i2c->msg_cnt],val,num);
					i2c->msg_cnt += num;
		
				}	
				goto prepare_read;
			}	
	#endif
			break;	
		default:
			break;
	}
}
#else
void i2c_tasklet(unsigned long data)
{
	struct silan_i2c *i2c = (struct silan_i2c*)data; 
	static unsigned long delay;
	unsigned int num = 0;
	unsigned int reg_value =0;
	int val,tmp,i;
	do{	
		switch (i2c->state) 
		{
			case STATE_IDLE:
				break;

			case STATE_STOP:
				reg_value = readl(&i2c->regs->MBCR);
				writel(reg_value & ~MBCR_MIEN, &i2c->regs->MBCR);
				break;

			case STATE_START:
				if (i2c->msg->flags & I2C_M_RD)
					i2c->state = STATE_READ;
				else
					i2c->state = STATE_WRITE;
		
				if (i2c->state == STATE_READ)
					goto prepare_read;

			case STATE_WRITE:
				if(i2c->msg->len - i2c->msg_cnt >0)
				{
					if((i2c->msg->len - i2c->msg_cnt) / MAX_NUM_SEND_ONCE > 0)
					{
						num = MAX_NUM_SEND_ONCE;	
						writel(num,&i2c->regs->MBNR);		
						for(i = 0;i < FIFO_LEN;i++)
						{
							tmp = get_buf_data(&i2c->msg->buf[i2c->msg_cnt],FIFO_LEN);
							writel(tmp,i2c->data_regs[i]);
							i2c->msg_cnt += FIFO_LEN;
						}
					}
					else
					{
						num = (i2c->msg->len - i2c->msg_cnt) % MAX_NUM_SEND_ONCE;	
						writel(num,&i2c->regs->MBNR);		
						for(i = 0;i < num / FIFO_LEN;i++)
						{
							tmp = get_buf_data(&i2c->msg->buf[i2c->msg_cnt],FIFO_LEN);
							writel(tmp,i2c->data_regs[i]);
							i2c->msg_cnt += FIFO_LEN;
						}	
						if(i2c->msg->len - i2c->msg_cnt > 0)	
						{
							tmp = get_buf_data(&i2c->msg->buf[i2c->msg_cnt],(i2c->msg->len-i2c->msg_cnt)%FIFO_LEN);
							writel(tmp,i2c->data_regs[i]);
							i2c->msg_cnt += (i2c->msg->len - i2c->msg_cnt);
						}
					}			 
					writel(CLEAR_INT_BIT,&i2c->regs->MBSR);
				
					delay =  jiffies + DEV_CMD_TIMEOUT;	
					while(!silan_i2c_get_status(i2c,num) && time_before(jiffies,delay));
				}
				else
				{
					if(i2c->msg_num == 1)
					{
						reg_value = STOP_CONFIG;
						writel(reg_value,&i2c->regs->MBCR);
						writel(CLEAR_INT_BIT,&i2c->regs->MBSR);
						i2c->msg_idx--;
						enable_irq(i2c->irq);
		
					}
					else
					{
						i2c->msg_ptr =0;
						i2c->msg_idx--;
						i2c->msg_cnt = 0;
						i2c->msg++;
					
						i2c->state = STATE_READ;
					}
			}
			break;
		
		case STATE_READ:
prepare_read:
			if((i2c->msg_ptr) == 0)							
			{
				i2c->msg_ptr++;
				writel(MBNR_SINGLE,&i2c->regs->MBNR);
				writel((i2c->msg->addr<<1)|1,&i2c->regs->MBDR0);

				reg_value = RESTART_CONFIG;
				writel(reg_value,&i2c->regs->MBCR);
				writel(CLEAR_INT_BIT,&i2c->regs->MBSR);

				delay =  jiffies + DEV_CMD_TIMEOUT;	
				while(!(readl(&i2c->regs->MBSR)& READ_FIFO_FULL) && time_before(jiffies,delay));
				break;
			}

			if((i2c->msg->len - i2c->msg_cnt) == 0)							
			{
				reg_value = STOP_CONFIG;
				writel(reg_value,&i2c->regs->MBCR);

				writel(CLEAR_INT_BIT,&i2c->regs->MBSR);
				i2c->msg_idx --;	
				enable_irq(i2c->irq);
				break;
			}
		
			if((i2c->msg->len - i2c->msg_cnt) > MAX_NUM_SEND_ONCE)
			{
				i2c->data_num = FIFO_LEN;
				writel(MAX_NUM_SEND_ONCE,&i2c->regs->MBNR);
			}
			else
			{
				i2c->data_num = (i2c->msg->len-i2c->msg_cnt)%FIFO_LEN == 0?(i2c->msg->len-i2c->msg_cnt)/FIFO_LEN:(i2c->msg->len-i2c->msg_cnt)/FIFO_LEN + 1;
				writel(RECV_LAST_BYTES(i2c->msg->len%MAX_NUM_SEND_ONCE),&i2c->regs->MBNR);
			}
			writel(CLEAR_INT_BIT,&i2c->regs->MBSR);
			delay =  jiffies + DEV_CMD_TIMEOUT;	
			while(!(readl(&i2c->regs->MBSR)& READ_FIFO_FULL) && time_before(jiffies,delay));

			for(i = 0;i < i2c->data_num ;i++)
			{
				val = readl(i2c->data_regs[i]);
				if((i2c->msg->len - i2c->msg_cnt) /FIFO_LEN == 0 && (i2c->msg->len - i2c->msg_cnt) % FIFO_LEN != 0 )
					num = (i2c->msg->len - i2c->msg_cnt)% FIFO_LEN;
				else
					num = FIFO_LEN;
	
				set_buf_data(&i2c->msg->buf[i2c->msg_cnt],val,num);
				i2c->msg_cnt += num;
		
			}	
			goto prepare_read;
		
			break;	
		default:
			break;
		}
	}while((i2c->msg->len- i2c->msg_cnt) > 0 || i2c->msg_idx >0);


}
#endif
static  irqreturn_t silan_i2c_irq(int irqno, void *dev_id)
{
	unsigned int status;
	struct silan_i2c *i2c = (struct silan_i2c *)dev_id;
	status =readl(&i2c->regs->MBSR);
	if (status & MBSR_ARBITR) 
	{
		printk("Deal with arbitration loss\n");
		//silan_i2c_stop(i2c);
		writel(CLEAR_INT_BIT,&i2c->regs->MBSR);
#ifndef I2C_TASKLET
		complete(i2c->done);
#endif
		//silan_module_rst(SILAN_SR_I2C);
		goto out;
	}
	
	if( readl(&i2c->regs->MBSR) & MBSR_NOACK ) 
	{
		printk("I2C Slave device do not reply ACK single \n");
		silan_i2c_stop(i2c);
		writel(CLEAR_INT_BIT,&i2c->regs->MBSR);
		//complete(i2c->done);
		goto out;
	}

	do_gettimeofday(&i2c->time_clk);
#ifndef I2C_TASKLET
	i2c_silan_irq_nextbyte(i2c, status);
#else
	tasklet_schedule(&i2c->tasklet);
	disable_irq_nosync(i2c->irq);
#endif
out:
	return IRQ_HANDLED;
}

static int silan_i2c_set_master(struct silan_i2c *i2c)
{
	unsigned int iicstat;
	int timeout = 10;

	while (timeout-- > 0) 
	{
		iicstat = readl(&i2c->regs->MBSR);
		if (!(iicstat & MBSR_ARBITR))
			return 0;

		msleep(1);
	}

	return -ETIMEDOUT;
}

static int silan_i2c_doxfer(struct silan_i2c *i2c,struct i2c_msg *msgs, int num)
{
	int ret;

//	if(i2c->suspended)
//		return -EIO;

	ret = silan_i2c_set_master(i2c);
	if (ret != 0) 
	{
		printk("cannot get bus\n");
		ret = -EAGAIN;
		return ret;
	}
	spin_lock_irq(&i2c->lock);
	
	i2c->msg		= msgs;
	i2c->msg_num	= num;
	i2c->msg_ptr	= 0;
	i2c->msg_cnt	= 0;
	i2c->data_num 	= 0;
	i2c->data_flag 	= 0;
	i2c->msg_idx	= num;
	i2c->state	    = STATE_START;
	
	spin_unlock_irq(&i2c->lock);
	silan_i2c_message_start(i2c, msgs);

	udelay(i2c->tx_setup);	
	return 0;
}

static int silan_i2c_xfer(struct i2c_adapter *adap,
			struct i2c_msg *msgs, int num)
{
	struct silan_i2c *i2c = (struct silan_i2c *)adap->algo_data;
	int ret;
	ret = silan_i2c_doxfer(i2c, msgs, num);
	return ret;
}

static u32 silan_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_PROTOCOL_MANGLING;
}

static struct i2c_algorithm silan_i2c_algorithm = 
{
	.master_xfer		= silan_i2c_xfer,
	.functionality		= silan_i2c_func,
};

unsigned int silan_i2c_get_clk(struct silan_i2c *i2c)
{
	unsigned int prescale;

	prescale = i2c->clk_rate / I2C_BUS_CLK;
	prescale = prescale / 2;
	
	return prescale;
}

static int silan_i2c_init(struct silan_i2c *i2c)
{
	int clk = 0;
	clk |= silan_i2c_get_clk(i2c);
	clk |= SCL_DUTY_RATIO(0x0);
	clk |= HD_DAT_CNT;
	writel(clk, &i2c->regs->TI2C);

	return 0;
}

static int silan_i2c_probe(struct platform_device *pdev)
{
	struct silan_i2c *i2c[3];
	struct resource *res;
	int ret,i;
	
	for(i = 0 ; i < 3;i++)
	{
		i2c[i] = kzalloc(sizeof(struct silan_i2c), GFP_KERNEL);
		if (!i2c[i]) 
		{
			printk("No memory for state\n");
			return -ENOMEM;
		}

		strlcpy(i2c[i]->adap.name, I2C_DRV_NAME, sizeof(i2c[i]->adap.name));
		i2c[i]->adap.owner   = THIS_MODULE;
		i2c[i]->adap.algo    = &silan_i2c_algorithm;
		i2c[i]->adap.retries = 2;
		i2c[i]->adap.class   = I2C_CLASS_HWMON | I2C_CLASS_SPD;
		i2c[i]->tx_setup     = 50;

		i2c[i]->clk = clk_get(&pdev->dev,"i2c");
		if(!i2c[i]->clk)
		{
			printk("Cannot get clk\n");
			return -1;
		}

		if(clk_enable(i2c[i]->clk))
		{
			printk("Enable i2c clock error\n");
			return -1; 
		}	

		i2c[i]->clk_rate = clk_get_rate(i2c[i]->clk);	
		if(!i2c[i]->clk_rate)
		{
			printk("Get i2c clock rate error\n");
			return -1;
		}	
#if 0
	#ifndef CONFIG_MIPS_SILAN_SUVII
		silan_padmux_ctrl(SILAN_PADMUX_I2C_##i,PAD_ON);	
	#else
		silan_padmux_ctrl(SILAN_PADMUX_I2C1,PAD_ON);
	#endif	
#endif	
		spin_lock_init(&i2c[i]->lock);

		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (res == NULL) 
		{
			printk("Cannot find IO resource\n");
			ret = -ENOENT;
			goto err_out;
		}
		i2c[i]->ioarea = request_mem_region(res->start, (res->end-res->start)+1,
				pdev->name);

		if (i2c[i]->ioarea == NULL) 
		{
			printk("Cannot Request IO\n");
			ret = -ENXIO;
			goto err_out;
		}

		i2c[i]->regs = (I2CREG*)ioremap(res->start, (res->end-res->start)+1);
		if (i2c[i]->regs == NULL) 
		{
			printk("Cannot Map IO\n");
			ret = -ENXIO;
			goto err_ioarea;
		}

		i2c[i]->adap.algo_data = i2c[i];

		ret = silan_i2c_init(i2c[i]);
		if(ret)
		{
			printk("Init Error\n");
			goto err_iomap;
		}

		i2c[i]->irq = ret = platform_get_irq(pdev, i);
		if (ret <= 0) 
		{
			printk("Cannot Find IRQ\n");
			goto err_iomap;
		}
		ret = request_irq(i2c[i]->irq,silan_i2c_irq, IRQF_DISABLED, I2C_DRV_NAME, i2c[i]);
		if (ret != 0) 
		{
			printk("Cannot Claim IRQ\n");
			goto err_irq;
		}
		
		i2c[i]->adap.nr = i;
		ret = i2c_add_numbered_adapter(&i2c[i]->adap);
		if (ret < 0) {
			printk("failed to add bus to i2c core\n");
		}

		platform_set_drvdata(pdev, i2c[i]);
	#ifdef I2C_TASKLET
		tasklet_init(&i2c[i]->tasklet,i2c_tasklet,(unsigned long)i2c[i]);
	#endif
		i2c[i]->data_regs[0] =(unsigned int*)&i2c[i]->regs->MBDR0;
		i2c[i]->data_regs[1] =(unsigned int*)&i2c[i]->regs->MBDR1;
		i2c[i]->data_regs[2] =(unsigned int*)&i2c[i]->regs->MBDR2;
		i2c[i]->data_regs[3] =(unsigned int*)&i2c[i]->regs->MBDR3;
	}

	silan_padmux_ctrl(SILAN_PADMUX_I2C1,PAD_ON);	
	silan_padmux_ctrl(SILAN_PADMUX_I2C2,PAD_ON);	
	silan_padmux_ctrl(SILAN_PADMUX_I2C3,PAD_ON);	

	return 0;

	err_irq:
		free_irq(i2c[i]->irq, i2c[i]);

	err_iomap:
		iounmap(i2c[i]->regs);

	err_ioarea:
		release_resource(i2c[i]->ioarea);
		kfree(i2c[i]->ioarea);
	err_out:
		return ret;
}

static int silan_i2c_remove(struct platform_device *pdev)
{
	struct silan_i2c *i2c = platform_get_drvdata(pdev);
	
	tasklet_kill(&i2c->tasklet);

	i2c_del_adapter(&i2c->adap);

	iounmap(i2c->regs);

	release_resource(i2c->ioarea);
	kfree(i2c->ioarea);
	kfree(i2c);

	return 0;
}

#ifdef CONFIG_PM 
static int silan_i2c_suspend_noirq(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct silan_i2c *i2c = platform_get_drvdata(pdev);

	i2c->suspended = 1;
	clk_disable(i2c->clk);

	return 0;
}

static int silan_i2c_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct silan_i2c *i2c = platform_get_drvdata(pdev);
	
	i2c->suspended = 0;
	
	clk_enable(i2c->clk);
	silan_i2c_init(i2c);
	
	return 0;
}

static const struct dev_pm_ops silan_i2c_dev_pm_ops = {
	.suspend_noirq = silan_i2c_suspend_noirq,
	.resume = silan_i2c_resume,
};

#define SILAN_DEV_PM_OPS (&silan_i2c_dev_pm_ops)
#else
#define SILAN_DEV_PM_OPS NULL
#endif

static struct platform_driver silan_i2c_driver = 
{
	.probe		=  silan_i2c_probe,
	.remove		=  silan_i2c_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= I2C_DRV_NAME,
		.pm     = SILAN_DEV_PM_OPS,
	},
};

static int __init silan_i2c_adap_init(void)
{
	int ret;

	ret = platform_driver_register(&silan_i2c_driver);
	if (ret == 0) 
	{
		printk("silan i2c register succeful \n");
		return ret;
	}

	platform_driver_unregister(&silan_i2c_driver);
	return ret;
}

static void __exit silan_i2c_adap_exit(void)
{
	platform_driver_unregister(&silan_i2c_driver);
}

module_init(silan_i2c_adap_init);
module_exit(silan_i2c_adap_exit);

MODULE_DESCRIPTION("SILAN I2C Bus driver");
MODULE_AUTHOR("yanghuajie@silan.com.cn");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:silan-i2c");
