
#include "Engine.hpp"
#include "Bezier.hpp"
#include "Ran.hpp"
#include "Particles.hpp"

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

class TunnelScene: public Scene
{
   Shader glass_shader, bokeh_shader, bokeh2_shader;
   Shader gaussbokeh_shader, composite_shader;
   Shader mb_accum_shader;
   Shader bgfg_shader;
   Shader tunnel_shader;
   Shader cloud_gen_shader;
   Shader cloud_render_shader;
   Shader electric_shader;
   Shader electric_render_shader;

   Mesh tetrahedron_mesh;
   Mesh tunnel_mesh;

   static const int max_tetrahedra=4;

/*
01:42.5
01:54
02:05
*/

   float tetrahedra_explode_times[max_tetrahedra];
   Mat4 tetrahedra_mats[max_tetrahedra];

   bool drawing_view_for_background;
   int num_subframes;
   float subframe_scale;

   float lltime;
   float scroll;
   float scroll_speed;
   float crash_length;
   int camera_tet;
   float track_start_time;

   static const int num_fbos = 16, num_texs = 16;

   GLuint fbos[num_fbos], texs[num_texs], renderbuffers[num_fbos];

   GLuint bokeh_temp_fbo;
   GLuint bokeh_temp_tex_1, bokeh_temp_tex_2;

   GLuint noise_tex;
   GLuint noise_tex_2d;

   static const int electric_size = 512;
   GLushort electric_tex_data[electric_size * electric_size * 3];
   GLuint electric_texs[3];
   GLuint electric_target_tex;
   GLuint electric_fbo;

   static const int cloud_size = 64;

   GLuint cloud_tex;
   GLuint cloud_fbo;
   GLuint cloud_nearest_sampler;
   float cloud_appear_time;

   GLuint tet_tex;

   bool show_cloud;

   void initializeTextures();
   void initializeShaders();
   void initializeBuffers();

   void initElectric();
   void genElectric(float ltime);
   void drawView(float ltime);
   void gaussianBlur(Real tint_r, Real tint_g, Real tint_b, Real radx, Real rady, GLuint fbo0, GLuint fbo1, GLuint src_tex, GLuint tex0);

   Vec3 tetrahedronFlyPath(int tet, float ltime);
   Vec3 tetrahedronCrashPath(int tet, float start_time, float ltime);
   Vec3 tetrahedronPath(int tet, float ltime);
   Vec3 smoothTetrahedronPath(int tet, float ltime);

   void generateCloud(float ltime);
   void renderCloud(float ltime);

   public:
      TunnelScene(): tetrahedron_mesh(8, 8), tunnel_mesh(4096*6,4096*8)
      {
         tet_tex = 0;
         camera_tet = 0;
         for(int i=0;i<max_tetrahedra;++i)
         {
            tetrahedra_explode_times[i] = 0;
         }
      }

      ~TunnelScene()
      {
         free();
      }

      void slowInitialize();
      void initialize();
      void render();
      void update();
      void free();
};

extern unsigned short electric_data[512*512*3];

Scene* tunnelScene = new TunnelScene();


void TunnelScene::slowInitialize()
{
   tet_tex = loadTexture(IMAGES_PATH "tet.png",true);

/*
   for(int i=0;i<max_tetrahedra;++i)
   {
      tetrahedra_explode_times[i] = (i + 1) * 10;
   }
*/

   tetrahedra_explode_times[0] = 11.631;// (1.0 * 60.0 + 42.5) - scene_start_time;
   tetrahedra_explode_times[1] = 23.287;//(1.0 * 60.0 + 54.0) - scene_start_time;
   tetrahedra_explode_times[2] = 34.918;//(2.0 * 60.0 + 05.8) - scene_start_time;
   tetrahedra_explode_times[3] = 1000.0;

   initElectric();

   tetrahedron_mesh.generateTetrahedron();
   tetrahedron_mesh.transform(Mat4::scale(Vec3(0.999)));
   tetrahedron_mesh.generateNormals();
   tunnel_mesh.generateCylinder(64, false, false, 256);
   tunnel_mesh.transform(Mat4::scale(Vec3(-4,-4,-64)));

   initializeShaders();
}

void TunnelScene::initElectric()
{
#if 0
   static const int num_particles = 80000;
   Ran rnd(112);

   int* particle_positions = new int[num_particles * 2];

   memset(electric_tex_data, 0xff, sizeof(electric_tex_data));

   for(int n=0;n<3;++n)
   {
      GLushort* dat = electric_tex_data + electric_size * electric_size * n;

      for(int i=0;i<num_particles;++i)
      {
         particle_positions[i * 2 + 0] = 2 + (rnd.int32() % (electric_size - 2));
         particle_positions[i * 2 + 1] = 2 + (rnd.int32() % (electric_size - 2));
      }

      GLushort seed_count = 2;

      dat[(1 + electric_size) * electric_size / 2] = 1;

      while(seed_count < num_particles - 2000)
      {
         for(int i=0;i<num_particles;++i)
         {
            int& x = particle_positions[i * 2 + 0];

            if(x < 0)
               continue;

            int& y = particle_positions[i * 2 + 1];

            int nx = x + (rnd.int32() % 3) - 1;
            int ny = y + (rnd.int32() % 3) - 1;

            if(nx<1)
               nx=electric_size-2;

            if(ny<1)
               ny=electric_size-2;

            if(nx>electric_size-2)
               nx=1;

            if(ny>electric_size-2)
               ny=1;

            if(dat[nx + ny * electric_size] < 0xffff)
            {
               dat[x + y * electric_size] = seed_count++;
               x = -128;
               continue;
            }

            x=nx;
            y=ny;
         }
      }

   }

   delete[] particle_positions;

   FILE* out=fopen("electric_data.cpp", "w");
   assert(out);
   fprintf(out, "unsigned short electric_data[1024*1024*3]={");
   for(int i=0;i<electric_size*electric_size*3;++i)
   {
      fprintf(out, "%d, ", electric_tex_data[i]);
   }
   fprintf(out, "};\r\n");
   fclose(out);
   exit(0);
#else
   assert(sizeof(electric_tex_data) == sizeof(electric_data));
   for(int i=0;i<electric_size*electric_size*3;++i)
   {
      electric_tex_data[i]=electric_data[i];
   }
#endif
}

void TunnelScene::initializeTextures()
{
   glGenSamplers(1, &cloud_nearest_sampler);
   glSamplerParameteri(cloud_nearest_sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glSamplerParameteri(cloud_nearest_sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   CHECK_FOR_ERRORS;

   glGenTextures(num_texs, texs);

   createWindowTexture(texs[0], GL_RGBA16F);
   createWindowTexture(texs[2], GL_RGBA16F);
   createWindowTexture(texs[4], GL_RGBA16F, 4);
   createWindowTexture(texs[5], GL_RGBA16F, 4);
   createWindowTexture(texs[6], GL_RGBA16F);
   createWindowTexture(texs[9], GL_RGBA16F, 4);
   createWindowTexture(texs[12], GL_RGBA16F, 2);
   createWindowTexture(texs[13], GL_RGBA16F, 2);

   CHECK_FOR_ERRORS;

   glBindTexture(GL_TEXTURE_2D, 0);

   electric_texs[0] = texs[1];
   cloud_tex = texs[3];
   noise_tex = texs[7];
   electric_texs[1] = texs[10];
   noise_tex_2d = texs[11];
   bokeh_temp_tex_1 = texs[12];
   bokeh_temp_tex_2 = texs[13];
   electric_target_tex = texs[14];
   electric_texs[2] = texs[15];

   for(int i=0;i<3;++i)
   {
      glBindTexture(GL_TEXTURE_2D, electric_texs[i]);
      CHECK_FOR_ERRORS;
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

      glTexStorage2D(GL_TEXTURE_2D, 1, GL_R16, electric_size, electric_size);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, electric_size, electric_size, GL_RED, GL_UNSIGNED_SHORT, electric_tex_data + electric_size * electric_size * i);

      CHECK_FOR_ERRORS;
      glBindTexture(GL_TEXTURE_2D, 0);
   }

   glBindTexture(GL_TEXTURE_2D, electric_target_tex);
   CHECK_FOR_ERRORS;
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, electric_size, electric_size);
   CHECK_FOR_ERRORS;
   glBindTexture(GL_TEXTURE_2D, 0);


   {
      uint s = 16;
      GLubyte* pix = new GLubyte[s * s * s * 3];
      for(uint i = 0; i < s * s * s * 3; ++i)
         pix[i] = lrand();

      glBindTexture(GL_TEXTURE_3D, noise_tex);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

      glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGB8, s, s, s);
      glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, s, s, s, GL_RGB, GL_UNSIGNED_BYTE, pix);

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
            pix[x+y*w]=(lrand()&255) | ((lrand()&255) <<8)|((lrand()&255))<<16;
         }

      glBindTexture(GL_TEXTURE_2D, noise_tex_2d);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pix);      CHECK_FOR_ERRORS;
   }

   glBindTexture(GL_TEXTURE_3D, cloud_tex);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   //glTexStorage3D(GL_TEXTURE_3D, 1, GL_R16, cloud_size, cloud_size, cloud_size);
   glTexImage3D(GL_TEXTURE_3D, 0, GL_R16, cloud_size, cloud_size, cloud_size, 0, GL_RED, GL_UNSIGNED_SHORT, 0);
   CHECK_FOR_ERRORS;
   glBindTexture(GL_TEXTURE_3D, 0);
}

void TunnelScene::initializeBuffers()
{
   glGenFramebuffers(num_fbos, fbos);
   glGenRenderbuffers(num_fbos, renderbuffers);

   cloud_fbo = fbos[1];
   bokeh_temp_fbo = fbos[12];
   electric_fbo = fbos[15];

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

   CHECK_FOR_ERRORS;
}

void TunnelScene::initializeShaders()
{
   electric_render_shader.load(SHADERS_PATH "electric_render_v.glsl", NULL, SHADERS_PATH "electric_render_f.glsl");
   electric_render_shader.uniform1i("tex0",0);
   electric_render_shader.uniform1i("tex1",1);

   electric_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "electric_f.glsl");
   electric_shader.uniform1i("tex0",0);

   glass_shader.load(SHADERS_PATH "glass_v.glsl", NULL, SHADERS_PATH "glass_f.glsl");
   glass_shader.uniform1i("tex0",0);
   glass_shader.uniform1i("tex1",1);

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

   bgfg_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "bgfg_f.glsl");
   bgfg_shader.uniform1i("tex0", 0);
   bgfg_shader.uniform1i("tex1", 1);

   tunnel_shader.load(SHADERS_PATH "tunnel_v.glsl", NULL, SHADERS_PATH "tunnel_f.glsl");
   tunnel_shader.uniform1i("tex0",0);

   cloud_gen_shader.load(SHADERS_PATH "cloud_gen_v.glsl", NULL, SHADERS_PATH "cloud_gen_f.glsl");
   cloud_gen_shader.uniform1i("tex0",0);
   cloud_gen_shader.uniform1i("noise_tex",1);

   cloud_render_shader.load(SHADERS_PATH "cloud_render_v.glsl", NULL, SHADERS_PATH "cloud_render_f.glsl");
   cloud_render_shader.uniform1i("tex0",0);
   cloud_render_shader.uniform1i("noise_tex",1);
   cloud_render_shader.uniform1i("arc_tex",2);

   CHECK_FOR_ERRORS;
}



void TunnelScene::initialize()
{
   if(initialized)
      return;

   initialized = true;

   initializeTextures();
   initializeBuffers();
}


void TunnelScene::update()
{
   ++g_ltime;
}

Vec3 TunnelScene::tetrahedronCrashPath(int tet, float start_time, float ltime)
{
   Vec3 p = tetrahedronFlyPath(tet, start_time);
   float t = clamp(ltime - start_time, 0.0f, crash_length);
   float zofs = t * scroll_speed - (t * t) / (2.0f * crash_length * 1.5f) * scroll_speed;
   const float rr = 1.0f + mix(0.0f, 3.0f, clamp(t / crash_length, 0.0f, 1.0f));
   return Vec3(std::cos(std::min(ltime, start_time + crash_length)*0.8f + tet) * rr, std::sin(std::min(ltime, start_time + crash_length)*0.8f + tet) * rr, p.z - zofs);
}

Vec3 TunnelScene::tetrahedronFlyPath(int tet, float ltime)
{
   const float rr=1.0f;
   return Vec3(std::cos(ltime*0.8+tet)*rr,std::sin(ltime*0.8+tet)*rr,-ltime*scroll_speed-1.5*GLfloat((tet-2)));
}

Vec3 TunnelScene::tetrahedronPath(int tet, float ltime)
{
   if(ltime >= tetrahedra_explode_times[tet] - crash_length)
   {
      return tetrahedronCrashPath(tet, tetrahedra_explode_times[tet] - crash_length, ltime);
   }
   else
   {
      return tetrahedronFlyPath(tet, ltime);
   }
}

void TunnelScene::renderCloud(float ltime)
{
   static const GLfloat vertices[] = { -0.5f, -0.65f, +0.45f, -0.65f, +0.45f, +0.7f, -0.5f, +0.7f };
   static const GLfloat vertices2[] = { -0.7f, -1.0f, +0.7f, -1.0f, +0.7f, +0.8f, -0.7f, +0.8f };
   static const GLubyte indices[] = { 3, 0, 2, 1 };

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, electric_target_tex);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_3D, noise_tex);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_3D, cloud_tex);
   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
   //glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
   glEnableVertexAttribArray(0);

   if(camera_tet==2)
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices2);
   else
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
   CHECK_FOR_ERRORS;

   cloud_render_shader.bind();

   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);

   glDisableVertexAttribArray(0);
   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_3D, 0);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_3D, 0);
   CHECK_FOR_ERRORS;

   glDisable(GL_BLEND);
   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void TunnelScene::generateCloud(float ltime)
{
   static const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f };
   static const GLubyte indices[] = { 3, 0, 2, 1 };

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, cloud_fbo);
   CHECK_FOR_ERRORS;
   glViewport(0, 0, cloud_size, cloud_size);
   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_3D, noise_tex);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_3D, cloud_tex);
   glBindSampler(0, cloud_nearest_sampler);
   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
   glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
   CHECK_FOR_ERRORS;

   cloud_gen_shader.bind();
   cloud_gen_shader.uniform1f("time", ltime);

   for(int z=0;z<cloud_size;++z)
   {
      cloud_gen_shader.uniform1i("layer",z);
      cloud_gen_shader.uniform1f("layerf",float(z)/float(cloud_size));
      glFramebufferTexture3D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, cloud_tex, 0, z);
      CHECK_FOR_ERRORS;
      glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
      CHECK_FOR_ERRORS;
   }

   glDisableVertexAttribArray(0);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_3D, 0);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_3D, 0);
   glBindSampler(0, 0);
   CHECK_FOR_ERRORS;
}

Vec3 TunnelScene::smoothTetrahedronPath(int tet, float ltime)
{
   static const int n = 16;
   float wsum=0;
   Vec3 tet_pos(0);
   for(int i=0;i<n;++i)
   {
      float t=float(i)/float(n-1);
      float w=(1.0f-std::cos(t*M_PI*2))*0.5f;
      tet_pos+=tetrahedronPath(tet, ltime - mix(-1.0f,1.1f,t))*w;
      wsum+=w;
   }
   return tet_pos*(1.0f/wsum);
}

void TunnelScene::genElectric(float ltime)
{
   static const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f };
   static const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
   static const GLubyte indices[] = { 3, 0, 2, 1 };
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, electric_fbo);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, electric_target_tex, 0);
   glViewport(0, 0, electric_size, electric_size);
   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }
   glColorMask(GL_TRUE,GL_FALSE,GL_FALSE,GL_FALSE);
   glDisable(GL_BLEND);
   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
   glActiveTexture(GL_TEXTURE0);
   assert(camera_tet >= 0 && camera_tet < 3);
   glBindTexture(GL_TEXTURE_2D, electric_texs[camera_tet]);
   electric_shader.bind();
   electric_shader.uniform1f("time", ltime);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
   CHECK_FOR_ERRORS;
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
   glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
}

void TunnelScene::drawView(float ltime)
{
   static const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f };
   static const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
   static const GLubyte indices[] = { 3, 0, 2, 1 };

   lltime = ltime;

   const float aspect_ratio = float(window_height) / float(window_width);

   const float jitter_x = frand() / window_width * 3;
   const float jitter_y = frand() / window_height * aspect_ratio * 3;

   Mat4 modelview = Mat4::identity(), projection = Mat4::identity();

   float fov = M_PI * (0.3f + 0.4f * std::pow(clamp((ltime - 40.0f) / 6.5f, 0.0f, 1.0f), 2.0f));

   if(show_cloud)
   {
      fov += 0.05f * std::cos((ltime - cloud_appear_time) * 16.0f / (1.0f + (ltime - cloud_appear_time))) / (1.0f + (ltime - cloud_appear_time));
   }

   const float znear = 0.4f / tan(fov * 0.5f), zfar = 1024.0f;

   const float frustum[4] = { -1.0f + jitter_x, +1.0f + jitter_x, -aspect_ratio + jitter_y, +aspect_ratio + jitter_y };

   projection = Mat4::frustum(frustum[0], frustum[1], frustum[2], frustum[3], znear, zfar) * projection;


   modelview = modelview * Mat4::translation(Vec3(0, 0, -2.5));

   Mat4 modelview_rotation = Mat4::identity();

   scroll_speed=17;
   scroll=ltime*scroll_speed;
   crash_length = 4.0f;

   Vec3 cam_pos=Vec3(0, 0, -scroll);

   if(camera_tet > -1)
   {
      float trans=cubic(clamp((ltime - track_start_time) / 3.0f, 0.0f, 1.0f));

      Vec3 tet_pos=smoothTetrahedronPath(camera_tet, ltime);

      cam_pos=mix(cam_pos, tet_pos, trans);

      tet_pos.z=mix(0.5f,1.5f,clamp((tetrahedra_explode_times[camera_tet] - ltime) * 0.5f, 0.0f, 1.0f)) +std::cos(time)*0.25;

      tet_pos=normalize(tet_pos);

      Real phi=mix(std::cos((ltime-track_start_time)*0.5f)*0.7f, std::atan2(tet_pos.x, tet_pos.z), trans);
      Real theta=mix((ltime-track_start_time)*0.6f, std::asin(-tet_pos.y), trans);

      modelview_rotation = Mat4::rotation(theta, Vec3(1.0f, 0.0f, 0.0f)) * Mat4::rotation(phi, Vec3(0.0f, 1.0f, 0.0f));
   }
   else
   {
      modelview_rotation = Mat4::rotation(cos(ltime*0.5)*0.7, Vec3(1.0f, 0.0f, 0.0f)) * Mat4::rotation(ltime*0.6, Vec3(0.0f, 1.0f, 0.0f));
   }

   modelview = modelview * modelview_rotation;

   glViewport(0, 0, window_width, window_height);

   CHECK_FOR_ERRORS;

   glDepthMask(GL_TRUE);
   glEnable(GL_CULL_FACE);
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LEQUAL);

   glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

   CHECK_FOR_ERRORS;

   glass_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);
   tunnel_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);
   electric_render_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);


   CHECK_FOR_ERRORS;


   glUseProgram(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

   glDisable(GL_BLEND);


   glDepthMask(GL_TRUE);
   glEnable(GL_DEPTH_TEST);
   if(!drawing_view_for_background)
   {
      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(2);

      const Mat4 m = modelview * Mat4::translation(Vec3(-cam_pos));

      glass_shader.bind();
      glass_shader.uniform1f("background",drawing_view_for_background?1.0f:0.0f);
      glass_shader.uniform1f("rayjitter",frand());
      glass_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, m.e);
      glass_shader.uniformMatrix4fv("modelview_inv", 1, GL_FALSE, m.inverse().e);
      glass_shader.uniform4f("frustum", frustum[0], frustum[1], frustum[2], frustum[3]);
      glass_shader.uniform1f("znear", znear);
      glass_shader.uniform1f("time", ltime);

      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D,tet_tex);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_3D,noise_tex);

      Mesh* mesh=&tetrahedron_mesh;

      mesh->bind();

      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
      glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(mesh->getVertexCount() * sizeof(GLfloat) * 3));

      for(int tet=0;tet<max_tetrahedra;++tet)
      {
         Vec3 p=tetrahedronPath(tet,ltime);
         tetrahedra_mats[tet]=Mat4::translation(p) * Mat4::rotation(ltime*0.03+tet,Vec3(1,0,0)) * Mat4::rotation(ltime*0.04,Vec3(0,1,0));
      }

      glass_shader.uniformMatrix4fv("tetrahedra_mats[0]", 1, GL_FALSE, tetrahedra_mats[0].e);
      glass_shader.uniformMatrix4fv("tetrahedra_mats_inv[0]", 1, GL_FALSE, tetrahedra_mats[0].inverse().e);
      glass_shader.uniformMatrix4fv("tetrahedra_mats[1]", 1, GL_FALSE, tetrahedra_mats[1].e);
      glass_shader.uniformMatrix4fv("tetrahedra_mats_inv[1]", 1, GL_FALSE, tetrahedra_mats[1].inverse().e);
      glass_shader.uniformMatrix4fv("tetrahedra_mats[2]", 1, GL_FALSE, tetrahedra_mats[2].e);
      glass_shader.uniformMatrix4fv("tetrahedra_mats_inv[2]", 1, GL_FALSE, tetrahedra_mats[2].inverse().e);
      glass_shader.uniformMatrix4fv("tetrahedra_mats[3]", 1, GL_FALSE, tetrahedra_mats[3].e);
      glass_shader.uniformMatrix4fv("tetrahedra_mats_inv[3]", 1, GL_FALSE, tetrahedra_mats[3].inverse().e);

      glass_shader.uniform1f("burn_up", clamp((ltime - 31.0f) / 3.0f, 0.0f, 1.0f) + std::pow(clamp((ltime - 40.0f) / 5.0f, 0.0f, 1.0f),3.0f));

      for(int tet=0;tet<max_tetrahedra;++tet)
      {
         if(tet==camera_tet && show_cloud)
            continue;

         glass_shader.uniformMatrix4fv("object", 1, GL_FALSE, tetrahedra_mats[tet].e);
         glDrawRangeElements(GL_TRIANGLES, 0, mesh->getVertexCount() - 1, mesh->getTriangleCount() * 3, GL_UNSIGNED_INT, 0);
      }

      CHECK_FOR_ERRORS;


      glDisableVertexAttribArray(0);
      glDisableVertexAttribArray(2);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
   }

   //if(drawing_view_for_background)
   {
      tunnel_shader.bind();
      glDepthMask(GL_TRUE);
      glEnable(GL_DEPTH_TEST);

      tunnel_shader.uniform1f("time",ltime);
      tunnel_shader.uniform1f("aspect_ratio",float(window_width) / float(window_height));
      tunnel_shader.uniform1f("background",drawing_view_for_background?1.0f:0.0f);
      tunnel_shader.uniform2f("jitter",jitter_x,jitter_y);
      tunnel_shader.uniform1f("rayjitter",frand());
      tunnel_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
      tunnel_shader.uniform4f("frustum", frustum[0], frustum[1], frustum[2], frustum[3]);

      if(show_cloud)
      {
         float tm=ltime-cloud_appear_time;
         tunnel_shader.uniform1f("fade",mix(0.0f,1.0f,tm/(tm+2.5f)));
      }
      else
         tunnel_shader.uniform1f("fade",1.0f);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_3D,noise_tex);


      Mesh* mesh=&tunnel_mesh;

      mesh->bind();

      Vec3 cam_pos2=cam_pos;

      static const float wrap = 8.0f;

      tunnel_shader.uniform1f("z_offset", std::floor(cam_pos2.z / wrap) * wrap);

      cam_pos2.z=fmodf(cam_pos2.z, wrap);
      const Mat4 m = modelview * Mat4::translation(Vec3(-cam_pos2));

      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(2);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
      glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(mesh->getVertexCount() * sizeof(GLfloat) * 3));
      tunnel_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, m.e);

      //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

      glDrawRangeElements(GL_TRIANGLES, 0, mesh->getVertexCount() - 1, mesh->getTriangleCount() * 3, GL_UNSIGNED_INT, 0);
      glDisableVertexAttribArray(0);
      glDisableVertexAttribArray(2);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

      //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

      CHECK_FOR_ERRORS;
   }


   if(!drawing_view_for_background && show_cloud)
   {
      assert(camera_tet > -1);
      const Vec3 tet_pos=tetrahedronPath(camera_tet, ltime);
      const Mat4 m = modelview * Mat4::translation(tet_pos) * Mat4::translation(Vec3(-cam_pos)) * modelview_rotation.transpose() * Mat4::scale(Vec3(3));
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE,GL_ONE);
      glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_FALSE);
      glDisable(GL_DEPTH_TEST);
      glDepthMask(GL_FALSE);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, noise_tex_2d);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, electric_target_tex);
      electric_render_shader.bind();
      electric_render_shader.uniform1f("time", ltime - cloud_appear_time);
      electric_render_shader.uniform1f("aspect_ratio", aspect_ratio);
      electric_render_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, m.e);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
      glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
      CHECK_FOR_ERRORS;
      glDisableVertexAttribArray(0);
      glDisableVertexAttribArray(1);
      glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
   }

   if(!drawing_view_for_background && show_cloud)
   {
      assert(camera_tet > -1);
      const Vec3 tet_pos=tetrahedronPath(camera_tet, ltime);
      const Mat4 m = modelview * Mat4::translation(tet_pos) * Mat4::translation(Vec3(-cam_pos));

      cloud_render_shader.uniform2f("jitter",jitter_x,jitter_y);
      cloud_render_shader.uniform1f("rayjitter",frand());
      cloud_render_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, m.e);
      {
         Vec3 cam_origin=m.inverse().column(3);
         cloud_render_shader.uniform3f("cam_origin",cam_origin.x,cam_origin.y,cam_origin.z);
      }
      cloud_render_shader.uniform4f("frustum", frustum[0], frustum[1], frustum[2], frustum[3]);
      cloud_render_shader.uniform1f("znear", znear);
      cloud_render_shader.uniform1f("time", ltime - cloud_appear_time);
      CHECK_FOR_ERRORS;

      renderCloud(ltime - cloud_appear_time);
   }

   glDepthMask(GL_TRUE);

   CHECK_FOR_ERRORS;
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void TunnelScene::gaussianBlur(Real tint_r, Real tint_g, Real tint_b, Real radx, Real rady, GLuint fbo0, GLuint fbo1, GLuint src_tex, GLuint tex0)
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


void TunnelScene::render()
{
   static const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f };
   static const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
   static const GLubyte indices[] = { 3, 0, 2, 1 };

   const float aspect_ratio = float(window_height) / float(window_width);

   track_start_time=0;
   camera_tet=-1;
   static const float cloud_show_length=3.0;
   for(int i=0;i<4;++i)
   {
      camera_tet=i;
      if(tetrahedra_explode_times[i] > (time - cloud_show_length))
         break;
      track_start_time=tetrahedra_explode_times[i] + cloud_show_length;
   }

   show_cloud=false;

   if(camera_tet > -1)
   {
      if(time >= tetrahedra_explode_times[camera_tet])
      {
         show_cloud=true;
         cloud_appear_time=tetrahedra_explode_times[camera_tet];
      }
   }

   if(show_cloud)
      generateCloud(time - cloud_appear_time);

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

   for(int nn = 0; nn < 2; ++nn)
   {
//if(drawing_view_for_background) continue; // **********************************************************************************

      drawing_view_for_background = !nn;

      // ----- motion blur ------

      glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[3]);
      glClear(GL_COLOR_BUFFER_BIT);

      if(show_cloud)
         num_subframes = drawing_view_for_background ? 1 : 1;
      else
         num_subframes = drawing_view_for_background ? 2 : 7;

      subframe_scale = 1.0/35.0;

      for(int subframe = 0;subframe<num_subframes;++subframe)
      {
         const float ltime = time + subframe_scale * float(subframe) / float(num_subframes);

         if(show_cloud)
         {
            // do the electricty
            genElectric(ltime-cloud_appear_time);
         }

         // draw the view

         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[6]);

         {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
         }

         glDisable(GL_BLEND);

         drawView(ltime);


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

      if(drawing_view_for_background)
      {
         // ----- aperture ------

         const float CoC = 0.01;// + sin(time) * 0.05;

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

         bokeh2_shader.bind();
         bokeh2_shader.uniform2f("CoC", CoC / aspect_ratio, CoC);
         bokeh2_shader.uniform1f("brightness", 0.2);

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

   glDrawBuffer(GL_BACK);

   composite_shader.bind();
   composite_shader.uniform1f("time", time);
   composite_shader.uniform1i("frame_num", frame_num);
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


void TunnelScene::free()
{
   glDeleteTextures(1,&tet_tex);
   glDeleteTextures(num_texs, texs);
   glDeleteFramebuffers(num_fbos, fbos);
}

