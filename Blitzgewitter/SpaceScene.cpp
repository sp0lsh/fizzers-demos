
#include "Engine.hpp"
#include "Bezier.hpp"
#include "Ran.hpp"
#include "Particles.hpp"
#include "Tower.hpp"

#include <list>

#define SHADERS_SUBPATH SHADERS_PATH "space/"

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

class SpaceScene: public Scene
{
   Shader mb_accum_shader;

   int num_subframes;
   float subframe_scale;

   float lltime;

   static const int num_fbos = 16, num_texs = 16;

   GLuint fbos[num_fbos], texs[num_texs], renderbuffers[num_fbos];

   GLuint noise_tex;
   GLuint noise_tex_2d;

   Shader shader;
   Shader particle_init_shader;
   Shader postproc_shader;
   Shader downsample_shader;
   Shader blur_shader;
   Shader particle_shader;
   Shader composite_shader;
   Shader hand_shader;
   Shader text_shader;

   Mesh tetrahedron_mesh;
   model::Mesh text_mesh;

   GLuint text_mesh_num_indices;
   GLuint text_mesh_num_vertices;
   GLfloat* text_mesh_vertices;
   GLuint* text_mesh_indices;

   GLuint text_vbo, text_ebo;

   GLuint   g_quad_vbo;
   GLuint   g_particle_data_fbo;
   GLuint   g_particle_output_fbo;

   GLuint   g_blur_sampler;

   static const int num_particle_data_textures = 4;
   GLuint   g_particle_data_texs[num_particle_data_textures];
   GLuint   g_particle_output0_tex;
   GLuint   g_particle_output1_tex;
   GLuint   g_particle_output2_tex;
   GLuint   g_particle_triangle_tex;

   static const int g_particle_data_tex_size = 2048 / 2; // 2048, 4 floats = 67 megabytes
   static const int g_particle_count = g_particle_data_tex_size * g_particle_data_tex_size;

   static const int random_grid_w = 256;
   static const int random_grid_h = 256;

   float random_grid[random_grid_w*random_grid_h];

   void initRandomGrid();
   float randomGridLookup(float u, float v) const;

   float hand_swipe_pos;
   float hand_swipe_vel;

   float roll(float t);

   static const int num_hand_frames=330;
   GLuint hand_frame_texs[num_hand_frames];

   void renderTextStuff(float ltime,const Mat4& modelview);
   void drawText(const std::string& str,bool gold=false,bool pink=false,bool bright=false);

   void initHandFrames();

   void initializeTextures();
   void initializeShaders();
   void initializeBuffers();

   void updateParticles(int tex, int frame_count);

   public:
      SpaceScene(): tetrahedron_mesh(8, 8)
      {
         hand_swipe_pos = 0;
         hand_swipe_vel = 0;

         g_quad_vbo = 0;
         g_particle_data_fbo = 0;
         g_particle_output_fbo = 0;

         g_blur_sampler = 0;

         for(int i=0;i<num_particle_data_textures;++i)
            g_particle_data_texs[i] = 0;

         g_particle_output0_tex = 0;
         g_particle_output1_tex = 0;
         g_particle_output2_tex = 0;

         for(int i=0;i<num_hand_frames;++i)
            hand_frame_texs[i]=0;

         g_particle_triangle_tex = 0;

         text_mesh_num_indices=0;
         text_mesh_num_vertices=0;
         text_mesh_vertices=0;
         text_mesh_indices=0;
         text_vbo=0;
         text_ebo=0;
      }

      ~SpaceScene()
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

Scene* spaceScene = new SpaceScene();

static const float hand_appear_time=24.0f;
static const float hand_frame_seconds=0.0325f;
static const float hand_roll_start_time=hand_appear_time+83*hand_frame_seconds;
static const float hand_roll_end_time=hand_appear_time+124*hand_frame_seconds;
static const float hand_swipe_start_time=hand_appear_time+134*hand_frame_seconds;
static const float hand_swipe_end_time=hand_appear_time+167*hand_frame_seconds;

static const int hand_swipe_frames[] = { 134, 135, 136, 147, 148, 149, 160, 161, 162 };
static const int num_hand_swipe_frames = sizeof(hand_swipe_frames)/sizeof(hand_swipe_frames[0]);

void SpaceScene::slowInitialize()
{
/*
   int err=text_mesh.loadFile(MESHES_PATH "blitzgewitter_text.obj");

   assert(err==0);
   assert(text_mesh.subobjects.size() > 5);

   text_mesh_num_indices=text_mesh.triangles.size()*3;
   text_mesh_num_vertices=text_mesh.vertices.size();
   text_mesh_vertices=new GLfloat[text_mesh_num_vertices*3];
   text_mesh_indices=new GLuint[text_mesh_num_indices];

   {
      int i=0;
      for(std::vector<model::Mesh::Triangle>::const_iterator it=text_mesh.triangles.begin();it!=text_mesh.triangles.end();++it,++i)
      {
         text_mesh_indices[i*3+0]=it->a;
         text_mesh_indices[i*3+1]=it->b;
         text_mesh_indices[i*3+2]=it->c;
      }
   }

   {
      int i=0;
      for(std::vector<Vec3>::const_iterator it=text_mesh.vertices.begin();it!=text_mesh.vertices.end();++it,++i)
      {
         text_mesh_vertices[i*3+0]=it->x;
         text_mesh_vertices[i*3+1]=it->y;
         text_mesh_vertices[i*3+2]=it->z;
      }
   }
*/

   tetrahedron_mesh.generateTetrahedron();

   // shader for updating particle data texture
   shader.load(SHADERS_SUBPATH "vertex_shader.glsl", NULL, SHADERS_SUBPATH "fragment_shader.glsl");
   shader.uniform1i("particle_data_texture", 0);

   // shader for initialising particle data texture
   particle_init_shader.load(SHADERS_SUBPATH "particle_init_vertex_shader.glsl", NULL, SHADERS_SUBPATH "particle_init_fragment_shader.glsl");
   particle_init_shader.uniform1i("particle_data_texture", 0);

   glGenBuffers(1, &g_quad_vbo);
   glBindBuffer(GL_ARRAY_BUFFER, g_quad_vbo);
   const GLfloat verts[16] = { +1.0f,  0.0f, +1.0f, +1.0f,  0.0f,  0.0f,  0.0f, +1.0f,
                               +1.0f, +1.0f,  0.0f,  1.0f, +1.0f,  0.0f,  0.0f,  0.0f };
   glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   CHECK_FOR_ERRORS;

   glDisable(GL_STENCIL_TEST);
   glDisable(GL_SCISSOR_TEST);
   glDisable(GL_CULL_FACE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);
   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
   glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
   CHECK_FOR_ERRORS;

   glGenFramebuffers(1, &g_particle_data_fbo);
   CHECK_FOR_ERRORS;

   glGenTextures(num_particle_data_textures, g_particle_data_texs);
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

   {
      glGenTextures(1,&g_particle_triangle_tex);
      Ran lrnd(661);
      glBindTexture(GL_TEXTURE_2D, g_particle_triangle_tex);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, g_particle_data_tex_size, g_particle_data_tex_size);
      GLfloat* data=new GLfloat[g_particle_data_tex_size*g_particle_data_tex_size*4];

      const Vec3 tri_v0=Vec3( 0.0f,  0.65f, 0.0f);
      const Vec3 tri_v1=Vec3(+0.55f, -0.65f, 0.0f);
      const Vec3 tri_v2=Vec3(-0.55f, -0.65f, 0.0f);

      for(long unsigned int i=0;i<g_particle_data_tex_size*g_particle_data_tex_size;++i)
      {
         float a=lrnd.doub();
         float b=lrnd.doub();
         if((a+b)>1.0f)
         {
            a=1.0f-a;
            b=1.0f-b;
         }
         float c=1.0f-a-b;
         assert((a+b+c)<1.001f);

         Vec3 p=tri_v0*a+tri_v1*b+tri_v2*c;

         data[i*4+0]=p.x;
         data[i*4+1]=p.y;
         data[i*4+2]=p.z;
         data[i*4+3]=0;
      }

      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_particle_data_tex_size, g_particle_data_tex_size, GL_RGBA, GL_FLOAT, data);
      glBindTexture(GL_TEXTURE_2D, 0);
      CHECK_FOR_ERRORS;
      delete[] data;
   }


   {
      float* pix = new float[g_particle_data_tex_size * g_particle_data_tex_size * 4];

      for(int y = 0; y < g_particle_data_tex_size; ++y)
         for(int x = 0; x < g_particle_data_tex_size; ++x)
         {
            pix[(x + y * g_particle_data_tex_size) * 4 + 3] = frand();
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

            pix[(x + y * g_particle_data_tex_size) * 4 + 0] = u * 2.0f;
            pix[(x + y * g_particle_data_tex_size) * 4 + 1] = v * 2.0f;// * 0.01f;
            pix[(x + y * g_particle_data_tex_size) * 4 + 2] = w * 2.0f;
         }

      for(int i = 0; i < num_particle_data_textures; ++i)
      {
         glBindTexture(GL_TEXTURE_2D, g_particle_data_texs[i]);
         glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_particle_data_tex_size,
                                       g_particle_data_tex_size, GL_RGBA, GL_FLOAT, pix);
         CHECK_FOR_ERRORS;
      }

      delete[] pix;
   }

   for(int j = 0; j < num_particle_data_textures; ++j)
   {
      for(int i = 0; i < ((j * 200) / (num_particle_data_textures - 1)); ++i)
      {
         updateParticles(j, i);
      }

   }

   initRandomGrid();

   //start_time = 0;

   initHandFrames();

   initializeShaders();
}

void SpaceScene::initHandFrames()
{
   CHECK_FOR_ERRORS;

   for(int i=0;i<num_hand_frames;++i)
   {
      char file_name[1024];
      sprintf(file_name,"hand_frames/%08d.png",i);
      hand_frame_texs[i] = loadTexture(file_name);
      assert(hand_frame_texs[i] > 0);
      glBindTexture(GL_TEXTURE_2D, hand_frame_texs[i]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
      CHECK_FOR_ERRORS;
   }
}


void SpaceScene::drawText(const std::string& str,bool gold,bool pink,bool bright)
{
   if(gold)
      text_shader.uniform3f("colour",1.0,0.9,0.7);
   else if(pink)
      text_shader.uniform3f("colour",1.0,0.65,0.85);
   else if(bright)
      text_shader.uniform3f("colour",2.0,2.0,2.0);
   else
      text_shader.uniform3f("colour",1.0,1.0,1.0);

   for(std::vector<model::Mesh::SubObject>::const_iterator it=text_mesh.subobjects.begin();it!=text_mesh.subobjects.end();++it)
   {
      if(it->name.find(str)==0)
      {
         glDrawElements(GL_TRIANGLES,it->triangle_count*3,GL_UNSIGNED_INT,(GLvoid*)(it->first_triangle*3*sizeof(GLuint)));
         return;
      }
   }
   assert(false);
}

void SpaceScene::initializeTextures()
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

   CHECK_FOR_ERRORS;

   CHECK_FOR_ERRORS;

   glBindTexture(GL_TEXTURE_2D, 0);

   noise_tex = texs[7];
   noise_tex_2d = texs[11];

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
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pix);
      CHECK_FOR_ERRORS;
   }



   glGenSamplers(1, &g_blur_sampler);
   glSamplerParameteri(g_blur_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   CHECK_FOR_ERRORS;

   glGenTextures(1, &g_particle_output0_tex);
   glGenTextures(1, &g_particle_output1_tex);
   glGenTextures(1, &g_particle_output2_tex);
   CHECK_FOR_ERRORS;




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


void SpaceScene::initializeBuffers()
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

   glGenFramebuffers(1, &g_particle_output_fbo);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_particle_output_fbo);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
   CHECK_FOR_ERRORS;
}


void SpaceScene::initializeShaders()
{
   text_shader.load(SHADERS_PATH "text_v.glsl", NULL, SHADERS_PATH "text_f.glsl");

   // MR. HANDS
   hand_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "hand_f.glsl");
   hand_shader.uniform1i("tex", 0);

   // shader for blurring
   blur_shader.load(SHADERS_SUBPATH "blur_vertex_shader.glsl", NULL, SHADERS_SUBPATH "blur_fragment_shader.glsl");
   blur_shader.uniform1i("tex", 0);

   // shader for downsampling
   downsample_shader.load(SHADERS_SUBPATH "downsample_vertex_shader.glsl", NULL, SHADERS_SUBPATH "downsample_fragment_shader.glsl");
   downsample_shader.uniform1i("tex", 0);

   // shader for drawing particles
   particle_shader.load(SHADERS_SUBPATH "particle_vertex_shader.glsl", SHADERS_SUBPATH "particle_geometry_shader.glsl", SHADERS_SUBPATH "particle_fragment_shader.glsl");
   particle_shader.uniform1i("particle_data_texture_size",g_particle_data_tex_size);
   particle_shader.uniform1i("particle_data_textures[0]", 0);
   particle_shader.uniform1i("particle_data_textures[1]", 1);
   particle_shader.uniform1i("particle_data_textures[2]", 2);
   particle_shader.uniform1i("particle_data_textures[3]", 3);
   particle_shader.uniform1i("particle_triangle_tex", 4);

   // shader for displaying everything
   postproc_shader.load(SHADERS_SUBPATH "postproc_vertex_shader.glsl", NULL, SHADERS_SUBPATH "postproc_fragment_shader.glsl");
   postproc_shader.uniform1i("particle_output_texture", 0);
   postproc_shader.uniform1i("blur_texture", 2);

   composite_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "cubes_composite2_f.glsl");
   composite_shader.uniform1i("tex0", 0);
   composite_shader.uniform1i("tex1", 1);

   mb_accum_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "mb_accum2_f.glsl");
   mb_accum_shader.uniform1i("tex0", 0);
   mb_accum_shader.uniform1i("noise_tex", 1);
}


void SpaceScene::initRandomGrid()
{
   for(int y=0;y<random_grid_h;++y)
      for(int x=0;x<random_grid_w;++x)
      {
         random_grid[x+y*random_grid_w]=(frand()-0.5)*2.0;
      }
}

float SpaceScene::randomGridLookup(float u, float v) const
{
   u=fmodf(u,float(random_grid_w));
   v=fmodf(v,float(random_grid_h));

   if(u<0)
      u+=random_grid_w;

   if(v<0)
      v+=random_grid_h;

   const int iu=int(u);
   const int iv=int(v);

   const float fu=u-float(iu);
   const float fv=v-float(iv);

   const float a=random_grid[iu+iv*random_grid_w];
   const float b=random_grid[((iu+1)%random_grid_w)+iv*random_grid_w];
   const float c=random_grid[((iu+1)%random_grid_w)+((iv+1)%random_grid_h)*random_grid_w];
   const float d=random_grid[iu+((iv+1)%random_grid_h)*random_grid_w];

   return (1.0-fv)*((1.0-fu)*a+fu*b) + fv*((1.0-fu)*d+fu*c);
}


void SpaceScene::initialize()
{
   if(initialized)
      return;

   initialized = true;

   glGenBuffers(1,&text_vbo);
   glGenBuffers(1,&text_ebo);

/*
   glBindBuffer(GL_ARRAY_BUFFER,text_vbo);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,text_ebo);

   glBufferData(GL_ARRAY_BUFFER,text_mesh_num_vertices*sizeof(GLfloat)*3, text_mesh_vertices, GL_STATIC_DRAW);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER,text_mesh_num_indices*sizeof(GLuint), text_mesh_indices, GL_STATIC_DRAW);
   CHECK_FOR_ERRORS;

   glBindBuffer(GL_ARRAY_BUFFER,0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
*/

   initializeTextures();
   initializeBuffers();

}


void SpaceScene::updateParticles(int tex, int frame_count)
{
   // update particle data texture

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_particle_data_fbo);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_particle_data_texs[tex], 0);

   glViewport(0, 0, g_particle_data_tex_size, g_particle_data_tex_size);

   glBindTexture(GL_TEXTURE_2D, g_particle_data_texs[tex]);

   if(frame_count == 0)
   {
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glDisable(GL_BLEND);
      particle_init_shader.bind();
   }
   else
   {
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
      glEnable(GL_BLEND);
      shader.bind();
      shader.uniform1f("time",float(frame_count) * 0.04f);
   }

   glBindBuffer(GL_ARRAY_BUFFER, g_quad_vbo);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
   glEnableVertexAttribArray(0);
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
   glDisableVertexAttribArray(0);
}

void SpaceScene::update()
{
   for(int i=0;i<num_hand_swipe_frames;++i)
   {
      if(hand_swipe_frames[i]==int((time-hand_appear_time)/hand_frame_seconds))
      {
         hand_swipe_vel+= 0.01f;
      }
   }

   hand_swipe_pos+=hand_swipe_vel;
   hand_swipe_vel*=0.97f;

   ++g_ltime;
}

float SpaceScene::roll(float t)
{
   float a=t*3.5f;
   float s=1;
   if(a>M_PI)
      a+=(a-M_PI)*2;
   return -std::sin(a)*0.6f*s-randomGridLookup(10,t*15)*0.02-randomGridLookup(10,t*30)*0.01;
}

void SpaceScene::renderParticles(float tttime)
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
   const int gtime = tttime * 1000;
   const float time = float(gtime - start_time) / 1000.0f;


   const float jitter_x = frand() / window_width * 3;
   const float jitter_y = frand() / window_height * aspect_ratio * 3;

   Mat4 modelview = Mat4::identity(), projection = Mat4::identity();

   const float pinch_amount = cubic(clamp((time-hand_roll_start_time+0.4f)/0.3f,0.0f,1.0f));// - cubic(clamp((time-hand_roll_end_time)/0.01f,0.0f,1.0f));

   const float fovscale = mix(1.0f, 1.5f, pinch_amount);
   const float znear = 0.125f;
   projection = Mat4::frustum(-znear + jitter_x * znear, +znear + jitter_x * znear,
                              -aspect_ratio * znear + jitter_y * znear, +aspect_ratio * znear + jitter_y * znear, znear / fovscale, 20.0f) * projection;


   text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
   text_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);

   modelview = modelview * Mat4::translation(Vec3(0.0f, 0.0f, -2.0f));
   modelview = modelview * Mat4::rotation(time * 0.1, Vec3(0.0f, 1.0f, 0.0f));
   //modelview = modelview * Mat4::scale(Vec3(3, 3, 3));

   modelview = Mat4::rotation(randomGridLookup(0,time)*0.015, Vec3(1.0f, 0.0f, 0.0f)) * modelview;
   modelview = Mat4::rotation(randomGridLookup(4,time)*0.015, Vec3(0.0f, 1.0f, 0.0f)) * modelview;

   const double looktime = 20.0;
   const double lookamount = cubic(clamp(1.0 - std::abs(time - looktime) * 0.15, 0.0, 1.0));

   modelview = Mat4::rotation(lookamount * 2.0, Vec3(1.0f, 0.0f, 0.0f)) * modelview;

   const float roll_amount = cubic(clamp((time-hand_roll_start_time)/0.2f,0.0f,1.0f));// - cubic(clamp((time-hand_roll_end_time)/0.01f,0.0f,1.0f));

   if(time < hand_roll_end_time)
      modelview = Mat4::rotation(roll_amount * roll(time-hand_roll_start_time), Vec3(0.0f, 0.0f, 1.0f)) * modelview;
   else
      modelview = Mat4::rotation(roll_amount * roll(hand_roll_end_time-hand_roll_start_time), Vec3(0.0f, 0.0f, 1.0f)) * modelview;

   modelview = Mat4::translation(Vec3(0.0f, 0.0f, +2.0f)) * modelview;
   modelview = Mat4::rotation( -hand_swipe_pos, Vec3(0.0f, 1.0f, 0.0f)) * modelview;
   modelview = Mat4::translation(Vec3(0.0f, 0.0f, -2.0f)) * modelview;


   glBindBuffer(GL_ARRAY_BUFFER, g_quad_vbo);

   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
   glEnableVertexAttribArray(0);

   // draw particles

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[6]);

   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);
   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

   glViewport(0, 0, window_width, window_height);

   glClear(GL_COLOR_BUFFER_BIT);

   particle_shader.bind();

   glActiveTexture(GL_TEXTURE4);
   glBindTexture(GL_TEXTURE_2D, g_particle_triangle_tex);
   glActiveTexture(GL_TEXTURE3);
   glBindTexture(GL_TEXTURE_2D, g_particle_data_texs[3]);
   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, g_particle_data_texs[2]);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, g_particle_data_texs[1]);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, g_particle_data_texs[0]);

   particle_shader.uniform1f("time", time);
   particle_shader.uniform1f("aspect_ratio", aspect_ratio);
   particle_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
   particle_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);
   particle_shader.uniformMatrix4fv("projection_inv", 1, GL_FALSE, projection.inverse().e);
   particle_shader.uniform1f("cscale", 1.0f+20.0f*(1.0f-cubic(clamp(time*2.0f,0.0f,1.0f))));

   glDrawArrays(GL_POINTS, 0, g_particle_count);

   glDisableVertexAttribArray(0);

   //renderTextStuff(time,modelview);
}

void SpaceScene::renderTextStuff(float ltime,const Mat4& modelview)
{
   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
   glDisable(GL_BLEND);
   glBindBuffer(GL_ARRAY_BUFFER,text_vbo);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,text_ebo);
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
   static const float sca=2;
   static const float zp=-7;
   static const float line_height=0.6;
   static const float second_set_y=-80;
   static const Mat4 scale_matrix=Mat4::scale(Vec3(0.1f));
   static const Mat4 rot_correction=Mat4::rotation(-1.0f,Vec3(0,1,0))*Mat4::rotation(M_PI*0.5f,Vec3(1,0,0));
   text_shader.bind();
   text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (modelview * scale_matrix * rot_correction).e);
   drawText("tickets");
   text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (modelview * scale_matrix * Mat4::translation(Vec3(0,-1,0)) * rot_correction).e);
   drawText("nordlicht.");
   text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (modelview * scale_matrix * Mat4::translation(Vec3(0,-2,0)) * rot_correction).e);
   drawText("join_us");

   glDisableVertexAttribArray(0);
   glBindBuffer(GL_ARRAY_BUFFER,0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);

}

void SpaceScene::render()
{
//time +=20;

   static const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f };
   static const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
   static const GLubyte indices[] = { 3, 0, 2, 1 };

   const float aspect_ratio = float(window_height) / float(window_width);

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);


   // ----- motion blur ------

   glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);
   glClear(GL_COLOR_BUFFER_BIT);

   static const float shake_start_time=32.7f;

   num_subframes = (time > shake_start_time) ? 1 : 2;
   subframe_scale = 1.0/30.0;

   for(int subframe = 0;subframe<num_subframes;++subframe)
   {
      const float mtime = float(subframe) / float(num_subframes);
      const float ltime = time + subframe_scale * mtime;

     // draw the view

      glDisable(GL_BLEND);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[6]);

      renderParticles(ltime);

      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);
      glViewport(0, 0, window_width, window_height);

      const int num_subsubframes = (ltime > shake_start_time) ? 16 : 1;

      mb_accum_shader.bind();
      mb_accum_shader.uniform1f("subtime", mtime);
      mb_accum_shader.uniform1f("scale", 1.0f / float(num_subframes * num_subsubframes));

      for(int j=0;j<num_subsubframes;++j)
      {
         float shake = ltime - shake_start_time + float(j) / float(num_subsubframes) * subframe_scale;
         if(shake < 0.0f)
            shake = -2048.0f;
         mb_accum_shader.uniform1f("time", std::abs(shake));

         glDepthMask(GL_FALSE);
         glDisable(GL_DEPTH_TEST);
         glEnable(GL_BLEND);
         glBlendFunc(GL_ONE, GL_ONE);
         glDisable(GL_CULL_FACE);


         glBindBuffer(GL_ARRAY_BUFFER, 0);
         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

         glActiveTexture(GL_TEXTURE1);
         glBindTexture(GL_TEXTURE_2D, noise_tex_2d);
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

      if(ltime>hand_appear_time)
      {
         glDepthMask(GL_FALSE);
         glDisable(GL_DEPTH_TEST);
         glEnable(GL_BLEND);
         glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
         glDisable(GL_CULL_FACE);
         hand_shader.bind();
         hand_shader.uniform2f("screen_res",1280,720);
         glActiveTexture(GL_TEXTURE0);
         glBindTexture(GL_TEXTURE_2D, hand_frame_texs[std::min(int((ltime-hand_appear_time)/hand_frame_seconds),num_hand_frames-1)]);

         glEnableVertexAttribArray(0);
         glEnableVertexAttribArray(1);
         glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
         glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
         glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
         glDisableVertexAttribArray(0);
         glDisableVertexAttribArray(1);
         CHECK_FOR_ERRORS;
      }

   }

   glDisable(GL_BLEND);
   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
   glDisable(GL_CULL_FACE);

   // ---------------------------------------------------------------------------------------------

   glBindBuffer(GL_ARRAY_BUFFER, g_quad_vbo);

   // downsample

   glDisable(GL_BLEND);
   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
   glEnableVertexAttribArray(0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_particle_output_fbo);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_particle_output1_tex, 0);
   glViewport(0, 0, window_width / 2, window_height / 2);

   downsample_shader.bind();

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, texs[0]);

   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_particle_output_fbo);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_particle_output2_tex, 0);
   glViewport(0, 0, window_width / 4, window_height / 4);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, g_particle_output1_tex);

   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);




   // blur

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_particle_output_fbo);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_particle_output1_tex, 0);

   blur_shader.bind();
   blur_shader.uniform2i("axis",1,0);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, g_particle_output2_tex);

   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

   blur_shader.uniform2i("axis",0,1);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_particle_output_fbo);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_particle_output2_tex, 0);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, g_particle_output1_tex);

   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);





   // draw output

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);
   glViewport(0, 0, window_width, window_height);

   postproc_shader.bind();
   postproc_shader.uniform1f("time", time);

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, g_particle_output2_tex);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, texs[0]);

   glBindSampler(2, g_blur_sampler);
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
   glBindSampler(2, 0);

   glDisableVertexAttribArray(0);

   // ---------------------------------------------------------------------------------------------



   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
   glViewport(0, 0, window_width, window_height);

   composite_shader.bind();
   composite_shader.uniform1i("frame_num", frame_num);

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, noise_tex_2d);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, texs[0]);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);


   CHECK_FOR_ERRORS;

}


void SpaceScene::free()
{
   glDeleteBuffers(1,&text_vbo);
   glDeleteBuffers(1,&text_ebo);
   glDeleteTextures(1,&g_particle_triangle_tex);
   glDeleteTextures(num_hand_frames, hand_frame_texs);
   glDeleteTextures(num_texs, texs);
   glDeleteFramebuffers(num_fbos, fbos);
}

