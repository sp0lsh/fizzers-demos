

#include "Engine.hpp"
#include "Bezier.hpp"
#include "Ran.hpp"
#include "Particles.hpp"
#include "Tower.hpp"

#include <list>

#define SHADERS_SUBPATH SHADERS_PATH "intro/"

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

class IntroScene: public Scene
{
   Shader mb_accum_shader;

   int num_subframes;
   float subframe_scale;

   float lltime;
   float* particle_pix;

   static const int num_fbos = 16, num_texs = 16;

   GLuint fbos[num_fbos], texs[num_texs], renderbuffers[num_fbos];

   GLuint noise_tex_2d;

   Shader shader;
   Shader particle_init_shader;
   Shader postproc_shader;
   Shader downsample_shader;
   Shader blur_shader;
   Shader particle_shader;
   Shader composite_shader;

   GLuint   g_quad_vbo;
   GLuint   g_particle_data_fbo;
   GLuint   g_particle_output_fbo;
   GLuint   g_dummy_particle_vbo;

   GLuint   g_blur_sampler;

   static const int num_particle_data_textures = 4;
   GLuint   g_particle_data_texs[num_particle_data_textures];
   GLuint   g_particle_output0_tex;
   GLuint   g_particle_output1_tex;
   GLuint   g_particle_output2_tex;
   GLuint   words_tex;

   static const int g_particle_data_tex_size = 2048 / 2; // 2048, 4 floats = 67 megabytes
   static const int g_particle_count = g_particle_data_tex_size * g_particle_data_tex_size;

   void initializeTextures();
   void initializeShaders();
   void initializeBuffers();

   void updateParticles(int tex, int frame_count);

   void drawView(float ltime,float ftime,int tex);

   public:
      IntroScene(): particle_pix(0)
      {
         g_quad_vbo = 0;
         g_particle_data_fbo = 0;
         g_particle_output_fbo = 0;

         g_blur_sampler = 0;

         for(int i=0;i<num_particle_data_textures;++i)
            g_particle_data_texs[i] = 0;

         g_particle_output0_tex = 0;
         g_particle_output1_tex = 0;
         g_particle_output2_tex = 0;
      }

      ~IntroScene()
      {
         free();
      }

      void slowInitialize();
      void initialize();
      void render();
      void renderParticles(float time);
      void update();
      void free();
};

Scene* introScene = new IntroScene();


void IntroScene::slowInitialize()
{
   particle_pix = new float[g_particle_data_tex_size * g_particle_data_tex_size * 4];

   for(int y = 0; y < g_particle_data_tex_size; ++y)
      for(int x = 0; x < g_particle_data_tex_size; ++x)
      {
         particle_pix[(x + y * g_particle_data_tex_size) * 4 + 3] = frand();
      }

   for(int y = 0; y < g_particle_data_tex_size; ++y)
      for(int x = 0; x < g_particle_data_tex_size; ++x)
      {
         float u, v, w, l;

         do
         {
            u = (frand() - 0.5f) * 2.0f;
            v = (frand() - 0.5f) * 2.0f;
            w = (frand() - 0.5f) * 2.0f;

            l = u * u + v * v + w * w;

         } while(l > 1.0f);

         particle_pix[(x + y * g_particle_data_tex_size) * 4 + 0] = u * 2.0f;
         particle_pix[(x + y * g_particle_data_tex_size) * 4 + 1] = v * 2.0f;// * 0.01f;
         particle_pix[(x + y * g_particle_data_tex_size) * 4 + 2] = w * 2.0f;
      }

   initializeShaders();
}

void IntroScene::initializeTextures()
{
   glGenTextures(num_texs, texs);

   words_tex = loadTexture(IMAGES_PATH "title.png");

   createWindowTexture(texs[0], GL_RGBA16F);
   createWindowTexture(texs[2], GL_RGBA16F);
   createWindowTexture(texs[4], GL_RGBA16F, 4);
   createWindowTexture(texs[5], GL_RGBA16F, 4);
   createWindowTexture(texs[6], GL_RGBA16F);
   createWindowTexture(texs[9], GL_RGBA16F, 4);
   createWindowTexture(texs[12], GL_RGBA16F, 2);
   createWindowTexture(texs[13], GL_RGBA16F, 2);

   CHECK_FOR_ERRORS;

   CHECK_FOR_ERRORS;

   glBindTexture(GL_TEXTURE_2D, 0);

   noise_tex_2d = texs[11];

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



   glGenSamplers(1, &g_blur_sampler);
   glSamplerParameteri(g_blur_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   CHECK_FOR_ERRORS;

   glGenTextures(num_particle_data_textures, g_particle_data_texs);
   glGenTextures(1, &g_particle_output0_tex);
   glGenTextures(1, &g_particle_output1_tex);
   glGenTextures(1, &g_particle_output2_tex);
   CHECK_FOR_ERRORS;


   for(int i = 0; i < num_particle_data_textures; ++i)
   {
      glBindTexture(GL_TEXTURE_2D, g_particle_data_texs[i]);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, g_particle_data_tex_size, g_particle_data_tex_size);
      CHECK_FOR_ERRORS;
   }



   for(int i = 0; i < num_particle_data_textures; ++i)
   {
      glBindTexture(GL_TEXTURE_2D, g_particle_data_texs[i]);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_particle_data_tex_size,
                                    g_particle_data_tex_size, GL_RGBA, GL_FLOAT, particle_pix);
      CHECK_FOR_ERRORS;
   }



   glBindTexture(GL_TEXTURE_2D, g_particle_output0_tex);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB16F, window_width, window_height);
   CHECK_FOR_ERRORS;


   glBindTexture(GL_TEXTURE_2D, g_particle_output1_tex);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB16F, window_width, window_height);
   CHECK_FOR_ERRORS;


   glBindTexture(GL_TEXTURE_2D, g_particle_output2_tex);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB16F, window_width, window_height);
   CHECK_FOR_ERRORS;

   glBindTexture(GL_TEXTURE_2D, 0);
}


void IntroScene::initializeBuffers()
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

   CHECK_FOR_ERRORS;

   glGenFramebuffers(1, &g_particle_data_fbo);
   glGenFramebuffers(1, &g_particle_output_fbo);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_particle_output_fbo);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
   CHECK_FOR_ERRORS;

   glGenBuffers(1, &g_quad_vbo);
   glBindBuffer(GL_ARRAY_BUFFER, g_quad_vbo);
   const GLfloat verts[16] = { +1.0f,  0.0f, +1.0f, +1.0f,  0.0f,  0.0f,  0.0f, +1.0f,
                               +1.0f, +1.0f,  0.0f,  1.0f, +1.0f,  0.0f,  0.0f,  0.0f };
   glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   CHECK_FOR_ERRORS;
}


void IntroScene::initializeShaders()
{
   // shader for blurring
   blur_shader.load(SHADERS_SUBPATH "blur_vertex_shader.glsl", NULL, SHADERS_SUBPATH "blur_fragment_shader.glsl");
   blur_shader.uniform1i("tex", 0);

   // shader for downsampling
   downsample_shader.load(SHADERS_SUBPATH "downsample_vertex_shader.glsl", NULL, SHADERS_SUBPATH "downsample_fragment_shader.glsl");
   downsample_shader.uniform1i("tex", 0);

   // shader for initialising particle data texture
   particle_init_shader.load(SHADERS_SUBPATH "particle_init_vertex_shader.glsl", NULL, SHADERS_SUBPATH "particle_init_fragment_shader.glsl");
   particle_init_shader.uniform1i("particle_data_texture", 0);

   // shader for updating particle data texture
   shader.load(SHADERS_SUBPATH "vertex_shader.glsl", NULL, SHADERS_SUBPATH "fragment_shader.glsl");
   shader.uniform1i("particle_data_texture", 0);

   // shader for drawing particles
   particle_shader.load(SHADERS_SUBPATH "particle_vertex_shader.glsl", SHADERS_SUBPATH "particle_geometry_shader.glsl", SHADERS_SUBPATH "particle_fragment_shader.glsl");
   particle_shader.uniform1i("particle_data_texture_size",g_particle_data_tex_size);
   particle_shader.uniform1i("particle_data_textures[0]", 0);
   particle_shader.uniform1i("particle_data_textures[1]", 1);
   particle_shader.uniform1i("particle_data_textures[2]", 2);
   particle_shader.uniform1i("particle_data_textures[3]", 3);

   // shader for displaying everything
   postproc_shader.load(SHADERS_SUBPATH "postproc_vertex_shader.glsl", NULL, SHADERS_SUBPATH "postproc_fragment_shader.glsl");
   postproc_shader.uniform1i("particle_output_texture", 0);
   postproc_shader.uniform1i("blur_texture", 2);

   composite_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "cubes_composite2_f.glsl");
   composite_shader.uniform1i("tex0", 0);
   composite_shader.uniform1i("tex1", 1);
   composite_shader.uniform1i("tex2", 2);

   mb_accum_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "mb_accum_f.glsl");
   mb_accum_shader.uniform1i("tex0", 0);

}



void IntroScene::initialize()
{
   if(initialized)
      return;

   initialized = true;

   initializeTextures();
   initializeBuffers();

   {
      glGenBuffers(1,&g_dummy_particle_vbo);
      glBindBuffer(GL_ARRAY_BUFFER,g_dummy_particle_vbo);
      GLfloat* data=new GLfloat[g_particle_count*2];
      for(GLuint i=0;i<g_particle_count;++i)
      {
         data[i*2+0]=GLfloat(i % g_particle_data_tex_size)/GLfloat(g_particle_data_tex_size);
         data[i*2+1]=GLfloat(i / g_particle_data_tex_size)/GLfloat(g_particle_data_tex_size);
      }
      glBufferData(GL_ARRAY_BUFFER, g_particle_count * sizeof(GLfloat) * 2, data, GL_STATIC_DRAW);
      CHECK_FOR_ERRORS;
      glBindBuffer(GL_ARRAY_BUFFER,0);
      delete[] data;
   }

   // init particle data texture
   glDisable(GL_STENCIL_TEST);
   glDisable(GL_SCISSOR_TEST);
   glDisable(GL_CULL_FACE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);
   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
   glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
   for(int n=0;n<2;++n)
   {
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_particle_data_fbo);
      glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_particle_data_texs[n], 0);

      glViewport(0, 0, g_particle_data_tex_size, g_particle_data_tex_size);

      {
         GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
         glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
      }

      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glDisable(GL_BLEND);
      particle_init_shader.bind();

      glBindBuffer(GL_ARRAY_BUFFER, g_quad_vbo);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
      glEnableVertexAttribArray(0);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      glDisableVertexAttribArray(0);
   }
   CHECK_FOR_ERRORS;
}


void IntroScene::updateParticles(int tex, int frame_count)
{
   // update particle data texture

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_particle_data_fbo);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_particle_data_texs[tex], 0);

   glViewport(0, 0, g_particle_data_tex_size, g_particle_data_tex_size);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   glBindTexture(GL_TEXTURE_2D, g_particle_data_texs[tex^1]);

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
   glDisable(GL_BLEND);
   shader.bind();
   shader.uniform1f("time", float(frame_count) * 0.04f);

   glBindBuffer(GL_ARRAY_BUFFER, g_quad_vbo);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
   glEnableVertexAttribArray(0);
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
   glDisableVertexAttribArray(0);
   CHECK_FOR_ERRORS;
}

void IntroScene::free()
{
   glDeleteBuffers(1,&g_dummy_particle_vbo);
   delete[] particle_pix;
   particle_pix = 0;
   glDeleteTextures(num_texs, texs);
   glDeleteFramebuffers(num_fbos, fbos);
}

void IntroScene::update()
{
}

void IntroScene::drawView(float ltime,float ftime,int tex)
{
   glDisable(GL_STENCIL_TEST);
   glDisable(GL_SCISSOR_TEST);
   glDisable(GL_CULL_FACE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);
   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
   glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

   static int start_time = 0;
   const float aspect_ratio = float(window_height) / float(window_width);

   Mat4 modelview = Mat4::identity(), projection = Mat4::identity();

   const float jitter_x = frand() / window_width;
   const float jitter_y = frand() / window_height * aspect_ratio;

   const float znear = 0.125f;
   const float zoom=1.75;
   projection = Mat4::frustum(-znear * zoom * (1+jitter_x), +znear * zoom * (1+jitter_y), -aspect_ratio * znear * zoom, +aspect_ratio * znear * zoom, znear, 512.0f) * projection;


   if(ltime>6)
   {
      modelview = modelview * Mat4::rotation(1.5, Vec3(1.0f, 0.0f, 0.0f));
      modelview = modelview * Mat4::translation(Vec3(2.0f, -2.2f, -2.5f+ltime*0.04 + 2.0));
   }
   else
   {
      modelview = modelview * Mat4::rotation(0.5, Vec3(1.0f, 0.0f, 0.0f));
      modelview = modelview * Mat4::translation(Vec3(0.0f, -1.0f, -2.5f+ltime*0.04));
   }

   modelview = modelview * Mat4::rotation(ltime * 0.04, Vec3(0.0f, 1.0f, 0.0f));
   //modelview = modelview * Mat4::scale(Vec3(3, 3, 3));


   glBindBuffer(GL_ARRAY_BUFFER, g_quad_vbo);

   // draw particles

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_particle_output_fbo);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_particle_output0_tex, 0);

   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);
   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

   glViewport(0, 0, window_width, window_height);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   glClear(GL_COLOR_BUFFER_BIT);

   particle_shader.bind();

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, g_particle_data_texs[tex]);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, g_particle_data_texs[tex^1]);

   particle_shader.uniform1f("time",ltime);
   particle_shader.uniform1f("ftime",ftime);
   particle_shader.uniform1f("aspect_ratio",aspect_ratio);
   particle_shader.uniformMatrix4fv("modelview",1,GL_FALSE,modelview.e);
   particle_shader.uniformMatrix4fv("projection",1,GL_FALSE,projection.e);

   glBindBuffer(GL_ARRAY_BUFFER,g_dummy_particle_vbo);
   glEnableVertexAttribArray(4);
   glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, 0, 0);
   glDrawArrays(GL_POINTS, g_particle_count/2, g_particle_count);
   glDisableVertexAttribArray(4);
   glBindBuffer(GL_ARRAY_BUFFER, g_quad_vbo);





   // downsample

   glDisable(GL_BLEND);
   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
   glEnableVertexAttribArray(0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_particle_output_fbo);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_particle_output1_tex, 0);
   glViewport(0, 0, window_width / 2, window_height / 2);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   downsample_shader.bind();

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, g_particle_output0_tex);

   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_particle_output_fbo);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_particle_output2_tex, 0);
   glViewport(0, 0, window_width / 4, window_height / 4);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, g_particle_output1_tex);

   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);




   // blur

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_particle_output_fbo);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_particle_output1_tex, 0);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   blur_shader.bind();
   blur_shader.uniform2i("axis",1,0);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, g_particle_output2_tex);

   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

   blur_shader.uniform2i("axis",0,1);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_particle_output_fbo);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_particle_output2_tex, 0);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, g_particle_output1_tex);

   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);





   // draw output

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[6]);
   glViewport(0, 0, window_width, window_height);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   postproc_shader.bind();
   postproc_shader.uniform1f("time",ltime);

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, g_particle_output2_tex);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, g_particle_output0_tex);

   glBindSampler(2, g_blur_sampler);
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
   glBindSampler(2, 0);

   glDisableVertexAttribArray(0);

}


void IntroScene::render()
{
   static const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f };
   static const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
   static const GLubyte indices[] = { 3, 0, 2, 1 };

   const float aspect_ratio = float(window_height) / float(window_width);

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);


   glDisable(GL_STENCIL_TEST);
   glDisable(GL_SCISSOR_TEST);
   glDisable(GL_CULL_FACE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);
   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
   glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

   // update the particles
   static int frame_count = 0;
   updateParticles(frame_count&1, frame_count++);
//   updateParticles(frame_count & 1, frame_count2++);

//   ++frame_count;

   // ----- motion blur ------

   glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);
   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }
   glClear(GL_COLOR_BUFFER_BIT);

   num_subframes = 2;
   subframe_scale = 1.0/35.0;

   for(int subframe = 0;subframe<num_subframes;++subframe)
   {
      // draw the view

      glDisable(GL_BLEND);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[6]);

      {
         GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
         glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
      }

      drawView(time + subframe_scale * float(subframe) / float(num_subframes), float(subframe) / float(num_subframes),frame_count&1);


      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);
      glViewport(0, 0, window_width, window_height);

      {
         GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
         glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
      }

      mb_accum_shader.bind();
      mb_accum_shader.uniform1f("time", float(subframe) / float(num_subframes));
      mb_accum_shader.uniform1f("scale", 1.0f / float(num_subframes));

      glDepthMask(GL_FALSE);
      glDisable(GL_DEPTH_TEST);
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ONE);
      glDisable(GL_CULL_FACE);


      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texs[6]);
      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
      glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
      glDisableVertexAttribArray(0);
      glDisableVertexAttribArray(1);
      CHECK_FOR_ERRORS;
   }


   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
   glViewport(0, 0, window_width, window_height);
   glDrawBuffer(GL_BACK);
   glDisable(GL_BLEND);

   composite_shader.bind();
   composite_shader.uniform1i("frame_num", frame_num);
   composite_shader.uniform1f("tex2_fade", clamp((time-6.0)/0.5,0.0,1.0));

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, words_tex);
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

