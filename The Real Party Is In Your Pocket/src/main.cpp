// custom build and feature flags
#ifdef DEBUG
	#define OPENGL_DEBUG        1
	#define FULLSCREEN          0
	#define DESPERATE           0
	#define BREAK_COMPATIBILITY 0
#else
	#define OPENGL_DEBUG        0
	#define FULLSCREEN          1
	#define DESPERATE           0
	#define BREAK_COMPATIBILITY 0
#endif
 
   
#define POST_PASS    0
#define USE_MIPMAPS  1
#define USE_AUDIO    1
#define NO_UNIFORMS  1
#define WRITEWAV	 0

//#define DO_PATH 1
 
#include "definitions.h"
#if OPENGL_DEBUG
	#include "debug.h"
#endif

#include <windows.h>
#include <GL/gl.h>

#ifdef EDITOR_CONTROLS
#include <cstdio>
#define CHECKGL {GLuint err=glGetError();if(err!=GL_NO_ERROR){printf("ogl error=0x%x\n",err); MessageBoxA(window,"ogl err on line " STRINGIFY(__LINE__) " in file " __FILE__,"ogl err",MB_OK);ExitProcess(-1);}}
//#define CHECKGL
#else
#define CHECKGL
#endif

#ifdef EDITOR_CONTROLS
#define NVPATHFUNC(type, name) 	const type name = (type)wglGetProcAddress(#name); if(!name) { MessageBoxA(window, "didnt get " #name, "error", MB_OK); ExitProcess(-1); }
#else
#define NVPATHFUNC(type, name) 	const type name = (type)wglGetProcAddress(#name);
#endif

/*
static const char *strs[] = {
	"glUseProgram",
	"glCreateShaderProgramv",
};

#define NUMFUNCIONES (sizeof(strs)/sizeof(strs[0]))

#define oglUseProgram					((PFNGLUSEPROGRAMPROC)myglfunc[0])
#define oglCreateShaderProgramv	      ((PFNGLCREATESHADERPROGRAMVPROC)myglfunc[1])
*/

      
#include "glext.h"
#include "shaders/fragment.inl"
#if POST_PASS
	#include "shaders/post.inl"
#endif
       
//#ifdef EDITOR_CONTROLS
// static allocation saves a few bytes
#pragma data_seg(".pids")
static GLuint pidMain;
#if POST_PASS
static GLuint pidPost;
#endif
//#endif
// static HDC hDC;
           


#ifndef EDITOR_CONTROLS
#pragma code_seg(".main")
void entrypoint(void)
#else
#include "editor.h"
int __cdecl main(int argc, char* argv[])
#endif
{
	const DWORD starttime = GetTickCount();

	// initialize window
	#if FULLSCREEN
		ChangeDisplaySettings(&screenSettings, CDS_FULLSCREEN);
		ShowCursor(0);
		const HDC hDC = GetDC(CreateWindow((LPCSTR)0xC018, 0, WS_POPUP | WS_VISIBLE | WS_MAXIMIZE, 0, 0, 0, 0, 0, 0, 0, 0));
	#else
		#ifdef EDITOR_CONTROLS
			HWND window = CreateWindow("static", 0, WS_POPUP | WS_VISIBLE, 0, 0, XRES, YRES, 0, 0, 0, 0);
			HDC hDC = GetDC(window);
		#else
			// you can create a pseudo fullscreen window by similarly enabling the WS_MAXIMIZE flag as above
			// in which case you can replace the resolution parameters with 0s and save a couple bytes
			// this only works if the resolution is set to the display device's native resolution
			HDC hDC = GetDC(CreateWindow((LPCSTR)0xC018, 0, WS_POPUP | WS_VISIBLE, 0, 0, XRES, YRES, 0, 0, 0, 0));
		#endif
	#endif  

	// initalize opengl context
	SetPixelFormat(hDC, ChoosePixelFormat(hDC, &pfd), &pfd);
	wglMakeCurrent(hDC, wglCreateContext(hDC));
	

CHECKGL;

	((PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram"))(((PFNGLCREATESHADERPROGRAMVPROC)wglGetProcAddress("glCreateShaderProgramv"))(GL_FRAGMENT_SHADER, 1, &fragment));

CHECKGL;

#ifdef EDITOR_CONTROLS
	GLuint pidPost=0;
pidMain=0;
	Leviathan::Editor editor = Leviathan::Editor();
	editor.updateShaders(window, &pidMain, &pidPost, true);
CHECKGL;
((PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram"))(pidMain);
CHECKGL;
#endif

#ifdef EDITOR_CONTROLS
		editor.beginFrame(0);
	#endif
			


	glClear(GL_ACCUM_BUFFER_BIT);

	int sample_count = 0;

	do
	{
		PeekMessageA(0, 0, 0, 0, PM_REMOVE); // <-- "fake" message handling.

		if(sample_count < 256 && ((GetTickCount() - starttime) < 29000UL))
		//if(sample_count < 256)
		{
			// render a pass

			glTexCoord1s(sample_count++);
			glRects(-1, -1, 1, 1);

			glAccum(GL_ACCUM, 0.00390625f);

			glFinish(); // this is needed
#ifdef EDITOR_CONTROLS
			printf("sample %d done.\n",sample_count);
#endif
		}
		else
		{
			glAccum(GL_RETURN, 256.0f / float(sample_count));
			SwapBuffers(hDC);
		}

	} while (!GetAsyncKeyState(VK_ESCAPE));

	ExitProcess(0);

}
