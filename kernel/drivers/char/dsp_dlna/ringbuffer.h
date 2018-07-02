#ifndef _DSP_RINGBUFFER_H_
#define _DSP_RINGBUFFER_H_

#include <linux/spinlock.h>
#include <linux/wait.h>

struct dsp_ringbuffer {
	u8               *data;
	ssize_t           size;
	ssize_t           pread;
	ssize_t           pwrite;
	int               error;

	wait_queue_head_t queue;
	spinlock_t        lock;
};

/*
** Notes:
** ------
** (1) For performance reasons read and write routines don't check buffer sizes
**     and/or number of bytes free/available. This has to be done before these
**     routines are called. For example:
**
**     *** write <buflen> bytes ***
**     free = dvb_ringbuffer_free(rbuf);
**     if (free >= buflen)
**         count = dvb_ringbuffer_write(rbuf, buffer, buflen);
**     else
**         ...
**
**     *** read min. 1000, max. <bufsize> bytes ***
**     avail = dvb_ringbuffer_avail(rbuf);
**     if (avail >= 1000)
**         count = dvb_ringbuffer_read(rbuf, buffer, min(avail, bufsize));
**     else
**         ...
**
** (2) If there is exactly one reader and one writer, there is no need
**     to lock read or write operations.
**     Two or more readers must be locked against each other.
**     Flushing the buffer counts as a read operation.
**     Resetting the buffer counts as a read and write operation.
**     Two or more writers must be locked against each other.
*/

/* initialize ring buffer, lock and queue */
extern void dsp_ringbuffer_init(struct dsp_ringbuffer *rbuf, void *data, size_t len);

/* test whether buffer is empty */
extern int dsp_ringbuffer_empty(struct dsp_ringbuffer *rbuf);

/* return the number of free bytes in the buffer */
extern ssize_t dsp_ringbuffer_free(struct dsp_ringbuffer *rbuf);

/* return the number of bytes waiting in the buffer */
extern ssize_t dsp_ringbuffer_avail(struct dsp_ringbuffer *rbuf);

/*
** Reset the read and write pointers to zero and flush the buffer
** This counts as a read and write operation
*/
extern void dsp_ringbuffer_reset(struct dsp_ringbuffer *rbuf);


/* read routines & macros */
/* ---------------------- */
/* flush buffer */
extern void dsp_ringbuffer_flush(struct dsp_ringbuffer *rbuf);

/* flush buffer protected by spinlock and wake-up waiting task(s) */
extern void dsp_ringbuffer_flush_spinlock_wakeup(struct dsp_ringbuffer *rbuf);

#define DSP_RINGBUFFER_SKIP(rbuf,num)	\
			(rbuf)->pread=((rbuf)->pread+(num))%(rbuf)->size

/*
** read <len> bytes from ring buffer into <buf>
** <usermem> specifies whether <buf> resides in user space
** returns number of bytes transferred or -EFAULT
*/
extern ssize_t dsp_ringbuffer_read_user(struct dsp_ringbuffer *rbuf,
				   u8 __user *buf, size_t len);
extern void dsp_ringbuffer_read(struct dsp_ringbuffer *rbuf,
				   u8 *buf, size_t len);


/* write routines & macros */
/* ----------------------- */
/* write single byte to ring buffer */
#define DSP_RINGBUFFER_WRITE_BYTE(rbuf,byte)	\
			{ (rbuf)->data[(rbuf)->pwrite]=(byte); \
			(rbuf)->pwrite=((rbuf)->pwrite+1)%(rbuf)->size; }
/*
** write <len> bytes to ring buffer
** <usermem> specifies whether <buf> resides in user space
** returns number of bytes transferred or -EFAULT
*/
extern ssize_t dsp_ringbuffer_write(struct dsp_ringbuffer *rbuf, const u8 *buf,
				    size_t len);

void dsp_set_ringbuffer_rptr(struct dsp_ringbuffer *rbuf,size_t len);
void dsp_increase_ringbuf_rptr(struct dsp_ringbuffer *rbuf,size_t len);
void dsp_increase_ringbuf_wptr(struct dsp_ringbuffer *rbuf,size_t len);

#endif /* _DSP_RINGBUFFER_H_ */
