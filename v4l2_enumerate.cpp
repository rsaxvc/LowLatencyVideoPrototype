#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <libv4l2.h>

#include <iostream>
#include <stdexcept>
#include <map>
#include <vector>

using namespace std;


struct buffer {
        void   *start;
        size_t length;
};


string strreq( int request )
{
    map< int, string > reqmap;
    reqmap[ VIDIOC_QUERYCAP ] = "VIDIOC_QUERYCAP";
    reqmap[ VIDIOC_ENUM_FMT ] = "VIDIOC_ENUM_FMT";

    map< int, string >::const_iterator itr = reqmap.find( request );
    if( itr == reqmap.end() )
    {
        return "UNKNOWN";
    }
    
    return itr->second;
}

void xioctl( int fh, int request, void* arg )
{
    int ret = 0;
    do 
    {
        ret = v4l2_ioctl( fh, request, arg );
    } 
    while( ret == -1 && ( (errno == EINTR) || (errno == EAGAIN) ) );

    if( ret == -1 )
    {
        throw std::runtime_error( "ioctl(" + strreq(request) + ") error: " + strerror(errno) );
    }
}


vector< v4l2_fmtdesc > 
EnumFormats
    ( 
    int fd, 
    v4l2_buf_type type 
    )
{
    vector< v4l2_fmtdesc > fmts;

    int curIndex = 0;
    while( true )
    {
        v4l2_fmtdesc fmt;
        fmt.index = curIndex;
        fmt.type = type;

        try
        {
            xioctl( fd, VIDIOC_ENUM_FMT, &fmt ); 
        }
        catch( std::exception& e )
        {
            // hit the end of the enumeration list, exit
            if( errno == EINVAL ) break;
            else throw e;
        }        
        
        fmts.push_back( fmt );
        curIndex++; 
    }
    
    return fmts;
}


vector< v4l2_frmsizeenum > 
EnumSizes
    ( 
    int fd, 
    const v4l2_fmtdesc& format 
    )
{
    vector< v4l2_frmsizeenum > sizes;

    int curIndex = 0;
    while( true )
    {
        v4l2_frmsizeenum size;
        size.index = curIndex;
        size.pixel_format = format.pixelformat;

        try
        {
            xioctl( fd, VIDIOC_ENUM_FRAMESIZES, &size ); 
        }
        catch( std::exception& e )
        {
            // hit the end of the enumeration list, exit
            if( errno == EINVAL ) break;
            else throw e;
        }        
        
        sizes.push_back( size );
        curIndex++; 
    }
    
    return sizes;
}


vector< v4l2_frmivalenum > 
EnumIntervals
    ( 
    int fd, 
    const v4l2_fmtdesc& format, 
    const v4l2_frmsizeenum& size 
    )
{
    vector< v4l2_frmivalenum > intervals;

    int curIndex = 0;
    while( true )
    {
        v4l2_frmivalenum interval;
        interval.index = curIndex;
        interval.pixel_format = format.pixelformat;
        interval.width = size.discrete.width;
        interval.height = size.discrete.height;

        try
        {
            xioctl( fd, VIDIOC_ENUM_FRAMEINTERVALS, &interval ); 
        }
        catch( std::exception& e )
        {
            // hit the end of the enumeration list, exit
            if( errno == EINVAL ) break;
            else throw e;
        }        
        
        intervals.push_back( interval );
        curIndex++; 
    }
    
    return intervals;
}



ostream& operator<<( ostream& os, const v4l2_frmsizeenum& size )
{
    if( size.type == V4L2_FRMSIZE_TYPE_DISCRETE )
    {
        os << size.discrete.width << "x" << size.discrete.height;
    }
    return os;    
}

ostream& operator<<( ostream& os, const v4l2_frmivalenum& interval )
{
    if( interval.type == V4L2_FRMIVAL_TYPE_DISCRETE )
    {
        float fps = interval.discrete.denominator / (float)interval.discrete.numerator;
        os << (int)fps;
    }
    return os;    
}



int main(int argc, char **argv)
{
    string device = "/dev/video0";
    if( argc == 2 )
    {
        device = argv[1];
    }

    int fd = -1;
    try
    {
        cout << "Opening device" << endl;
        fd = v4l2_open( device.c_str(), O_RDWR | O_NONBLOCK, 0 );
        if( fd < 0 )
            throw std::runtime_error("v4l2_open() failed");

        v4l2_capability cap;
        xioctl( fd, VIDIOC_QUERYCAP, &cap ); 

        if( !( cap.capabilities & V4L2_CAP_VIDEO_CAPTURE ) )
        {
            throw std::runtime_error("Not a video capture device");
        }

        cout << "device : " << device << endl;
        cout << "card   : " << cap.card << endl;
        cout << "driver : " << cap.driver << endl;
        cout << "bus    : " << cap.bus_info << endl;
        cout << endl;

        cout << "Formats: " << endl;
        vector< v4l2_fmtdesc > fmts = EnumFormats( fd, V4L2_BUF_TYPE_VIDEO_CAPTURE );
        for( size_t i = 0; i < fmts.size(); ++i ) 
        {
            const v4l2_fmtdesc& fmt = fmts[i];

            string flags = "(";
            if( fmt.flags & V4L2_FMT_FLAG_COMPRESSED )
                flags += "C";
            if( fmt.flags & V4L2_FMT_FLAG_EMULATED )
                flags += "E";
            else
                flags += "N";
            flags += ")";   

            cout << fmt.index << ": " << fmt.description << " " << flags << endl;
            
            vector< v4l2_frmsizeenum > sizes = EnumSizes( fd, fmt );
            for( size_t j = 0; j < sizes.size(); ++j )
            {
                const v4l2_frmsizeenum& size = sizes[j];
                cout << "\t" << size << endl;
                
                vector< v4l2_frmivalenum > intervals = EnumIntervals( fd, fmt, size );
                for( size_t k = 0; k < intervals.size(); ++k )
                {
                    const v4l2_frmivalenum& interval = intervals[k];
                    cout <<  "\t\t" << interval << endl;
                }
            }
        }   
    } 
    catch( std::exception& e )
    {
        cout << "Error: " << e.what() << endl;    
    }
    
    v4l2_close( fd );

    return 0;
}
