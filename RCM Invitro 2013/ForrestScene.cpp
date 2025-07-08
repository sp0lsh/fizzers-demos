
#include "Engine.hpp"
#include "Bezier.hpp"


class ForrestScene: public Scene
{
   Shader forrest_shader, bokeh_shader, bokeh2_shader;
   Shader gaussbokeh_shader, composite_shader, ao_shader;
   Shader game_shader;

   static const int num_sprite_meshes = 5, max_sprite_cols = 8;

   Mesh sprite_meshes[num_sprite_meshes][max_sprite_cols];
   Vec3 sprite_cols[num_sprite_meshes][max_sprite_cols];

   Spline<Vec3> tint_spline;
   Spline<Real> cam_dist_spline, cam_rotY_spline, cam_rotX_spline, cam_focus_spline;
   Spline<Real> cam_posY_spline;

   static const int num_fbos = 16, num_texs = 16;

   static const int stuff_appear_offset = -3;

   GLuint fbos[num_fbos], texs[num_texs], renderbuffers[num_fbos];


   GLuint noise_tex, depth_tex, game_tex;
   GLuint noise_tex_2d;

   GLuint game_font;

   void initializeTextures();
   void initializeShaders();
   void initializeBuffers();

   void drawGame();
   void drawView();

   static const int game_width = 256, game_height = 256;
   GLfloat game_coords[8192 * 2];
   GLfloat game_vertices[8192 * 2];
   GLushort game_indices[8192], num_game_vertices, num_game_indices;


   void addGameQuad(Vec2 p0, Vec2 p1, int c = 0);
   void addGameText(Vec2 org, const std::string& text);
   Vec2 gameQuantise(const Vec2& v) const;



   struct Pong
   {
      Vec2 ballpos, ballvel;
      Real bat0x, bat1x;
      Real bat0y, bat1y;
      Real ballsize;
      Vec2 batsize;

      Pong()
      {
         ballpos = Vec2(0, 0);
         bat0x = -0.84;
         bat1x = +0.84;
         bat0y = 0;
         bat1y = 0;
         batsize = Vec2(0.06, 0.3) * 0.5;
         ballsize = 0.03;
         ballvel = Vec2(0.16, 0.09) * 0.1;
      }

   } pong;




   struct Snake
   {
      static const int num_snakes = 12, grid_w = 32, grid_h = 32, snake_length = 10;

      int snakes[num_snakes][4]; // x, y, dir, free
      int update_count;
      int grid[grid_w * grid_h][2]; // snake index, counter

      template<class T>
      static int sign(T t)
      {
         return t < 0 ? -1 : t > 0 ? 1 : 0;
      }

      Snake()
      {
         for(int y=0;y<grid_h;++y)
            for(int x=0;x<grid_w;++x)
            {
               grid[x+y*grid_w][0]=-1;
               grid[x+y*grid_w][1]=0;
            }

         for(int i=0;i<num_snakes;++i)
         {
            snakes[i][0]=rand() % grid_w;
            snakes[i][1]=rand() % grid_h;
            snakes[i][2]=rand() % 4;
         }

         update_count = 0;
      }

      bool cellFree(int x, int y)
      {
         if(x < 0 || y < 0 || x >= grid_w || y >= grid_h)
            return false;

         return grid[x+y*grid_w][1] == 0;
      }

      void update()
      {
         ++update_count;

         if(update_count & 15)
            return;

         for(int i=0;i<num_snakes;++i)
         {
            int nx=snakes[i][0];
            int ny=snakes[i][1];

            if(((update_count / 16 + i) & 7) == 0)
            {
               snakes[i][2] = rand() % 4;
            }

            switch(snakes[i][2])
            {
               case 0: nx += -1; break;
               case 1: nx += +1; break;
               case 2: ny += -1; break;
               case 3: ny += +1; break;
            }
            if(cellFree(nx, ny))
            {
               snakes[i][0]=nx;
               snakes[i][1]=ny;
               snakes[i][3] = 1;
               grid[nx+ny*grid_w][0]=i;
               grid[nx+ny*grid_w][1]=snake_length;
            }
            else
            {
               snakes[i][3] = 0;
               const int xf = (rand() & 1) ? +1 : -1;
               const int yf = (rand() & 1) ? +1 : -1;
               for(int y=-1;y<2 && !snakes[i][3];++y)
                  for(int x=-1;x<2;++x)
                  {
                     const int x2 = x * xf, y2 = y * yf;
                     if(labs(x) != labs(y) && cellFree(snakes[i][0]+x2, snakes[i][1]+y2))
                     {
                        snakes[i][2] = (sign(x2+y2)+1)/2 + labs(y2)*2;
                        snakes[i][3] = 1;
                        snakes[i][0] += x2;
                        snakes[i][1] += y2;
                        grid[snakes[i][0]+snakes[i][1]*grid_w][0]=i;
                        grid[snakes[i][0]+snakes[i][1]*grid_w][1]=snake_length;
                        break;
                     }
                  }
            }
         }

         for(int y=0;y<grid_h;++y)
            for(int x=0;x<grid_w;++x)
            {
               int* g = grid[x+y*grid_w];

               if(g[1] > 0 && g[0] != -1 && snakes[g[0]][3])
               {
                  g[1] -= 1;

                  if(g[1] == 0)
                     g[0] = -1;
               }
            }
      }

   } snake;




   public:
      ForrestScene()
      {
         num_game_vertices = 0;
         num_game_indices = 0;
      }

      void initialize();
      void render();
      void update();
      void free();
};

Scene* forrestScene = new ForrestScene();



void ForrestScene::initializeTextures()
{
   glGenTextures(num_texs, texs);

   game_font = loadTexture("images/font.png");

   createWindowTexture(texs[0], GL_RGBA16F);
   createWindowTexture(texs[1], GL_RGBA16F);
   createWindowTexture(texs[2], GL_RGBA16F);
   createWindowTexture(texs[4], GL_RGBA16F, 4);
   createWindowTexture(texs[5], GL_RGBA16F, 4);
   createWindowTexture(texs[6], GL_RGBA16F);

   CHECK_FOR_ERRORS;

   glBindTexture(GL_TEXTURE_2D, 0);

   noise_tex = texs[7];
   depth_tex = texs[8];
   game_tex = texs[9];
   noise_tex_2d = texs[11];

   {
      uint s = 16;
      GLubyte* pix = new GLubyte[s * s * s * 3];
      for(uint i = 0; i < s * s * s * 3; ++i)
         pix[i] = rand();

      glBindTexture(GL_TEXTURE_3D, noise_tex);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

      glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB8, s, s, s, 0, GL_RGB, GL_UNSIGNED_BYTE, pix);

      delete[] pix;
      CHECK_FOR_ERRORS;
      glBindTexture(GL_TEXTURE_3D, 0);
   }

   {
      uint w = 128, h = 128;
      uint pix[w*h];

      for(uint y = 0; y < h; ++y)
         for(uint x=0;x<w;++x)
         {
            pix[x+y*w]=(rand()&255) | ((rand()&255) <<8)|((rand()&255))<<16;
         }

      glBindTexture(GL_TEXTURE_2D, noise_tex_2d);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pix);
      CHECK_FOR_ERRORS;
   }

   glBindTexture(GL_TEXTURE_2D, depth_tex);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexStorage2D(GL_TEXTURE_2D, 1, GL_R16F, window_width, window_height);
   CHECK_FOR_ERRORS;
   glBindTexture(GL_TEXTURE_2D, 0);

   glBindTexture(GL_TEXTURE_2D, game_tex);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB16F, game_width, game_height);
   CHECK_FOR_ERRORS;
   glBindTexture(GL_TEXTURE_2D, 0);

   glBindTexture(GL_TEXTURE_2D, texs[12]);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB16F, game_width / 2, game_height / 2);
   CHECK_FOR_ERRORS;
   glBindTexture(GL_TEXTURE_2D, 0);

   glBindTexture(GL_TEXTURE_2D, texs[13]);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB16F, game_width / 2, game_height / 2);
   CHECK_FOR_ERRORS;
   glBindTexture(GL_TEXTURE_2D, 0);
}

void ForrestScene::initializeBuffers()
{
   glGenFramebuffers(num_fbos, fbos);
   glGenRenderbuffers(num_fbos, renderbuffers);

   glBindRenderbuffer(GL_RENDERBUFFER, renderbuffers[1]);
   glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, window_width, window_height);
   CHECK_FOR_ERRORS;
   glBindRenderbuffer(GL_RENDERBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texs[0], 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[1]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texs[1], 0);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texs[2], 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[3]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texs[2], 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[4]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texs[4], 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[5]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texs[5], 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[6]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texs[6], 0);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, depth_tex, 0);
   glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffers[1]);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[9]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texs[9], 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[12]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texs[12], 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[13]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texs[13], 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   CHECK_FOR_ERRORS;
}

void ForrestScene::initializeShaders()
{
   forrest_shader.load(SHADERS_PATH "forrest_v.glsl", NULL, SHADERS_PATH "forrest_f.glsl");
   forrest_shader.uniform1i("noiseTex", 0);
   forrest_shader.uniform1i("colour_tex", 1);

   game_shader.load(SHADERS_PATH "game_v.glsl", NULL, SHADERS_PATH "game_f.glsl");


   bokeh_shader.load(SHADERS_PATH "bokeh_v.glsl", NULL, SHADERS_PATH "bokeh_f.glsl");
   bokeh_shader.uniform1i("tex0", 0);
   bokeh_shader.uniform1i("tex1", 1);
   bokeh_shader.uniform1i("tex2", 2);
   bokeh_shader.uniform1f("aspect_ratio", float(window_height) / float(window_width));

   bokeh2_shader.load(SHADERS_PATH "bokeh2_v.glsl", NULL,SHADERS_PATH "bokeh2_f.glsl");
   bokeh2_shader.uniform1i("tex0", 0);
   bokeh2_shader.uniform1i("tex1", 1);
   bokeh2_shader.uniform1i("tex2", 2);
   bokeh2_shader.uniform1f("aspect_ratio", float(window_height) / float(window_width));

   gaussbokeh_shader.load(SHADERS_PATH "gaussblur_v.glsl", NULL, SHADERS_PATH "gaussblur_f.glsl");
   gaussbokeh_shader.uniform1i("tex0", 0);

   composite_shader.load(SHADERS_PATH "cubes_composite_v.glsl", NULL, SHADERS_PATH "cubes_composite_f.glsl");
   composite_shader.uniform1i("tex0", 0);
   composite_shader.uniform1i("tex1", 1);
   composite_shader.uniform1i("game_tex", 2);
   composite_shader.uniform1i("game_blur_tex", 3);
   composite_shader.uniform1i("bloom_tex", 4);
   composite_shader.uniform1f("aspect_ratio", float(window_width) / float(window_height));

   ao_shader.load(SHADERS_PATH "ao_v.glsl", NULL, SHADERS_PATH "ao_f.glsl");
   ao_shader.uniform1i("depth_tex", 0);
   ao_shader.uniform1i("noise_tex", 1);
}

static inline Real frand()
{
   return Real(rand()) / Real(RAND_MAX);
}

void ForrestScene::initialize()
{
   if(initialized)
      return;

   initialized = true;

   initializeShaders();
   initializeTextures();
   initializeBuffers();



   tint_spline.insert(Vec3(0, 0, 0), Vec3(0, 0, 0), 0);
   tint_spline.insert(Vec3(1, 1, 1), Vec3(0, 0.5, 1), 400*2);
   tint_spline.initialize();


   cam_dist_spline.insert(4.0f, 0.0f, 0);
   cam_dist_spline.insert(2.0, 0.0f, 800);
   cam_dist_spline.insert(8.0f, 0.0f, 1600);

   cam_dist_spline.insert(8.0f, -1.0f, 1700);
   cam_dist_spline.insert(7, 0.0f, 3200);
   cam_dist_spline.initialize();


   cam_rotY_spline.insert(0, 0, 0);
   cam_rotY_spline.initialize();

   cam_rotX_spline.insert(0, 0, 0);
   cam_rotX_spline.initialize();

   cam_posY_spline.insert(0, 0, 0);
   cam_posY_spline.initialize();

   cam_focus_spline.insert(0, 0, 0);
   cam_focus_spline.initialize();




   {
      const int w=16,h=16;
      const int sprites[num_sprite_meshes][w][h] = {


         { // space invader 1
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0 },
         { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 },
         { 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0 },
         { 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         },

         { // space invader 2
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 },
         { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0 },
         { 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0 },
         { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0 },
         { 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0 },
         { 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         },

         { // space invader 3
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         },

         { // digdug
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
         { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 },
         { 1, 1, 1, 2, 2, 2, 2, 3, 2, 3, 2, 0, 0, 0, 0, 0 },
         { 1, 1, 1, 2, 2, 2, 2, 3, 2, 3, 2, 0, 0, 0, 0, 0 },
         { 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 2, 2, 1, 1, 1, 1, 1, 0, 0, 4, 0, 0, 0, 0 },
         { 0, 1, 2, 2, 1, 1, 1, 1, 1, 1, 0, 4, 4, 0, 0, 0 },
         { 4, 4, 4, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 0, 0 },
         { 0, 0, 1, 1, 2, 2, 2, 1, 1, 1, 0, 4, 4, 0, 0, 0 },
         { 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 4, 0, 0, 0, 0 },
         { 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         },

         { // pooka
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0 },
         { 0, 0, 0, 0, 0, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 0 },
         { 0, 0, 0, 0, 4, 4, 2, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
         { 0, 0, 0, 0, 2, 2, 2, 1, 1, 1, 3, 1, 1, 3, 1, 2 },
         { 0, 0, 0, 0, 2, 2, 2, 1, 1, 1, 3, 1, 1, 3, 1, 2 },
         { 0, 0, 0, 0, 4, 4, 2, 2, 1, 1, 1, 1, 2, 1, 1, 2 },
         { 0, 0, 0, 1, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
         { 0, 0, 0, 1, 0, 4, 4, 4, 2, 2, 2, 4, 4, 2, 2, 4 },
         { 0, 0, 0, 1, 1, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0 },
         { 0, 0, 0, 0, 0, 0, 4, 2, 4, 4, 4, 4, 2, 4, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0 },
         { 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 0, 2, 2, 2, 2, 0 },
         },
      };

      sprite_cols[0][0] = Vec3(1.0, 0.5, 0.5);

      sprite_cols[1][0] = Vec3(0.5, 1.0, 0.5);

      sprite_cols[2][0] = Vec3(0.5, 0.5, 1.0);

      sprite_cols[3][0] = Vec3(1.0, 1.0, 1.0);
      sprite_cols[3][1] = Vec3(0.4, 0.4, 1.0);
      sprite_cols[3][2] = Vec3(0.0, 0.0, 0.0);
      sprite_cols[3][3] = Vec3(1.0, 0.0, 0.0);

      sprite_cols[4][0] = Vec3(1.0, 1.0, 1.0);
      sprite_cols[4][1] = Vec3(1.0, 1.0, 0.4);
      sprite_cols[4][2] = Vec3(0.0, 0.0, 0.0);
      sprite_cols[4][3] = Vec3(1.0, 0.0, 0.0);

      for(int i =0;i<num_sprite_meshes;++i)
      {
         for(int j=0;j<max_sprite_cols;++j)
         {
            Mesh cube(60, 24);
            Mesh& mesh = sprite_meshes[i][j];

            cube.generateSimpleCube();
            cube.transform(Mat4::scale(Vec3(0.5, 0.5, 0.5)));
            cube.separate();

            for(int y=0;y<h;++y)
            {
               for(int x=0;x<w;++x)
               {
                  if(sprites[i][y][x]==(j+1))
                  {
                     Vec3 pos(x-w/2, -(y-h/2), 0);
                     mesh.addInstance(cube, Mat4::translation(pos));
                  }
               }
            }

            //cubes_mesh.subdivide();
            mesh.generateNormals();
         }
      }
   }

}

Vec2 ForrestScene::gameQuantise(const Vec2& v) const
{
   return Vec2(floor((v.x + 1.0) * 0.5 * game_width) / game_width,
               floor((v.y + 1.0) * 0.5 * game_height) / game_height ) * 2.0 - Vec2(1.0, 1.0);

//   return Vec2(floor((v.x + 1.0) * 0.5 * game_width * 0.5) / game_width * 2, floor(v.y * game_height * 0.5) / game_height * 2);
}

void ForrestScene::addGameQuad(Vec2 p0, Vec2 p1, int c)
{
   p0 = gameQuantise(p0);
   p1 = gameQuantise(p1);

   GLushort i0 = num_game_vertices, i1 = i0 + 1, i2 = i0 + 2, i3 = i0 + 3;

   const int num_chars = 38;

   c = c % num_chars;

   const Vec2 c0 = Vec2(Real(c) / num_chars + 0.5 / (num_chars * 8), 0), c1 = Vec2(Real(c + 1) / num_chars, 1);

   game_vertices[i0 * 2 + 0] = p0.x;
   game_vertices[i0 * 2 + 1] = p0.y;

   game_vertices[i1 * 2 + 0] = p1.x;
   game_vertices[i1 * 2 + 1] = p0.y;

   game_vertices[i2 * 2 + 0] = p0.x;
   game_vertices[i2 * 2 + 1] = p1.y;

   game_vertices[i3 * 2 + 0] = p1.x;
   game_vertices[i3 * 2 + 1] = p1.y;

   game_coords[i0 * 2 + 0] = c0.x;
   game_coords[i0 * 2 + 1] = c0.y;

   game_coords[i1 * 2 + 0] = c1.x;
   game_coords[i1 * 2 + 1] = c0.y;

   game_coords[i2 * 2 + 0] = c0.x;
   game_coords[i2 * 2 + 1] = c1.y;

   game_coords[i3 * 2 + 0] = c1.x;
   game_coords[i3 * 2 + 1] = c1.y;

   game_indices[num_game_indices + 0] = i0;
   game_indices[num_game_indices + 1] = i1;
   game_indices[num_game_indices + 2] = i2;

   game_indices[num_game_indices + 3] = i1;
   game_indices[num_game_indices + 4] = i3;
   game_indices[num_game_indices + 5] = i2;

   num_game_vertices += 4;
   num_game_indices += 6;
}



void ForrestScene::update()
{
   // snake

   if(time > (12 + stuff_appear_offset))
      snake.update();

   // pong

   pong.ballpos += pong.ballvel;


   if(time > (12 + stuff_appear_offset))
   {
      pong.bat0y += 0.02;
      pong.bat1y -= 0.02;
   }
   else
   {
      pong.bat0y = mix(cos(time) * 0.4, pong.ballpos.y, std::min(1.0f, std::max(0.0f, -pong.ballpos.x * 1.2f)));
      pong.bat1y = mix(sin(time * 0.8) * 0.4, pong.ballpos.y, std::min(1.0f, std::max(0.0f, +pong.ballpos.x * 1.2f)));

      if(std::abs(pong.ballpos.x) > pong.bat1x - pong.batsize.x - pong.ballsize)
      {
         pong.ballvel.x *= -1;
         pong.ballvel.y += (frand() - 0.5) * 0.01;
      }
   }

   if(std::abs(pong.ballpos.y) > 0.7 - pong.ballsize)
      pong.ballvel.y *= -1;
}

void ForrestScene::addGameText(Vec2 org, const std::string& text)
{
   const Vec2 sz(2.0 / (game_width / 8.0), 2.0 / (game_height / 8.0));

   org *= Vec2(2.0 / game_width, 2.0 / game_height);

   for(std::string::const_iterator it = text.begin(); it != text.end(); ++it)
   {
      if(!isspace(*it))
      {
         if(*it == '.')
            addGameQuad(org, org + sz, 37);
         if(isalpha(*it))
            addGameQuad(org, org + sz, 1 + tolower(*it) - 'a');
         else if(isdigit(*it))
            addGameQuad(org, org + sz, 1 + 26 + *it - '0');
      }
      org.x += sz.x;
   }
}

void ForrestScene::drawGame()
{
   const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f,
                                +1.0f, +1.0f, -1.0f, +1.0f };


   const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f,
                              1.0f, 1.0f, 0.0f, 1.0f };

   const GLubyte indices[] = { 3, 0, 2, 1 };



   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[9]);

   Mat4 modelview = Mat4::identity(), projection = Mat4::identity();

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   glViewport(0, 0, game_width, game_height);

   CHECK_FOR_ERRORS;

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
   glDepthMask(GL_TRUE);
   glDisable(GL_CULL_FACE);
   glDisable(GL_DEPTH_TEST);

   CHECK_FOR_ERRORS;

   glClear(GL_COLOR_BUFFER_BIT);

   num_game_vertices = 0;
   num_game_indices = 0;


   CHECK_FOR_ERRORS;

   // SNAKE
   for(int y=0;y<Snake::grid_h;++y)
      for(int x=0;x<Snake::grid_w;++x)
      {
         const double s = 0.1;

         double x0 = s;
         double y0 = s;
         double x1 = 1.0 - s;
         double y1 = 1.0 - s;

         const int snake_index = snake.grid[x+y*Snake::grid_w][0];
         const int c = snake.grid[x+y*Snake::grid_w][1];

         for(int v=-1;v<2;++v)
            for(int u=-1;u<2;++u)
            {
               int x2=x+u, y2=y+v;

               if(labs(u) == labs(v) ||  x2 < 0 || y2 < 0 || x2 >= Snake::grid_w || y2 >= Snake::grid_h)
                  continue;

               if(snake.grid[x2+y2*Snake::grid_w][0] == snake_index && labs(snake.grid[x2+y2*Snake::grid_w][1] - c) == 1)
               {
                  x0 += std::min(u, 0) * s;
                  y0 += std::min(v, 0) * s;
                  x1 += std::max(u, 0) * s;
                  y1 += std::max(v, 0) * s;
               }
            }

         if(c > 0)
         {
            addGameQuad(Vec2(-1.0 + 2.0 * (double(x)+x0)/double(Snake::grid_w), -1.0 + 2.0 * (double(y)+0+s)/double(Snake::grid_h)) * 0.9,
                        Vec2(-1.0 + 2.0 * (double(x)+x1)/double(Snake::grid_w), -1.0 + 2.0 * (double(y)+1-s)/double(Snake::grid_h)) * 0.9);

            addGameQuad(Vec2(-1.0 + 2.0 * (double(x)+0+s)/double(Snake::grid_w), -1.0 + 2.0 * (double(y)+y0)/double(Snake::grid_h)) * 0.9,
                        Vec2(-1.0 + 2.0 * (double(x)+1-s)/double(Snake::grid_w), -1.0 + 2.0 * (double(y)+y1)/double(Snake::grid_h)) * 0.9);
         }
      }


   // PONG
   addGameQuad(pong.ballpos - Vec2(pong.ballsize), pong.ballpos + Vec2(pong.ballsize));
   addGameQuad(Vec2(pong.bat0x, pong.bat0y) - pong.batsize, Vec2(pong.bat0x, pong.bat0y) + pong.batsize);
   addGameQuad(Vec2(pong.bat1x, pong.bat1y) - pong.batsize, Vec2(pong.bat1x, pong.bat1y) + pong.batsize);


   CHECK_FOR_ERRORS;


   // TEXT

   static const std::string texts0[] = {
   "the retro computer museum",
   "are hosting a half term",
   "weekend special.",
   "",
   "so come and visit us at",
   "",
   "snibston and the century theatre",
   "ashby road",
   "coalville",
   "leicestershire",
   "le67 3ln",
   "",
   "on the 26th and 27th",
   "of october.",
   "",
   "",
   "",
   };

//Snibston and the Century Theatre, Ashby Road, Coalville, Leicestershire LE67 3LN

   static const std::string texts1[] = {
   "",
   "",
   "",
   "",
   "",
   "for more information go to",
   "www.retrocomputermuseum.co.uk"
   "",
   "",
   "",
   "",
   "",
   "",
   };

   static const int num_texts0 = sizeof(texts0) / sizeof(texts0[0]);
   static const int num_texts1 = sizeof(texts1) / sizeof(texts1[0]);

   const int textness0 = (time - 13 - stuff_appear_offset) * 16;
   const int textness1 = (time - 27 - stuff_appear_offset) * 16;

   if(textness1 > 0)
   {
      int c = 0;
      float y = 26+8*3;
      for(int i = 0; i < num_texts1; ++i)
      {
         std::string str = "";

         for(std::string::const_iterator it = texts1[i].begin(); it != texts1[i].end(); ++it)
         {
            if(c >= textness1)
               break;

            str += *it;

            if(!isspace(*it))
               ++c;
         }

         addGameText(Vec2(-(112+16), y), str);
         y -= 8;

         if(c >= textness1)
            break;
      }
   }
   else if(textness0 > 0)
   {
      int c = 0;
      float y = 26+8*3;
      for(int i = 0; i < num_texts0; ++i)
      {
         std::string str = "";

         for(std::string::const_iterator it = texts0[i].begin(); it != texts0[i].end(); ++it)
         {
            if(c >= textness0)
               break;

            str += *it;

            if(!isspace(*it))
               ++c;
         }

         addGameText(Vec2(-(112+16), y), str);
         y -= 8;

         if(c >= textness0)
            break;
      }
   }

   CHECK_FOR_ERRORS;


   if(num_game_indices > 0)
   {
      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);

      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, game_vertices);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, game_coords);

      game_shader.bind();
      game_shader.uniform1f("time", time);
      game_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
      game_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);
      game_shader.uniform3f("colour", 1, 1, 1);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, game_font);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

      glDrawRangeElements(GL_TRIANGLES, 0, num_game_vertices, num_game_indices, GL_UNSIGNED_SHORT, game_indices);

      glDisableVertexAttribArray(0);
      glDisableVertexAttribArray(1);
   }


   glUseProgram(0);

   CHECK_FOR_ERRORS;
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);




   // gaussian blur pass 1 for GAME

   {
      gaussbokeh_shader.uniform4f("tint", 1.0f, 1.0f, 1.0f, 1.0f);
   }

   const float gauss_rad = 0.04f;

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[12]);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   glViewport(0, 0, game_width / 2, game_height / 2);

   CHECK_FOR_ERRORS;

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);

   gaussbokeh_shader.bind();
   gaussbokeh_shader.uniform2f("direction", gauss_rad, 0.0f);

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, game_tex);

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);



   // gaussian blur pass 2 for GAME


   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[13]);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   glViewport(0, 0, game_width / 2, game_height / 2);


   CHECK_FOR_ERRORS;

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);

   gaussbokeh_shader.bind();
   gaussbokeh_shader.uniform2f("direction", 0.0f, gauss_rad);

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, texs[12]);

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   CHECK_FOR_ERRORS;
}

void ForrestScene::drawView()
{
   const float aspect_ratio = float(window_height) / float(window_width);

   Mat4 modelview = Mat4::identity(), projection = Mat4::identity();

   const float fov = M_PI * 0.5f;

   const float znear = 1.0f / tan(fov * 0.5f), zfar = 200.0f;

   projection = Mat4::frustum(-1.0f, +1.0f, -aspect_ratio, +aspect_ratio, znear, zfar) * projection;

   const float A = 1.0f / 10.0f, focallength = znear, focalplane = 25.0f + cam_focus_spline.evaluate(frame_num);

   const float CoCScale = (A * focallength * focalplane * (zfar - znear)) / ((focalplane - focallength) * znear * zfar);
   const float CoCBias = (A * focallength * (znear - focalplane)) / ((focalplane * focallength) * znear);

   forrest_shader.uniform2f("CoCScaleAndBias", CoCScale, CoCBias);
   forrest_shader.uniform1f("time", time);

   modelview = modelview * Mat4::rotation(cam_rotX_spline.evaluate(frame_num), Vec3(1.0f, 0.0f, 0.0f));
   modelview = modelview * Mat4::translation(Vec3(0.0f, -1.0f + cam_posY_spline.evaluate(frame_num), -3.0f - cam_dist_spline.evaluate(frame_num)));
   modelview = modelview * Mat4::rotation(cam_rotY_spline.evaluate(frame_num), Vec3(0.0f, 1.0f, 0.0f));

   //modelview = modelview * Mat4::translation(Vec3(0.0f, -1.0f, -6.0f + sin(time)));
   //modelview = modelview * Mat4::rotation(time * 0.3f, Vec3(0.0f, 1.0f, 0.0f));

   CHECK_FOR_ERRORS;

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   glViewport(0, 0, window_width, window_height);

   CHECK_FOR_ERRORS;

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
   glDepthMask(GL_TRUE);
   glEnable(GL_CULL_FACE);
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LEQUAL);

   glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

   CHECK_FOR_ERRORS;

   Mesh cube_mesh(8, 12);

   cube_mesh.generateSimpleCube();
   cube_mesh.reverseWindings();

   cube_mesh.bind();
   forrest_shader.bind();

   forrest_shader.uniform1f("material", 1.0f);
   forrest_shader.uniform1f("bump_scale", 1.0f);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_3D, noise_tex);
   //glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

   forrest_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);

   {
      const int s=80;
      Mat4 modelview2 = modelview * Mat4::scale(Vec3(s, s, s));
      forrest_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview2.e);
   }

   glDrawElements(GL_TRIANGLES, cube_mesh.getTriangleCount() * 3, GL_UNSIGNED_INT, 0);

   glDisableVertexAttribArray(0);

   glUseProgram(0);
   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

   CHECK_FOR_ERRORS;



   forrest_shader.bind();
   forrest_shader.uniform1f("material", 0.0f);

   if(time > 1)
   for(int i=0;i<800;++i)
   {
      for(int j=0;j<max_sprite_cols;++j)
      {
         const int k = i%num_sprite_meshes;

         Mesh& mesh = sprite_meshes[k][j];

         if(mesh.getTriangleCount() == 0)
            continue;

         const Vec3& col = sprite_cols[k][j];

         mesh.bind();

         Vec3 pos(sin(i*7)*5+sin(time*0.1+i)*0.06, sin(i*26)*2+cos(time*0.1+i)*0.3, (-sin(i*88))*1.2 + 2 - cubic(std::min(1.0f, time / 9)) * 2 );

         pos = pos * 10;

         const double fall = std::pow(std::max(0.0, time - 33.0), 4.0), rfall = 1.0 + fall * 0.02;

         pos.y -= fall;

         Mat4 rot = Mat4::rotation(cos((i+time*0.3*rfall)*0.4), Vec3(1, 0, 0)) *
                    Mat4::rotation(sin((i+time*0.2*rfall)*0.5), Vec3(0, 1, 0));

         Mat4 modelview2 = Mat4::translation(Vec3(0, 0, -2.6 * 1.2 * 10)) * Mat4::translation(pos) * rot * Mat4::scale(Vec3(0.6,0.6,0.6));

         glEnableVertexAttribArray(0);
         glEnableVertexAttribArray(2);

         glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
         glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(mesh.getVertexCount() * sizeof(GLfloat) * 3));

         forrest_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview2.e);
         //forrest_shader.uniform3f("colour", (i%3)*0.33f, ((i+1)%3)*0.33f, ((i+2)%3)*0.33f);
         forrest_shader.uniform3f("colour", col.x, col.y, col.z);

         glDrawRangeElements(GL_TRIANGLES, 0, mesh.getVertexCount() - 1, mesh.getTriangleCount() * 3, GL_UNSIGNED_INT, 0);

         glDisableVertexAttribArray(0);
         glDisableVertexAttribArray(2);

         CHECK_FOR_ERRORS;
      }
   }


   glUseProgram(0);

   CHECK_FOR_ERRORS;
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


void ForrestScene::render()
{

   const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f,
                                +1.0f, +1.0f, -1.0f, +1.0f };


   const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f,
                              1.0f, 1.0f, 0.0f, 1.0f };

   const GLubyte indices[] = { 3, 0, 2, 1 };



   glDisable(GL_BLEND);

   // draw the game

   drawGame();


   // draw the view

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[6]);

   drawView();

   glBindFramebuffer(GL_READ_FRAMEBUFFER, fbos[6]);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);

   glViewport(0, 0, window_width, window_height);

   glBlitFramebuffer(0, 0, window_width, window_height,
                     0, 0, window_width, window_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

   glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);



   // ambient occlusion

   ao_shader.bind();
   ao_shader.uniform4f("ao_settings", 0.007f * float(window_width) / float(window_height),
                                      0.007f, 0.9f, 0.0f);

   glEnable(GL_BLEND);
   glBlendFunc(GL_ZERO, GL_SRC_COLOR);

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE); // preserve the DoF radii
   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, noise_tex_2d);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, depth_tex);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   glDisable(GL_BLEND);
   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);







   // first pass

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[1]);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   glViewport(0, 0, window_width, window_height);

   CHECK_FOR_ERRORS;

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);

   bokeh_shader.bind();

   CHECK_FOR_ERRORS;

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, texs[0]);

   CHECK_FOR_ERRORS;

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   CHECK_FOR_ERRORS;

   // second pass

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   glViewport(0, 0, window_width, window_height);

   CHECK_FOR_ERRORS;

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);

   bokeh2_shader.bind();

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, texs[2]);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, texs[1]);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, texs[0]);


   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   CHECK_FOR_ERRORS;


   downsample8x(texs[0], fbos[3], texs[2], fbos[4], window_width, window_height);


   // gaussian blur pass 1

   {
      gaussbokeh_shader.uniform4f("tint", 1.0f, 1.0f, 1.0f, 1.0f);
   }

   const float gauss_rad = 0.35f;

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[5]);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   glViewport(0, 0, window_width / 4, window_height / 4);

   CHECK_FOR_ERRORS;

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);

   gaussbokeh_shader.bind();
   gaussbokeh_shader.uniform2f("direction", gauss_rad * float(window_height) / float(window_width), 0.0f);

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, texs[4]);

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);



   // gaussian blur pass 2

   {
      Vec3 tint = tint_spline.evaluate(frame_num);
      gaussbokeh_shader.uniform4f("tint", tint.x, tint.y, tint.z, 1.0f);
   }


   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[4]);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   glViewport(0, 0, window_width / 4, window_height / 4);


   CHECK_FOR_ERRORS;

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);

   gaussbokeh_shader.bind();
   gaussbokeh_shader.uniform2f("direction", 0.0f, gauss_rad);

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, texs[5]);

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   CHECK_FOR_ERRORS;




/*
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
   glBindFramebuffer(GL_READ_FRAMEBUFFER, fbos[4]);
   glDrawBuffer(GL_BACK);
   glViewport(0, 0, window_width, window_height);
   glBlitFramebuffer(0, 0, window_width / 4, window_height / 4,
                     0, 0, window_width, window_height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
   glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
*/

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
   glViewport(0, 0, window_width, window_height);
   glClear(GL_COLOR_BUFFER_BIT);


   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);

   composite_shader.bind();
   composite_shader.uniform1f("darken", 0.02f);
   composite_shader.uniform2f("pong_ball_pos", pong.ballpos.x, pong.ballpos.y);
   composite_shader.uniform1f("time", time);

   {
      Vec3 tint = tint_spline.evaluate(frame_num);
      composite_shader.uniform4f("tint", tint.x, tint.y, tint.z, 1.0f);
   }

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);


   glActiveTexture(GL_TEXTURE4);
   glBindTexture(GL_TEXTURE_2D, texs[4]);
   glActiveTexture(GL_TEXTURE3);
   glBindTexture(GL_TEXTURE_2D, texs[13]);
   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, game_tex);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, noise_tex_2d);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, texs[0]);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   glDisable(GL_BLEND);
}


void ForrestScene::free()
{
   glDeleteTextures(num_texs, texs);
   glDeleteFramebuffers(num_fbos, fbos);
   glDeleteRenderbuffers(num_fbos, renderbuffers);
}

