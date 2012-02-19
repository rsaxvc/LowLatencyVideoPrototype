#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
//extern "C" {
#include <x264.h>
//}

#include "config.h"
#define NUM_FRAMES (FPS*10)

int main()
{
x264_param_t param;
x264_param_default_preset(&param, "ultrafast", "zerolatency");
x264_param_default_preset(&param, "fast", "zerolatency");

param.i_width = WIDTH;
param.i_height = HEIGHT;
param.i_fps_num = FPS;
param.i_fps_den = 1;

//For streaming:
param.b_repeat_headers = 1;
param.b_annexb = 1;
x264_param_apply_profile(&param, "baseline");


//encoder
x264_t* encoder = x264_encoder_open(&param);
x264_picture_t pic_in, pic_out;
x264_picture_alloc(&pic_in, X264_CSP_I420, WIDTH, HEIGHT );

int i;
int retn;
int fd = open("output.264",O_RDWR|O_CREAT);
	x264_nal_t* nals;
	int i_nals;
	int i_nals_iter;

x264_encoder_headers(encoder, &nals, &i_nals );
printf("inals=%i\n",i_nals);
for( i_nals_iter = 0; i_nals_iter < i_nals; i_nals_iter++ )
	{
	write( fd, nals[i_nals_iter].p_payload, nals[i_nals_iter].i_payload );
	}

//PERFRAME
for( i = 0; i < NUM_FRAMES; ++i )
	{
	int x,p;
	for(p=0;p<pic_in.img.i_plane;++p)
		for(x=0;x<HEIGHT*pic_in.img.i_stride[p]/2;++x)
			pic_in.img.plane[p][x]=rand()%256;

	//data is a pointer to you RGB structure
	//sws_scale(convertCtx, &data, &srcstride, 0, h, pic_in.img.plane, pic_in.img.stride);
	int frame_size = x264_encoder_encode(encoder, &nals, &i_nals, &pic_in, &pic_out);
	for( i_nals_iter = 0; i_nals_iter < i_nals; i_nals_iter++ )
		{
		write( fd, nals[i_nals_iter].p_payload, nals[i_nals_iter].i_payload );
		}
	}

close(fd);
}
