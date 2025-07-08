
#include "Engine.hpp"

#include <SDL/SDL.h>
#include <bass.h>

#define FULL         1
#define PLAY_MUSIC   1


extern Scene *forrestScene, *tvScene, *psxScene;

extern FILE* g_logfile = NULL;

extern int initAudio();
extern void uninitAudio();



int main(int argc, char** argv)
{
   int scrw = 0, scrh = 0;
   int scrf = FULL;

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

   SDL_Surface* surf = NULL;

   if(!(surf = SDL_SetVideoMode(scrw, scrh, 0, SDL_HWSURFACE | SDL_OPENGL | (scrf ? SDL_FULLSCREEN : 0))))
   {
      log("SDL_SetVideoMode failed.\n");
      SDL_Quit();
      return -1;
   }

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

   const struct SceneCue
   {
      Scene* scene;
      unsigned long start_time; // in milliseconds

      } scenes[] = { { tvScene, 0 },
                  { forrestScene, 34 * 1000 },
                  { psxScene, 70 * 1000 },
                  { NULL, 100 * 1000 } };
/*
      } scenes[] = { { tvScene, 0 },
                  { forrestScene, 34 * 1000 },
                  { psxScene, 70 * 1000 },
                  { NULL, 100 * 1000 } };
*/

   const SceneCue* sc = scenes;
   Scene* scene = NULL;

/*
   scene = curvesScene;
   scene->window_width = g_window_width;
   scene->window_height = g_window_height;
   scene->initialize();
*/

/*
   if(initAudio())
   {
      log("initAudio failed.\n");
      return -1;
   }
*/

    BASS_Init(-1, 44100, 0, 0, NULL);

    HSTREAM audio_stream = BASS_StreamCreateFile(FALSE, "music/eep2013_v5_multicomp.mp3", 0, 0, BASS_STREAM_PRESCAN);

   SDL_Event event;

   // absorb any events accumulated during startup
   while(SDL_PollEvent(&event))
      ;

   // pre-initialize to prevent freeeezy peeak
   if(sc && sc->scene)
   {
      sc->scene->window_width = surf->w;
      sc->scene->window_height = surf->h;
      sc->scene->initialize();
   }


   bool finished = false;
   unsigned long t_offset = 0;

   const unsigned long debug_t_adjust = 0 * 1000;
   unsigned long next_logic_update = 0;

   //BASS_ChannelSetPosition(audio_stream, 0, BASS_POS_BYTE);
   BASS_ChannelPlay(audio_stream, TRUE);

#if !PLAY_MUSIC
   BASS_ChannelSetAttribute(audio_stream, BASS_ATTRIB_VOL, 0);
#endif

   BASS_ChannelSetPosition(audio_stream, BASS_ChannelSeconds2Bytes(audio_stream, debug_t_adjust * 0.001), BASS_POS_BYTE);

   unsigned long manual_music_time_offset = 50;

/*
   {
      FILE* mmto_file = fopen("boop_time_offset.txt", "r");
      if(mmto_file)
      {
         fscanf(mmto_file, "%d", &manual_music_time_offset);
         fclose(mmto_file);
      }
   }
*/

   while(!finished)
   {
      while(SDL_PollEvent(&event))
      {
         if(finished)
            break;

         switch(event.type)
         {
            case SDL_KEYDOWN:
               if(event.key.keysym.sym == SDLK_ESCAPE)
                  finished = true;
               break;
         }
      }

       const QWORD pos = BASS_ChannelGetPosition(audio_stream, BASS_POS_BYTE);
       const unsigned long t = BASS_ChannelBytes2Seconds(audio_stream, pos) * 1000;

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
            scene->window_width = surf->w;
            scene->window_height = surf->h;
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
         scene->frame_num = (t - t_offset + bbb) / 10;
         scene->render();
      }


      {
         char str[256];
         snprintf(str, sizeof(str), "%d", t);
         //SDL_WM_SetCaption(str, NULL);
         SDL_WM_SetCaption("", NULL);
      }


      SDL_GL_SwapBuffers();
   }

   fclose(g_logfile);

    BASS_Free();
   //uninitAudio();

   SDL_Quit();

   return 0;
}

