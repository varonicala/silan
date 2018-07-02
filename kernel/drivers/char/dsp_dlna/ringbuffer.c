#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <asm/uaccess.h>

#include "ringbuffer.h"

void dsp_ringbuffer_init(struct dsp_ringbuffer *rbuf, void *data, size_t len)
{
	rbuf->pread=rbuf->pwrite=0;
	rbuf->data=data;
	rbuf->size=len;
	rbuf->error=0;

	init_waitqueue_head(&rbuf->queue);

	spin_lock_init(&(rbuf->lock));
}

int dsp_ringbuffer_empty(struct dsp_ringbuffer *rbuf)
{
	return (rbuf->pread==rbuf->pwrite);
}

ssize_t dsp_ringbuffer_free(struct dsp_ringbuffer *rbuf)
{
	ssize_t free;
	free = rbuf->pread - rbuf->pwrite;
	if (free <= 0)
		free += rbuf->size;
	return free-1;
}

ssize_t dsp_ringbuffer_avail(struct dsp_ringbuffer *rbuf)
{
	ssize_t avail;

	avail = rbuf->pwrite - rbuf->pread;
	if (avail < 0)
		avail += rbuf->size;
	return avail;
}

void dsp_ringbuffer_flush(struct dsp_ringbuffer *rbuf)
{
	rbuf->pread = rbuf->pwrite;
	rbuf->error = 0;
}
EXPORT_SYMBOL(dsp_ringbuffer_flush);

void dsp_ringbuffer_reset(struct dsp_ringbuffer *rbuf)
{
	rbuf->pread = rbuf->pwrite = 0;
	rbuf->error = 0;
}

void dsp_ringbuffer_flush_spinlock_wakeup(struct dsp_ringbuffer *rbuf)
{
	unsigned long flags;

	spin_lock_irqsave(&rbuf->lock, flags);
	dsp_ringbuffer_flush(rbuf);
	spin_unlock_irqrestore(&rbuf->lock, flags);

	wake_up(&rbuf->queue);
}

ssize_t dsp_ringbuffer_read_user(struct dsp_ringbuffer *rbuf, u8 __user *buf, size_t len)
{
	size_t todo = len;
	size_t split;

	split = (rbuf->pread + len > rbuf->size) ? rbuf->size - rbuf->pread : 0;
	if (split > 0) {
		if (copy_to_user(buf, rbuf->data+rbuf->pread, split))
			return -EFAULT;
		buf += split;
		todo -= split;
		rbuf->pread = 0;
	}
	if (copy_to_user(buf, rbuf->data+rbuf->pread, todo))
		return -EFAULT;

	rbuf->pread = (rbuf->pread + todo) % rbuf->size;

	return len;
}

void dsp_ringbuffer_read(struct dsp_ringbuffer *rbuf, u8 *buf, size_t len)
{
	size_t todo = len;
	size_t split;

	split = (rbuf->pread + len > rbuf->size) ? rbuf->size - rbuf->pread : 0;
	if (split > 0) {
		memcpy(buf, rbuf->data+rbuf->pread, split);
		buf += split;
		todo -= split;
		rbuf->pread = 0;
	}
	memcpy(buf, rbuf->data+rbuf->pread, todo);

	rbuf->pread = (rbuf->pread + todo) % rbuf->size;
}


ssize_t dsp_ringbuffer_write(struct dsp_ringbuffer *rbuf, const u8 *buf, size_t len)
{
	size_t todo = len;
	size_t split;

	split = (rbuf->pwrite + len > rbuf->size) ? rbuf->size - rbuf->pwrite : 0;

	if (split > 0) {
		memcpy(rbuf->data+rbuf->pwrite, buf, split);
		buf += split;
		todo -= split;
		rbuf->pwrite = 0;
	}
	memcpy(rbuf->data+rbuf->pwrite, buf, todo);
	rbuf->pwrite = (rbuf->pwrite + todo) % rbuf->size;

	return len;
}

void dsp_set_ringbuffer_rptr(struct dsp_ringbuffer *rbuf,size_t len)
{
	size_t todo = len;
	int split = 0;

	split = ((int)(rbuf->pread - len) < 0) ? rbuf->pread:0;
	if(split > 0)
	{
		todo -= split;
		rbuf->pread = rbuf->size;
	}
	rbuf->pread = rbuf->pread - todo;
}

void dsp_increase_ringbuf_rptr(struct dsp_ringbuffer *rbuf,size_t len)
{
	size_t todo = len;
	int split = 0;
	
	split = (rbuf->pread +len >=  rbuf->size)?(rbuf->size -rbuf->pread):0;
	if(split > 0)
	{
		todo -= split;
		rbuf->pread = 0;
	}
	rbuf->pread = rbuf->pread + todo;
}

void dsp_increase_ringbuf_wptr(struct dsp_ringbuffer *rbuf,size_t len)
{
	size_t todo = len;
	int split = 0;
	
	split = (rbuf->pwrite +len >= rbuf->size)?(rbuf->size - rbuf->pwrite):0;
	if(split > 0)
	{
		todo -= split;
		rbuf->pwrite = 0;
	}
	
	rbuf->pwrite = rbuf->pwrite + todo;
}

EXPORT_SYMBOL(dsp_ringbuffer_init);
EXPORT_SYMBOL(dsp_ringbuffer_empty);
EXPORT_SYMBOL(dsp_ringbuffer_free);
EXPORT_SYMBOL(dsp_ringbuffer_avail);
EXPORT_SYMBOL(dsp_ringbuffer_flush_spinlock_wakeup);
EXPORT_SYMBOL(dsp_ringbuffer_read_user);
EXPORT_SYMBOL(dsp_ringbuffer_read);
EXPORT_SYMBOL(dsp_ringbuffer_write);
