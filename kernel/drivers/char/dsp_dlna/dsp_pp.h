#ifndef _SL_PP_H_
#define _SL_PP_H_

typedef enum _BOOL{
	SL_FALSE = 0,
	SL_TRUE = 1
}BOOL;

typedef struct SLAUD_VolumeDB {
    s8 left;
    s8 right;
    s8	center;
    s8 lfe;
    s8	sleft;
    s8	sright;
    s8	sbleft;
    s8	sbright;
} SLAUD_VolumeDB_t;

#define EQ_MAX_BANDS 10
typedef struct SLAUD_EqMode
{ 
	u8    band; 
	u8    db_bands[EQ_MAX_BANDS]; 
}SLAUD_EqMode_t;

typedef enum SLAUD_Samplerate {
    SLAUD_RATE_8000,
    SLAUD_RATE_11025,
    SLAUD_RATE_16000,
    SLAUD_RATE_22050,
    SLAUD_RATE_32000,
    SLAUD_RATE_44100,
    SLAUD_RATE_48000,
    SLAUD_RATE_96000,
    SLAUD_RATE_192000
} SLAUD_Samplerate_t;

typedef enum SLAUD_Channel {
    SLAUD_MONO,
    SLAUD_STEREO,
    SLAUD_SURROUND40,
    SLAUD_SURROUND51,
    SLAUD_SURROUND71
} SLAUD_Channel_t;


typedef enum SLAUD_Format {
    PCM_FORMAT_S16_LE,
    PCM_FORMAT_S24_LE
} SLAUD_Format_t;

typedef enum SLAUD_ReverbMode {
    ReverbMode_A,
    ReverbMode_B
} SLAUD_ReverbMode_t;

struct pp_eq {
    BOOL                flag;
    SLAUD_EqMode_t        mode;
};

struct pp_rsp {
    BOOL                flag;
    SLAUD_Samplerate_t    rate;
};

struct pp_chmix {
    BOOL                flag;
    SLAUD_Channel_t        channel;
};

struct pp_surround {
    BOOL                flag;
};

struct pp_loudness {
    BOOL                flag;
    short                limit;
};

struct pp_samplingsize {
    BOOL                flag;
    SLAUD_Format_t        format;
};

struct pp_pitch {
    BOOL                flag;
    u8                percent;
};

struct pp_reverb {
    BOOL                flag;
    SLAUD_ReverbMode_t    mode;
};

struct pp_echo {
    BOOL                flag;
};

struct pp_volume {
	BOOL	flag;
    SLAUD_VolumeDB_t db;
};

typedef struct SLAUD_PostProcess {
    BOOL    mute;
	struct pp_volume volume;
	struct pp_eq eq;
    struct pp_rsp rsp;
    struct pp_chmix chmix;
    struct pp_surround surround;
    struct pp_loudness loudness;
    struct pp_samplingsize samplingsize;
    struct pp_pitch pitch;
    struct pp_reverb reverb;
    struct pp_echo echo;
}SLAUD_PostProcess_t;

#endif
