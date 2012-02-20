#include "v4l2device.hpp"

extern "C" {
#include <libswscale/swscale.h>
#include <x264.h>
}

#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>

#include "config.h"

using namespace std;


int main( int argc, char** argv )
{
    unsigned int inputWidth = 640;
    unsigned int inputHeight = 480;

    unsigned int outputWidth = WIDTH;
    unsigned int outputHeight = HEIGHT;

    // Initialize encoder
    x264_param_t param;
    x264_param_default_preset( &param, "ultrafast", "zerolatency" );

    param.i_width   = outputWidth;
    param.i_height  = outputHeight;
    param.i_fps_num = 30;
    param.i_fps_den = 1;

    // Settings as explained by http://x264dev.multimedia.cx/archives/249

    // Slicing is the splitting of frame data into a series of NALs,
    // each having a maximum size so that they can be transported over
    // interfaces that has a limited packet size/MTU
    // Practically disables slicing.
    x264_param_parse( &param, "slice-max-size", "8192" );

    // Set VBV mode and max bitrate (kbps).
    // VBV is variable bitrate, which means the rate will vary depending
    // on how complex the scene is at the moment - detail, motion, etc.
    x264_param_parse( &param, "vbv-maxrate", "100" );

    // Enable single-frame VBV.
    // This will cap all frames so that they only contain a maximum
    // amount of information, which in turn means that each frame can
    // always be sent in one packet and packets will be of a much
    // more unform size.
    x264_param_parse( &param, "vbv-bufsize", "10" );

    // Constant Rate Factor.
    // Tells VBV to target a specific quality.
    x264_param_parse( &param, "crf", "20" );

    // Enable intra-refresh.
    // Intra-refresh allows single-frame VBV to work well by disabling
    // I-frames and replacing them with a periodic refresh that scans
    //  across the image, refreshing only a smaller portion each frame.
    // I-frames can be seen as a full-frame refresh that needs no other
    //  data to decode the picture, and takes a lot more space
    // than P/B (differential) frames. */
    x264_param_parse( &param, "intra-refresh", NULL );

    // Use Annex-B packaging.
    // This appends a marker at the start of each NAL unit.
    param.b_annexb = 1;

    // Needed for intra-refresh.
    param.i_frame_reference = 1;

    // Apply HIGH profile.
    // Allows for better compression, but needs to be supported by the
    // decoder.  We use FFMPEG, which does support this profile.
    x264_param_apply_profile( &param, "high" );

    // Open encoder
    x264_t* encoder = x264_encoder_open( &param );

      // Allocate I420 picture
    x264_picture_t pic_in;
    if( x264_picture_alloc( &pic_in, X264_CSP_I420, outputWidth, outputHeight ) != 0 )
    {
        cout << "pic alloc fail" << endl;
        exit( EXIT_FAILURE );
    }

    // Allocate conversion context
    SwsContext* swsCtx = sws_getContext
        (
        inputWidth,
        inputHeight,
        PIX_FMT_YUYV422,
        outputWidth,
        outputHeight,
        PIX_FMT_YUV420P,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
        );
    if( swsCtx == NULL )
    {
        cout << "swsctx alloc fail" << endl;
        exit( EXIT_FAILURE );
    }

    //ofstream ofile( "dump.h264", ios::binary );
    //FILE* ofile = fopen( "dump.h264", "w" );
    //FILE* ofile = fopen( "dump.h264", "w" );


    //cout << "opening video0" << endl;
    capture::readwrite dev;
    dev.open("/dev/video0", true);
    dev.set_image_format(V4L2_PIX_FMT_YUYV);
    dev.set_image_extents( inputWidth, inputHeight );
    dev.set_buffer_queue_length(8);
    dev.configure();
    //usleep(100000);

    unsigned int frames = 0;
    while( true )
    {
        //cout << "frame: " << frames << endl;
        frames++;

        /*
        {
        stringstream ss;
        ss << "img" << fc << ".ppm";
        ofstream ppm( ss.str().c_str(), ios::binary );
        ppm << "P6" << endl;
        ppm << inputWidth << " " << inputHeight << endl;
        ppm << "256" << endl;
        ppm.write( (const char*)&iframe[0], iframe.size() );
        }
        */

        //cout << "scaling frame down" << endl;

        devicebase::buffer_type const* b = dev.get_next_frame();

	    // Convert to I420, as explained by
	    // http://stackoverflow.com/q/2940671/44729
        //uint8_t* ptr = &iframe[0];
        uint8_t* ptr = reinterpret_cast< unsigned char* >( const_cast< char* >( b->get_buffer() ) );

        // 2x becuase...dunno?  yuyv-related
        int stride = inputWidth * 2;
        sws_scale
            (
            swsCtx,
            &ptr,
            (int*)&stride, // stride
            0,
            inputHeight,
            pic_in.img.plane,
            pic_in.img.i_stride
            );

        dev.recycle_frame(b);

        // when to do intra?
        // maybe this is user-triggered
        // x264_encoder_intra_refresh( encoder );

        //cout << "encoding frame" << endl;

        // Encode frame
        x264_nal_t* nals;
        int num_nals;
        x264_picture_t pic_out;
        x264_encoder_encode
            (
            encoder,
            &nals,
            &num_nals,
            &pic_in,
            &pic_out
            );

        //cout << "collecting nals into buffer" << endl;

        // dump nals into a buffer
        vector< unsigned char > buf;
        for( int i = 0; i < num_nals; ++i )
        {
            uint8_t* beg = nals[i].p_payload;
            uint8_t* end = nals[i].p_payload + nals[i].i_payload;
            buf.insert( buf.end(), beg, end );
        }

        //cout << "dumping buffer to file" << endl;

        fwrite( (const char*)&buf[0], 1, buf.size(), stdout );
        //ofile.write( (const char*)&buf[0], buf.size() );

        //cout << "end of frame" << endl;
        //cout << endl;
    }

    dev.release();
    dev.close();

    x264_encoder_close( encoder );

    return 0;
}
