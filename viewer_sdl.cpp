#define __STDC_CONSTANT_MACROS

#include <sstream>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <fstream>
#include <queue>

#include <SDL.h>
#include <SDL_thread.h>
#include <SDL_mutex.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
}

#include "data_source.h"
#include "x264_destreamer.h"


using namespace std;

#define THROW( d ) \
    { \
	std::ostringstream oss; \
    oss << "[" << __FILE__ << ":" << __LINE__ << "]"; \
	oss << " " << d; \
	throw std::runtime_error( oss.str() ); \
	}


SDL_Rect ScaleAspect( const SDL_Rect& src, const SDL_Rect& dst )
{
    SDL_Rect ret;
    ret.w = dst.w;
    ret.h = dst.h;

    double srcRatio = (double)src.w / src.h;
    double dstRatio = (double)dst.w / dst.h;
    if( srcRatio > dstRatio )
        ret.h = dst.w / srcRatio;
    else
        ret.w = dst.h * srcRatio;

    ret.x = (dst.w - ret.w) / 2;
    ret.y = (dst.h - ret.h) / 2;

    return ret;
}


// needs:
// av_register_all();
// avcodec_register_all();
bool GetBufferInfo
    (
    const vector< unsigned char >& buf,
    const unsigned int sz,
    unsigned int& width,
    unsigned int& height
    )
{
    if( sz > buf.size() )
        return false;

    unsigned char* abuf = reinterpret_cast<unsigned char*>( av_malloc( sz ) );
    copy( buf.begin(), buf.begin() + sz, abuf );

    AVIOContext* ioCtx = avio_alloc_context
        (
        abuf,
        sz,
        0,
        NULL,
        NULL,
        NULL,
        NULL
        );

    AVFormatContext* fmtCtx = avformat_alloc_context();
    fmtCtx->pb = ioCtx;

    int ret = 0;
    if( ret == 0 )
    {
        ret = avformat_open_input
            (
            &fmtCtx,
            "",
            NULL,
            NULL
            );
    }

    if( ret == 0 )
    {
        ret = avformat_find_stream_info( fmtCtx, NULL );
    }

    if( ret == 0 )
    {
        for( unsigned int i = 0; i < fmtCtx->nb_streams; ++i )
        {
            AVCodecContext* codec = fmtCtx->streams[i]->codec;
            if( codec->width > 0 && codec->height > 0 )
            {
                width = codec->width;
                height = codec->height;
                break;
            }
        }
    }

    av_free( ioCtx );
    av_free( abuf );

    return ( ret == 0 );
}


struct FrameQueue
{
    FrameQueue()
    {
        eventNumber = SDL_RegisterEvents(1);
        mutex = SDL_CreateMutex();
    }
    ~FrameQueue()
    {
        SDL_DestroyMutex( mutex );
    }

    void Lock()
    {
        SDL_mutexP( mutex );
    }

    void Unlock()
    {
        SDL_mutexV( mutex );
    }

    Uint32 eventNumber;
    queue< vector< unsigned char > > frames;

private:
    SDL_mutex* mutex;
};


class data_source_packetizer: public data_source
{
public:
    void write( const uint8_t * data, size_t bytes )
    {
        packets.push_back( std::vector< uint8_t >( data, data + bytes ) );
    }

    std::deque< std::vector< uint8_t > > packets;
};


int FrameThread( void* ptr )
{
    FrameQueue& fq = *((FrameQueue*)ptr);

    av_register_all();
    avcodec_register_all();

    // vector< unsigned char > buf( LoadFile("dump.x264") );
    //unsigned int width = 0;
    //unsigned int height = 0;
    //GetBufferInfo( buf, 10*1024, width, height );

    AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if( codec == NULL )
        cout << "bad codec" << endl;

    AVCodecContext* codecCtx = avcodec_alloc_context3( codec );
    if( codecCtx == NULL )
        cout << "bad codecCtx" << endl;

    codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    codecCtx->width = 160;
    codecCtx->height = 120;
    if( avcodec_open2( codecCtx, codec, NULL ) < 0 )
        cout << "couldn't open codec" << endl;

    if( codecCtx->time_base.num > 1000 && codecCtx->time_base.den == 1 )
        codecCtx->time_base.den = 1000;

    AVFrame* frame = av_frame_alloc();
    if( frame == NULL )
        cout << "bad frame" << endl;

    x264_destreamer ds;
    data_source_packetizer dspkt;
    ds.server.register_callback( &dspkt );
    while( !feof(stdin) )
    {
        uint8_t data;
        data = getchar();
        ds.write( &data, sizeof( data ) );

        while( !dspkt.packets.empty() )
        {
            vector< uint8_t > pkt = dspkt.packets.front();
            dspkt.packets.pop_front();

            // Decode video frame
            AVPacket avpkt;
            av_init_packet( &avpkt );
            avpkt.data = &pkt[0];
            avpkt.size = pkt.size();
            int gotFrame = 0;
            avcodec_decode_video2( codecCtx, frame, &gotFrame, &avpkt );

            if( gotFrame == 0 )
                continue;

            vector< unsigned char > fbuf;

            vector< unsigned int > widths(3);
            widths[0] = frame->width;
            widths[1] = frame->width / 2;
            widths[2] = frame->width / 2;
            vector< unsigned int > heights(3);
            heights[0] = frame->height;
            heights[1] = frame->height / 2;
            heights[2] = frame->height / 2;
            for( unsigned int plane = 0; plane < 3; ++plane )
            {
                unsigned char* base = frame->data[ plane ];
                for( unsigned int y = 0; y < heights[ plane ]; ++y )
                {
                    fbuf.insert( fbuf.end(), base, base + widths[ plane ] );
                    base += frame->linesize[ plane ];
                }
            }

            fq.Lock();
            fq.frames.push( fbuf );
            fq.Unlock();

            SDL_Event event;
            event.type = fq.eventNumber;
            SDL_PushEvent( &event );
        }
    }
}



int main( int argc, char **argv )
{
    if( SDL_Init(SDL_INIT_VIDEO) < 0 )
        THROW( "Couldn't initialize SDL: " << SDL_GetError() );

    SDL_Rect winRect;
    winRect.w = 640;
    winRect.h = 360;

    SDL_Window* window = SDL_CreateWindow
        (
        "SDL",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        winRect.w, winRect.h,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
        );
    if( !window )
        THROW( "Couldn't create window: " << SDL_GetError() );

    SDL_Renderer* renderer = SDL_CreateRenderer( window, -1, 0 );
    if( !renderer )
        THROW( "Couldn't create renderer: " << SDL_GetError() );

    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer, &info);
    cout << "Using renderer: " << info.name << endl;

    SDL_Texture* tex = SDL_CreateTexture
        (
        renderer,
        SDL_PIXELFORMAT_IYUV,
        SDL_TEXTUREACCESS_STREAMING,
        160, 120
        );
    if( !tex )
        THROW( "Couldn't create texture: " << SDL_GetError() );

    FrameQueue fq;
    SDL_Thread* ft = SDL_CreateThread( FrameThread, "FrameThread", (void*)&fq );

    bool running = true;
    while( running )
    {
        SDL_Event event;
        while( SDL_PollEvent( &event ) )
        {
            switch ( event.type )
            {
            case SDL_QUIT:
                running = false;
                break;

            case SDL_KEYUP:
                if( event.key.keysym.sym == SDLK_ESCAPE )
                    running = false;
                if( event.key.keysym.sym == SDLK_f )
                {
                }
                break;

            case SDL_WINDOWEVENT:
                if( event.window.event == SDL_WINDOWEVENT_RESIZED )
                {
                    winRect.w = event.window.data1;
                    winRect.h = event.window.data2;
                    SDL_RenderSetViewport( renderer, NULL );
                }
                if( event.window.event == SDL_WINDOWEVENT_MOVED )
                {
                    winRect.x = event.window.data1;
                    winRect.y = event.window.data2;
                }
                break;
            }

            if( event.type == fq.eventNumber )
            {
                fq.Lock();
                vector< unsigned char > frame = fq.frames.front();
                fq.frames.pop();
                fq.Unlock();
                SDL_UpdateTexture(tex, NULL, &frame[0], 160 * SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_IYUV) );
            }
        }

        SDL_SetRenderDrawColor( renderer, 0, 0, 0, 0 );
        SDL_RenderClear( renderer );

        SDL_Rect src;
        src.w = 160;
        src.h = 120;
        SDL_Rect s = ScaleAspect(src, winRect );

        SDL_SetRenderDrawColor( renderer, 255, 0, 0, 0 );
        SDL_RenderFillRect( renderer, &s );

        SDL_RenderCopy( renderer, tex, NULL, &s );

        SDL_RenderPresent( renderer );

        SDL_Delay( 10 );
    }

    SDL_DestroyRenderer( renderer );
    SDL_DestroyWindow( window );

    SDL_Quit();

    return 0;
}
