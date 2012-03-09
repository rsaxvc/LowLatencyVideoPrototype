#ifndef DATA_SOURCE_OCV_AVCODEC_H
#define DATA_SOURCE_OCV_AVCODEC_H

#ifndef UINT64_C
    #define UINT64_C(c) c ## ULL
#endif

extern "C"
{
#include <libavcodec/avcodec.h>
}

#include <cstring>
#include <stddef.h>
#include <stdint.h>
#include "data_source.h"

//passes each write into avcodec, then displays output in an opencv window
class data_source_ocv_avcodec: public data_source
	{
	public:
	data_source_ocv_avcodec(const char * name);
	~data_source_ocv_avcodec();
	void write( const uint8_t * data, size_t bytes );

	private:
		const char * m_name;
		AVCodecContext  *pCodecCtx;
		AVCodec         *pCodec;
		AVFrame         *pFrame;
		AVFrame         *pFrameRGB;
		void            *buffer;

	};

#endif
