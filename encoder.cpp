#include "v4l2device.hpp"

extern "C" {
#include <libswscale/swscale.h>
#include <x264.h>
}

#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <map>

using namespace std;


__u32 string_to_fourcc( const string& fourcc )
{
    return v4l2_fourcc
        (
        fourcc[ 0 ],
        fourcc[ 1 ],
        fourcc[ 2 ],
        fourcc[ 3 ]
        );
}

// TODO: little-endian (x86) specific?
string fourcc_to_string( const __u32 fourcc )
{
    string ret = "0000";
    ret[0] = static_cast<char>( (fourcc & 0x000000FF) >>  0 );
    ret[1] = static_cast<char>( (fourcc & 0x0000FF00) >>  8 );
    ret[2] = static_cast<char>( (fourcc & 0x00FF0000) >> 16 );
    ret[3] = static_cast<char>( (fourcc & 0xFF000000) >> 24 );
    return ret;
}

ostream& operator<<( ostream& os, const v4l2_fmtdesc& desc )
{
    os << fourcc_to_string( desc.pixelformat ) << " ";

    string flags = "";
    if( desc.flags & V4L2_FMT_FLAG_COMPRESSED )
        flags += "C";
    if( desc.flags & V4L2_FMT_FLAG_EMULATED )
        flags += "E";
    else
        flags += "N";
    os << "(" << flags << ") ";

    os << "[" << desc.description << "]";
    return os;    
}

ostream& operator<<( ostream& os, const v4l2_frmsizeenum& size )
{
    if( size.type == V4L2_FRMSIZE_TYPE_DISCRETE )
    {
        os << size.discrete.width << "x" << size.discrete.height;
    }
    else
    {
        os << size.stepwise.min_width << "-" << size.stepwise.max_width;
        os << "," << size.stepwise.step_width;
        
        os << "x";
        
        os << size.stepwise.min_height << "-" << size.stepwise.max_height;
        os << "," << size.stepwise.step_height;
    }
    return os;    
}

ostream& operator<<( ostream& os, const v4l2_fract& frac )
{
    os << frac.numerator << "/" << frac.denominator;
    return os;
}

ostream& operator<<( ostream& os, const v4l2_frmivalenum& interval )
{
    if( interval.type == V4L2_FRMIVAL_TYPE_DISCRETE )
    {
        os << interval.discrete;
    }
    else
    {
        os << interval.stepwise.min;
        os << "-";
        os << interval.stepwise.max;
        os << ",";
        os << interval.stepwise.step;
    }
    return os;    
}


template< typename T >
string TS( const T& val )
{
    ostringstream oss;
    oss << val;
    return oss.str();
}



void GetLayout( const v4l2_pix_format& fmt, vector< int >& offsets, vector< int >& strides )
{
    offsets.clear();
    strides.clear();
    if( fmt.pixelformat == V4L2_PIX_FMT_YUV420 )
    {
        // planar format
        offsets.push_back( 0 );
        offsets.push_back( offsets.back() + ( fmt.height * fmt.bytesperline ) );
        offsets.push_back( offsets.back() + ( (fmt.height/2) * (fmt.bytesperline/2) ) );
        
        strides.push_back( fmt.bytesperline );
        strides.push_back( fmt.bytesperline / 2 );
        strides.push_back( fmt.bytesperline / 2 );
    }
    else
    {
        // assume packed format
        offsets.push_back( 0 );
        strides.push_back( fmt.bytesperline );
    }
}


int main( int argc, char** argv )
{
    VideoCapture dev( "/dev/video0" );

    cerr << "IO Methods:" << endl;
    vector< VideoCapture::IO > ios = dev.SupportedIO();
    for( size_t i = 0; i < ios.size(); ++i )
    {
        string name;
        if( ios[i] == VideoCapture::READ )    name = "READ";
        if( ios[i] == VideoCapture::USERPTR ) name = "USERPTR";
        if( ios[i] == VideoCapture::MMAP )    name = "MMAP";
        if( ios[i] == dev.SelectedIO() )      name += " *";
        cerr << name << endl;
    }
    cerr << endl;

    // dump supported formats/sizes/fps
    cerr << "Supported formats:" << endl;
    vector< v4l2_fmtdesc > fmts = dev.GetFormats();
    for( size_t i = 0; i < fmts.size(); ++i ) 
    {
        const v4l2_fmtdesc& fmt = fmts[i];
        cerr << fmt << endl;
        
        vector< v4l2_frmsizeenum > sizes = dev.GetSizes( fmt );
        for( size_t j = 0; j < sizes.size(); ++j )
        {
            const v4l2_frmsizeenum& size = sizes[j];
            cerr << "\t" << size << ":";
    
            vector< v4l2_frmivalenum > intervals = dev.GetIntervals( fmt, size );
            for( size_t k = 0; k < intervals.size(); ++k )
            {
                const v4l2_frmivalenum& interval = intervals[k];
                cerr << " (" << interval << ")";
            }
            cerr << endl;
        }
    }
    cerr << endl;

    v4l2_pix_format fmt = dev.GetFormat();
    v4l2_fract fps = dev.GetInterval();
    cerr << "Frame info: " << endl;
    cerr << "  Fourcc: " << fourcc_to_string( fmt.pixelformat ) << endl;
    cerr << "    Size: " << fmt.width << "x" << fmt.height << endl;
    cerr << "Interval: " << fps << endl;
    cerr << endl;

    // v4l2 pixelformat -> libswscale colorspace
    map< __u32, PixelFormat > FormatMap;
    FormatMap[ V4L2_PIX_FMT_YUYV ]      =  PIX_FMT_YUYV422;
    FormatMap[ V4L2_PIX_FMT_YUV420 ]    =  PIX_FMT_YUV420P;
    FormatMap[ V4L2_PIX_FMT_RGB24 ]     =  PIX_FMT_RGB24;
    FormatMap[ V4L2_PIX_FMT_BGR24 ]     =  PIX_FMT_BGR24;    
    
    bool formatFound = false;
    map< __u32, PixelFormat >::iterator i;
    for( i = FormatMap.begin(); i != FormatMap.end(); ++i )
    {
        if( fmt.pixelformat == i->first )
        {
            formatFound = true;
            break;
        }
    }
    
    if( !formatFound )
    {
        __u32 fallback = V4L2_PIX_FMT_YUV420;

        cerr << fourcc_to_string( fmt.pixelformat ) << " not supported!" << endl;
        cerr << "Switching to " << fourcc_to_string( fallback ) << "...";
        
        fmt.pixelformat = fallback;
        if( dev.SetFormat(fmt) )
        {
            cerr << "success!" << endl;
        }
        else
        {
            cerr << "failed!" << endl;
            exit( EXIT_FAILURE );
        }
        cerr << endl;

        fmt = dev.GetFormat();
        fps = dev.GetInterval();
        cerr << "Frame info: " << endl;
        cerr << "  Fourcc: " << fourcc_to_string( fmt.pixelformat ) << endl;
        cerr << "    Size: " << fmt.width << "x" << fmt.height << endl;
        cerr << "Interval: " << fps << endl;
        cerr << endl;
    }

    // get offsets/strides for selected frame format
    vector< int > offsets;
    vector< int > strides;
    GetLayout( fmt, offsets, strides );
    vector< uint8_t* > planes( offsets.size() );
    
    // Allocate conversion context 
    unsigned int outputWidth = 160;
    unsigned int outputHeight = 120;
    SwsContext* swsCtx = sws_getContext
        ( 
        fmt.width, 
        fmt.height, 
        FormatMap[ fmt.pixelformat ], 
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
        cerr << "swsctx alloc fail" << endl;
        exit( EXIT_FAILURE );
    }


    // Initialize encoder
    x264_param_t param;

    // --slice-max-size A 
    // --vbv-maxrate B 
    // --vbv-bufsize C 
    // --crf D 
    // --intra-refresh 
    // --tune zerolatency

    // A is your packet size
    // B is your connection speed
    // C is (B / FPS)
    // D is a number from 18-30 or so (quality level, lower is better but higher bitrate).

    // Equally, you can do constant bitrate instead of capped constant quality, 
    // by replacing CRF with --bitrate B, where B is the maxrate above.

    int packetsize = 1500; // bytes
    int maxrate = 220; // kbps
    int f =  fps.denominator / fps.numerator;
    int C = maxrate / f;

    x264_param_default_preset( &param, "veryfast", "zerolatency" );

    param.i_width   = outputWidth;
    param.i_height  = outputHeight;
    param.i_fps_num = fps.denominator;
    param.i_fps_den = fps.numerator;

    x264_param_parse( &param, "slice-max-size", TS(packetsize).c_str() );
    x264_param_parse( &param, "vbv-maxrate", TS(maxrate).c_str() ); 
    x264_param_parse( &param, "vbv-bufsize", TS(C).c_str() ); 
    x264_param_parse( &param, "bitrate", TS(maxrate).c_str() ); 

    x264_param_parse( &param, "intra-refresh", NULL );
    param.i_frame_reference = 1;
    param.b_annexb = 1;

    x264_param_apply_profile( &param, "high" );

    // Open encoder
    x264_t* encoder = x264_encoder_open( &param );

      // Allocate I420 picture
    x264_picture_t pic_in;
    if( x264_picture_alloc( &pic_in, X264_CSP_I420, outputWidth, outputHeight ) != 0 )
    {
        cerr << "x264 pic alloc fail" << endl;
        exit( EXIT_FAILURE );
    }

    dev.StartCapture();
    while( true )
    {
        const VideoCapture::Buffer& b = dev.LockFrame();
        uint8_t* ptr = reinterpret_cast< unsigned char* >( const_cast< char* >( b.start ) );

        // apply plane offsets
        for( size_t i = 0; i < planes.size(); ++i )
            planes[i] = ptr + offsets[i];
        
        sws_scale
            ( 
            swsCtx, 
            &planes[0], 
            &strides[0], 
            0, 
            fmt.height, 
            pic_in.img.plane, 
            pic_in.img.i_stride
            );

        dev.UnlockFrame();

        // Encode frame
        x264_nal_t* nals;
        int num_nals;
        x264_picture_t pic_out;   
        x264_encoder_encode( encoder, &nals, &num_nals, &pic_in, &pic_out );

        // dump nals into a buffer    
        vector< unsigned char > buf;
        for( int i = 0; i < num_nals; ++i )
        {
            uint8_t* beg = nals[i].p_payload;
            uint8_t* end = nals[i].p_payload + nals[i].i_payload; 
            buf.insert( buf.end(), beg, end );
        }
        fwrite( (const char*)&buf[0], 1, buf.size(), stdout ); 
        fflush( stdout );
    }

    dev.StopCapture();
    x264_encoder_close( encoder );

    return 0;
}
