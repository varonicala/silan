#ifndef _AUD_PRO_H_
#define _AUD_PRO_H_
#include "dsp_core.h"
#include "dsp_ctl.h"
#include "mailbox.h"
#include "queue.h"
#include "task.h"
#include "dsp_mbcmd.h"
#include "dsp_io.h"
#include "ringbuffer.h"

int dsp_aud_valid_frame(struct dsp_ringbuffer *rbuf,DSP_CODEC_TYPE type);
int dsp_get_packet_len(struct dsp_ringbuffer *rbuf,int flag);

#endif	
