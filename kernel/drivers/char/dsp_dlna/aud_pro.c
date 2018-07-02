#include "aud_pro.h"

int dsp_aud_valid_frame(struct dsp_ringbuffer *rbuf,DSP_CODEC_TYPE type)
{
	return 1;
}

int dsp_get_packet_len(struct dsp_ringbuffer *rbuf,int flag)
{
	int packet_size = 0;
	if(flag == 1)
		packet_size  = dsp_ringbuffer_avail(rbuf);
	return packet_size;
}
