// metatracker, an agnostic tool
// - rlyeh

// ui todo
// [ ] shortcuts text input shift-ins ctrl-x shift-left
// [ ] buffered text input
// [ ] add horiz scroll
// [ ] decouple begin area scroll, titlebar, window

// app todo
// [ ] create json api
// [ ] create authoring doc
// [ ] create knot server
// [ ] widget dragging
// [ ] multiple ranges in same line
// [ ] make it to work
// [ ] asset handling

// usage
// app args | meta
// meta app args
// meta
// drag 'n drop

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <cmath>
#include <iostream>

#include "imgui.hpp"
#include "imguiGL.hpp"
//#include "fonts/WenQuanYiMicroHei.ttf.hpp"
//#include "fonts/DroidSansFallback.ttf.hpp"
#include "fonts/Ubuntu-B.ttf.hpp"
#include "fonts/Entypo-Social.ttf.hpp"
#include "fonts/Entypo.ttf.hpp"
#include "fonts/FontAwesome.ttf.hpp"

#include <GLFW/glfw3.h>

#include <vector>
#include <fstream>
#include <sstream>

#include <medea.hpp>
#include <sand.hpp>

#include "dialog.hpp"
#include "locate.hpp"

#include "stb_image.h"

// ~similar
#define IMGUI_ICON_ENT_MANDATORY      IMGUI_ICON_ENT_NEW
#define IMGUI_ICON_ENT_LOAD           IMGUI_ICON_ENT_ARCHIVE
#define IMGUI_ICON_ENT_ONTOP_ON       IMGUI_ICON_ENT_BOOKMARK
#define IMGUI_ICON_ENT_ONTOP_OFF      IMGUI_ICON_ENT_BOOKMARKS
#define IMGUI_ICON_ENT_MINIMIZE       IMGUI_ICON_ENT_DOWN_BOLD
#define IMGUI_ICON_ENT_CLOSE_APP      IMGUI_ICON_ENT_CIRCLED_CROSS

#define ICON(x) ( std::string("\2") + cpToUTF8(x) )
#define JOIN(a,b) ( (a) + (b) ).c_str()

#undef INSERT_TOOLBAR
#define INSERT_TOOLBAR(where) \
    switch( imguiToolbar(icons) ) { \
        break; default: \
        break; case 4: where.bit( 0, where.bit(0) ^ true ); \
        break; case 3: where.bit( 1, where.bit(1) ^ true ); \
        break; case 2: where.bit( 2, where.bit(2) ^ true ); \
        break; case 1: where.bit( 3, where.bit(3) ^ true ); \
    }

#undef INSERT_TOOLBAR
#define INSERT_TOOLBAR(where) \
    if( imguiBitmask( "", &where.bitmask, DEFAULT_BITS) ) { \
    }


enum config {
    IS_TOOLBAR = 0
};


static std::string cpToUTF8(int cp)
{
    static char str[8];
    int n = 0;
    if (cp < 0x80) n = 1;
    else if (cp < 0x800) n = 2;
    else if (cp < 0x10000) n = 3;
    else if (cp < 0x200000) n = 4;
    else if (cp < 0x4000000) n = 5;
    else if (cp <= 0x7fffffff) n = 6;
    str[n] = '\0';
    switch (n) {
    case 6: str[5] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0x4000000;
    case 5: str[4] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0x200000;
    case 4: str[3] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0x10000;
    case 3: str[2] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0x800;
    case 2: str[1] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0xc0;
    case 1: str[0] = cp;
    }
    return str;
}

unsigned CRC32( const void *pMem, size_t iLen, unsigned hash = 0 )
{
    if( pMem == 0 || iLen == 0 ) return hash;

    static const unsigned crcTable[16] =
    {
        0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC,
        0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
        0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C,
        0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C
    };

    const unsigned char *pPtr = (const unsigned char *)pMem;
    unsigned h = ~hash;

    while( iLen-- )
    {
        h ^= (*pPtr++);
        h = crcTable[h & 0x0f] ^ (h >> 4);
        h = crcTable[h & 0x0f] ^ (h >> 4);
    }

    return ~h;
}

float HUE( const std::vector<unsigned> &text ) {
    if( text.size() < 2 ) {
        return 0;
    }
    static auto compute = []( unsigned ch, unsigned ch2 ) {
        return ( CRC32( &ch2, sizeof(ch2), CRC32( &ch, sizeof(ch) ) ) & 0xF ) / float(0xF);
    };
    static std::map< std::pair<unsigned,unsigned>, float > memo;
    if( memo.empty() ) {
        for( unsigned i = 0; i <= 255; ++i ) {
        for( unsigned j = 0; j <= 255; ++j ) {
            memo[std::pair<unsigned,unsigned>(i,j)] = compute(i,j);
        }}
    }
    std::pair<unsigned,unsigned> elem( text.front(),text.back() );
    auto found = memo.find( elem );
    if( found != memo.end() ) {
        return found->second;
    }
    float hue = compute( elem.first, elem.second );
    memo[ elem ] = hue;
    return hue;
}

const float preinit = HUE( std::vector<unsigned>({0,0}) );

enum { DEFAULT_BITMASK = 1, DEFAULT_BITS = 4 };

struct props {
    unsigned bitmask = DEFAULT_BITMASK;

    void bit( int no, bool on ) {
        if( on ) bitmask |= (1<<no);
        else bitmask &= ~(1<<no);
    }
    bool bit( int no ) const {
        return (bitmask & (1<<no)) > 0;
    }

#   define bit(name,no) \
    void name( bool on ) { bit(no,on); } \
    bool name() const { return bit(no); }

    bit(enabled,0)
    bit(to_create,1)
    bit(to_delete,2)
    bit(mandatory,3)

#   undef bit
};

struct range : props {
    float min = 100, max = 100;
    float mmin = 0, mmax = 1000.f;
    float lo = 0.5f, hi = 0.5f;
    int tween = 0, shake = 0;
    float cache = -1;

    bool update() {
        return to_delete();
    }
    void check( const std::string &name, float at ) {
        if( cache == at )
            return;
        cache = at;
        if( enabled() ) {
            if( at >= min && at <= max ) {
                std::cout << "[" << this << "] " << name << " triggered!" << std::endl;
            }
        }
    }
};

struct track : props {
    std::vector< unsigned > name;
    std::vector< range > ranges;
    float cache = -1;

    track() : ranges(1) {
    }

    void check( float at ) {
        if( cache == at )
            return;
        cache = at;
        if( enabled() ) {
            for( auto it = ranges.begin(), end = ranges.end(); it != end; ++it ) {
                it->check( imguiTextConv( name ) + "." + std::to_string(it - ranges.begin()), at );
            }
        }
    }
    bool update() {
        for( auto it = ranges.begin(); it != ranges.end(); ) {
            it->update();
            /**/ if( it->to_delete() ) it = ranges.erase( it );
            else if( it->to_create() ) {
                it->to_create( false );

                ranges.insert( it, *it );

                it = ranges.begin();
            }
            else                 ++it;
        }
		return to_delete();
    }
};

struct meta : props {
    unsigned version = 0x0100;
    std::string name = "untitled";
    float ms = 2000, hz = 60;

    std::vector< track > tracks;

    meta()
    {}

    float cache = -1;
    void check( float at ) {
        if( cache == at )
            return;
        cache = at;
        if( enabled() ) {
            for( auto &track : tracks) {
                track.check( at );
            }
        }
    }
    bool update() {
        auto insert = [&]() {
        };
        if(to_delete())
            *this = meta();
        if(to_create()) {
            to_create( false );
            tracks.push_back( track() );
            tracks.back().name = imguiTextConv( std::string() + "track #" + std::to_string( tracks.size() ) );
        }
        for( auto it = tracks.begin(); it != tracks.end(); ) {
            it->update();
            /**/ if( it->to_delete() ) it = tracks.erase( it );
            else if( it->to_create() ) {
                it->to_create( false );

                it = tracks.insert( ++it, *it );
                it->name = imguiTextConv( std::string() + "track #" + std::to_string( tracks.size() ) );

                it = tracks.begin();
            }
            else                     ++it;
        }
        return to_delete();
    }
} prj;

MEDEA_DEFINE( range &it, (it.min, it.max, it.mmin, it.mmax, it.bitmask, it.lo, it.hi, it.tween ) );
MEDEA_DEFINE( track &it, (it.name, it.ranges, it.bitmask) );
MEDEA_DEFINE( meta &it, (it.version, it.name, it.ms, it.hz, it.tracks, it.bitmask) );

double mx = 0, my = 0;
void _glfwSetCursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
    mx = xpos;
    my = ypos;
}


// time server
#include <thread>
#include <chrono>
#include <algorithm>
#include <sstream>
static float t0 = 0, t1 = 0, t2 = 1000;
volatile bool is_working = true;

template<bool wait>
void refresh( float dt ) {
    t1 = std::fmod( t1 + dt, t2 );
    if( wait )
        std::this_thread::sleep_for( std::chrono::milliseconds(int(dt)) );
}

bool intserver() {
    while(is_working) {
        refresh<true>( 1.f );
    }
    return true;
}

bool streamserver( FILE *fp ) {
    float t0, t1;
    std::string line, tag;

    while(is_working) {

        std::stringstream ss;
        std::getline( std::cin, line );

        if( (ss << line) && (ss >> tag >> t0 >> t1) && (tag == "metatracker") ) {
            if( t0 > t1 ) std::swap( t0, t1 );
            refresh<false>( t1 - t0 );
            continue;
        }

        std::cout << line << std::endl;
        //std::this_thread::sleep_for( std::chrono::milliseconds(1) );
    }
    return true;
}

#ifdef _MSC_VER
#define popen _popen
#define pclose _pclose
#endif

bool fserver( std::string &cmdline ) {
   FILE *fp = popen( cmdline.c_str(), "r" );
   if( !fp ) {
    return false;
   }
   if( !streamserver(fp) ) {
    return false;
   }
   pclose(fp);
   return true;
}





unsigned int KeyUNICODE=0, ctrlv=0, ctrlc=0, ctrlx=0;

void GetUNICODE(GLFWwindow *window, unsigned int unicode ){
    KeyUNICODE = unicode;
}

void GetControlKEY(GLFWwindow *window, int glfw_key, int sys_scancode, int glfw_action, int glfw_mods){
    if(glfw_action == GLFW_PRESS)
    {
        switch (glfw_key)
        {
        case GLFW_KEY_BACKSPACE:
            KeyUNICODE=0x08;
            break;
        case GLFW_KEY_ENTER:
            KeyUNICODE=0x0D;
            break;
        case GLFW_KEY_C:
            if( glfw_mods & GLFW_MOD_CONTROL ) ctrlc = 1;
            break;
        case GLFW_KEY_V:
            if( glfw_mods & GLFW_MOD_CONTROL ) ctrlv = 1;
            break;
        case GLFW_KEY_X:
            if( glfw_mods & GLFW_MOD_CONTROL ) ctrlx = 1;
            break;
        default:
            break;
        }
    }
    if(glfw_action == GLFW_RELEASE)
        KeyUNICODE = ctrlx = ctrlc = ctrlv = 0;
}

#include <thread>

#ifdef _WIN32

#include <windows.h>
void set_cursor( LPCTSTR B = 0 ) {
    auto ptr = B ? LoadCursor( NULL, B ) : nullptr;
#if !defined(__MINGW64__) && _MSC_VER <= 1200
    SetCursor( ptr );
    SetClassLong( 0, GCL_HCURSOR, ( LONG )ptr );
#else
    SetCursor( ptr );
    SetClassLongPtr( 0, GCLP_HCURSOR, ( LONG )( LONG_PTR )ptr );
#endif
}

#define $windows(...) __VA_ARGS__
#define $welse(...)
#else
#define $windows(...)
#define $welse(...)   __VA_ARGS__
#endif

#include <cassert>
#include <vector>
template < typename T, template <class U, class = std::allocator<U> > class container_t = std::vector >
class spline : public container_t< T >
{
    public:

    class vector : public std::vector<T>
    {
        public:

        vector( const size_t N = 0, const T& t = T() ) : std::vector<T>(N, t)
        {}

        template<typename S>
        void append( const S &t )
        {
            /*
            size_t pos = this->size() ? this->size() - 1 : 0;
            this->resize( this->size() + t.size() );
            std::copy( t.begin(), t.end(), this->begin() + pos );
            */
            for( size_t i = 0; i < t.size(); ++i)
                this->push_back( t[i] );
        }
    };

    spline() : container_t<T>()
    {}

    spline( size_t size ) : container_t<T>( size )
    {}

    T position( float dt01 ) const
    {
        assert( this->size() >= 4 );
        assert( dt01 >= 0 && dt01 <= 1.0 );

        size_t size = this->size();
        register float t = dt01 * ( ((float)(size - 3)) - 0.0001f );

        // Determine which spline segment we're on
        //if (t < 0) t = 0;
        //if (t > size - 3) t = (float)(size - 3) - 0.0001f;
        int segment = (int)std::floor(t);
        t -= std::floor(t);

        // Determine the four Hermite control values
        const T P0 =  this->operator[](segment+1);
        const T P1 =  this->operator[](segment+2);
        const T T0 = (this->operator[](segment+2) - this->operator[](segment)) * 0.5f;
        const T T1 = (this->operator[](segment+3) - this->operator[](segment+1)) * 0.5f;

        // Evaluate the Hermite basis functions
        const float hP0 = (2*t - 3)*t*t + 1;
        const float hP1 = (-2*t + 3)*t*t;
        const float hT0 = ((t - 2)*t + 1)*t;
        const float hT1 = (t - 1)*t*t;

        // Evaluate the curve
        return P0 * hP0 + P1 * hP1 + T0 * hT0 + T1 * hT1;
    }

    T tangent( float dt01 ) const
    {
        assert( this->size() >= 4 );
        assert( dt01 >= 0 && dt01 <= 1.0 );

        size_t size = this->size();
        float t = dt01 * ( ((float)(size - 3)) - 0.0001f );

        // Determine which spline segment we're on
        //if (t < 0) t = 0;
        //if (t > size - 3) t = (float)(size - 3) - 0.0001f;
        int segment = (int)std::floor(t);
        t -= std::floor(t);

        // Determine the four Hermite control values
        const T P0 =  this->operator[](segment+1);
        const T P1 =  this->operator[](segment+2);
        const T T0 = (this->operator[](segment+2) - this->operator[](segment)) * 0.5f;
        const T T1 = (this->operator[](segment+3) - this->operator[](segment+1)) * 0.5f;

        // Evaluate the Hermite basis functions' derivatives
        const float hP0 = (6*t - 6)*t;
        const float hP1 = (-6*t + 6)*t;
        const float hT0 = (3*t - 4)*t + 1;
        const float hT1 = (3*t - 2)*t;

        // Evaluate the curve
        return P0 * hP0 + P1 * hP1 + T0 * hT0 + T1 * hT1;
    }

    T position_w_edges( float dt01 ) const
    {
        // spline pos (border knots included)

        assert( this->size() >= 1 );

/*
        spline knotList;
        knotList.push_back( this->front() );
        knotList.append( *this );
        knotList.push_back( this->back() );
*/
        spline knotList( this->size() + 2 );
        std::copy( this->begin(), this->end(), knotList.begin() + 1 );
        knotList.front() = *( this->begin() );
        knotList.back() = *( this->end() - 1 );

        return knotList.position( dt01 );
    }

    T tangent_w_edges( float dt01 ) const
    {
        // spline tangent (border knots included)

        assert( this->size() >= 1 );

/*
        spline knotList;

        knotList.push_back( this->front() );
        knotList.append( *this );
        knotList.push_back( this->back() );
*/
        spline knotList( this->size() + 2 );
        std::copy( this->begin(), this->end(), knotList.begin() + 1 );
        knotList.front() = *( this->begin() );
        knotList.back() = *( this->end() - 1 );

        return knotList.tangent( dt01 );
    }

    // legacy {

    T tanf( float dt01 ) const
    {
        return tangent_w_edges( dt01 );
    }

    T atf( float dt01 ) const
    {
        return position_w_edges( dt01 );
    }

    float length( size_t segments /* >= 1 */ )
    {
        float overall = 0.f;

        for (size_t i = 0; i < segments; ++i)
        {
            float dt01 = i / (float)segments;

            vec3 A = atdt( dt01 );
            vec3 B = atdt( dt01 + ( 1.f / float(segments) ) );

            overall += len( B - A ); // this is not portable!
        }

        return overall;
    }

    // }
};

typedef std::pair<float,float> vec2;

class vec2f : public vec2
{
public:

    vec2f() : vec2(0.f,0.f)
    {}

    vec2f( const vec2 &v ) : vec2(v)
    {}

    explicit
    vec2f( float x, float y = 0.f) : vec2(x,y)
    {}

    vec2f operator-( const vec2 &other ) const {
        return vec2f( first - other.first, second - other.second );
    }
    vec2f operator+( const vec2 &other ) const {
        return vec2f( first + other.first, second + other.second );
    }
    vec2f operator*( const vec2 &other ) const {
        return vec2f( first * other.first, second * other.second );
    }
    vec2f operator-( const float &other ) const {
        return *this - vec2f(other,other);
    }
    vec2f operator+( const float &other ) const {
        return *this + vec2f(other,other);
    }
    vec2f operator*( const float &other ) const {
        return *this * vec2f(other,other);
    }
};


#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795f
#endif

double wheelx = 0, wheely = 0;

void scroll_callback(GLFWwindow *win, double xoff, double yoff ) {
    wheelx = xoff;
    wheely = yoff;
}

void scroll_reset() {
    wheelx = 0;
    wheely = 0;
}

static float timeline = 0, timeline_dt = 0, timeline_px = 0;
static float &fixedstep = prj.hz;
static float &milliseconds = prj.ms;
static bool paused = false;
static bool running = true;

int main( int argc, char **argv )
{
    if( argc > 1 ) {
        std::string args;
        for( int i = 1; i < argc; ++i ) {
            args += std::string() + argv[i] + " ";
        }
        args += std::string() + "|" + argv[0];
        return system( args.c_str() );
    }

    std::thread server( streamserver,stdin ); // argc > 1 ? cinserver : intserver );

    int border = 1;
    int width = 200 + border * 2, height=640;

    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        exit( EXIT_FAILURE );
    }

    // Open a window and create its OpenGL context
$apple( $GL3(
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
) )

    if( IS_TOOLBAR ) {
        glfwWindowHint(GLFW_DECORATED, GL_FALSE);
    }

    GLFWmonitor *monitor = 0; /* 0 for windowed, glfwGetPrimaryMonitor() for primary monitor, etc */
    GLFWwindow *shared = 0;
    GLFWwindow* window = glfwCreateWindow(width, height, "metatracker - " __TIMESTAMP__, monitor, shared);
    if( !window )
    {
        fprintf( stderr, "Failed to open GLFW window\n" );
        glfwTerminate();
        exit( EXIT_FAILURE );
    }

    glfwMakeContextCurrent(window);

$apple( $GL3(
    glewExperimental = GL_TRUE;
) )
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
          /* Problem: glewInit failed, something is seriously wrong. */
          fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
          exit( EXIT_FAILURE );
    }

    const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowPos( window, mode->width/8 - width/2, mode->height/8 );

    // Ensure we can capture the mouse wheel
    glfwSetScrollCallback( window, scroll_callback );

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode( window, GLFW_STICKY_KEYS, GL_TRUE );

    //
    glfwSetCursorPosCallback( window, _glfwSetCursorPosCallback );

    // Enable vertical sync (on cards that support it)
    // glfwSwapInterval( 1 );

    // Init UI
    if (!imguiRenderGLInit())
    {
        fprintf(stderr, "Could not init GL renderer.\n");
        exit(EXIT_FAILURE);
    }
    if (!imguiRenderGLFontInit(1,14.f,"meta.ttf"))
    {
        if (!imguiRenderGLFontInit(1,14.f,ubuntu_b_ttf::data(),ubuntu_b_ttf::size()))
    //    if (!imguiRenderGLFontInit(1,14.f,wenquanyimicrohei_ttf::data(),wenquanyimicrohei_ttf::size()))
    //    if (!imguiRenderGLFontInit(1,14.f,droidsansfallback_ttf::data(),droidsansfallback_ttf::size()))
        {
            fprintf(stderr, "Could not init font renderer.\n");
            exit(EXIT_FAILURE);
        }
    }

    if (!imguiRenderGLFontInit(2,32.f,entypo_ttf::data(), entypo_ttf::size()))
    {
        fprintf(stderr, "Could not init font renderer.\n");
        exit(EXIT_FAILURE);
    }

    if (!imguiRenderGLFontInit(3,14.f,fontawesome_ttf::data(), fontawesome_ttf::size()))
    {
        fprintf(stderr, "Could not init font renderer.\n");
        exit(EXIT_FAILURE);
    }

    glClearColor(0.8f, 0.8f, 0.8f, 1.f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    // imgui states
    bool checked1 = false;
    bool checked2 = false;
    bool checked3 = true;
    bool checked4 = false;
    float value1 = 50.f;
    float value2 = 30.f;
    float value3A = 33.f, value3B = 66.f;
    float value4A = 33.f, value4B = 66.f;
    int scrollarea1 = 0;
    int scrollarea2 = 0;

    // glfw scrolling
    int glfwscroll = 0;

    //glfw callback
    glfwSetCharCallback(window, GetUNICODE);
    glfwSetKeyCallback(window, GetControlKEY);
    char input[15] = "type here\0";

    // main loop
    while( running && (!glfwWindowShouldClose(window)) )
    {
        glfwGetFramebufferSize(window, &width, &height);
        if( width * height <= 1 ) {
            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }
        glViewport(0, 0, width, height);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Mouse states
        unsigned char mousebutton = 0;

        int mscroll = int(-wheely);
        scroll_reset();

        static double _mpx = 0, _mpy = 0;
        static double  mpx = 0,  mpy = 0;
        double mousex, mousey;
        mpx = _mpx, mpy = _mpy;
        glfwGetCursorPos(window, &mousex, &mousey);
        _mpx = mousex, _mpy = mousey;
        mousey = height - mousey;

        int leftButton = glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_LEFT );
        int rightButton = glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_RIGHT );
        int middleButton = glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_MIDDLE );
        int toggle = 0;
        if( leftButton == GLFW_PRESS )
            mousebutton |= IMGUI_MOUSE_BUTTON_LEFT;
        if( rightButton == GLFW_PRESS )
            mousebutton |= IMGUI_MOUSE_BUTTON_RIGHT;

        // toolbar window movement
        if( IS_TOOLBAR ) {
            int wx, wy;
            glfwGetWindowPos( window, &wx, &wy );

            static int dragX = 0, dragY = 0;
            if( !leftButton ) {
                dragX = 0;
                dragY = 0;
            } else {
                if( dragX == 0 && dragY == 0 ) {
                    dragX = wx;
                    dragY = wy;
                }
            }

            if( leftButton && imguiIsIdle() ) {
                int diffx = int(_mpx-mpx), diffy = int(_mpy-mpy);
                glfwSetWindowPos( window, wx + wx - ( dragX - diffx ), wy + wy - ( dragY - diffy ) );
                dragX = wx;
                dragY = wy;
            }
        }

        // Draw UI
$GL2(
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        float projection[16] = { 2.f/width, 0.f, 0.f,  0.f,
                                 0.f, 2.f/height,  0.f,  0.f,
                                 0.f,  0.f, -2.f, 0.f,
                                 -1.f, -1.f,  -1.f,  1.f };
        glLoadMatrixf(projection);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glUseProgram(0);
)

        imguiBeginFrame(int(mousex), int(mousey), mousebutton, mscroll, KeyUNICODE);

        imguiBeginScrollArea( (std::string("metatracker - ") + prj.name).c_str(), border, border, width - border * 2, height - border * 2, &scrollarea1, false);

        {
            static bool ontop = false;

            unsigned k1 = (paused ? IMGUI_ICON_FA_PLAY_CIRCLE : IMGUI_ICON_FA_PLAY_CIRCLE_O ); // IMGUI_ICON_FA_PAUSE);
            unsigned k3 = IMGUI_ICON_FA_THUMB_TACK; //(ontop ? IMGUI_ICON_ENT_ONTOP_OFF : IMGUI_ICON_ENT_ONTOP_ON);

            std::vector<unsigned> icons( {
                IMGUI_ICON_FA_EDIT,
                IMGUI_ICON_FA_PLUS_SQUARE, //ENT_SQUARED_PLUS,
                IMGUI_ICON_FA_EYE, //ENT_CD,
                IMGUI_ICON_FA_FOLDER_OPEN, //FA_CLOUD_DOWNLOAD //ENT_LOAD,
                IMGUI_ICON_FA_SAVE, //FA_ClOUD_UPLOAD //IMGUI_ICON_ENT_SAVE,
                k1,
                IMGUI_ICON_FA_STOP, //ENT_STOP,
                k3,
                IMGUI_ICON_FA_ARROW_DOWN, //IMGUI_ICON_ENT_MINIMIZE,
                IMGUI_ICON_FA_POWER_OFF, //FA_TIMES_CIRCLE, //ENT_CLOSE_APP,
                IMGUI_ICON_FA_WARNING, //IMGUI_ICON_ENT_MANDATORY,
                IMGUI_ICON_FA_QUESTION_CIRCLE, //ENT_CIRCLED_HELP
            } );

            switch( imguiToolbar( icons ) ) {
                default:
                break; case 1: prj = meta();
                break; case 3:  prj.enabled( prj.enabled() ^ true );
                break; case 4:  paused = true; timeline = t0 = t1 = 0;
                                {
                                    std::string file = moon9::load_dialog("Meta Files (*.meta.json)\0*.meta.json\0");
                                    if( !file.empty() ) {
                                        std::ifstream t( file.c_str() );
                                        std::stringstream buffer;
                                        buffer << t.rdbuf();
                                        medea::from_json( prj, buffer.str() );
                                    }
                                }
                break; case 5:  //paused = true; timeline = t0 = t1 = 0;
                                {
                                    std::string buffer = medea::to_json( prj );
                                    std::string file = moon9::save_dialog("Meta Files (*.meta.json)\0*.meta.json\0");
                                    if( !file.empty() ) {
                                        std::ofstream t( file.c_str() );
                                        t << buffer;
                                    }
                                }
                break; case 6: paused ^= true;
                break; case 7: paused = true; timeline = t0 = t1 = 0;
                break; case 8: $windows({
                                ontop ^= true;
                                HWND hwnd = GetForegroundWindow();
                                ::SetWindowPos(hwnd,       // handle to window
                                            ontop ? HWND_TOPMOST : HWND_NOTOPMOST,  // placement-order handle
                                            0,     // horizontal position
                                            0,      // vertical position
                                            0,  // width
                                            0, // height
                                            SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE// window-positioning options
                                            );
                            })
                break; case 9: glfwIconifyWindow( window );
                break; case 10: running = false;
            }
        }

        {
            static unsigned id = 0;
            if( !id ) {
                id = imguiRenderGLMakeTexture( "example.png" );
            }
            imguiImage( id );
        }

        {
            imguiLoadingBar(1, 20);

            int pos = imguiStackSet(-1);
            imguiLoadingBar(0, 30);
            pos = imguiStackSet( pos );

            {
                static float A = 0, B = 0;
                if( imguiXYSlider( "test", &A, &B, 100, 0.1f ) ) {
                    printf("clicked\n");
                }
            }

            t1 = timeline;

            prj.update();
            prj.check( t1 );
        }

        std::vector< track > &tracks = prj.tracks;

        imguiSeparatorLine();
        imguiSeparator();

        {
            if( !paused ) {
                t0 += 1000.f/fixedstep;
            }
            if( t0  > milliseconds ) {
                t0 -= milliseconds;

                //std::this_thread::sleep_for( std::chrono::seconds(int(t0)) );
                //t0 = 0;
            }
            timeline = t0;
            timeline_dt = timeline / milliseconds;
            std::this_thread::sleep_for( std::chrono::milliseconds(int(1000/fixedstep)) );

            imguiSlider( "ms", &milliseconds, 1, 5000, 1 );
            imguiSlider( "hz", &fixedstep, 5, 120, 5, "%d fps" );
        }

        std::vector<unsigned> icons( {
            IMGUI_ICON_ENT_MANDATORY,
            IMGUI_ICON_ENT_BAG,
            IMGUI_ICON_ENT_SQUARED_PLUS,
            IMGUI_ICON_ENT_CD
        } );

        {
            INSERT_TOOLBAR( prj )

            int pos = imguiStackSet(-2);
            unsigned id = imguiId() + 1;
            imguiSlider( "timeline", &timeline, 0, milliseconds, 1 ); // JOIN( "project ", ICON(SAVE) ) );
            imguiGfxRect rect = imguiRect(id);
            timeline_px = rect.x+timeline_dt*(rect.w-10)+10/2; // 10 = SLIDER_MARKER_WIDTH
            imguiDrawLine( timeline_px, 0.f, timeline_px, float(height), 1.f, imguiRGBA(128,128,128,64));
            if( imguiClicked(id) ) {
            }
            if( imguiOver(id) ) {
            }
            imguiStackSet(pos);
        }

        imguiPushEnable( prj.enabled() );
        for( auto it = tracks.begin(), end = tracks.end(); it != end; ++it ) {
            auto &track = *it;

            float hue = HUE(track.name); //( it - tracks.begin() ) / float(tracks.size() ); // HUE(track.name);
            imguiPushColor( imguiHSLA(hue /*+ (t1/t2)*/, 0.65f,0.55f) );

            INSERT_TOOLBAR( track )

                imguiPushEnable( track.enabled() );

                    {
                        int pos = imguiStackSet(-2);
                        {
                            unsigned id = imguiId() + 1;
                            if( ctrlv && imguiHot(id) ) {
                                const char *utf8 = glfwGetClipboardString(window);
                                if( utf8 ) track.name = imguiTextConv( utf8 );
                                ctrlv = 0;
                            }
                            if( imguiTextInput( "", track.name ) ) {
                                if( track.name.empty() ) {
                                    track.name = imguiTextConv("{untitled}");
                                }
                            }
                        }
                        imguiStackSet(pos);
                    }

                    for( auto &range : track.ranges ) {
                        INSERT_TOOLBAR( range );

                        int pos2 = imguiStackSet(-2);
                            imguiPushEnable( range.enabled() );
                                unsigned bar_id = imguiId() + 1;
                                unsigned slide0 = bar_id + 1;
                                unsigned slide1 = bar_id + 2;
                                imguiGfxRect rect = imguiRect( bar_id );
                                if( timeline_px > rect.x && timeline_px < rect.x + rect.w ) {
                                    ++range.shake;
                                } else {
                                    range.shake = 0;
                                }
                                imguiTween( range.tween + range.shake * prj.enabled() * track.enabled() * range.enabled() );
                                imguiQuadRange("", &range.min, &range.max, range.mmin, range.mmax, 1.f, &range.lo, &range.hi, "%d - %d" );
                                if( imguiClicked(slide0) || imguiClicked(slide1) ) {
                                    if( ++range.tween == sand::TOTAL ) range.tween = 0;
                                }
                            imguiPopEnable();
                        imguiStackSet(pos2);
                    }

                imguiPopEnable();
            imguiPopColor();
        }

        if(0)
        {
            int answer = 0;
            if( imguiShowDialog("are you sure?", &answer) ) {
                std::cout << (answer ? "yes" : "nop") << std::endl;
            }
        }

#if 0
        imguiButton("Disabled button", false);
        imguiItem("Item");
        imguiItem("Disabled item", false);
        toggle = imguiCheck("Checkbox", checked1);
        if (toggle)
            checked1 = !checked1;
        toggle = imguiCheck("Disabled checkbox", checked2, false);
        if (toggle)
            checked2 = !checked2;
        toggle = imguiCollapse("Collapse", "subtext", checked3);
        if (checked3)
        {
            imguiIndent();
            imguiLabel("Collapsible element");
            imguiUnindent();
        }
        if (toggle)
            checked3 = !checked3;
        toggle = imguiCollapse("Disabled collapse", "subtext", checked4, false);
        if (toggle)
            checked4 = !checked4;
        imguiLabel("Label");
        imguiValue("Value");

        imguiSlider("Slider", &value1, 0.f, 100.f, 1.f);
        imguiSlider("Disabled slider", &value2, 0.f, 100.f, 1.f, false);

        imguiRange("Disabled range", &value4A, &value4B, 0.f, 100.f, 1.f, false);

        imguiIndent();
        imguiLabel("Indented");
        imguiUnindent();
        imguiLabel("Unindented");

        imguiTextInput("Text input", input, 15);

        imguiPair( "hello", "pair" );

        {
            const char *list[] = {"hello", "world"};
            static int choosing = 0, clicked = 0;
            imguiList("hello", 2, list, choosing, clicked );
        }

        {
            const char *list[] = {"hello", "world"};
            static int clicked = 0;
            imguiRadio("hello", 2, list, clicked );
        }

        imguiEndScrollArea();

        imguiBeginScrollArea("Scroll area", 20 + width / 5, 500, width / 5, height - 510, &scrollarea2);
        imguiSeparatorLine();
        imguiSeparator();
        for (int i = 0; i < 100; ++i)
            imguiLabel("A wall of text");

        imguiDrawText(30 + width / 5 * 2, height - 20, IMGUI_ALIGN_LEFT, "Free text",  imguiRGBA(32,192, 32,192));
        imguiDrawText(30 + width / 5 * 2 + 100, height - 40, IMGUI_ALIGN_RIGHT, "Free text",  imguiRGBA(32, 32, 192, 192));
        imguiDrawText(30 + width / 5 * 2 + 50, height - 60, IMGUI_ALIGN_CENTER, "Free text",  imguiRGBA(192, 32, 32,192));
        imguiDrawText(30 + width / 5 * 2 + 50, height - 60, IMGUI_ALIGN_CENTER, (std::string("\2") + cpToUTF8(ICON_TRASH)).c_str(),  imguiRGBA(192, 32, 32,192));

        imguiDrawLine(30 + width / 5 * 2, height - 100, 30 + width / 5 * 2 + 100, height - 80, 2.f, imguiRGBA(32, 32, 192, 192));
        imguiDrawLine(30 + width / 5 * 2, height - 120, 30 + width / 5 * 2 + 100, height - 100, 3.f, imguiRGBA(192, 32, 32,192));

        imguiDrawRoundedRect(30 + width / 5 * 2, height - 240, 100, 100, 5.f, imguiRGBA(32,192, 32,192));
        imguiDrawRoundedRect(30 + width / 5 * 2, height - 350, 100, 100, 10.f, imguiRGBA(32, 32, 192, 192));
        imguiDrawRoundedRect(30 + width / 5 * 2, height - 470, 100, 100, 20.f, imguiRGBA(192, 32, 32,192));

        imguiDrawRect(30 + width / 5 * 2, height - 590, 100, 100, imguiRGBA(32, 192, 32, 192));
        imguiDrawRect(30 + width / 5 * 2, height - 710, 100, 100, imguiRGBA(32, 32, 192, 192));
        imguiDrawRect(30 + width / 5 * 2, height - 830, 100, 100, imguiRGBA(192, 32, 32,192));
#endif

#if 0
        {
            std::vector<vec2> points(5);
            points[0] = vec2(0.f,0.f);
            points[1] = vec2(width*1/4,height/4);
            points[2] = vec2(width  /2,height/2);
            points[3] = vec2(width*3/4,height/2);
            points[4] = vec2(width    ,height);
            imguiDrawLines(points, 2.f, imguiRGBA(32, 32, 192, 192));
            spline<vec2f> splined;
            for( auto &pt: points ) {
                splined.push_back( pt );
            }
            std::vector<vec2> tessellate;
            for( int i = 0; i <= 32; ++i ) {
                tessellate.push_back( splined.atf( i / 32.f ) );
            }
            imguiDrawLines(tessellate, 2.f, imguiRGBA(192, 32, 32, 192));
        }
#endif

        //drawHollowCircle( width/2, height/2, 100 );
        //drawHollowCircle( width/2, height/2, 120 );

            {
                const char *list[] = {
                    "LINEAR_01",

                    "QUADIN_01",          // t^2
                    "QUADOUT_01",
                    "QUADINOUT_01",
                    "CUBICIN_01",         // t^3
                    "CUBICOUT_01",
                    "CUBICINOUT_01",
                    "QUARTIN_01",         // t^4
                    "QUARTOUT_01",
                    "QUARTINOUT_01",
                    "QUINTIN_01",         // t^5
                    "QUINTOUT_01",
                    "QUINTINOUT_01",
                    "SINEIN_01",          // sin(t)
                    "SINEOUT_01",
                    "SINEINOUT_01",
                    "EXPOIN_01",          // 2^t
                    "EXPOOUT_01",
                    "EXPOINOUT_01",
                    "CIRCIN_01",          // sqrt(1-t^2)
                    "CIRCOUT_01",
                    "CIRCINOUT_01",
                    "ELASTICIN_01",       // exponentially decaying sine wave
                    "ELASTICOUT_01",
                    "ELASTICINOUT_01",
                    "BACKIN_01",          // overshooting cubic easing: (s+1)*t^3 - s*t^2
                    "BACKOUT_01",
                    "BACKINOUT_01",
                    "BOUNCEIN_01",        // exponentially decaying parabolic bounce
                    "BOUNCEOUT_01",
                    "BOUNCEINOUT_01",

                    "SINESQUARE",         // gapjumper's
                    "EXPONENTIAL",        // gapjumper's
                    "SCHUBRING1",         // terry schubring's formula 1
                    "SCHUBRING2",         // terry schubring's formula 2
                    "SCHUBRING3",         // terry schubring's formula 3

                    "SINPI2_01",          // tomas cepeda's
                    "ACELBREAK_01"
                };
                const int elems = sizeof(list) / sizeof(list[0]);

                static int choice = 0, choosing = 0, scrollY = 0;

                if( imguiList("hello", elems, list, choosing, choice, &scrollY ) ) {
                    imguiTween( choice );
                }
            }

        imguiEndScrollArea();
        imguiEndFrame();

        imguiRenderGLDraw(width, height);

$windows(
        static int prev = ~0, curr;
        curr = imguiGetMouseCursor();
        set_cursor( curr == IMGUI_MOUSE_ICON_HAND ? IDC_HAND : IDC_ARROW );
)

        // Check for errors
        GLenum err = glGetError();
        if(err != GL_NO_ERROR)
        {
            fprintf(stderr, "OpenGL Error : %d %x\n", err, err );
        }

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

        // Check keys
        if( glfwGetKey( window, GLFW_KEY_ESCAPE ) == GLFW_PRESS ) {
            running = false;
        }
    }

    // Clean UI
    imguiRenderGLDestroy();

    // Close OpenGL window and terminate GLFW
    glfwDestroyWindow( window );
    glfwTerminate();

    exit( EXIT_SUCCESS );
}

