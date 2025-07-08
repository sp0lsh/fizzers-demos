
// Dinner Is Served

#pragma warning( disable : 4730 )
#pragma warning( disable : 4799 )

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include "../config.h"
#include <GL/gl.h>
#include "../glext.h"
#include "../shader_code.h"

#define WRITEBITMAPS 0
#define CHECK_ERRORS 0
#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

#if CHECK_ERRORS
#define CHECKGL {GLuint err=glGetError();if(err){MessageBoxA(hWnd,"ogl err on line " STRINGIFY(__LINE__),"ogl err",MB_OK);ExitProcess(-1);}}
#else
#define CHECKGL
#endif

#include "../writebitmaps.h"



static const PIXELFORMATDESCRIPTOR pfd = {
    sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER, PFD_TYPE_RGBA,
    32, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 32, 0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 };

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



HWND hWnd;
#if CHECK_ERRORS
void check2(char* progname,GLuint prog2)
{
{
GLint val=0;
			((PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv"))(prog2, GL_LINK_STATUS, &val);

if(!val)
{
        static GLchar log[10240];
        GLsizei length;
        ((PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog"))(prog2, 10239, &length, log);
static char filename[1024];
wsprintf(filename, "bad with %s", progname);
MessageBoxA(hWnd,log,filename,MB_OK);
ExitProcess(-1);
}
}
}
#endif
#define check(x) check2(#x,x)

void entrypoint( void )
{
    // full screen
    //if( ChangeDisplaySettings(&screenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL) return;
    ChangeDisplaySettings(&screenSettings,CDS_FULLSCREEN);
    ShowCursor( 0 );
    // create window
///    hWnd = CreateWindow( "edit",0,WS_POPUP|WS_VISIBLE,0,0,XRES,YRES,0,0,0,0);
	hWnd = CreateWindow((LPCSTR)0x0C018, 0, WS_POPUP|WS_VISIBLE,0,0,XRES,YRES,0,0,0,0);;
//    if( !hWnd ) return;
    HDC hDC = GetDC(hWnd);
    // initalize opengl
    SetPixelFormat(hDC,ChoosePixelFormat(hDC,&pfd),&pfd);
    //if( !SetPixelFormat(hDC,ChoosePixelFormat(hDC,&pfd),&pfd) ) return;
    wglMakeCurrent(hDC,wglCreateContext(hDC));


CHECKGL;

	static const char *const fragment_source = fragment_glsl;


#if CHECK_ERRORS
	const auto oglUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
	auto p1 = ((PFNGLCREATESHADERPROGRAMVPROC)wglGetProcAddress("glCreateShaderProgramv"))(GL_FRAGMENT_SHADER, 1, &fragment_source);
	check(p1);
	oglUseProgram(p1);
#endif

	GLuint prog = ((PFNGLCREATESHADERPROGRAMVPROC)wglGetProcAddress("glCreateShaderProgramv"))(GL_FRAGMENT_SHADER, 1, &fragment_source);

	((PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram"))(prog);

	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES, YRES, 0, GL_RGBA, GL_FLOAT, NULL);

	GLuint fbo = 0;

	((PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers"))(1, &fbo);

	const auto oglBindFramebuffer = ((PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer"));

	oglBindFramebuffer(GL_FRAMEBUFFER, fbo);

	((PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2D"))(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

	oglBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);

	const GLenum bufs[] = { GL_COLOR_ATTACHMENT0 };

	((PFNGLDRAWBUFFERSPROC)wglGetProcAddress("glDrawBuffers"))(1, bufs);

	CHECK_ERRORS;

	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT);

	const int sampleCount = 16;

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	for(int i=0;i<sampleCount;++i)
	{
		glTexCoord3f(float(1920*i),float(1080*i),float(sampleCount));
		const int N = 8;
		for(int j=0;j<N;++j)
		{
			glRectf(-1.0f,-1.0f+float(j)/float(N)*2.0f,1.0f,-1.0f+float(j+1)/float(N)*2.0f);
			glFinish();
			PeekMessageA(0, 0, 0, 0, PM_REMOVE); // <-- "fake" message handling.
			if(GetAsyncKeyState(VK_ESCAPE))
			    ExitProcess(0);
		}
		SwapBuffers( hDC );
	}

	oglBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	oglBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);

	CHECK_ERRORS;

	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glDrawBuffer(GL_BACK);

	CHECK_ERRORS;

	((PFNGLBLITFRAMEBUFFERPROC)wglGetProcAddress("glBlitFramebuffer"))(0, 0, XRES, YRES, 0, 0, XRES, YRES, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	CHECK_ERRORS;

	glFinish();

#if WRITEBITMAPS
static unsigned char framepixels[XRES*YRES*4];
glReadBuffer(GL_BACK);
glPixelStorei(GL_PACK_ALIGNMENT, 1);
glReadPixels(0,0,XRES,YRES,GL_BGRA,GL_UNSIGNED_BYTE,framepixels);
//for(int i=0;i<sizeof(framepixels);++i)framepixels[i]=255+i;
for(int y=0;y<(YRES+1)/2;++y)for(int x=0;x<XRES;++x)for(int c=0;c<4;++c){auto b=framepixels[(x+y*XRES)*4+c];framepixels[(x+y*XRES)*4+c]=framepixels[(x+(YRES-1-y)*XRES)*4+c];framepixels[(x+(YRES-1-y)*XRES)*4+c]=b;}
HBITMAP bitmap=CreateBitmap(XRES,YRES,1,32,framepixels);
PBITMAPINFO bitmapinfo=CreateBitmapInfoStruct(hWnd, bitmap);
char filename[1024];
wsprintf(filename, "frames\\frame%06d.bmp", counter);
CreateBMPFile(hWnd, filename, bitmapinfo, bitmap, hDC);
DeleteObject(bitmap);
#endif

        SwapBuffers( hDC );
//        wglSwapLayerBuffers( hDC, WGL_SWAP_MAIN_PLANE ); //SwapBuffers( hDC );

	do
{
		PeekMessageA(0, 0, 0, 0, PM_REMOVE); // <-- "fake" message handling.

	} while (!GetAsyncKeyState(VK_ESCAPE));

    #ifdef CLEANDESTROY
    sndPlaySound(0,0);
    ChangeDisplaySettings( 0, 0 );
    ShowCursor(1);
    #endif

    ExitProcess(0);
	}
