// g++ -g network_joystick.cpp `pkg-config sdl2 SDL_net --libs --cflags`
// run without arguments to start server
// run with <hostname> <stick #> <steer_axis> <throttle axis> to connect to server
// run with <unused hostname> to list sticks/axes

// joystick setup:
// rebuild SDL2 with --disable-input-events
// jstest-gtk, remap/calibrate as desired
// sudo jscal-store /dev/input/js0
// stuffs calibration in /var/lib/joystick/joystick.state
// which is loaded by /iib/udev/rules.d/60-joystick.rules during hotplug

#include <sstream>
#include <stdexcept>
#include <iostream>
#include <limits>

#include <SDL.h>
#include <SDL_net.h>

using namespace std;

#define THROW( d ) \
    { \
	std::ostringstream oss; \
    oss << "[" << __FILE__ << ":" << __LINE__ << "]"; \
	oss << " " << d; \
	throw std::runtime_error( oss.str() ); \
	}


const unsigned short PORT = 2000;


ostream& operator<<( ostream& out, const IPaddress& addr )
{
    const Uint8* quad = reinterpret_cast< const Uint8* >( &addr.host );
    out << static_cast< unsigned int >( quad[0] ) << ".";
    out << static_cast< unsigned int >( quad[1] ) << ".";
    out << static_cast< unsigned int >( quad[2] ) << ".";
    out << static_cast< unsigned int >( quad[3] ) << ":";
    out << SDLNet_Read16( const_cast< Uint16* >( &addr.port ) );
}


struct InputState
{
    char steering;
    char throttle;
};


void server()
{
    UDPsocket sd = SDLNet_UDP_Open( PORT );
    if( !sd )
        THROW( "SDLNet_UDP_Open: " << SDLNet_GetError() );

    SDLNet_SocketSet sset = SDLNet_AllocSocketSet( 1 );
    if( !sset )
        THROW( "SDLNet_AllocSocketSet: " << SDLNet_GetError() );

    if( -1 == SDLNet_UDP_AddSocket( sset, sd ) )
        THROW( "SDLNet_UDP_AddSocket: " << SDLNet_GetError() );
    
    UDPpacket* pkt = SDLNet_AllocPacket( 1500 );
    if( NULL == pkt )
        THROW( "SDLNet_AllocPacket: " << SDLNet_GetError() );

    bool running = true;
    while( running )
    {
        int numReady = SDLNet_CheckSockets( sset, 1000 );
        if( numReady < 0 )
            THROW( "SDLNet_CheckSockets: " << SDLNet_GetError() );
        
        if( numReady == 0 )
        {
            cerr << "timeout, looping" << endl;
            continue;
        }
        
        // at least one descriptor ready, grab packet
        if( -1 == SDLNet_UDP_Recv( sd, pkt ) )
            THROW( "SDLNet_UDP_Recv: " << SDLNet_GetError() );

        InputState* state = reinterpret_cast< InputState* >( pkt->data );
    
        //cout << "Chan: "    << pkt->channel         << endl;
        //cout << "Data: "    << (char*)(pkt->data)   << endl;
        //cout << "Len:  "    << pkt->len             << endl;
        cout << "Addr: "    << pkt->address         << endl;
        cout << (int)state->steering << ", " << (int)state->throttle << endl;
    }
    
    SDLNet_FreeSocketSet( sset );
    
    SDLNet_FreePacket( pkt );    
}


template< typename T >
void clamp( T& val, const T& lo, const T& hi )
{
    if( val <= lo ) val = lo;
    if( val >= hi ) val = hi;
}


template< typename T1, typename T2 >
T2 remap( const T1& val, const T1& srcMin, const T1& srcMax, const T2& dstMin, const T2& dstMax )
{
    double t = ( val - srcMin ) / static_cast<double>(srcMax - srcMin );
    double dstVal = ( t * ( dstMax - dstMin ) ) + dstMin;
    return static_cast<T2>( dstVal );
}


void client
    ( 
    const string& host, 
    int stick = -1, 
    int steeringAxis = -1,
    int throttleAxis = -1
    )
{
    if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_JOYSTICK ) < 0 )    
        THROW( "SDL_Init(): " << SDL_GetError() );
    
    if( stick < 0 )
    {
        cout << "Number of joysticks: " << SDL_NumJoysticks() << endl;
        for( size_t i = 0; i < SDL_NumJoysticks(); ++i )
        {
            string name = SDL_JoystickName( i );
            name = ( name == "" ? "Unknown" : name );
            cout << "Joystick " << i << ": " << name << endl;; 
            
            SDL_Joystick* joy = SDL_JoystickOpen( i );
            if( NULL == joy )
                THROW( "SDL_JoystickOpen(" << i << "): " << SDL_GetError() );

            cout << "   axes: " << SDL_JoystickNumAxes( joy ) << endl;
            cout << "  balls: " << SDL_JoystickNumBalls( joy ) << endl;
            cout << "   hats: " << SDL_JoystickNumHats( joy ) << endl;
            cout << "buttons: " << SDL_JoystickNumButtons( joy ) << endl;
            
            SDL_JoystickClose( joy );
        }    
        
        return;
    }
        
    SDL_Joystick* joy = NULL;
    if( stick >= 0 && stick < SDL_NumJoysticks() )
    {
        joy = SDL_JoystickOpen( stick );
        //SDL_JoystickEventState( SDL_ENABLE );
    }
                
    if( steeringAxis < 0 )
        steeringAxis = 0;
    if( throttleAxis < 0 )
        throttleAxis = 1;
    clamp( steeringAxis, 0, SDL_JoystickNumAxes( joy ) );
    clamp( throttleAxis, 0, SDL_JoystickNumAxes( joy ) );

    const int WIDTH = 640;
    const int HEIGHT = 480;
    SDL_Window* window = SDL_CreateWindow
        (
        "Joystick",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIDTH, HEIGHT,
        SDL_WINDOW_SHOWN
        );
    if( NULL == window )
        THROW( "SDL_CreateWindow(): " << SDL_GetError() );
    
    SDL_Renderer* renderer = SDL_CreateRenderer( window, -1, 0 );
    if( NULL == renderer )
        THROW( "SDL_CreateRenderer(): " << SDL_GetError() );
                
    UDPsocket sd = SDLNet_UDP_Open( 0 );
    if( !sd )
        THROW( "SDLNet_UDP_Open: " << SDLNet_GetError() );

    UDPpacket pkt;
    if( -1 == SDLNet_ResolveHost( &pkt.address, host.c_str(), PORT ) )
        THROW( "SDLNet_ResolveHost: " << SDLNet_GetError() );
    
    bool running = true;
    while( running )
    {
        SDL_Event e;
        while( SDL_PollEvent(&e) )
        {
            if( ( e.type == SDL_QUIT ) ||
                ( e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE ) )
            {
                running = false;
            }
        }

        SDL_SetRenderDrawColor( renderer, 0, 0, 0, 0 );
        SDL_RenderClear( renderer );

        // render joystick buttons
        for( size_t i = 0; i < SDL_JoystickNumButtons(joy); ++i )
        {
            SDL_Rect area;
            area.x = i * 34;
            area.y = HEIGHT - 34;
            area.w = 32;
            area.h = 32;
            if (SDL_JoystickGetButton(joy, i) == SDL_PRESSED) 
                SDL_SetRenderDrawColor( renderer, 255, 255, 255, 255 );
            else
                SDL_SetRenderDrawColor( renderer, 96, 96, 96, 0 );
            SDL_RenderFillRect( renderer, &area );
        }

        // render joystick axes
        for( size_t i = 0; i < SDL_JoystickNumAxes(joy); ++i )
        {
            float y = (float)SDL_JoystickGetAxis(joy, i) / 32768.0f;

            int w = 32;
            int h = 32;

            int beg = h/2 + 1;
            int end = HEIGHT - 32 - h/2 - 2;
            y *= (end-beg)/2.0f;

            y += beg + ((end-beg)/2.0f);

            SDL_Rect area;
            area.x = (Sint16)( i * (w + 8) ) + 8;
            area.y = (Sint16)y - h/2;
            area.w = w;
            area.h = h;

            SDL_SetRenderDrawColor( renderer, 255, 255, 255, 255 );
            SDL_RenderFillRect( renderer, &area );
        }

        SDL_RenderPresent( renderer );
      
        InputState state;
        state.steering = remap
            ( 
            SDL_JoystickGetAxis( joy, steeringAxis ), 
            numeric_limits< short >::min(), 
            numeric_limits< short >::max(),
            numeric_limits< char >::min(),
            numeric_limits< char >::max()
            );
        state.throttle = remap
            ( 
            SDL_JoystickGetAxis( joy, throttleAxis ), 
            numeric_limits< short >::min(), 
            numeric_limits< short >::max(),
            (char)0,
            numeric_limits< char >::max()
            );
            
        cout << (int)state.steering << " " << (int)state.throttle << endl;
        
        pkt.data = reinterpret_cast< Uint8* >( &state );
        pkt.len = sizeof( state );
        if( 0 == SDLNet_UDP_Send( sd, -1, &pkt ) )
            THROW( "SDLNet_UDP_Send: " << SDLNet_GetError() );
        
        SDL_Delay(25);
    }
    
    if( joy )
    {
        SDL_JoystickClose( joy );
    }
    
    SDL_Quit();
}


int main( int argc, char** argv )
{
	if( SDLNet_Init() < 0 )
        THROW( "SDLNet_Init: " << SDLNet_GetError() );

    if( argc == 1 )
        server();
    else if( argc == 2 )
        client( argv[1], -1 );
    else if( argc == 3 )
        client( argv[1], atoi( argv[2] ) );
    else if( argc == 5 )
        client( argv[1], atoi( argv[2] ), atoi( argv[3] ), atoi( argv[4] ) );

    SDLNet_Quit();
    return EXIT_SUCCESS;    
}
