// g++ -g network_joystick.cpp `pkg-config SDL_net --libs --cflags`
// run without arguments to start server
// run with single hostname argument to connect to server on that host

#include <sstream>
#include <stdexcept>
#include <iostream>

#include <SDL_net.h>

using namespace std;

#define THROW( d ) \
    { \
	std::ostringstream oss; \
    oss << "[" << __FILE__ << ":" << __LINE__ << "]"; \
	oss << " " << d; \
	throw std::runtime_error( oss.str() ); \
	}


const unsigned short PORT = 5000;


ostream& operator<<( ostream& out, const IPaddress& addr )
{
    const Uint8* quad = reinterpret_cast< const Uint8* >( &addr.host );
    out << static_cast< unsigned int >( quad[0] ) << ".";
    out << static_cast< unsigned int >( quad[1] ) << ".";
    out << static_cast< unsigned int >( quad[2] ) << ".";
    out << static_cast< unsigned int >( quad[3] ) << ":";
    out << SDLNet_Read16( const_cast< Uint16* >( &addr.port ) );
}


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

        cout << "Chan: "    << pkt->channel         << endl;
        cout << "Data: "    << (char*)(pkt->data)   << endl;
        cout << "Len:  "    << pkt->len             << endl;
        cout << "Addr: "    << pkt->address         << endl;
    }
    
    SDLNet_FreeSocketSet( sset );
    
    SDLNet_FreePacket( pkt );    
}


void client( const string& host, int stick = -1 )
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
            
            SDL_Joystick* stick = SDL_JoystickOpen( i );
            if( NULL == stick )
                THROW( "SDL_JoystickOpen(" << i << "): " << SDL_GetError() );

            cout << "   axes: " << SDL_JoystickNumAxes( stick ) << endl;
            cout << "  balls: " << SDL_JoystickNumBalls( stick ) << endl;
            cout << "   hats: " << SDL_JoystickNumHats( stick ) << endl;
            cout << "buttons: " << SDL_JoystickNumButtons( stick ) << endl;
            
            SDL_JoystickClose( stick );
        }    
    }
    
    UDPsocket sd = SDLNet_UDP_Open( 0 );
    if( !sd )
        THROW( "SDLNet_UDP_Open: " << SDLNet_GetError() );

    UDPpacket* pkt = SDLNet_AllocPacket( 1500 );
    if( NULL == pkt )
        THROW( "SDLNet_AllocPacket: " << SDLNet_GetError() );

    if( -1 == SDLNet_ResolveHost( &pkt->address, host.c_str(), PORT ) )
        THROW( "SDLNet_ResolveHost: " << SDLNet_GetError() );
    
    while( true )
    {
        cout << "input: ";
        string msg;
        cin >> msg;
        
        if( msg == "" )
            break;
        
        pkt->data = reinterpret_cast< Uint8* >( const_cast<char*>( msg.c_str() ) );
        pkt->len = msg.size()+1;
        if( 0 == SDLNet_UDP_Send( sd, -1, pkt ) )
            THROW( "SDLNet_UDP_Send: " << SDLNet_GetError() );
    }
 
    SDLNet_FreePacket( pkt );    
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

    SDLNet_Quit();
    return EXIT_SUCCESS;    
}
