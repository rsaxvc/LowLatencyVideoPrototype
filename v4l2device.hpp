#ifndef INCLUDE_V4L2_DEVICE_GENERIC_HPP
#define INCLUDE_V4L2_DEVICE_GENERIC_HPP

#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>

#include <linux/videodev2.h>
#include <libv4l2.h>

#include <cstring>
#include <queue>
#include <sstream>
#include <stdexcept>


#define THROW( d ) \
	{ \
	std::ostringstream oss; \
    oss << "[" << __FILE__ << ":" << __LINE__ << "]"; \
	oss << " " << d; \
	throw std::runtime_error( oss.str() ); \
	} \


// wraps a V4L2_CAP_VIDEO_CAPTURE fd
class VideoCapture
{
public:
    struct Buffer
    {
        Buffer() : start(NULL), length(0) {}
	    char* start;
	    size_t length;
    };

    enum IO { READ, USERPTR, MMAP };

    VideoCapture( const std::string& device, const IO aIO = MMAP )
    {
        mFd = v4l2_open( device.c_str(), O_RDWR | O_NONBLOCK, 0);
        if( mFd == - 1 )
            THROW( "can't open " << device << ": " << strerror(errno) );

        // make sure this is a capture device
        v4l2_capability cap;
        xioctl( mFd, VIDIOC_QUERYCAP, &cap );
        if( !(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) )
            THROW("not a video capture device!");

        mSupported = IOMethods();
        if( mSupported.empty() ) THROW("no supported IO methods!");

        bool found_requested = false;
        for( size_t i = 0; i < mSupported.size(); ++i )
        {
            if( mSupported[i] == aIO )
            {
                found_requested = true;
                break;
            }
        }

        // use requested IO if supported, otherwise "fastest"
        mIO = ( found_requested ? aIO : mSupported.back() );

        mCapturing = false;
        mIsLocked = false;
    }

    ~VideoCapture()
    {
        UnlockFrame();
        StopCapture();
        v4l2_close( mFd );
    }

    IO SelectedIO()
    {
        return mIO;
    }

    std::vector< IO > SupportedIO()
    {
        return mSupported;
    }

    void StartCapture()
    {
        if( mCapturing ) THROW("already capturing!");
        mCapturing = true;

        // grab current frame format
        v4l2_pix_format fmt = GetFormat();

        // from the v4l2 docs: "Buggy driver paranoia."
        unsigned int min = fmt.width * 2;
        if (fmt.bytesperline < min)
            fmt.bytesperline = min;
        min = fmt.bytesperline * fmt.height;
        if (fmt.sizeimage < min)
            fmt.sizeimage = min;

        const unsigned int bufCount = 4;

        if( mIO == READ )
        {
            // allocate buffer
            mBuffers.resize( 1 );
            mBuffers[ 0 ].length = fmt.sizeimage;
            mBuffers[ 0 ].start = new char[ fmt.sizeimage ];
        }
        else
        {
            // request buffers
            v4l2_requestbuffers req;
            memset( &req, 0, sizeof(req) );
            req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            req.memory = ( mIO == MMAP ? V4L2_MEMORY_MMAP : V4L2_MEMORY_USERPTR );
            req.count = bufCount;
            xioctl( mFd, VIDIOC_REQBUFS, &req );

            if( mIO == USERPTR )
            {
                // allocate buffers
                mBuffers.resize( req.count );
                for( size_t i = 0; i < mBuffers.size(); ++i )
                {
                    mBuffers[ i ].length = fmt.sizeimage;
                    mBuffers[ i ].start = new char[ fmt.sizeimage ];
                }
            }
            else
            {
                // mmap buffers
                mBuffers.resize( req.count );
                for( size_t i = 0; i < mBuffers.size(); ++i )
                {
                    v4l2_buffer buf;
                    memset( &buf, 0, sizeof(buf) );
                    buf.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    buf.memory  = V4L2_MEMORY_MMAP;
                    buf.index   = i;
                    xioctl( mFd, VIDIOC_QUERYBUF, &buf );

                    mBuffers[i].length = buf.length;
                    mBuffers[i].start = (char*)v4l2_mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, mFd, buf.m.offset);
                    if( mBuffers[i].start == MAP_FAILED )
                        THROW("mmap() failed!");
                }
            }

            // queue buffers
            for( size_t i = 0; i < mBuffers.size(); ++i )
            {
                v4l2_buffer buf;
                memset( &buf, 0, sizeof(buf) );
                buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.index       = i;
                buf.memory = ( mIO == MMAP ? V4L2_MEMORY_MMAP : V4L2_MEMORY_USERPTR );
                if( mIO == USERPTR )
                {
                    buf.m.userptr   = (unsigned long)mBuffers[i].start;
                    buf.length      = mBuffers[i].length;
                }

                xioctl( mFd, VIDIOC_QBUF, &buf );
            }

            // start streaming
            v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            xioctl( mFd, VIDIOC_STREAMON, &type );
        }
    }

    void StopCapture()
    {
        if( !mCapturing ) return;
        mCapturing = false;

        if( mIO == READ )
        {
            delete[] mBuffers[0].start;
        }
        else
        {
            // stop streaming
            v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            xioctl( mFd, VIDIOC_STREAMOFF, &type );

            if( mIO == USERPTR )
            {
                // free memory
                for( size_t i = 0; i < mBuffers.size(); ++i )
                    delete[] mBuffers[i].start;
            }
            else
            {
                // unmap memory
                for( size_t i = 0; i < mBuffers.size(); ++i )
                    if( -1 == v4l2_munmap(mBuffers[i].start, mBuffers[i].length) )
                        THROW( "munmap() failed!" );
            }
        }

    }

    // wait for next frame and return it
    // timeout in seconds
    // someway to indicate timeout?  zero buffer?
    const Buffer& LockFrame( const float aTimeout = -1.0f )
    {
        if( mIsLocked ) THROW( "already locked!" );
        mIsLocked = true;

        // wait for frame
        while( true )
        {
            fd_set fds;
            FD_ZERO( &fds);
            FD_SET( mFd, &fds );

            timeval tv;
            tv.tv_sec = 2;
            tv.tv_usec = 0;

            int r = select( mFd + 1, &fds, NULL, NULL, &tv);
            if( -1 == r && EINTR == errno )
            {
                if( EINTR == errno )
                    continue;
                THROW( "select() error" );
            }

            // timeout
            if( 0 == r ) continue;

            // fd readable
            break;
        }

        if( mIO == READ )
        {
            if( -1 == v4l2_read( mFd, mBuffers[0].start, mBuffers[0].length) )
            {
                if( errno != EAGAIN && errno != EIO )
                    THROW( "read() error" );
            }

            mLockedFrame.start = mBuffers[0].start;
            mLockedFrame.length = mBuffers[0].length;
        }
        else
        {
            memset( &mLockedBuffer, 0, sizeof(mLockedBuffer) );
            mLockedBuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            mLockedBuffer.memory = ( mIO == MMAP ? V4L2_MEMORY_MMAP : V4L2_MEMORY_USERPTR );
            if( -1 == v4l2_ioctl( mFd, VIDIOC_DQBUF, &mLockedBuffer) )
            {
                if( errno != EAGAIN && errno != EIO )
                    THROW( "ioctl() error" );
            }

            size_t i;
            if( mIO == USERPTR )
            {
                // only given pointers, find corresponding index
                for( i = 0; i < mBuffers.size(); ++i )
                {
                    if( mLockedBuffer.m.userptr == (unsigned long)mBuffers[i].start &&
                        mLockedBuffer.length == mBuffers[i].length )
                    {
                        break;
                    }
                }
            }
            else
            {
                i = mLockedBuffer.index;
            }

            if( i >= mBuffers.size() )
                THROW( "buffer index out of range" );

            mLockedFrame.start = mBuffers[i].start;
            mLockedFrame.length = mLockedBuffer.bytesused;
        }

        return mLockedFrame;
    }

    void UnlockFrame()
    {
        if( !mIsLocked ) return;
        mIsLocked = false;

        if( mIO == READ ) return;

        xioctl( mFd, VIDIOC_QBUF, &mLockedBuffer );
    }


    v4l2_pix_format GetFormat()
    {
        v4l2_format fmt;
        memset( &fmt, 0, sizeof(fmt) );
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        xioctl( mFd, VIDIOC_G_FMT, &fmt );
        return fmt.fmt.pix;
    }

    bool SetFormat( const v4l2_pix_format& aFmt )
    {
        v4l2_format fmt;
        memset( &fmt, 0, sizeof(fmt) );
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix = aFmt;

        while( true )
        {
            try
            {
                xioctl( mFd, VIDIOC_S_FMT, &fmt );
            }
            catch( std::exception& e )
            {
                //if( errno == EBUSY ) continue;
                if( errno == EINVAL ) return false;
                throw e;
            }
            break;
        }

        return true;
    }


    v4l2_fract GetInterval()
    {
        v4l2_streamparm param;
        memset( &param, 0, sizeof(param) );
        param.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        xioctl( mFd, VIDIOC_G_PARM, &param );
        return param.parm.capture.timeperframe;
    }


    std::vector< v4l2_fmtdesc > GetFormats()
    {
        std::vector< v4l2_fmtdesc > fmts;

        int curIndex = 0;
        while( true )
        {
            v4l2_fmtdesc fmt;
            fmt.index = curIndex++;
            fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            try
            {
                xioctl( mFd, VIDIOC_ENUM_FMT, &fmt );
            }
            catch( std::exception& e )
            {
                // hit the end of the enumeration list, exit
                if( errno == EINVAL ) break;
                else throw e;
            }

            fmts.push_back( fmt );
        }

        return fmts;
    }

    std::vector< v4l2_frmsizeenum > GetSizes( const v4l2_fmtdesc& format )
    {
        std::vector< v4l2_frmsizeenum > sizes;

        int curIndex = 0;
        while( true )
        {
            v4l2_frmsizeenum size;
            size.index = curIndex++;
            size.pixel_format = format.pixelformat;

            try
            {
                xioctl( mFd, VIDIOC_ENUM_FRAMESIZES, &size );
            }
            catch( std::exception& e )
            {
                // hit the end of the enumeration list, exit
                if( errno == EINVAL ) break;
                else throw e;
            }

            sizes.push_back( size );

            // only discrete has multiple enumerations
            if( size.type != V4L2_FRMSIZE_TYPE_DISCRETE )
                break;
        }

        return sizes;
    }

    std::vector< v4l2_frmivalenum > GetIntervals( const v4l2_fmtdesc& format, const v4l2_frmsizeenum& size )
    {
        std::vector< v4l2_frmivalenum > intervals;

        int curIndex = 0;
        while( true )
        {
            v4l2_frmivalenum interval;
            interval.index = curIndex++;
            interval.pixel_format = format.pixelformat;
            interval.width = size.discrete.width;
            interval.height = size.discrete.height;

            try
            {
                xioctl( mFd, VIDIOC_ENUM_FRAMEINTERVALS, &interval ); 
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

private:
    std::vector< IO > IOMethods()
    {
        std::vector< IO > supported;

        v4l2_capability cap;
        xioctl( mFd, VIDIOC_QUERYCAP, &cap );

        // test read/write
        if( cap.capabilities & V4L2_CAP_READWRITE )
            supported.push_back( READ );

        if( cap.capabilities & V4L2_CAP_STREAMING )
        {
            v4l2_requestbuffers req;
            int ret = 0;

            // test userptr
            memset( &req, 0, sizeof(req) );
            req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            req.count = 1;
            req.memory = V4L2_MEMORY_USERPTR;
            if( 0 == v4l2_ioctl( mFd, VIDIOC_REQBUFS, &req ) )
            {
                supported.push_back( USERPTR );
                req.count = 0;
                // blind ioctl, some drivers get pissy with count = 0
                v4l2_ioctl( mFd, VIDIOC_REQBUFS, &req );
            }

            // test mmap
            memset( &req, 0, sizeof(req) );
            req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            req.count = 1;
            req.memory = V4L2_MEMORY_MMAP;
            if( 0 == v4l2_ioctl( mFd, VIDIOC_REQBUFS, &req ) )
            {
                supported.push_back( MMAP );
                req.count = 0;
                // blind ioctl, some drivers get pissy with count = 0
                v4l2_ioctl( mFd, VIDIOC_REQBUFS, &req );
            }
        }

        return supported;
    }

    static const char* strreq( int r )
    {
        if( r == VIDIOC_QUERYCAP )  return "VIDIOC_QUERYCAP";
        if( r == VIDIOC_ENUM_FMT )  return "VIDIOC_ENUM_FMT";
        if( r == VIDIOC_G_FMT )     return "VIDIOC_G_FMT";
        if( r == VIDIOC_REQBUFS )   return "VIDIOC_REQBUFS";
        return "UNKNOWN";
    }

    static void xioctl( int fd, int request, void* arg )
    {
        int ret = 0;
        do
        {
            ret = v4l2_ioctl( fd, request, arg );
        }
        while( ret == -1 && ( (errno == EINTR) || (errno == EAGAIN) ) );

        if( ret == -1 )
            THROW( "Req: " << strreq(request) << ", ioctl() error: " << strerror(errno) );
    }

    int mFd;
    IO mIO;
    std::vector< IO > mSupported;
    bool mCapturing;

    bool mIsLocked;
    Buffer mLockedFrame;
    v4l2_buffer mLockedBuffer;

    std::vector< Buffer > mBuffers;
};

#endif
