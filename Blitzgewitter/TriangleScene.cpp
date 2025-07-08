
// midnight platonic

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

class TriangleScene: public Scene
{
   struct Drip
   {
      void update();
      std::list<Vec2> pos_history;
      Vec2 pos;
      float r;
   };

   static const int num_drips = 17;

   Drip drips[num_drips];

   static const int mountains_w = 128;
   static const int mountains_h = 128;
   static const int mountains_max_vertices = mountains_w * mountains_h;
   static const int mountains_max_indices = mountains_w * mountains_h * 2;
   GLfloat mountain_vertices[mountains_max_vertices * 3];
   GLushort mountain_indices[mountains_max_indices];
   GLushort mountain_num_vertices, mountain_num_indices;
   GLuint mountain_vbo;
   GLuint mountain_ebo;

   static const int num_balls=2048;
   Vec3 balls[num_balls];

   static const int random_grid_w = 256;
   static const int random_grid_h = 256;

   float random_grid[random_grid_w*random_grid_h];

   void initRandomGrid();
   float randomGridLookup(float u, float v) const;

   GLfloat spiral_vertices[1024 * 4 * 3];
   GLfloat spiral_coords[1024 * 4 * 2];
   GLushort spiral_indices[1024 * 4 * 3];
   GLushort spiral_num_vertices, spiral_num_indices;

   Shader forrest_shader;
   Shader gaussbokeh_shader, composite_shader;
   Shader mb_accum_shader;
   Shader screendisplay_shader;
   Shader bgfg_shader;
   Shader cloud_gen_shader;
   Shader cloud_render_shader;
   Shader lightning_shader;

   int num_subframes;
   float subframe_scale;

   float lltime;
   double drip_scale;

   Mesh icosahedron_mesh;
   Mesh tetrahedron_mesh;
   Mesh octahedron_mesh;
   Mesh road_mesh;

   static const int cloud_size = 64;

   GLuint cloud_tex;
   GLuint cloud_fbo;
   GLuint cloud_nearest_sampler;

   static const int num_fbos = 16, num_texs = 16;

   GLuint fbos[num_fbos], texs[num_texs], renderbuffers[num_fbos];

   GLuint noise_tex;
   GLuint noise_tex_2d;

   void generateCloud(float ltime);
   void renderCloud(float ltime);

   void initializeTextures();
   void initializeShaders();
   void initializeBuffers();
   GLfloat evaluateMountainHeight(GLfloat u,GLfloat v);
   void initMountain();
   void drawMountains(float ltime);

   void drawView(float ltime);
   void drawView2(float ltime);
   void createSpiral();
   void gaussianBlur(Real tint_r, Real tint_g, Real tint_b, Real radx, Real rady, GLuint fbo0, GLuint fbo1, GLuint src_tex, GLuint tex0);


   public:
      TriangleScene(): icosahedron_mesh(32, 32), tetrahedron_mesh(8, 8), octahedron_mesh(8, 8), road_mesh(32, 32)
      {
         mountain_num_vertices=0;
         mountain_num_indices=0;
         mountain_vbo=0;
         mountain_ebo=0;
      }

      ~TriangleScene()
      {
      }

      void slowInitialize();
      void initialize();
      void render();
      void update();
      void free();
};

Scene* triangleScene = new TriangleScene();

static const float switch_time = 13.0;
//static const float switch_time = 1.0;

void TriangleScene::slowInitialize()
{
   road_mesh.generateGrid(4,4);
   road_mesh.transform(Mat4::translation(Vec3(-0.25,0,0))*Mat4::rotation(M_PI*-0.5,Vec3(1,0,0))*Mat4::scale(Vec3(-1.0/2.0,1,100)));

   icosahedron_mesh.generateIcosahedron();
   tetrahedron_mesh.generateTetrahedron2();
   octahedron_mesh.generateOctahedron();

   initializeShaders();

   glGenTextures(num_texs, texs);

   cloud_tex = texs[3];
   noise_tex = texs[7];
   noise_tex_2d = texs[11];


   glGenFramebuffers(num_fbos, fbos);
   glGenRenderbuffers(num_fbos, renderbuffers);

   cloud_fbo = fbos[1];

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

   glBindTexture(GL_TEXTURE_3D, cloud_tex);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexImage3D(GL_TEXTURE_3D, 0, GL_R16, cloud_size, cloud_size, cloud_size, 0, GL_RED, GL_UNSIGNED_SHORT, 0);
   CHECK_FOR_ERRORS;
   glBindTexture(GL_TEXTURE_3D, 0);

   glGenSamplers(1, &cloud_nearest_sampler);
   glSamplerParameteri(cloud_nearest_sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glSamplerParameteri(cloud_nearest_sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   CHECK_FOR_ERRORS;

   generateCloud(0);
}

void TriangleScene::initializeTextures()
{
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
}


void TriangleScene::initializeBuffers()
{
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
}

void TriangleScene::initializeShaders()
{
   lightning_shader.load(SHADERS_PATH "lines_lightning_v.glsl", SHADERS_PATH "lines_lightning_g.glsl", SHADERS_PATH "lines_lightning_f.glsl");

   forrest_shader.load(SHADERS_PATH "forrest3_v.glsl", NULL, SHADERS_PATH "forrest3_f.glsl");
   forrest_shader.uniform1i("noise_tex", 0);

   gaussbokeh_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "gaussblur_f.glsl");
   gaussbokeh_shader.uniform1i("tex0", 0);

   mb_accum_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "mb_accum2_f.glsl");
   mb_accum_shader.uniform1i("tex0", 0);
   mb_accum_shader.uniform1i("noise_tex", 1);


   composite_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "cubes_composite_f.glsl");
   composite_shader.uniform1i("tex0", 0);
   composite_shader.uniform1i("tex1", 1);
   composite_shader.uniform1i("bloom_tex", 4);
   composite_shader.uniform1i("bloom2_tex", 5);

   screendisplay_shader.load(SHADERS_PATH "lines_ink_v.glsl", SHADERS_PATH "lines_ink_g.glsl", SHADERS_PATH "lines_ink_f.glsl");

   bgfg_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "bgfg_f.glsl");
   bgfg_shader.uniform1i("tex0", 0);
   bgfg_shader.uniform1i("tex1", 1);

   cloud_gen_shader.load(SHADERS_PATH "cloud2_gen_v.glsl", NULL, SHADERS_PATH "cloud2_gen_f.glsl");
   cloud_gen_shader.uniform1i("tex0",0);
   cloud_gen_shader.uniform1i("noise_tex",1);

   cloud_render_shader.load(SHADERS_PATH "cloud2_render_v.glsl", NULL, SHADERS_PATH "cloud2_render_f.glsl");
   cloud_render_shader.uniform1i("tex0",0);
   cloud_render_shader.uniform1i("noise_tex",1);
   cloud_render_shader.uniform1i("noise_tex_2d",2);
}

void TriangleScene::initRandomGrid()
{
   for(int y=0;y<random_grid_h;++y)
      for(int x=0;x<random_grid_w;++x)
      {
         random_grid[x+y*random_grid_w]=(frand()-0.5)*2.0;
      }
}

float TriangleScene::randomGridLookup(float u, float v) const
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

void TriangleScene::initialize()
{
   if(initialized)
      return;

   initialized = true;

   initializeTextures();
   initializeBuffers();

   initRandomGrid();

   initMountain();

   for(int i=0;i<num_drips/2;++i)
   {
      drips[i].pos=Vec2((frand()*2.0-1.0)*1.5,1+frand()*0.02);
      drips[i].r=mix(0.5,1.0,frand())*0.02;
   }

   for(int i=num_drips/2;i<num_drips;++i)
   {
      drips[i].pos=Vec2((frand()*2.0-1.0)*3.0,1+frand()*0.02);
      drips[i].r=0.005;
   }
}


void TriangleScene::initMountain()
{
   static const GLfloat width_scale=300.0;
   static const GLfloat height_scale=300.0;

   mountain_num_vertices = 0;
   mountain_num_indices = 0;

   for(int y=0;y<mountains_h;++y)
      for(int x=0;x<mountains_w;++x)
      {
         GLfloat u=GLfloat(x)/GLfloat(mountains_w);
         GLfloat v=GLfloat(y)/GLfloat(mountains_h);
         assert(mountain_num_vertices < mountains_max_vertices);
         mountain_vertices[mountain_num_vertices * 3 + 0] = (u - 0.5) * -2 * width_scale;
         mountain_vertices[mountain_num_vertices * 3 + 1] = (v - 0.5) * 2 * height_scale+120;
         mountain_vertices[mountain_num_vertices * 3 + 2] = evaluateMountainHeight(u, v);
         mountain_num_vertices++;
      }

   for(int y=0;y<mountains_h-1;++y)
      for(int x=0;x<mountains_w;++x)
      {
         assert(mountain_num_indices < mountains_max_indices);
         mountain_indices[mountain_num_indices++]=y*mountains_w+x;
         assert(mountain_num_indices < mountains_max_indices);
         mountain_indices[mountain_num_indices++]=(y+1)*mountains_w+x;
      }

   glGenBuffers(1, &mountain_vbo);
   glGenBuffers(1, &mountain_ebo);
   glBindBuffer(GL_ARRAY_BUFFER, mountain_vbo);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mountain_ebo);
   glBufferData(GL_ARRAY_BUFFER, mountain_num_vertices * 3 * sizeof(GLfloat), mountain_vertices, GL_STATIC_DRAW);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, mountain_num_indices * sizeof(GLushort), mountain_indices, GL_STATIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


   {
      Ran lrnd(556);
      int j=0;
      while(j<num_balls)
      {
         float u=lrnd.doub();
         float v=lrnd.doub();
         float h=evaluateMountainHeight(u, v);
         float p=lrnd.doub()*0.1f;
         if(p<h)
         {
            balls[j]=Vec3((u - 0.5) * -2 * width_scale,(v - 0.5) * 2 * height_scale+120,h);
            ++j;
         }
      }
   }
}

GLfloat TriangleScene::evaluateMountainHeight(GLfloat u, GLfloat v)
{
   GLfloat n=0.0f;
   GLfloat scale=16.0f,mag=0.5f;

   for(int i=0;i<10;++i)
   {
      n += randomGridLookup(u * scale, v * scale) * mag;
      mag *= 0.5f;
      scale *= 2.0f;
   }

   n*=1.0f-(1.0f-cubic(clamp(std::abs(u-0.5f)*5.0f,0.0f,1.0f)))*(1.0f-cubic(clamp((v-0.5f)*1.5f,0.0f,1.0f)));

   return n * 10 - 1e-2;
}

void TriangleScene::Drip::update()
{
   if(pos_history.size() >= 1024)
      return;

   pos_history.push_back(pos);
}

void TriangleScene::update()
{
   for(int i=0;i<num_drips;++i)
   {
      drips[i].update();
      drips[i].pos.y-=(drips[i].r+0.01)*0.1-randomGridLookup(drips[i].pos.x*16,drips[i].pos.y*16)*0.002+((i==0)?0.002:0.0);
      drips[i].pos.x+=(drips[i].r+0.01)*randomGridLookup(drips[i].pos.x*20,drips[i].pos.y*20)*0.2;
   }
}



void TriangleScene::createSpiral()
{
   for(int i=0;i<num_drips;++i)
   {
      spiral_num_vertices = 0;
      spiral_num_indices = 0;

      screendisplay_shader.uniform2f("radii", drips[i].r * drip_scale, drips[i].r * drip_scale);

      int j=0;
      const int sz = drips[i].pos_history.size();

      for(std::list<Vec2>::const_iterator it = drips[i].pos_history.begin(); it != drips[i].pos_history.end(); ++it)
      {
         spiral_indices[spiral_num_indices++] = spiral_num_vertices;

         spiral_vertices[spiral_num_vertices * 3 + 0] = it->x;
         spiral_vertices[spiral_num_vertices * 3 + 1] = it->y;
         spiral_vertices[spiral_num_vertices * 3 + 2] = 0;

         float s=std::pow(std::max(0.0f, float(j-(sz - 30)) / 30.0f),0.5);

         spiral_coords[spiral_num_vertices * 2 + 0] = 1.0+s*0.75;
         spiral_coords[spiral_num_vertices * 2 + 1] = 1.0;

         ++spiral_num_vertices;

         ++j;
         ++it;

         if(it == drips[i].pos_history.end())
            break;

         ++j;
         ++it;

         if(it == drips[i].pos_history.end())
            break;

         ++j;
      }

      spiral_indices[spiral_num_indices++] = spiral_num_vertices;

      spiral_vertices[spiral_num_vertices * 3 + 0] = drips[i].pos.x;
      spiral_vertices[spiral_num_vertices * 3 + 1] = drips[i].pos.y;
      spiral_vertices[spiral_num_vertices * 3 + 2] = 0;

      spiral_coords[spiral_num_vertices * 2 + 0] = 1.0;
      spiral_coords[spiral_num_vertices * 2 + 1] = 1.0;

      ++spiral_num_vertices;

      glDrawRangeElements(GL_LINE_STRIP, 0, spiral_num_vertices - 1, spiral_num_indices, GL_UNSIGNED_SHORT, spiral_indices);
   }

}


void TriangleScene::renderCloud(float ltime)
{
   static const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f };
   static const GLubyte indices[] = { 3, 0, 2, 1 };

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, noise_tex_2d);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_3D, noise_tex);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_3D, cloud_tex);
   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
   //glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
   glEnableVertexAttribArray(0);
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

void TriangleScene::generateCloud(float ltime)
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
   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
   CHECK_FOR_ERRORS;
}

template<typename T>
static inline T sq(const T& t)
{
   return t*t;
}

void TriangleScene::drawMountains(float ltime)
{
   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);

   glBindBuffer(GL_ARRAY_BUFFER, mountain_vbo);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mountain_ebo);

   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
   glEnableVertexAttribArray(0);

//glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);

   for(int y=0;y<mountains_h - 1;++y)
   {
      glDrawRangeElements(GL_TRIANGLE_STRIP, y * mountains_w, (y + 2) * mountains_w - 1, mountains_w * 2, GL_UNSIGNED_SHORT, (GLvoid*)(y * mountains_w * 2 * sizeof(GLushort)));
   }

//glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

   CHECK_FOR_ERRORS;

   glDisableVertexAttribArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void TriangleScene::drawView2(float ltime)
{
   lltime = ltime;

   const float aspect_ratio = float(window_height) / float(window_width);

   const float jitter_x = frand() / window_width * 3;
   const float jitter_y = frand() / window_height * aspect_ratio * 3;

   Mat4 modelview = Mat4::identity(), projection = Mat4::identity();

   const float fov = M_PI * 0.6f;

   const float znear = 1.0f / tan(fov * 0.5f), zfar = 4096.0f;

   const float frustum[4] = { -1.0f + jitter_x, +1.0f + jitter_x, -aspect_ratio + jitter_y, +aspect_ratio + jitter_y };

   projection = Mat4::frustum(frustum[0], frustum[1], frustum[2], frustum[3], znear, zfar) * projection;


   static const float velocity_time0 = 4.0;
   static const float velocity_value0 = 6.0/4.0;
   static const float velocity_time1 = 8.0;
   static const float velocity_time2 = 15.0;
   static const float velocity_value1 = 12.0/4.0;
   static const float vp0=(velocity_time0*velocity_time0*velocity_value0)*0.5f;
   static const float vp1=vp0+(velocity_time1-velocity_time0)*velocity_value0*velocity_time0;
   //static const float vp2=vp1+sq(velocity_time2-velocity_time1)*velocity_value1*0.5f;
   static const float vp2=vp1+sq(velocity_time2-velocity_time1)*velocity_value1*0.5f+(velocity_time2-velocity_time1)*velocity_value0*velocity_time0;

   const float vt=ltime-switch_time;
   float py=0;

   if(vt > velocity_time2)
      py=vp2+(vt-velocity_time2)*(velocity_value1*(velocity_time2-velocity_time1)+velocity_value0*velocity_time0);
   else if(vt > velocity_time1)
      py=vp1+sq(vt-velocity_time1)*velocity_value1*0.5f+(vt-velocity_time1)*velocity_value0*velocity_time0;
   else if(vt > velocity_time0)
      py=vp0+(vt-velocity_time0)*velocity_value0*velocity_time0;
   else
      py=sq(vt)*velocity_value0*0.5f;

   //modelview = Mat4::translation(Vec3(0.0, 0.0, -2.4));
   modelview = Mat4::translation(Vec3(0.0, -py, -0.95 - std::max(0.0f,(vt-15.0f)*0.75f)));
   const double ft=(ltime-switch_time)*0.25;
   const double f=(ltime-switch_time)*0.5;
   modelview = Mat4::rotation(-1.5*cubic(clamp(ft,0.0,1.0)), Vec3(1,0,0)) * modelview;
   modelview = Mat4::rotation(-0.4*cubic(clamp((vt-12.0f)*0.2f,0.0f,1.0f)), Vec3(1,0,0)) * modelview;
   modelview = Mat4::rotation((std::cos(ltime*4.0)*0.03+(randomGridLookup(0,ltime*10.0)-0.5)*0.02) * (cubic(clamp((vt-5.0f)*0.4f,0.0f,1.0f)) - cubic(clamp((vt-10.0f)*1.2f,0.0f,1.0f))), Vec3(0,0,1)) * modelview;
   modelview = Mat4::rotation(-0.1*(cubic(clamp((vt-16.0f)*0.4f,0.0f,1.0f)) - cubic(clamp((vt-20.0f)*0.4f,0.0f,1.0f))), Vec3(0,0,1)) * modelview;


   //modelview = modelview * Mat4::translation(Vec3(0, 0, -20));

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

   glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

   CHECK_FOR_ERRORS;

   forrest_shader.uniform2f("jitter", jitter_x, jitter_y);
   forrest_shader.uniform1f("time", ltime);
   forrest_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);


   CHECK_FOR_ERRORS;


   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, noise_tex_2d);


   forrest_shader.bind();
   forrest_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
   forrest_shader.uniform3f("colour",0,0,0);
   forrest_shader.uniform1f("fog",1);
   drawMountains(ltime);


   forrest_shader.bind();
   forrest_shader.uniform3f("colour",1,1,1);
   forrest_shader.uniform1f("fog",1);
   forrest_shader.uniform1f("flash",1.0f-cubic(clamp((ltime-switch_time)*0.2f,0.0f,1.0f)));

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, noise_tex_2d);

   forrest_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(2);

   road_mesh.bind();

   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
   glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(road_mesh.getVertexCount() * sizeof(GLfloat) * 3));

   glDrawRangeElements(GL_TRIANGLES, 0, road_mesh.getVertexCount() - 1, road_mesh.getTriangleCount() * 3, GL_UNSIGNED_INT, 0);

   tetrahedron_mesh.bind();

   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
   glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(tetrahedron_mesh.getVertexCount() * sizeof(GLfloat) * 3));

   forrest_shader.uniform3f("colour",4,4,4);
   for(int i=0;i<50;++i)
   {
      forrest_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (modelview * Mat4::translation(Vec3(-3,3+i*5,0)) * Mat4::scale(Vec3(0.5))).e);
      glDrawRangeElements(GL_TRIANGLES, 0, tetrahedron_mesh.getVertexCount() - 1, tetrahedron_mesh.getTriangleCount() * 3, GL_UNSIGNED_INT, 0);
      forrest_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (modelview * Mat4::translation(Vec3(+3,3+i*5,0)) * Mat4::scale(Vec3(0.5))).e);
      glDrawRangeElements(GL_TRIANGLES, 0, tetrahedron_mesh.getVertexCount() - 1, tetrahedron_mesh.getTriangleCount() * 3, GL_UNSIGNED_INT, 0);
   }

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(2);

   CHECK_FOR_ERRORS;


   glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
   glEnable(GL_BLEND);
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_CULL_FACE);
   glDepthMask(GL_FALSE);

   CHECK_FOR_ERRORS;

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, spiral_vertices);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, spiral_coords);
   //createSpiral();
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   CHECK_FOR_ERRORS;

   glDisable(GL_BLEND);


   cloud_render_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
   cloud_render_shader.uniformMatrix4fv("modelview_inv", 1, GL_FALSE, modelview.inverse().e);
   cloud_render_shader.uniform4f("frustum", frustum[0], frustum[1], frustum[2], frustum[3]);
   cloud_render_shader.uniform1f("znear", znear);
   cloud_render_shader.uniform1f("time", ltime - switch_time);
   CHECK_FOR_ERRORS;

   if(ltime > (switch_time + 1.0f))
      renderCloud(ltime);


   glUseProgram(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);


   glDepthMask(GL_TRUE);

   CHECK_FOR_ERRORS;
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void TriangleScene::gaussianBlur(Real tint_r, Real tint_g, Real tint_b, Real radx, Real rady, GLuint fbo0, GLuint fbo1, GLuint src_tex, GLuint tex0)
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

void TriangleScene::drawView(float ltime)
{
   lltime = ltime;

   const float aspect_ratio = float(window_height) / float(window_width);

   const float jitter_x = frand() / window_width * 3;
   const float jitter_y = frand() / window_height * aspect_ratio * 3;

   Mat4 modelview = Mat4::identity(), projection = Mat4::identity();

   //projection = Mat4::frustum(-1.0f + jitter_x, +1.0f + jitter_x, -aspect_ratio + jitter_y, +aspect_ratio + jitter_y, znear, zfar) * projection;
   projection = Mat4::translation(Vec3(jitter_x,jitter_y,0)); // Mat4::frustum(-1.0f + jitter_x, +1.0f + jitter_x, -aspect_ratio + jitter_y, +aspect_ratio + jitter_y, znear, zfar) * projection;

   const double scale_x = mix(1.0,19.0,cubic((clamp(ltime,10.0,14.0)-10.0)/4.0));
   const double scale_y = mix(1.0,19.0,cubic((clamp(ltime,10.0,14.0)-10.0)/4.0)) + mix(0.0,400.0,cubic((clamp(ltime,12.0,17.0)-12.0)/5.0));
   const double tx = 0.065 * cubic((clamp(ltime,7.0,13.0)-7.0)/6.0);

   drip_scale = scale_x;

   modelview = Mat4::scale(Vec3(scale_x,scale_y,1.0)) * Mat4::translation(Vec3(tx, 0.0, 0.0)) * modelview;


/*
   modelview = modelview * Mat4::rotation(cos(ltime*3) * 0.3 / ltime * 0.03, Vec3(0.0f, 0.0f, 1.0f));
   modelview = modelview * Mat4::rotation(cos(ltime * 15.0) * 0.01 / 128, Vec3(0.0f, 1.0f, 0.0f));
   modelview = modelview * Mat4::rotation(sin(ltime * 17.0) * 0.01 / 128  + cos(ltime*7) * 0.3 / ltime * 0.06, Vec3(1.0f, 0.0f, 0.0f));

   modelview = modelview * Mat4::translation(Vec3(0, 0, -40 - move_back + ltime * 0.2));
*/

   //modelview = modelview * Mat4::translation(Vec3(0, 0, -20));

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

   glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

   CHECK_FOR_ERRORS;

   forrest_shader.uniform3f("colour",1,1,1);
   forrest_shader.uniform1f("fog",0);
   forrest_shader.uniform1f("flash",1);
   forrest_shader.uniform2f("jitter", jitter_x, jitter_y);
   forrest_shader.uniform1f("time", ltime);
   forrest_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);
   screendisplay_shader.uniformMatrix4fv("projection", 1, GL_FALSE, (Mat4::scale(Vec3(0.5,1.0,1.0)) * projection).e);


   CHECK_FOR_ERRORS;



   forrest_shader.bind();

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, noise_tex_2d);

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(2);

   tetrahedron_mesh.bind();

   Mat4 modelview2 = modelview;

   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
   glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(tetrahedron_mesh.getVertexCount() * sizeof(GLfloat) * 3));

   forrest_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (modelview2 * Mat4::scale(Vec3(aspect_ratio * 0.5 * 2,0.8 * 2,1.0))).e);

   glDrawRangeElements(GL_TRIANGLES, 0, tetrahedron_mesh.getVertexCount() - 1, tetrahedron_mesh.getTriangleCount() * 3, GL_UNSIGNED_INT, 0);

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(2);

   CHECK_FOR_ERRORS;


   glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
   glEnable(GL_BLEND);
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_CULL_FACE);
   screendisplay_shader.bind();
   screendisplay_shader.uniform4f("colour", 0, 0, 0, 1);
   screendisplay_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (modelview * Mat4::scale(Vec3(0.2,1.0,1.0))).e);
   glDepthMask(GL_FALSE);

   CHECK_FOR_ERRORS;

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, spiral_vertices);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, spiral_coords);
   createSpiral();
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   CHECK_FOR_ERRORS;

   glDisable(GL_BLEND);




   glUseProgram(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);


   glDepthMask(GL_TRUE);

   CHECK_FOR_ERRORS;
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void TriangleScene::render()
{
//   time+=15;

   static const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f };
   static const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
   static const GLubyte indices[] = { 3, 0, 2, 1 };

   const float aspect_ratio = float(window_height) / float(window_width);

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

   if(time>switch_time)
      generateCloud(time);

   //for(int nn = 0; nn < 2; ++nn)
   {

      // ----- motion blur ------

      glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);
      glClear(GL_COLOR_BUFFER_BIT);

      //num_subframes = 16;
      num_subframes = (time>switch_time) ? 2 : 8;
      subframe_scale = 1.0/35.0;

      for(int subframe = 0;subframe<num_subframes;++subframe)
      {
         // draw the view

         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[6]);

         glDisable(GL_BLEND);

         const float ltime = time + subframe_scale * float(subframe) / float(num_subframes);
         if(ltime<switch_time)
            drawView(ltime);
         else
            drawView2(ltime);


         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);

         mb_accum_shader.bind();
         mb_accum_shader.uniform1f("subtime", float(subframe) / float(num_subframes));
         mb_accum_shader.uniform1f("time", ltime);
         mb_accum_shader.uniform1f("scale", 1.0f / float(num_subframes));

         glDepthMask(GL_FALSE);
         glDisable(GL_DEPTH_TEST);
         glEnable(GL_BLEND);
         glBlendFunc(GL_ONE, GL_ONE);

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
      const float l=std::pow(clamp(time-33.0f,0.0f,1.0f),2.0f);
      composite_shader.uniform3f("addcolour2",l,l,l);
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


void TriangleScene::free()
{
   glDeleteTextures(num_texs, texs);
   glDeleteFramebuffers(num_fbos, fbos);
}

