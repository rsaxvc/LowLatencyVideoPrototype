/** \file
* Defines the device::devicebase class.
*
* \author Martin F. Krafft <krafft@ailab.ch>
* \date $Date: 2004/05/21 09:52:38 $
* \version $Revision: 1.9 $
*/

#ifndef INCLUDE_V4L2_DEVICE_GENERIC_HPP
#define INCLUDE_V4L2_DEVICE_GENERIC_HPP

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>

#include <linux/videodev2.h>
#include <libv4l2.h>

#include <cstring>
#include <queue>
#include <sstream>
#include <stdexcept>


#define V4L2_ERROR( d ) \
	{ \
	std::ostringstream oss; \
	oss << d; \
	throw std::runtime_error( oss.str() ); \
	} \


///////////////////////////////////////////////////////////
//// BUFFER INTERFACE /////////////////////////////////////
template <typename T>
class bufferbase
{
public:
	inline bufferbase(size_t const SIZE) : _M_size(SIZE)
	{
	}

	virtual inline ~bufferbase()
	{
	}

	inline bufferbase& operator=(bufferbase const& THAT)
	{
		if (_M_size != THAT._M_size)
		{
			V4L2_ERROR("attempt to copy objects of different size.");
		}
		memcpy(get_buffer(), THAT.get_buffer(), _M_size);
		return *this;
	}

	virtual T& operator[](size_t const I) = 0;
	virtual T const& operator[](size_t const I) const = 0;

	inline size_t get_size() const
	{
		return _M_size;
	}

	virtual T* get_buffer() = 0;
	virtual T const* get_buffer() const = 0;

private:
	size_t const _M_size;
};


///////////////////////////////////////////////////////////
//// HEAP BUFFER //////////////////////////////////////////
template <typename T>
class heapbuffer : public bufferbase<T>
{
public:
	inline heapbuffer(size_t const N = 0) : bufferbase<T>(N), _M_buffer(NULL)
	{
		_M_new(N);
	}

	virtual inline ~heapbuffer()
	{
		_M_delete();
	}

	inline T& operator[](size_t const I)
	{
		if (I >= this->get_size())
		{
			V4L2_ERROR("index " << I << " is out of range on buffer " << this->get_buffer() << " (size=" << this->get_size() << ").");
		}
		return this->get_buffer()[I];
	}

	inline T const& operator[](size_t const I) const
	{
		if (I >= this->get_size())
		{
			V4L2_ERROR("index " << I << " is out of range on buffer " << this->get_buffer() << " (size=" << this->get_size() << ").");
		}
		return this->get_buffer()[I];
	}

private:
	inline void _M_new(size_t const N)
	{
        if (N > 0) _M_buffer = new T[N];
	}

	inline void _M_delete()
	{
		if (!_M_buffer)
		{
			// attempted to destroy NULL buffer, ignore
			return;
		}
		delete[] _M_buffer;
	}

	inline T* get_buffer()
	{
		return _M_buffer;
	}

	inline T const* get_buffer() const
	{
		return _M_buffer;
	}

	T* _M_buffer;
};


///////////////////////////////////////////////////////////
//// MMAP BUFFER //////////////////////////////////////////
template <typename T>
class mmapbuffer : public bufferbase<T>
{
public:
	inline mmapbuffer(size_t const LEN, int const PROT, int const FLAGS, int const FD, off_t const OFFSET)
		: bufferbase<T>(LEN), _M_buffer(NULL)
	{
		_M_map(LEN, PROT,FLAGS, FD, OFFSET);
	}

	virtual inline ~mmapbuffer()
	{
		_M_unmap();
	}

	inline T& operator[](size_t const I)
	{
		if (I >= this->get_size())
		{
			V4L2_ERROR("index " << I << " is out of range of memory map " << this->get_buffer() << " (size=" << this->get_size() << ").");
		}
		return this->get_buffer()[I];
	}

	inline T const& operator[](size_t const I) const
	{
		if (I >= this->get_size())
		{
			V4L2_ERROR("index " << I << " is out of range of memory map " << this->get_buffer() << " (size=" << this->get_size() << ").");
		}
		return this->get_buffer()[I];
	}

private:
	inline void _M_map(size_t const LEN, int const PROT, int const FLAGS, int const FD, off_t const OFFSET)
	{
		if (LEN > 0)
		{
			_M_buffer = reinterpret_cast<T*>(::mmap(0, LEN, PROT, FLAGS, FD, OFFSET));
			if (_M_buffer == MAP_FAILED)
			{
				V4L2_ERROR("failed to create memory map.");
			}
		}
	}

	inline void _M_unmap()
	{
		if (!_M_buffer)
		{
			// attempt to destroy NULL memory map, ignore
			return;
		}
		munmap(reinterpret_cast<void*>(_M_buffer), this->get_size());
	}

	inline T* get_buffer()
	{
		return _M_buffer;
	}

	inline T const* get_buffer() const
	{
		return _M_buffer;
	}

	T* _M_buffer;
};


///////////////////////////////////////////////////////////
//// NONCOPYABLE TYPE /////////////////////////////////////
class noncopyable
{
protected:
    inline noncopyable() {}
    virtual inline ~noncopyable() {}

private:
    inline noncopyable(noncopyable const&) {}
    inline noncopyable& operator=(noncopyable const&) { return *this; }
};


///////////////////////////////////////////////////////////
//// DEVICE ///////////////////////////////////////////////
class devicebase : noncopyable
{
public:
    typedef char byte;
    typedef bufferbase<byte> buffer_type;

    virtual devicebase& open(std::string const& DEVICE, bool const BLOCKING)
    {
        if(BLOCKING)
            _M_fd = v4l2_open(DEVICE.c_str(), O_RDWR);
        else
            _M_fd = v4l2_open(DEVICE.c_str(), O_RDWR | O_NONBLOCK);
        if(!this->is_opened()) 
            V4L2_ERROR("could not open device " << DEVICE);
        _M_retrieve_capabilities();
        _M_check_requirements();
        return *this;
    }

    virtual devicebase& close()
    {
        if(!this->is_opened()) 
            V4L2_ERROR("attempt to close unopened device.");
        v4l2_close(_M_fd);
        _M_fd = -1;
        return *this;
    }

    inline bool is_opened() const
    {
        return _M_fd > -1;
    }

    inline devicebase& set_buffer_queue_length(size_t const DEPTH)
    {
        // TODO: protect against change after allocation
        _M_count = DEPTH;
        // TODO: this should print debugging and be in .cpp really.
        return *this;
    }

    devicebase& set_video_standard(v4l2_std_id std)
    {
        _M_ensure_is_opened();
        if(v4l2_ioctl(_M_fd, VIDIOC_S_STD, &std) != 0)
        {
            V4L2_ERROR("failed to set video standard to " << std << '.');
        }
        return *this;
    }

    virtual buffer_type const* get_next_frame() = 0;
    virtual void recycle_frame(buffer_type const*) = 0;

protected:
    devicebase(v4l2_buf_type const TYPE)
        : _M_format(), _M_qbuf(), _M_queue(), _M_fd(-1), _M_count(0), _M_type(TYPE), _M_capability()
    {
        _M_clear_struct(_M_capability);
        _M_clear_struct(_M_format);
    }

    virtual ~devicebase()
    {
        if(this->is_opened())
        {
            this->close();
        }
        _M_destroy_buffers();
    }

    inline int _M_get_fd() const
    {
        return _M_fd;
    }

    inline v4l2_buf_type _M_get_type() const
    {
        return _M_type;
    }

    inline void _M_ensure_is_opened() const
    {
        if (!this->is_opened())
        {
            V4L2_ERROR("attempt to access unopened device.");
        }
    }

    void _M_retrieve_capabilities()
    {
        _M_clear_struct(_M_capability);
        if(v4l2_ioctl(_M_fd, VIDIOC_QUERYCAP, &_M_capability) != 0)
        {
            V4L2_ERROR("could not read device capabilities.");
        }
    }

    virtual void _M_check_requirements() = 0;

    inline bool _M_has_capability(int const CAP)
    {
        return _M_capability.capabilities & CAP;
    }

    void _M_retrieve_format()
    {
        _M_format.type = _M_type;
        if(v4l2_ioctl(_M_fd, VIDIOC_G_FMT, &_M_format) != 0)
        {
            V4L2_ERROR("could not read device format specifications.");
        }
    }

    void _M_set_format()
    {
        _M_format.type = _M_type;
        if(v4l2_ioctl(_M_fd, VIDIOC_S_FMT, &_M_format) != 0)
        {
            V4L2_ERROR("could not set image extents.");
        }
    }

    void _M_check_buffer_count()
    {
        if(_M_get_buffer_count() == 0)
        {
            V4L2_ERROR("cannot operate with 0 buffers.");
        }
    }

    void _M_destroy_buffers()
    {
        while(!_M_queue.empty())
        {
            delete _M_queue.front();
            _M_queue.pop();
        }
    }

    buffer_type* _M_alloc_heap(size_t const LEN)
    {
        buffer_type* ret = NULL;
        try
        {
            ret = new heapbuffer<byte>(LEN);
            return ret;
        }
        catch(std::bad_alloc const& E)
        {
            V4L2_ERROR("failed to allocate memory for heap buffer.");
        }
    }

    buffer_type* _M_alloc_mmap(size_t const LEN, off_t const OFFSET)
    {
        buffer_type* ret = NULL;
        try
        {
            ret = new mmapbuffer<byte>(LEN, PROT_READ | PROT_WRITE, MAP_SHARED, _M_fd, OFFSET);
            return ret;
        }
        catch(std::bad_alloc const& E)
        {
            V4L2_ERROR("failed to allocate memory for memory map buffer.");
        }
    }

    void _M_prepare_userptr();

    void _M_prepare_mmap();

    inline size_t _M_get_buffer_count() const
    {
        return _M_count;
    }

    template <typename T>
    inline void _M_clear_struct(T& s)
    {
        memset(&s, 0, sizeof(s));
    }

    template <typename T>
    inline void _M_copy_struct(T const& SRC, T& dest)
    {
        memcpy(&dest, &SRC, sizeof(SRC));
    }

    v4l2_format _M_format;
    v4l2_buffer _M_qbuf;

    typedef std::queue<buffer_type*> queue_type;
    queue_type _M_queue;

private:
    int _M_fd;

    size_t _M_count;

    v4l2_buf_type _M_type;
    v4l2_capability _M_capability;
};


///////////////////////////////////////////////////////////
//// CAPTURE //////////////////////////////////////////////
class capture : public devicebase
{
public:
    capture& set_capture_channel(int channel)
    {
        _M_ensure_is_opened();
        if (v4l2_ioctl(_M_get_fd(), VIDIOC_S_INPUT, &channel) != 0)
        {
            V4L2_ERROR("failed to set capture channel to " << channel << '.');
        }
        return *this;
    }

    capture& set_image_extents(__u32 const WIDTH, __u32 const HEIGHT)
    { 
        if (WIDTH == 0 || HEIGHT == 0)
        { 
            V4L2_ERROR("cannot set image extents to zero values.");
        }
        _M_ensure_is_opened();
        _M_retrieve_format();
        _M_format.fmt.pix.width = WIDTH;
        _M_format.fmt.pix.height = HEIGHT;
        _M_set_format();
        _M_retrieve_format();
        return *this;
    }

    capture& set_image_format(__u32 const FORMAT)
    { 
        _M_ensure_is_opened();
        _M_retrieve_format();
        _M_format.fmt.pix.pixelformat = FORMAT;
        _M_set_format();
        _M_retrieve_format();
        return *this;
    }

    capture& set_image_field_order(v4l2_field const ORDER)
    { 
        _M_ensure_is_opened();
        _M_retrieve_format();
        _M_format.fmt.pix.field = ORDER;
        _M_set_format();
        _M_retrieve_format();
        return *this;
    }

    virtual capture& configure()
    {
        _M_ensure_is_opened();
        _M_prepare();
        return *this;
    }

    virtual bool is_configured()
    {
        return false; // TODO
    }

    buffer_type const* get_next_frame()
    {
        if (_M_dequeue_single_buffer())
        {
            buffer_type const* ret = _M_queue.front();
            _M_queue.pop();
            return ret;
        }
        return NULL;
    }

    void recycle_frame(buffer_type const* buf)
    {
        if (!buf)
            V4L2_ERROR("null buffer");
        _M_queue.push(const_cast<buffer_type*>(buf));
        _M_queue_single_buffer();
    }

    virtual capture& release()
    {
        _M_release();
        return *this;
    }

    class readwrite;
    class streaming;
    class mmap;
    class userptr;

protected:
    capture()
        : devicebase(V4L2_BUF_TYPE_VIDEO_CAPTURE)
    {
    }

    virtual ~capture()
    {
        if (this->is_configured())
        {
            try { this->release(); }
            catch (...) {} // TODO: this is kinda ugly...
        }
    }

    virtual void _M_check_requirements()
    {
        if (!_M_has_capability(V4L2_CAP_VIDEO_CAPTURE))
        {
            V4L2_ERROR("device does not support capture acquisition.");
        }
    }

    inline size_t _M_get_image_size()
    {
        return _M_format.fmt.pix.sizeimage;
    }

    inline size_t _M_get_image_size_aligned()
    {
        int const PS = getpagesize();
        return PS * ((size_t)((float)_M_get_image_size() / PS) + 1);
    }

    virtual void _M_prepare() = 0;

    virtual void _M_queue_single_buffer() = 0;

    virtual bool _M_dequeue_single_buffer() = 0;

    virtual void _M_dequeue_all_buffers() = 0;

    virtual void _M_release() = 0;
};


///////////////////////////////////////////////////////////
//// READ/WRITE ///////////////////////////////////////////
class capture::readwrite : public capture
{
public:
    readwrite() : capture(), _M_index(0)
    {
    }

    virtual ~readwrite()
    {
    }

protected:
    virtual void _M_check_requirements()
    {
        capture::_M_check_requirements();
        if (!_M_has_capability(V4L2_CAP_READWRITE))
        {
            V4L2_ERROR("device does not support read/write access.");
        }
    }

    void _M_prepare()
    {
        for (size_t i = 0; i < _M_get_buffer_count(); ++i)
        {
            this->recycle_frame(_M_alloc_heap(_M_get_image_size_aligned()));
        }
    }

    void _M_release()
    {
        _M_dequeue_all_buffers();
        _M_destroy_buffers();
    }

    void _M_queue_single_buffer()
    {
        if (0); // noop;
    }

    virtual bool _M_dequeue_single_buffer()
    {
        size_t const LEN = _M_get_image_size();
        size_t const N = read(_M_get_fd(), _M_queue.front()->get_buffer(), LEN);
        if (N == 0) return false;
        if (N != LEN)
        {
            V4L2_ERROR("failed to read " << LEN << " bytes from device.");
        }
        _M_index = (_M_index + 1) % _M_get_buffer_count();
        return true;
    }

    void _M_dequeue_all_buffers()
    {
        if (0); // noop;
    }

private:
    size_t _M_index;
};


///////////////////////////////////////////////////////////
//// STREAMING ////////////////////////////////////////////
class capture::streaming : public capture
{
public:
    class mmap;
    class userptr;

protected:
    streaming(v4l2_memory const TYPE) : capture(), _M_index(0), _M_memory(TYPE)
    {
    }

    virtual ~streaming()
    {
        if (this->_M_is_streaming())
        {
            try { this->release(); }
            catch (...) {} // TODO: this is kinda ugly...
        }
    }

    virtual void _M_check_requirements()
    {
        capture::_M_check_requirements();
        if (!_M_has_capability(V4L2_CAP_STREAMING))
        {
            V4L2_ERROR("device does not support streaming.");
        }
    }

    virtual void _M_prepare()
    {
        if (!_M_has_capability(V4L2_CAP_STREAMING))
        {
            V4L2_ERROR("the device does not support this buffer method.");
        }
        _M_activate_streaming();
    }

    virtual void _M_release()
    {
        _M_deactivate_streaming();
    }

    virtual void _M_request_buffers()
    {
        v4l2_requestbuffers req;
        _M_clear_struct(req);
        req.type = _M_get_type();
        req.count = _M_get_buffer_count();
        req.memory = _M_memory;
        if (v4l2_ioctl(_M_get_fd(), VIDIOC_REQBUFS, &req) != 0)
        {
            V4L2_ERROR("request to map " << req.count << " buffers failed.");
        }
    }

    virtual void _M_queue_single_buffer()
    {
        if (v4l2_ioctl(_M_get_fd(), VIDIOC_QBUF, &_M_qbuf) != 0)
        {
            V4L2_ERROR("failed to queue buffer with index " << _M_index << '.');
        }
        //assert(_M_qbuf.index == _M_index);
        _M_index = (_M_index + 1) % _M_get_buffer_count();
    }

    virtual bool _M_dequeue_single_buffer()
    {
        _M_clear_struct(_M_qbuf);
        _M_qbuf.type = _M_get_type();
        if (v4l2_ioctl(_M_get_fd(), VIDIOC_DQBUF, &_M_qbuf) != 0)
        {
            if (errno == EAGAIN)
            {
                // no buffer ready for dequeing.
                return false;
            }
            V4L2_ERROR("failed to dequeue buffer: " << strerror(errno) );
        }
        return true;
    }

    void _M_dequeue_all_buffers()
    {
        for (size_t i = 0; i < _M_get_buffer_count(); ++i)
        {
            if (!_M_dequeue_single_buffer()) break;
        }
    }

    void _M_activate_streaming()
    {
        if (v4l2_ioctl(_M_get_fd(), VIDIOC_STREAMON, &_M_qbuf.type) != 0)
        {
            V4L2_ERROR("failed to activate the video stream.");
        }
    }

    void _M_deactivate_streaming()
    {
        if (v4l2_ioctl(_M_get_fd(), VIDIOC_STREAMOFF, &_M_qbuf.type) != 0)
        {
            V4L2_ERROR("failed to deactivate the video stream.");
        }
    }

    inline bool _M_is_streaming()
    {
        return false; // TODO
    }

    inline size_t _M_cur_index()
    {
        return _M_index;
    }

private:
    size_t _M_index;
    v4l2_memory _M_memory;
};


///////////////////////////////////////////////////////////
//// USERPTR //////////////////////////////////////////////
class capture::userptr : public capture::streaming
{
public:
    userptr() : streaming(V4L2_MEMORY_USERPTR)
    {
    }

    virtual ~userptr()
    {
    }

protected:
    void _M_prepare()
    {
        _M_request_buffers();
        _M_clear_struct(_M_qbuf);
        _M_qbuf.type = _M_get_type();
        for (_M_qbuf.index = 0; _M_qbuf.index < _M_get_buffer_count(); ++_M_qbuf.index)
        {
            if (v4l2_ioctl(_M_get_fd(), VIDIOC_QUERYBUF, &_M_qbuf) != 0)
            {
                V4L2_ERROR("failed to query buffer with index " << _M_qbuf.index << '.');                                                                                     
            }
            this->recycle_frame(_M_alloc_heap(_M_get_image_size_aligned()));
        }
        streaming::_M_prepare();
    }

    virtual void _M_release()
    {
        _M_dequeue_all_buffers();
        _M_destroy_buffers();
    }
};


///////////////////////////////////////////////////////////
//// MMAP /////////////////////////////////////////////////
class capture::mmap : public capture::streaming
{
public:
    mmap() : streaming(V4L2_MEMORY_MMAP)
    {
    }

    virtual ~mmap()
    {
    }

protected:
    void _M_prepare()
    {
        _M_request_buffers();
        _M_clear_struct(_M_qbuf);
        _M_qbuf.type = _M_get_type();
        for (_M_qbuf.index = 0; _M_qbuf.index < _M_get_buffer_count(); ++_M_qbuf.index)
        {
            if (v4l2_ioctl(_M_get_fd(), VIDIOC_QUERYBUF, &_M_qbuf) != 0)
            { 
                V4L2_ERROR("failed to query buffer with index " << _M_qbuf.index << '.');   
            } 
            this->recycle_frame(_M_alloc_mmap(_M_qbuf.length, _M_qbuf.m.offset));
        } 
        streaming::_M_prepare();
    }

    virtual void _M_release()
    {
        _M_dequeue_all_buffers();
        _M_destroy_buffers();
    }

    virtual void _M_queue_single_buffer()
    {
        _M_qbuf.index = _M_cur_index();
        _M_qbuf.m.userptr = reinterpret_cast<unsigned long>(_M_queue.back()->get_buffer());
        _M_qbuf.length = _M_queue.back()->get_size();
        streaming::_M_queue_single_buffer();
    }
};

#endif // include guard
