//THIS IS NOT A TRADITIONAL LOW-LATENCY ENCODER
//Rather, it is a passthrough for H264 cameras.

#include "v4l2device.hpp"

extern "C" {
#include <libavutil/pixfmt.h>
}

#include <cmath>
#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <map>
#include <numeric>
#include <deque>
#include <algorithm>

#include "config.h"

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

double now()
{
    timespec temp;
    clock_gettime( CLOCK_MONOTONIC, &temp );

    return (double)temp.tv_sec + ( (double)temp.tv_nsec / 1e9 );
}


template< typename Cont >
double median( const Cont& arr )
{
    Cont v( arr );

    size_t n = v.size() / 2;
    nth_element( v.begin(), v.begin() + n, v.end() );
    if( n % 2 == 0 )
        return 0.5 * ( v[n] + v[n-1] );
    else
        return v[n];
}


template< typename Cont >
double stdev( const Cont& v )
{
    double sum = accumulate( v.begin(), v.end(), 0.0 );
    double mean = sum / v.size();

    vector<double> diff( v.size() );
    transform( v.begin(), v.end(), diff.begin(), bind2nd(minus<double>(), mean) );

    double sq_sum = inner_product( diff.begin(), diff.end(), diff.begin(), 0.0 );
    double stdev = sqrt( sq_sum / v.size() );

    return stdev;
}




int main( int argc, char** argv )
{
    string device = "/dev/video0";
    if( argc == 2 )
        device = argv[1];

    VideoCapture dev( device );

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
    map< __u32, AVPixelFormat > FormatMap;
    FormatMap[ V4L2_PIX_FMT_H264 ]      =  AV_PIX_FMT_NONE;
    FormatMap[ V4L2_PIX_FMT_H264_NO_SC ]=  AV_PIX_FMT_NONE;
    FormatMap[ V4L2_PIX_FMT_H264_MVC ]  =  AV_PIX_FMT_NONE;
    FormatMap[ V4L2_PIX_FMT_MJPEG ]     =  AV_PIX_FMT_NONE;

    bool formatFound = false;
    map< __u32, AVPixelFormat >::iterator i;
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
        cerr << fourcc_to_string( fmt.pixelformat ) << " not supported!" << endl;
        exit( EXIT_FAILURE );
    }

    typedef map< string, deque<double> > Acc;
    Acc acc;

    double prv = 0;

    dev.StartCapture();
    while( true )
    {
        prv = now();

        //send out the NAL header
        //static const char nal_header[4] = {0x00,0x00,0x00,0x01};
        //fwrite( (const char*)nal_header, 1, sizeof( nal_header ), stdout );
        //fflush( stdout );

        const VideoCapture::Buffer& b = dev.LockFrame();
        uint8_t* ptr = reinterpret_cast< unsigned char* >( const_cast< char* >( b.start ) );

        acc["1 - capture(ms):    "].push_back( ( now() - prv ) * 1000.0 );

        prv = now();

        //send everything except the first NAL header, which we already sent
        if( b.length > 4 )
        {
            //fwrite( ptr+4, 1, b.length-4, stdout );
            fwrite( ptr, 1, b.length, stdout );
            fflush( stdout ); //No need to fflush here, since the NAL header will do so
        }

        acc["2 - fwrite(ms):     "].push_back( ( now() - prv ) * 1000.0 );
        acc["3 - bytes/frame:    "].push_back( b.length );

        dev.UnlockFrame();

        prv = now();

        static double start = now();
        if( now() - start > 5.0 )
        {
            // print times
            for( Acc::iterator i = acc.begin(); i != acc.end(); ++i )
            {
                deque<double>& arr = i->second;
                while( arr.size() > 300 )
                    arr.pop_front();

                cerr << i->first;
                cerr << "\t" << "Median: " << ( median( arr ) );
                cerr << "\t" << "Stdev: " << ( stdev( arr ) );
                cerr << endl;
            }
            cerr << endl;

            start = now();
        }
    }

    dev.StopCapture();

    return 0;
}
