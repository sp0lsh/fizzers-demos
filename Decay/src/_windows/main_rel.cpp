
// ITS A CONIFER

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <GL/gl.h>
#include "../glext.h"
#include "../config.h"

#include "text.h"

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

#define volume_size 256
static GLfloat volumedata[volume_size * volume_size * volume_size];
static GLfloat volumedata2b[volume_size * volume_size * volume_size];

static GLfloat volume_min[3] = {-1.689990, 2.104494, 0};
static GLfloat volume_max[3] = {0.561597, -0.299820, 3.255563 };

#define VOLUME_TEXTURE 1
#define TEXT_TEXTURE 2

static const int max_lines = 65536 * 8;
static int num_lines = 0;
GLfloat lines[max_lines*12];
//static const int tw=1202, th=149;
//static const int tw=2404, th=298;
#define tw 1202
#define th 149

void colourForDepth(int depth,GLfloat* res)
{
    //static const GLfloat c0[3] = {0.3, 0.6, 0.0};
	//static const GLfloat c1[3] = {0.5, 1.0, 0.6};
	//static const GLfloat c0[3] = {0.6, 0.6, 0.6};
	//static const GLfloat c1[3] = {1.0, 1.0, 1.0};
    static const GLfloat c0[3] = {0.4, 0.55, 0.2};
	static const GLfloat c1[3] = {0.8, 1.0, 0.7};

    GLfloat t = 1.0f - GLfloat(depth) / GLfloat(3);

	for(int i=0;i<3;++i)
		res[i]=c0[i] * (1.0f - t) + c1[i] * t;
}

void recusrivelyGenerateBranches(const GLfloat* origin, int branches_per_node, GLfloat length, int depth)
{
    if(depth < 1)
        return;

	const GLfloat line_length = length * 0.3f;
	GLfloat trans[16],c0[3],c1[3];
	glGetFloatv(GL_MODELVIEW_MATRIX, trans);

	colourForDepth(depth, c0);
	colourForDepth(depth - 1, c1);

	for(int i = 0; i < 64; ++i)
	{
		GLfloat ct = GLfloat(i) / GLfloat(64);

		GLfloat t = GLfloat(i) / GLfloat(64) * length;
		GLfloat t2 = GLfloat(i + 1) / GLfloat(64) * length;

		{
			GLfloat* line = lines + (num_lines++) * 12;
			for(int k=0;k<3;++k)
			{
				line[k] = origin[k] + trans[8+k] * t;
				line[k+3] = origin[k] + trans[8+k] * t2;
				line[k+6] = c0[k] * (1.0f - ct) + c1[k] * ct;
				line[k+9] = c0[k] * (1.0f - ct) + c1[k] * ct;
			}
		}

		for(int j = 0; j < 3; ++j)
		{
			static int rand_data = 31415926;
			rand_data *= 20077;
			rand_data += 12345;

			int ang = (rand_data >> 16) & 127;

			GLfloat angx, angy;

			__asm {
				fild ang
				fsincos
				fstp angx
				fstp angy
			}
			
			{
				GLfloat* line = lines + (num_lines++) * 12;
				for(int k=0;k<3;++k)
				{
					line[k] = origin[k] + trans[8+k] * t;
					line[k+3] = line[k] + trans[ 0+k] * angx * line_length +
											trans[ 4+k] * angy * line_length +
											trans[ 8+k] * 0.5f * line_length;

					line[k+6] = c0[k] * (1.0f - ct) + c1[k] * ct;
					line[k+9] = c0[k] * (1.0f - ct) + c1[k] * ct;
				}
			}

		}
	}

    for(int i = 0; i < branches_per_node; ++i)
    {
		GLfloat norigin[3];

		for(int k=0;k<3;++k)
			norigin[k]=origin[k]+trans[8+k]*length;

        GLfloat ang = 0.1f + GLfloat(i) - GLfloat(branches_per_node - 1) * 0.5f;

		glLoadIdentity();
		glRotatef(GLfloat(i) * 2.0f * 180.0f / 3.1415926f, trans[8+0], trans[8+1], trans[8+2]);
		glRotatef(ang * 180.0f / 3.1415926f, 1.0f, 0.0f, 0.0f);
		glMultMatrixf(trans);
        recusrivelyGenerateBranches(norigin, branches_per_node, length * 0.75f, depth - 1);
    }
}


//char textpix[tw*th];

void entrypoint( void )
{
    // full screen
    if( ChangeDisplaySettings(&screenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL) return; ShowCursor( 0 );

    // create windows
    HDC hDC = GetDC( CreateWindow("edit", 0, WS_POPUP|WS_VISIBLE|WS_MAXIMIZE, 0,0,0,0,0,0,0,0) );

    // init opengl
    SetPixelFormat(hDC, ChoosePixelFormat(hDC, &pfd), &pfd);
    wglMakeCurrent(hDC, wglCreateContext(hDC));

	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();

	{
	 GLfloat origin[3] = { 0.0f, 0.0f, 0.0f };

    recusrivelyGenerateBranches(origin, 3, 1.0, 6);
	}

    // plotLines

#if 0
    for(int k = 0; k < volume_size; ++k)
        for(int j = 0; j < volume_size; ++j)
            for(int i = 0; i < volume_size; ++i)
            {
                volumedata[i + (j + k * volume_size) * volume_size] = 0.0f;
            }
#else
	__asm {
		; push ecx
		mov ecx, 16777216
clearvolumedata:
		dec ecx
		mov [ecx*4+volumedata], 0
		loop clearvolumedata
		; pop ecx
	}
#endif

	for(int i=0;i<num_lines;++i)
	{
		GLfloat* line = lines + i * 12;

		GLfloat delta[3];
		GLfloat lsq = 0.0f;

		for(int k=0;k<3;++k)
		{
			delta[k]=line[k+3]-line[k];
			lsq += delta[k] * delta[k];
		}

		__asm {
			fld lsq
			fsqrt
			fstp lsq
		}

        int n = int(lsq / 0.00025);

        for(int j = 0; j < n; ++j)
        {
            GLfloat t = GLfloat(j) / GLfloat(n);

			unsigned long int pnt[3];

			for(int k=0;k<3;++k)
				pnt[k]=(unsigned long int)(((line[k]*(1.0f-t)+line[k+3]*t) - volume_min[k]) / (volume_max[k] - volume_min[k]) * GLfloat(volume_size));

			volumedata[pnt[0] + (pnt[1] + pnt[2] * volume_size) * volume_size] += 0.001f;

			/*
			GLfloat pnt[3];

			for(int k=0;k<3;++k)
				pnt[k]=line[k]*(1.0f-t)+line[k+3]*t;

			unsigned long int x = (unsigned long int)((pnt[0] - volume_min[0]) / (volume_max[0] - volume_min[0]) * GLfloat(volume_size));
			unsigned long int y = (unsigned long int)((pnt[1] - volume_min[1]) / (volume_max[1] - volume_min[1]) * GLfloat(volume_size));
			unsigned long int z = (unsigned long int)((pnt[2] - volume_min[2]) / (volume_max[2] - volume_min[2]) * GLfloat(volume_size));

			volumedata[x + (y + z * volume_size) * volume_size] += 0.001f;
			*/

        }
    }

    // blurVolume
	
    for(int k = 0; k < volume_size; ++k)
        for(int i = 0; i < volume_size; ++i)
        {
            GLfloat a = 0.0f;
            for(int j = volume_size - 2; j >= 0; --j)
            {
                volumedata2b[i + (k + j * volume_size) * volume_size] = 0.0f;
				const GLfloat b = volumedata[i + (k + j * volume_size) * volume_size];
				__asm {
					movss xmm0, b
					maxss xmm0, a
					movss a, xmm0
				}
                volumedata[i + (k + j * volume_size) * volume_size] = a;
            }
        }
		

		{
			GLfloat* v0=volumedata;
			GLfloat* v1=volumedata2b;
    for(int n = 0; n < 4; ++n)
    {
        for(int k = 1; k < volume_size - 1; ++k)
            for(int j = 1; j < volume_size - 1; ++j)
                for(int i = 1; i < volume_size - 1; ++i)
                {
                    for(int w = -1; w < 2; ++w)
                        for(int v = -1; v < 2; ++v)
                            for(int u = -1; u < 2; ++u)
                            {
                                v1[i + (j + k * volume_size) * volume_size] += v0[i + u + (j + v + (k + w) * volume_size) * volume_size];
                            }

                    v1[i + (j + k * volume_size) * volume_size] /= 21.0f;
                }
				
		GLfloat* temp=v1;
		v1=v0;
		v0=temp;
	}
		}

#if 0

#if 0
	{
		GLfloat a=0;
	__asm {
		mov ecx, 16777216
postprocvolumedata:
		dec ecx
		movss xmm0, [ecx*4+volumedata]

		maxss xmm0, a
		mulss xmm0, xmm0
		mulss xmm0, xmm0
		movss a, xmm0
		mov eax, a
		mov [ecx*4+volumedata2b], eax
		loop postprocvolumedata
	}
	}
#else
	for(int i=0;i<16777216;++i)
	{
		GLfloat a = 0;
		const GLfloat b = 1 - volumedata[i];
		__asm {
			movss xmm0, b
			maxss xmm0, a
			movss a, xmm0
		}
		volumedata2b[i] = a * a;
		volumedata2b[i] = volumedata2b[i] * volumedata2b[i];
	}
#endif

#else
	for(int k = 0; k < volume_size; ++k)
		for(int j = 0; j < volume_size; ++j)
			for(int i = 0; i < volume_size; ++i)
			{
				GLfloat a = 0;
				const GLfloat b = 1 - volumedata[i + (j + k * volume_size) * volume_size];
				__asm {
					movss xmm0, b
					maxss xmm0, a
					movss a, xmm0
				}
				volumedata2b[i + (j + k * volume_size) * volume_size] = a;
				volumedata2b[i + (j + k * volume_size) * volume_size] = volumedata2b[i + (j + k * volume_size) * volume_size] * volumedata2b[i + (j + k * volume_size) * volume_size];
				volumedata2b[i + (j + k * volume_size) * volume_size] = volumedata2b[i + (j + k * volume_size) * volume_size] * volumedata2b[i + (j + k * volume_size) * volume_size];
			}
#endif

    // uploadVolume
	
    //glGenTextures(1, &volume_texture);
    glBindTexture(GL_TEXTURE_3D, VOLUME_TEXTURE);
    //glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ((PFNGLTEXIMAGE3DPROC)wglGetProcAddress("glTexImage3D"))(GL_TEXTURE_3D, 0, GL_LUMINANCE8, volume_size, volume_size, volume_size, 0, GL_LUMINANCE, GL_FLOAT, volumedata2b);


	{
		GLfloat c=0;
		for(int i=0,j=0;i<sizeof(imagepixeldata)/sizeof(imagepixeldata[0]);++i)
		{
			int r;
			if(i==1206)
			{
				r=82678;
			}
			else
			{
				r=imagepixeldata[i];
				if(r&128)
					r=(r&127)|(imagepixeldata[++i]<<7);
			}

			c=1.0f-c;
			while(r>=0)
			{
				volumedata[j++]=c;
				--r;
			}
		}
	}

	//GLuint text_texture=2;
	//glGenTextures(1, &text_texture);
    glBindTexture(GL_TEXTURE_2D, TEXT_TEXTURE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8, tw, th, 0, GL_LUMINANCE, GL_FLOAT, volumedata);

    glEnable(GL_DEPTH_TEST);

      glViewport(0, 0, XRES, YRES);


	glBindTexture(GL_TEXTURE_3D, VOLUME_TEXTURE);
	//glEnable(GL_TEXTURE_3D);


	  glClear(GL_ACCUM_BUFFER_BIT);

	static const int w=16,h=16;
	for(int y=0;y<h;++y)
		for(int x=0;x<w;++x)
		{
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
			/*
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			*/

			double u=2.5 * (double(x)/double(w)) / XRES;
			double v=2.5 * (double(y)/double(h)) / YRES;
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glFrustum(-1 + u, +1 + u, -GLfloat(YRES)/GLfloat(XRES) * (1 - v), +GLfloat(YRES)/GLfloat(XRES) * (1 + v), 1, 1024);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			GLfloat ww=GLfloat(tw)/GLfloat(1920);
			GLfloat hh=GLfloat(th)/GLfloat(1080)*GLfloat(YRES)/GLfloat(XRES);
			GLfloat ho=-0.4;

			glDepthMask(GL_FALSE);
			glDisable(GL_TEXTURE_3D);
			glEnable(GL_TEXTURE_2D);
			glBegin(GL_TRIANGLE_STRIP);
			glColor3f(1,1,1);
			glTexCoord2s(0,1);
			glVertex3f(-ww,ho-hh,-1);
			glTexCoord2s(1,1);
			glVertex3f(+ww,ho-hh,-1);
			glTexCoord2s(0,0);
			glVertex3f(-ww,ho+hh,-1);
			glTexCoord2s(1,0);
			glVertex3f(+ww,ho+hh,-1);
			glEnd();
			glDisable(GL_TEXTURE_2D);
			glEnable(GL_TEXTURE_3D);
			glDepthMask(GL_TRUE);
			
			

			static const GLfloat xs[3]={-2.2+0.4,0.5+0.4,3.2+0.4};
			static const GLfloat as[3]={60,80,80};
			static const GLfloat cs[9]={  1.0f,1.0f,1.0f,  1.0f,0.9f,0.5f,  0.7f,0.5f,0.3f,  };
			for(int m=0;m<3;++m)
			{
				glLoadIdentity();
				glTranslatef(xs[m], -1.2, -5.0f);
				glRotatef(as[m], 0.0f, 1.0f, 0.0f);
				glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

				glBegin(GL_LINES);
				for(int i=0;i<num_lines;++i)
				{
					if(m==2 && ((i&31)<25))
						continue;

					glColor3f(lines[i * 12 + 6 + 0] * cs[m*3+0], lines[i * 12 + 6 + 1] * cs[m*3+1], lines[i * 12 + 6 + 2] * cs[m*3+2]);
					glTexCoord3f((lines[i * 12 + 0] - volume_min[0])/(volume_max[0]-volume_min[0]), (lines[i * 12 + 1] - volume_min[1])/(volume_max[1]-volume_min[1]), (lines[i * 12 + 2] - volume_min[2])/(volume_max[2]-volume_min[2]));
					glVertex3fv(lines + i * 12);

					glColor3f(lines[i * 12 + 9 + 0] * cs[m*3+0], lines[i * 12 + 9 + 1] * cs[m*3+1], lines[i * 12 + 9 + 2] * cs[m*3+2]);

					glTexCoord3f((lines[i * 12 + 3] - volume_min[0])/(volume_max[0]-volume_min[0]), (lines[i * 12 + 4] - volume_min[1])/(volume_max[1]-volume_min[1]), (lines[i * 12 + 5] - volume_min[2])/(volume_max[2]-volume_min[2]));
					glVertex3fv(lines + i * 12 + 3);
				}
				glEnd();
			}


			glAccum(GL_ACCUM, 1.0/double(w*h));
		}

	glAccum(GL_RETURN,1);

	glFinish();

    //wglSwapLayerBuffers( hDC, WGL_SWAP_MAIN_PLANE );
	SwapBuffers( hDC );

	do 
    {

		PeekMessageA(0, 0, 0, 0, PM_REMOVE); // <-- "fake" message handling.

    //}while ( !GetAsyncKeyState(VK_ESCAPE) && t<(MZK_DURATION*1000) );
	} while (!GetAsyncKeyState(VK_ESCAPE));

    #ifdef CLEANDESTROY
    ChangeDisplaySettings( 0, 0 );
    ShowCursor(1);
    #endif

    ExitProcess(0);
}
