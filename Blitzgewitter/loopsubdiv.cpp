
#include "Engine.hpp"


#include <SDL/SDL.h>
#include <bass.h>
#include <bass_ac3.h>

#define FULL         1
#define PLAY_MUSIC   1
#define FULL_RES     0
#define LOADING_BAR  1

static const int loading_bar_flood_fill_image_width=664;
static const int loading_bar_flood_fill_image_height=664;
extern unsigned short loading_bar_flood_fill_image[loading_bar_flood_fill_image_width*loading_bar_flood_fill_image_height];

extern Scene *platonicScene;
extern Scene *frostScene;
extern Scene *tunnelScene;
extern Scene *bubblesScene;
extern Scene *spaceScene;
extern Scene *unfoldingScene;
extern Scene *introScene;
extern Scene *platonic2Scene;
extern Scene *triangleScene;
extern Scene *preIntroScene;
extern Scene *roomScene;
extern Scene *caveScene;

extern FILE* g_logfile = NULL;

extern int initAudio();
extern void uninitAudio();


extern void generateAreaLightTables();
extern void preprocessLoadingBar();

static const unsigned long int subframe_ms=150;
static GLuint loading_bar_texture=0;
static GLuint loading_bar_bg_texture=0;
static GLuint fill_tex=0;
static Shader loading_bar_shader;

static const float tunnel_scene_time_hack=5;

#if FULL_RES
static int scrw = 0, scrh = 0;
#else
static int scrw = 1280, scrh = 720;
#endif
static int scrf = FULL;

#if LOADING_BAR
void initLoadingBar()
{
   loading_bar_texture=Scene::loadTexture(IMAGES_PATH "lines2.png");
   loading_bar_bg_texture=Scene::loadTexture(IMAGES_PATH "stone.png");

   assert(loading_bar_texture != 0);

   glBindTexture(GL_TEXTURE_2D, loading_bar_bg_texture);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

   glBindTexture(GL_TEXTURE_2D, loading_bar_texture);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

   glGenTextures(1,&fill_tex);
   assert(fill_tex != 0);
   glBindTexture(GL_TEXTURE_2D, fill_tex);

   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, loading_bar_flood_fill_image_width, loading_bar_flood_fill_image_height, 0, GL_RED, GL_UNSIGNED_SHORT, loading_bar_flood_fill_image);
   CHECK_FOR_ERRORS;

   loading_bar_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "loading_bar_f.glsl");
   loading_bar_shader.uniform1i("tex0",0);
   loading_bar_shader.uniform1i("tex1",1);
   loading_bar_shader.uniform1i("tex2",2);
   loading_bar_shader.uniform1f("aspect_ratio",float(scrw)/float(scrh));
   //loading_bar_shader.uniform1f("fade",0);
}


void animateLoadingBar(int i, int n, bool fade_out, bool fade_in)
{
   const unsigned long int stime=SDL_GetTicks();
   const unsigned long int ttime=stime+(fade_out ? 700 : fade_in ? 1000 : subframe_ms);

   SDL_Event event;

   while(SDL_GetTicks()<ttime)
   {
      while(SDL_PollEvent(&event))
      {
         switch(event.type)
         {
            case SDL_KEYDOWN:
               if(event.key.keysym.sym == SDLK_ESCAPE)
                  exit(0);
               break;
         }
      }

      static const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f };
      static const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
      static const GLubyte indices[] = { 3, 0, 2, 1 };

      glBindFramebuffer(GL_DRAW_FRAMEBUFFER,0);
      glDrawBuffer(GL_BACK);
      glClearColor(0,0,0,0);
      glClear(GL_COLOR_BUFFER_BIT);
      glViewport(0,0,scrw,scrh);
      glDisable(GL_DEPTH_TEST);
      glDepthMask(GL_FALSE);
      glDisable(GL_BLEND);
      glActiveTexture(GL_TEXTURE2);
      assert(loading_bar_bg_texture!=0);
      glBindTexture(GL_TEXTURE_2D, loading_bar_bg_texture);
      glActiveTexture(GL_TEXTURE1);
      assert(fill_tex!=0);
      glBindTexture(GL_TEXTURE_2D, fill_tex);
      glActiveTexture(GL_TEXTURE0);
      assert(loading_bar_texture!=0);
      glBindTexture(GL_TEXTURE_2D, loading_bar_texture);
      assert(loading_bar_texture!=0);
      glBindBuffer(GL_ARRAY_BUFFER,0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);

      loading_bar_shader.bind();

      if(fade_out)
      {
         loading_bar_shader.uniform1f("fade",GLfloat(SDL_GetTicks()-stime)/GLfloat(ttime-stime));
         loading_bar_shader.uniform1f("time",1);
      }
      else if(!fade_in)
      {
         loading_bar_shader.uniform1f("fade",0);
         loading_bar_shader.uniform1f("time",GLfloat(i*subframe_ms+SDL_GetTicks()-stime)/GLfloat((n - 3)*subframe_ms));
      }
      else if(fade_in)
      {
         loading_bar_shader.uniform1f("fade",std::pow(1.0f - GLfloat(SDL_GetTicks()-stime)/GLfloat(ttime-stime),2.0f));
         loading_bar_shader.uniform1f("time",0);
      }


      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);

      glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);

      glDisableVertexAttribArray(0);
      glDisableVertexAttribArray(1);

      CHECK_FOR_ERRORS;

      SDL_GL_SwapBuffers();
      SDL_WM_SetCaption("", NULL);
   }

}
#endif

unsigned long int makeMS(int minutes,int seconds,int frames)
{
   return (minutes * 60 + seconds) * 1000 + frames * 41;
}

int main(int argc, char** argv)
{
   //preprocessLoadingBar();
   //return 0;

   if(argc > 1)
      scrw = atoi(argv[1]);

   if(argc > 2)
      scrh = atoi(argv[2]);

   if(argc > 3)
      scrf = atoi(argv[3]);

    g_logfile = fopen("error_log.txt", "w");

   if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO))
   {
      log("SDL_Init failed.\n");
      return -1;
   }

   SDL_WM_SetCaption("", NULL);

   SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, true);
   //SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);

   SDL_Surface* surf = NULL;

   if(!(surf = SDL_SetVideoMode(scrw, scrh, 0, SDL_DOUBLEBUF | SDL_HWSURFACE | SDL_OPENGL | (scrf ? SDL_FULLSCREEN : 0))))
   {
      log("SDL_SetVideoMode failed.\n");
      SDL_Quit();
      return -1;
   }

   scrw=surf->w;
   scrh=surf->h;

   SDL_ShowCursor(SDL_FALSE);

   if(gl3wInit())
   {
      log("OpenGL init failed.\n");
      return -1;
   }

   if(!gl3wIsSupported(3, 0))
   {
      log("OpenGL 3.0 is not supported.\n");
      return -1;
   }

   CHECK_FOR_ERRORS;

   const GLubyte* gl_version = glGetString(GL_VERSION);
   const GLubyte* gl_vendor = glGetString(GL_VENDOR);
   const GLubyte* gl_renderer = glGetString(GL_RENDERER);
   const GLubyte* gl_extensions = glGetString(GL_EXTENSIONS);

   CHECK_FOR_ERRORS;

   log("GL_VERSION = '%s'\n", gl_version);
   log("GL_VENDOR = '%s'\n", gl_vendor);
   log("GL_RENDERER = '%s'\n", gl_renderer);
   log("GL_EXTENSIONS = '%s'\n", gl_extensions);

   static const unsigned long int iofs = (1 * 60 + 12.2 + 6.85) * 1000;
   static const unsigned long int extra0 = 11636;

   const struct SceneCue
   {
      Scene* scene;
      unsigned long start_time; // in milliseconds

//} scenes[] = {       { caveScene, 0 },


} scenes[] = {       { preIntroScene, 0 },
                     { introScene, makeMS(1,19,23) },
                     { caveScene, makeMS(1,31,14) },
                     { tunnelScene, makeMS(1,36,14) },
                     { bubblesScene, makeMS(2,18,2)  },
                     { frostScene, makeMS(2,41,9)  },
                     { platonic2Scene, makeMS(2,53,0)  },
                     { roomScene, makeMS(3,16,7)  },
                     { platonicScene, makeMS(3,54,3)  },
                     { spaceScene, makeMS(4,40,16)  },
                     { triangleScene, makeMS(5,15,14) },
                     { unfoldingScene, makeMS(5,50,11)   },

/*
} scenes[] = {       { preIntroScene, 0 },
                     { introScene, iofs + 0 },
                     { caveScene, iofs + 11.5 * 1000 },
                     { tunnelScene, iofs + 11.5 * 1000 + tunnel_scene_time_hack * 1000  },
                     { bubblesScene, iofs + 58 * 1000  },
                     { frostScene, iofs + (1 * 60 + 21.5) * 1000  },
                     { platonic2Scene, iofs + (1 * 60 + 33) * 1000  },
                     { roomScene, (3 * 60 + 15.5) * 1000  },
                     { platonicScene, extra0 + iofs + (2 * 60 + 22.5) * 1000  },
                     { spaceScene, extra0 + iofs + (3 * 60 + 9) * 1000  },
                     { triangleScene, extra0 + iofs + (3 * 60 + 44) * 1000  },
                     { unfoldingScene, extra0 + iofs + (4 * 60 + 19) * 1000  },
*/

/*
} scenes[] = {       { preIntroScene, 0 },
                     { introScene, iofs + 0 },
                     { caveScene, iofs + 11.5 * 1000 },
                     { tunnelScene, iofs + 11.5 * 1000 + tunnel_scene_time_hack * 1000  },
                     { bubblesScene, iofs + 58 * 1000  },
                     { frostScene, iofs + (1 * 60 + 21.5) * 1000  },
                     { platonic2Scene, iofs + (1 * 60 + 33) * 1000  },
                     { roomScene, (3 * 60 + 15.5) * 1000  },
                     { platonicScene, extra0 + iofs + (2 * 60 + 22.5) * 1000  },
                     { spaceScene, extra0 + iofs + (3 * 60 + 9) * 1000  },
                     { triangleScene, extra0 + iofs + (3 * 60 + 44) * 1000  },
                     { unfoldingScene, extra0 + iofs + (4 * 60 + 19) * 1000  },
                     { unfoldingScene, extra0 + iofs + (4 * 60 + 19) * 1000  },
*/

                  { NULL, extra0 + iofs + (4 * 60 + 53) * 1000 } };

   const SceneCue* sc = scenes;
   Scene* scene = NULL;

#if LOADING_BAR
   initLoadingBar();
#endif

   SDL_Delay(3*1000);

   SDL_Event event;
   bool finished = false;

   for(int i=0;i<70;++i)
   {
      while(SDL_PollEvent(&event))
         ;
      glClearColor(0,0,0,0);
      glClear(GL_COLOR_BUFFER_BIT);
      SDL_GL_SwapBuffers();
   }

#if LOADING_BAR
   animateLoadingBar(0, 0, false, true);
#endif

   const int num_scenes=sizeof(scenes)/sizeof(scenes[0]);
   const int num_load_sections=num_scenes+2;

   for(int i=0;i<num_scenes;++i)
   {
      if(scenes[i].scene)
      {
         scenes[i].scene->window_width = surf->w;
         scenes[i].scene->window_height = surf->h;
         scenes[i].scene->scene_start_time = scenes[i].start_time * 0.001;
         if(scenes[i].scene==tunnelScene)
         {
            scenes[i].scene->scene_start_time -= tunnel_scene_time_hack;
         }
         scenes[i].scene->slowInitialize();
         if(scenes[i].scene==tunnelScene)
         {
            scenes[i].scene->scene_start_time += tunnel_scene_time_hack;
         }
      }
      else
         break;

#if LOADING_BAR
      animateLoadingBar(i,num_load_sections,false,false);
#endif
   }

/*
   if(initAudio())
   {
      log("initAudio failed.\n");
      return -1;
   }
*/

    BASS_Init(-1, 44100, 0, 0, NULL);

    //HSTREAM audio_stream = BASS_StreamCreateFile(FALSE, "music/blitzgewitter_full_07_04_2014.mp3", 0, 0, BASS_STREAM_PRESCAN);
    HSTREAM audio_stream = BASS_AC3_StreamCreateFile(FALSE, "music/sunspire_-_blitzgewitter.ac3", 0, 0, BASS_STREAM_PRESCAN);
   float ac3_correction_seconds=0;//0.52;

/*
   FILE* in=fopen("time_offset.txt","r");
   if(in)
   {
      float x=0;
      if(fscanf(in," %f ",&x)==1)
         ac3_correction_seconds=x;
      fclose(in);
   }
*/

/*
   if(!audio_stream)
   {
      ac3_correction_seconds=0;
      audio_stream = BASS_StreamCreateFile(FALSE, "music/blitzgewitter_full_07_04_2014.mp3", 0, 0, BASS_STREAM_PRESCAN);
   }
*/

   assert(audio_stream != 0);

   // absorb any events accumulated during startup
   while(SDL_PollEvent(&event))
      ;

#if LOADING_BAR
   animateLoadingBar(num_scenes-1,num_load_sections,false,false);
#endif

   // pre-initialize to prevent freeeezy peeak
   if(sc && sc->scene)
   {
      sc->scene->window_width = surf->w;
      sc->scene->window_height = surf->h;
      sc->scene->initialize();
   }


   //for(int n=0;n<4;++n)
     // SDL_GL_SwapBuffers();

   SDL_Delay(1*1000);

#if LOADING_BAR
   animateLoadingBar(0,0,true,false);
#endif

   glDeleteTextures(1,&fill_tex);
   fill_tex=0;
   glDeleteTextures(1,&loading_bar_texture);
   loading_bar_texture=0;

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER,0);
   glClearColor(0,0,0,0);
   glClear(GL_COLOR_BUFFER_BIT);


   unsigned long t_offset = 0;

   const unsigned long debug_t_adjust = 0;//makeMS(5,15,14) - 1000;//(4 * 60 + 0) * 1000;//(2*60+22.5) * 1000;//(1*60+33) * 1000;
   unsigned long next_logic_update = 0;

   //BASS_ChannelSetPosition(audio_stream, 0, BASS_POS_BYTE);
   BASS_ChannelPlay(audio_stream, TRUE);

   BASS_ChannelSetAttribute(audio_stream, BASS_ATTRIB_VOL, 0.8);

#if !PLAY_MUSIC
   BASS_ChannelSetAttribute(audio_stream, BASS_ATTRIB_VOL, 0);
#endif

   BASS_ChannelSetPosition(audio_stream, BASS_ChannelSeconds2Bytes(audio_stream, debug_t_adjust * 0.001), BASS_POS_BYTE);

   unsigned long manual_music_time_offset = 0;


   while(!finished)
   {
      const Uint32 start_time = SDL_GetTicks();

      while(SDL_PollEvent(&event))
      {
         if(finished)
            break;

         switch(event.type)
         {
            case SDL_KEYDOWN:
               if(event.key.keysym.sym == SDLK_ESCAPE)
                  finished = true;
               if(event.key.keysym.sym == SDLK_SPACE && sc)
               {
                  BASS_ChannelSetPosition(audio_stream, BASS_ChannelSeconds2Bytes(audio_stream, sc->start_time * 0.001 + ac3_correction_seconds), BASS_POS_BYTE);
               }
               break;
         }
      }

       const QWORD pos = BASS_ChannelGetPosition(audio_stream, BASS_POS_BYTE);
       const unsigned long t = std::max(0.0, (BASS_ChannelBytes2Seconds(audio_stream, pos) - ac3_correction_seconds)) * 1000;

      if(BASS_ChannelIsActive(audio_stream)!=BASS_ACTIVE_PLAYING	)
         break;

      while(t >= sc->start_time)
      {
         if(!sc->scene)
         {
            finished = true;
            break;
         }

         delete scene;

         scene = sc->scene;

         if(scene)
         {
            if(scene==tunnelScene)
            {
               scene->scene_start_time -= tunnel_scene_time_hack;
            }
            scene->initialize();
         }

         t_offset = t;

         ++sc;
      }

      if(finished)
         break;

      if(scene)
      {
         while(t > next_logic_update)
         {
            scene->update();
            next_logic_update += 10;
         }

         const int bbb = 0 * 1000;

         scene->music_time = Real(t + manual_music_time_offset) * 0.001;
         scene->time = Real(t - t_offset + bbb) * 0.001;

         if(scene==tunnelScene)
         {
            scene->time += tunnel_scene_time_hack;
         }

         scene->frame_num = (t - t_offset + bbb) / 10;
         scene->render();
      }


      SDL_GL_SwapBuffers();

      {
         char str[256];
         //snprintf(str, sizeof(str), "%d", t);
         //SDL_WM_SetCaption(str, NULL);
         SDL_WM_SetCaption("", NULL);

         snprintf(str, sizeof(str), "%d", SDL_GetTicks()-start_time);
         SDL_WM_SetCaption(str, NULL);
      }
   }

   fclose(g_logfile);

    BASS_Free();
   //uninitAudio();

   SDL_Quit();

   return 0;
}

