
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <GL/gl.h>
#include "../glext.h"
#include "../config.h"
#include "../shader_code.h"

const static PIXELFORMATDESCRIPTOR pfd = {0,0,PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static DEVMODE screenSettings = { {0},
    #if _MSC_VER < 1400
    0,0,148,0,0x001c0000,{0},0,0,0,0,0,0,0,0,0,{0},0,32,XRES,YRES,0,0,      // Visual C++ 6.0
    #else
    0,0,156,0,0x001c0000,{0},0,0,0,0,0,{0},0,32,XRES,YRES,{0}, 0,           // Visuatl Studio 2005
    #endif
    #if(WINVER >= 0x0400)
    0,0,0,0,0,0,
    #if (WINVER >= 0x0500) || (_WIN32_WINNT >= 0x0400)
    0,0
    #endif
    #endif
    };

#ifdef __cplusplus
extern "C" 
{
#endif
int  _fltused = 0;
#ifdef __cplusplus
}
#endif

//----------------------------------------------------------------------------
void entrypoint( void )
{
    // full screen
    //if( ChangeDisplaySettings(&screenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL) return;
    ChangeDisplaySettings(&screenSettings,CDS_FULLSCREEN);
    ShowCursor( 0 );
    // create window
    HWND hWnd = CreateWindow( "edit",0,WS_POPUP|WS_VISIBLE,0,0,XRES,YRES,0,0,0,0);
//    if( !hWnd ) return;
    HDC hDC = GetDC(hWnd);
    // initalize opengl
    SetPixelFormat(hDC,ChoosePixelFormat(hDC,&pfd),&pfd);
    //if( !SetPixelFormat(hDC,ChoosePixelFormat(hDC,&pfd),&pfd) ) return;
    wglMakeCurrent(hDC,wglCreateContext(hDC));

    // create shader
		static const char* shp = fragment_glsl;

		const int p = ((PFNGLCREATESHADERPROGRAMVPROC)wglGetProcAddress("glCreateShaderProgramv"))(GL_FRAGMENT_SHADER, 1, &shp);
    ((PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram"))(p);

		glRects(-1,-1,+1,+1);

	glFinish();

				wglSwapLayerBuffers( hDC, WGL_SWAP_MAIN_PLANE ); //SwapBuffers( hDC );

	do 
    {

		PeekMessageA(0, 0, 0, 0, PM_REMOVE); // <-- "fake" message handling.

    //}while ( !GetAsyncKeyState(VK_ESCAPE) && t<(MZK_DURATION*1000) );
	} while (!GetAsyncKeyState(VK_ESCAPE));


    ExitProcess(0);
}
