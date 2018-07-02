/*
* dma.h --
* This is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the 
* Free Software Foundation; either version 2 of the License,or(at you
* option) any later version.
*
* ALSA PCM interface for the Silan XX CPU
*/

#ifndef _SL_AUDIO_DMA_H
#define _SL_AUDIO_DMA_H

enum sl_dma_peripheral_type {
	SL_DMATYPE_SPDIF_OUT,
	SL_DMATYPE_SPDIF_IN,
	SL_DMATYPE_IIS_OUT,
	SL_DMATYPE_IIS_IN
};

struct sl_dma_data {
	int dma_request;	//dma channel.
	enum sl_dma_peripheral_type peripheral_type;
	int priority;
};

struct sl_pcm_dma_params {
	int dma;			//dma channel.
	unsigned long dma_addr;		//dma addr.
	int burstsize;		//dma burst size.
	enum sl_dma_peripheral_type type;
};

enum sl_dma_prio {
	DMA_PRIO_HIGH = 0,
	DMA_PRIO_MEDIUM = 1,
	DMA_PRIO_LOW = 2
};

#endif
