
#include "Engine.hpp"
#include "Bezier.hpp"
#include "Ran.hpp"
#include "Particles.hpp"
#include "Tower.hpp"

#include <list>

static inline Real frand()
{
   static Ran rnd(643);
   return rnd.doub();
}

static long int lrand()
{
   static Ran rnd(112);
   return rnd.int32();
}

static Vec3 normalize(const Vec3& v)
{
   Real rl=Real(1)/std::sqrt(v.lengthSquared());
   return v*rl;
}

static double g_ltime=0;

static const int g_bubbles_div = 1;
static const int g_bubbles_div2 = 2;

static double switch_view_time = 11.656;
static double zoom_start_time = 20.0;

class BubblesScene: public Scene
{
public:
   static const int num_monster_texs=4;
private:
   GLfloat spiral_vertices[3][1024 * 3];
   GLushort spiral_indices[3][1024 * 3];
   GLushort spiral_num_vertices[3], spiral_num_indices[3];

   Shader bokeh_shader, bokeh2_shader;
   Shader gaussbokeh_shader, composite_shader;
   Shader mb_accum_shader;
   Shader screendisplay_shader;
   Shader bgfg_shader;
   Shader bubbles_post_shader;
   Shader monster_shader;

   bool drawing_view_for_background;
   int num_subframes;
   float subframe_scale;

   float lltime;

   static const int num_fbos = 16, num_texs = 16;

   GLuint fbos[num_fbos], texs[num_texs], renderbuffers[num_fbos];

   GLuint bokeh_temp_fbo;
   GLuint bokeh_temp_tex_1, bokeh_temp_tex_2;

   GLuint bubbles_fbo[2];
   GLuint bubbles_tex[2];

   GLuint noise_tex_2d;

   GLuint monster_texs[num_monster_texs];

   void initializeTextures();
   void initializeShaders();
   void initializeBuffers();

   void drawTower(tower::Tower& twr, const Mat4& modelview);
   void drawView(float ltime);
   void drawMonster(float ltime);
   void createSpiral(int fart);
   void gaussianBlur(Real tint_r, Real tint_g, Real tint_b, Real radx, Real rady, GLuint fbo0, GLuint fbo1, GLuint src_tex, GLuint tex0);

   void genSpiralPoints(int fart);

   public:
      BubblesScene()
      {
         for(int i=0;i<num_monster_texs;++i)
            monster_texs[i]=0;
      }

      ~BubblesScene()
      {
         free();
      }

      void slowInitialize();
      void initialize();
      void render();
      void update();
      void free();
};

Scene* bubblesScene = new BubblesScene();

static const int monster_eye_pixel_x = 770;
static const int monster_eye_pixel_y = 435;
static const float monster_zoomed_offset_u = -0.15f;
static const float monster_zoomed_offset_v = 0.1f;
static const float monster_zoomed_scale = 0.6f;

//static const float phase_times[BubblesScene::num_monster_texs] = { 0.0f, 3.0f, 6.0f, 9.0f };
static const float phase_times[BubblesScene::num_monster_texs] = { 0.0f, 2.918f, 5.861f, 8.738f };


void BubblesScene::slowInitialize()
{
   genSpiralPoints(0);
   genSpiralPoints(1);
   genSpiralPoints(2);

   monster_texs[0] = loadTexture(IMAGES_PATH "p1.png");
   assert(monster_texs[0] != 0);

   monster_texs[1] = loadTexture(IMAGES_PATH "p2.png");
   assert(monster_texs[1] != 0);

   monster_texs[2] = loadTexture(IMAGES_PATH "p3.png");
   assert(monster_texs[2] != 0);

   monster_texs[3] = loadTexture(IMAGES_PATH "p4.png");
   assert(monster_texs[3] != 0);

   for(int i=0;i<num_monster_texs;++i)
   {
      if(monster_texs[i] != 0)
      {
         glBindTexture(GL_TEXTURE_2D, monster_texs[i]);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
         glBindTexture(GL_TEXTURE_2D, 0);
      }
   }

   initializeShaders();
}



void BubblesScene::initializeTextures()
{
   glGenTextures(num_texs, texs);

   createWindowTexture(texs[0], GL_RGBA16F);
   createWindowTexture(texs[2], GL_RGBA16F);
   createWindowTexture(texs[4], GL_RGBA16F, 4);
   createWindowTexture(texs[5], GL_RGBA16F, 4);
   createWindowTexture(texs[6], GL_RGBA16F);
   createWindowTexture(texs[9], GL_RGBA16F, 4);
   createWindowTexture(texs[12], GL_RGBA16F, 2);
   createWindowTexture(texs[13], GL_RGBA16F, 2);
   createWindowTexture(texs[14], GL_R16F, g_bubbles_div);
   createWindowTexture(texs[15], GL_R16F, g_bubbles_div2);

   CHECK_FOR_ERRORS;

   glBindTexture(GL_TEXTURE_2D, 0);

   noise_tex_2d = texs[11];
   bokeh_temp_tex_1 = texs[12];
   bokeh_temp_tex_2 = texs[13];
   bubbles_tex[0] = texs[14];
   bubbles_tex[1] = texs[15];

   {
      uint w = 128, h = 128;
      uint pix[w*h];

      for(uint y = 0; y < h; ++y)
         for(uint x=0;x<w;++x)
         {
            pix[x+y*w]=(lrand()&255) | ((lrand()&255) <<8)|((lrand()&255))<<16;
         }

      glBindTexture(GL_TEXTURE_2D, noise_tex_2d);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pix);

      CHECK_FOR_ERRORS;
   }

}

void BubblesScene::initializeBuffers()
{
   glGenFramebuffers(num_fbos, fbos);
   glGenRenderbuffers(num_fbos, renderbuffers);

   bokeh_temp_fbo = fbos[12];
   bubbles_fbo[0] = fbos[14];
   bubbles_fbo[1] = fbos[15];

   glBindRenderbuffer(GL_RENDERBUFFER, renderbuffers[1]);
   glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, window_width, window_height);
   CHECK_FOR_ERRORS;
   glBindRenderbuffer(GL_RENDERBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texs[0], 0);
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
   glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffers[1]);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[9]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texs[9], 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, bokeh_temp_fbo);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bokeh_temp_tex_1, 0);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, bokeh_temp_tex_2, 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, bubbles_fbo[0]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bubbles_tex[0], 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, bubbles_fbo[1]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bubbles_tex[1], 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   CHECK_FOR_ERRORS;
}

void BubblesScene::initializeShaders()
{
   monster_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "monster_f.glsl");
   monster_shader.uniform1i("tex0",0);

   bokeh_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "bokeh_f.glsl");
   bokeh_shader.uniform1i("tex0", 0);
   bokeh_shader.uniform1f("aspect_ratio", float(window_height) / float(window_width));

   bokeh2_shader.load(SHADERS_PATH "generic_v.glsl", NULL,SHADERS_PATH "bokeh2_f.glsl");
   bokeh2_shader.uniform1i("tex1", 0);
   bokeh2_shader.uniform1i("tex2", 1);
   bokeh2_shader.uniform1f("aspect_ratio", float(window_height) / float(window_width));

   gaussbokeh_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "gaussblur_f.glsl");
   gaussbokeh_shader.uniform1i("tex0", 0);

   mb_accum_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "mb_accum_f.glsl");
   mb_accum_shader.uniform1i("tex0", 0);


   composite_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "cubes_composite_f.glsl");
   composite_shader.uniform1i("tex0", 0);
   composite_shader.uniform1i("tex1", 1);
   composite_shader.uniform1i("bloom_tex", 4);
   composite_shader.uniform1i("bloom2_tex", 5);

   screendisplay_shader.load(SHADERS_PATH "bubbles_v.glsl", SHADERS_PATH "bubbles_g.glsl", SHADERS_PATH "bubbles_f.glsl");
   screendisplay_shader.uniform1i("tex0", 0);

   bgfg_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "bgfg_f.glsl");
   bgfg_shader.uniform1i("tex0", 0);
   bgfg_shader.uniform1i("tex1", 1);

   bubbles_post_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "bubbles_post_f.glsl");
   bubbles_post_shader.uniform1i("tex0", 0);
}



void BubblesScene::initialize()
{
   if(initialized)
      return;

   initialized = true;

   initializeTextures();
   initializeBuffers();
}


void BubblesScene::update()
{
   ++g_ltime;
}

void BubblesScene::genSpiralPoints(int fart)
{
   spiral_num_vertices[fart] = 0;
   spiral_num_indices[fart] = 0;


   Ran rnd(123*fart);

if(fart < 2)
{
   for(int n=0;n<1000;++n)
   {
      spiral_indices[fart][spiral_num_indices[fart]++] = spiral_num_vertices[fart];

      float& x = spiral_vertices[fart][spiral_num_vertices[fart] * 3 + 0];
      float& y = spiral_vertices[fart][spiral_num_vertices[fart] * 3 + 1];
      float& z = spiral_vertices[fart][spiral_num_vertices[fart] * 3 + 2];

      for(int m=0;m<200;++m)
      {
         x = std::pow(rnd.doub()*2.0-1.0, 2.0)*4;
         y = std::pow(rnd.doub()*2.0-1.0, 1.5)*8+1.2;
         z = (rnd.doub()*2.0-1.0)*2;

         if(std::sqrt(x*x+z*z+y*y*0.125)<2.0)
            break;
      }

      ++spiral_num_vertices[fart];
   }
}
else
{
   for(int n=0;n<800;++n)
   {
      spiral_indices[fart][spiral_num_indices[fart]++] = spiral_num_vertices[fart];

      float& x = spiral_vertices[fart][spiral_num_vertices[fart] * 3 + 0];
      float& y = spiral_vertices[fart][spiral_num_vertices[fart] * 3 + 1];
      float& z = spiral_vertices[fart][spiral_num_vertices[fart] * 3 + 2];

      for(int m=0;m<200;++m)
      {
         x = std::pow(rnd.doub()*2.0-1.0, 2.0)*8;
         y = std::pow(rnd.doub()*2.0-1.0, 1.5)*32;
         z = (rnd.doub()*2.0-1.0)*4;

         //if(std::sqrt(x*x+z*z+y*y*0.125)<2.0)
           // break;
      }

      ++spiral_num_vertices[fart];
   }
}

}

void BubblesScene::createSpiral(int fart)
{
   glDrawRangeElements(GL_POINTS, 0, spiral_num_vertices[fart] - 1, spiral_num_indices[fart], GL_UNSIGNED_SHORT, spiral_indices[fart]);
   CHECK_FOR_ERRORS;
}

void BubblesScene::drawTower(tower::Tower& twr, const Mat4& modelview)
{

}

void BubblesScene::drawView(float ltime)
{
   static const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f };
   static const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
   static const GLfloat coords2[] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f };
   static const GLubyte indices[] = { 3, 0, 2, 1 };

   lltime = ltime;


   glUseProgram(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);


   bubbles_post_shader.uniform1f("time", ltime);

   if(!drawing_view_for_background)
   {
      bubbles_post_shader.bind();
      bubbles_post_shader.uniform2f("res", window_width / g_bubbles_div, window_height / g_bubbles_div);
      glDepthMask(GL_FALSE);

      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, bubbles_tex[0]);

      glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);

      glDisableVertexAttribArray(0);
      glDisableVertexAttribArray(1);
   }

   else
   {
      bubbles_post_shader.bind();
      bubbles_post_shader.uniform2f("res", window_width / g_bubbles_div2, window_height / g_bubbles_div2);
      glDepthMask(GL_FALSE);

      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, bubbles_tex[1]);

      glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);

      glDisableVertexAttribArray(0);
      glDisableVertexAttribArray(1);
   }

   glDepthMask(GL_TRUE);

   CHECK_FOR_ERRORS;
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void BubblesScene::drawMonster(float ltime)
{
   static const GLfloat vertices2[] = { -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f };
   static const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
   static const GLubyte indices[] = { 3, 0, 2, 1 };

   static const int phase_texture_sizes[num_monster_texs][2] = { { 320, 320 }, { 720, 720 }, { 1080, 1080 }, { 1600, 1200 } };

   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

   if(!drawing_view_for_background)
   {
      GLuint tex=0;
      GLfloat sx=1,sy=1;

      for(int i=0;i<num_monster_texs;++i)
      {
         if(ltime > phase_times[i])
         {
            tex=monster_texs[i];
            sx=phase_texture_sizes[i][0]/1600.0*2;
            sy=phase_texture_sizes[i][1]/1200.0*2;
         }
      }

      GLfloat vertices[8];

      for(int i=0;i<4;++i)
      {
         vertices[i*2+0]=sx*vertices2[i*2+0];
         vertices[i*2+1]=sy*vertices2[i*2+1];
      }

      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      monster_shader.bind();
      monster_shader.uniform2f("screen_res", 1280, 720);
      monster_shader.uniform1f("time", ltime);
      monster_shader.uniform1f("switch_view_time", switch_view_time);
      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, tex);
      glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
      glDisableVertexAttribArray(0);
      glDisableVertexAttribArray(1);
      CHECK_FOR_ERRORS;
      glDisable(GL_BLEND);
   }
}

void BubblesScene::gaussianBlur(Real tint_r, Real tint_g, Real tint_b, Real radx, Real rady, GLuint fbo0, GLuint fbo1, GLuint src_tex, GLuint tex0)
{
   static const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f };
   static const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
   static const GLubyte indices[] = { 3, 0, 2, 1 };

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);

   glViewport(0, 0, window_width / 4, window_height / 4);

   // gaussian blur pass 1

   {
      gaussbokeh_shader.uniform4f("tint", tint_r, tint_g, tint_b, 1.0f);
   }

   const float gauss_radx = radx;
   const float gauss_rady = rady;

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo0);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }


   CHECK_FOR_ERRORS;

   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);

   gaussbokeh_shader.bind();
   gaussbokeh_shader.uniform2f("direction", gauss_radx * float(window_height) / float(window_width), 0.0f);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, src_tex);

   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);



   // gaussian blur pass 2

   gaussbokeh_shader.uniform4f("tint", 1, 1, 1, 1);


   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo1);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }


   CHECK_FOR_ERRORS;

   gaussbokeh_shader.uniform2f("direction", 0.0f, gauss_rady);

   glBindTexture(GL_TEXTURE_2D, tex0);

   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   CHECK_FOR_ERRORS;
}


void BubblesScene::render()
{
   static const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f };
   static const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
   static const GLubyte indices[] = { 3, 0, 2, 1 };

   const float aspect_ratio = float(window_height) / float(window_width);

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);


   for(int nn = 0; nn < 2; ++nn)
   {
//if(drawing_view_for_background) continue; // **********************************************************************************

      drawing_view_for_background = !nn;

      // ----- motion blur ------

      glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[3]);
      glClear(GL_COLOR_BUFFER_BIT);

      num_subframes = drawing_view_for_background ? 1 : 7;
      subframe_scale = 1.0/35.0;

      for(int subframe = 0;subframe<num_subframes;++subframe)
      {
         const float ltime=time + subframe_scale * float(subframe) / float(num_subframes);

         screendisplay_shader.uniform1f("time", ltime);

         const float aspect_ratio = float(window_height) / float(window_width);

         const float jitter_x = frand() / window_width * 3;
         const float jitter_y = frand() / window_height * aspect_ratio * 3;

         const float fov = M_PI * 0.3f;

         const float znear = 1.0f / tan(fov * 0.5f), zfar = 1024.0f;

         Mat4 projection = Mat4::frustum(-1.0f + jitter_x, +1.0f + jitter_x, -aspect_ratio + jitter_y, +aspect_ratio + jitter_y, znear, zfar);

         screendisplay_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);

         const float move_back = drawing_view_for_background ? -10 : 0;

         Mat4 modelview = Mat4::rotation(cos(ltime * 15.0) * 0.01 / 128, Vec3(0.0f, 1.0f, 0.0f))  * Mat4::translation(Vec3(0, 0, -30 - move_back + ltime * 0.2));

         if(ltime >= switch_view_time)
         {
            if(ltime >= zoom_start_time)
               modelview = modelview * Mat4::translation(Vec3(0, 0, 10 + std::pow(float(ltime - zoom_start_time), 2.0f) ));
            else
               modelview = modelview * Mat4::translation(Vec3(0, 0, 10));
         }
         modelview = modelview * Mat4::rotation(M_PI/2, Vec3(0.0f, 1.0f, 0.0f));

         screendisplay_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);

for(int fart=0;fart<3;++fart)
{

         {


            if(drawing_view_for_background)
            {
               // draw the bubbles
               glBindFramebuffer(GL_DRAW_FRAMEBUFFER, bubbles_fbo[1]);
               {
                  GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
                  glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
               }
               glViewport(0, 0, window_width / g_bubbles_div2, window_height / g_bubbles_div2);
            }
            else
            {
               // draw the bubbles
               glBindFramebuffer(GL_DRAW_FRAMEBUFFER, bubbles_fbo[0]);
               {
                  GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
                  glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
               }
               glViewport(0, 0, window_width / g_bubbles_div, window_height / g_bubbles_div);
            }

            glClear(GL_COLOR_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            glBindTexture(GL_TEXTURE_2D,noise_tex_2d);
            screendisplay_shader.bind();
            if(fart<2)
               screendisplay_shader.uniform2f("radii", 0.7-fart*0.3, 0.7-fart*0.3); // 0.4
            else
               screendisplay_shader.uniform2f("radii", 0.3, 0.3); // 0.4


            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, spiral_vertices[fart]);
            createSpiral(fart);
            CHECK_FOR_ERRORS;
            glDisableVertexAttribArray(0);
         }

         // draw the view

         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[6]);

         glDisable(GL_BLEND);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   glViewport(0, 0, window_width, window_height);

   CHECK_FOR_ERRORS;

   glDepthMask(GL_TRUE);
   glEnable(GL_CULL_FACE);
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LEQUAL);

   if(fart==0)
      glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

   CHECK_FOR_ERRORS;

         if(fart==1)
         {
            drawMonster(ltime);
         }

         drawView(ltime);

}

         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[3]);

         mb_accum_shader.bind();
         mb_accum_shader.uniform1f("time", float(subframe) / float(num_subframes));
         mb_accum_shader.uniform1f("scale", 1.0f / float(num_subframes));

         glDepthMask(GL_FALSE);
         glDisable(GL_DEPTH_TEST);
         glEnable(GL_BLEND);
         glBlendFunc(GL_ONE, GL_ONE);

         glActiveTexture(GL_TEXTURE0);
         glBindTexture(GL_TEXTURE_2D, texs[6]);
         glEnableVertexAttribArray(0);
         glEnableVertexAttribArray(1);
         glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
         glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
         glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
         glDisableVertexAttribArray(0);
         glDisableVertexAttribArray(1);
      }

      CHECK_FOR_ERRORS;

      float fg_CoC=0;
      float fg_brightness=0;
      float nt=0,ntd=1e5;
      for(int k=1;k<num_monster_texs;++k)
      {
         float d=std::abs(phase_times[k]-time);
         if(d<ntd)
         {
            nt=phase_times[k];
            ntd=d;
         }
      }

      fg_CoC=(1.0f-cubic(clamp(ntd*2,0.0f,1.0f)))*0.04f;
      fg_brightness=mix(0.0f,0.3f,std::sqrt(clamp(ntd,0.0f,1.0f)));

      if(drawing_view_for_background || fg_CoC > 1e-3)
      {
         // ----- aperture ------

         const float CoC = drawing_view_for_background ? 0.02 : fg_CoC;// + sin(time) * 0.05;
         const float brightness = drawing_view_for_background ? 0.2f : fg_brightness;

         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, bokeh_temp_fbo);

         {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
            glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
         }

         glViewport(0, 0, window_width / 2, window_height / 2);

         bokeh_shader.bind();
         bokeh_shader.uniform2f("CoC", CoC / aspect_ratio, CoC);

         glDepthMask(GL_FALSE);
         glDisable(GL_DEPTH_TEST);
         glDisable(GL_BLEND);

         glActiveTexture(GL_TEXTURE0);
         glBindTexture(GL_TEXTURE_2D, texs[2]);
         glEnableVertexAttribArray(0);
         glEnableVertexAttribArray(1);
         glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
         glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
         glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
         glDisableVertexAttribArray(0);
         glDisableVertexAttribArray(1);

         CHECK_FOR_ERRORS;


         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);

         glViewport(0, 0, window_width, window_height);

         {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
         }

         glEnable(GL_BLEND);
         glBlendFunc(GL_ONE,GL_ONE);
         if(drawing_view_for_background)
            glClear(GL_COLOR_BUFFER_BIT);

         bokeh2_shader.bind();
         bokeh2_shader.uniform2f("CoC", CoC / aspect_ratio, CoC);
         bokeh2_shader.uniform1f("brightness", brightness);

         glActiveTexture(GL_TEXTURE1);
         glBindTexture(GL_TEXTURE_2D, bokeh_temp_tex_2);
         glActiveTexture(GL_TEXTURE0);
         glBindTexture(GL_TEXTURE_2D, bokeh_temp_tex_1);
         glEnableVertexAttribArray(0);
         glEnableVertexAttribArray(1);
         glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
         glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
         glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
         glDisableVertexAttribArray(0);
         glDisableVertexAttribArray(1);
      }
      else
      {
         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);
         glViewport(0, 0, window_width, window_height);
         glDisable(GL_BLEND);

         bgfg_shader.bind();

         glActiveTexture(GL_TEXTURE1);
         glBindTexture(GL_TEXTURE_2D, texs[2]);
         glActiveTexture(GL_TEXTURE0);
         glBindTexture(GL_TEXTURE_2D, texs[0]);
         glEnableVertexAttribArray(0);
         glEnableVertexAttribArray(1);
         glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
         glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
         glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
         glDisableVertexAttribArray(0);
         glDisableVertexAttribArray(1);
      }

      CHECK_FOR_ERRORS;
   }

   // ----- bloom ------

   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_BLEND);

   downsample8x(texs[0], fbos[3], texs[2], fbos[4], window_width, window_height);

   gaussianBlur(1, 1, 1, 0.5, 0.5, fbos[5], fbos[9], texs[4], texs[5]);
   gaussianBlur(1, 1, 1, 0.1, 0.1, fbos[5], fbos[4], texs[4], texs[5]);

   CHECK_FOR_ERRORS;


   // ----- composite ------


   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
   glViewport(0, 0, window_width, window_height);


   composite_shader.bind();
   composite_shader.uniform1f("time", time);
   composite_shader.uniform1i("frame_num", frame_num);
   {
      float s=std::pow(clamp(time-23.0f,0.0f,1.0f),2.0f);
      composite_shader.uniform3f("addcolour",s,s,s);
   }
   composite_shader.uniform4f("tint", 1, 1, 1, 1);

   glActiveTexture(GL_TEXTURE5);
   glBindTexture(GL_TEXTURE_2D, texs[9]);
   glActiveTexture(GL_TEXTURE4);
   glBindTexture(GL_TEXTURE_2D, texs[4]);
   glActiveTexture(GL_TEXTURE3);
   glBindTexture(GL_TEXTURE_2D, texs[13]);
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

   CHECK_FOR_ERRORS;
}


void BubblesScene::free()
{
   glDeleteTextures(num_monster_texs,monster_texs);
   glDeleteTextures(num_texs, texs);
   glDeleteFramebuffers(num_fbos, fbos);
}

