
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

class PreIntroScene: public Scene
{
   struct Creature
   {
      int type;
      int path_type;
      float path_offset;
      float speed;
      Vec2 path_origin;
      float plane;
      Vec2 position;
      Vec2 direction;
      Vec2 size;
   };

   struct Particle
   {
      int tet_idx;
      double px,py,pz;
      double ppx,ppy,ppz;
      double vx,vy,vz;
      double w;
   };

   struct Chain
   {
      int tet_idx;
      Real link_length;
      std::vector<Vec3> points;
   };

   static const int max_creatures=1024;
   Creature creatures[max_creatures];
   int num_creatures;

   static const int max_particles=4096;
   Particle particles[max_particles];
   int num_particles;

   static const int num_chains=64;
   Chain prev_chains[4][num_chains];
   Chain chains[num_chains];

   static const int max_creature_indices=max_creatures*6;
   static const int max_creature_vertices=max_creatures*4;
   GLuint creature_indices[max_creature_indices];
   GLfloat creature_vertices[max_creature_vertices*3];
   GLfloat creature_coords[max_creature_vertices*8];

   static const int mountains_w = 64;
   static const int mountains_h = 64;
   static const int mountains_max_vertices = mountains_w * mountains_h;
   static const int mountains_max_indices = mountains_w * mountains_h * 2;
   GLfloat mountain_vertices[mountains_max_vertices * 3];
   GLushort mountain_indices[mountains_max_indices];
   GLushort mountain_num_vertices, mountain_num_indices;
   GLuint mountain_vbo;
   GLuint mountain_ebo;

   static const int random_grid_w = 256;
   static const int random_grid_h = 256;

   float random_grid[random_grid_w*random_grid_h];

   void initRandomGrid();
   float randomGridLookup(float u, float v) const;

   GLfloat spiral_vertices[1024 * 4 * 3];
   GLfloat spiral_coords[1024 * 4 * 2];
   GLushort spiral_indices[1024 * 4 * 3];
   GLushort spiral_num_vertices, spiral_num_indices;

   Shader glass_shader, bokeh_shader, bokeh2_shader;
   Shader gaussbokeh_shader, composite_shader;
   Shader mb_accum_shader;
   Shader bgfg_shader;
   Shader screendisplay_shader;
   Shader background_shader;
   Shader mountain_shader;
   Shader creature_shader;

   Mesh tetrahedron_mesh;
   Mesh cube_mesh;
   Mesh ruin_mesh;

   static const int max_tetrahedra=4;

   Vec3 tetofs[max_tetrahedra];
   Mat4 tetrahedra_mats[max_tetrahedra];
   Mat4 tetrahedra_mats_inverse[max_tetrahedra];
   bool tetrahedra_broken[max_tetrahedra];

   bool drawing_view_for_background;
   int num_subframes;
   float subframe_scale;

   float lltime;
   float sub_frame_time;

   float fade;
   Vec3 bg_colour;

   static const int num_fbos = 16, num_texs = 16;

   GLuint fbos[num_fbos], texs[num_texs], renderbuffers[num_fbos];

   GLuint bokeh_temp_fbo;
   GLuint bokeh_temp_tex_1, bokeh_temp_tex_2;

   GLuint noise_tex, depth_tex;
   GLuint noise_tex_2d;

   GLuint tet_tex;

   GLuint creatures_ebo;

   void initializeTextures();
   void initializeShaders();
   void initializeBuffers();

   GLfloat evaluateMountainHeight(GLfloat u,GLfloat v);
   void initMountain();
   void initChains();
   void createSpiral();
   void drawParticles();
   void initCreatures();

   void updateCreatures(float ltime);
   void renderCreatures(float ltime);

   void moveChains(int tet=-1);
   void drawView(float ltime);
   void gaussianBlur(Real tint_r, Real tint_g, Real tint_b, Real radx, Real rady, GLuint fbo0, GLuint fbo1, GLuint src_tex, GLuint tex0);
   void pushPrevChains();

   public:
      PreIntroScene(): num_particles(0), tetrahedron_mesh(8, 8), cube_mesh(256, 256), ruin_mesh(2048*4, 2048*4)
      {
         mountain_num_vertices=0;
         mountain_num_indices=0;
         mountain_vbo=0;
         mountain_ebo=0;
         tet_tex=0;
         creatures_ebo =0;
      }

      ~PreIntroScene()
      {
         free();
      }

      void slowInitialize();
      void initialize();
      void render();
      void update();
      void free();
};

Scene* preIntroScene = new PreIntroScene();


static const Real tetrahedronPlanes[16]={0.000000, 0.000000, 0.816496, -0.816496,
-1.000000, 0.333334, 0.333333, 0.333333,
0.000000, 0.942809, -0.471405, -0.471405,
-0.666000, -0.444000, -0.111000, -0.111000};

static const float release_time = 50.0;


void PreIntroScene::slowInitialize()
{
   tet_tex = loadTexture(IMAGES_PATH "tet.png",true);
   initializeShaders();
}

void PreIntroScene::initializeTextures()
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

   glBindTexture(GL_TEXTURE_2D, 0);

   noise_tex = texs[7];
   depth_tex = texs[8];
   noise_tex_2d = texs[11];
   bokeh_temp_tex_1 = texs[12];
   bokeh_temp_tex_2 = texs[13];

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

   glBindTexture(GL_TEXTURE_2D, depth_tex);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, window_width, window_height);
   CHECK_FOR_ERRORS;
   glBindTexture(GL_TEXTURE_2D, 0);

}

void PreIntroScene::initializeBuffers()
{
   glGenFramebuffers(num_fbos, fbos);
   glGenRenderbuffers(num_fbos, renderbuffers);

   bokeh_temp_fbo = fbos[12];

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
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, depth_tex, 0);
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

void PreIntroScene::initializeShaders()
{
   creature_shader.load(SHADERS_PATH "creature_v.glsl", NULL, SHADERS_PATH "creature_f.glsl");

   glass_shader.load(SHADERS_PATH "glass2_v.glsl", NULL, SHADERS_PATH "glass2_f.glsl");
   glass_shader.uniform1i("tex0", 0);
   glass_shader.uniform1i("tex1", 1);

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

   screendisplay_shader.load(SHADERS_PATH "lines_chain_v.glsl", SHADERS_PATH "lines_chain_g.glsl", SHADERS_PATH "lines_chain_f.glsl");

   background_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "preintro_bg_f.glsl");
   background_shader.uniform1i("tex0", 0);

   mountain_shader.load(SHADERS_PATH "mountains_v.glsl", NULL, SHADERS_PATH "mountains_f.glsl");
   mountain_shader.uniform1i("tex0", 0);
}


static Vec2 evaluateCreaturePath(int type,int i,float t)
{
   switch(type)
   {
      case 0:
         return Vec2(-1.0f+std::cos(t)*0.15+t*0.1, std::sin(t)*0.15+std::cos(i*124.5+t*2)*0.02);
   }
   return Vec2(0,0);
}

static Vec2 normalize(const Vec2& v)
{
   return v * (Real(1) / std::sqrt(v.lengthSquared()));
}

/*
      int type;
      int path_type;
      float path_offset;
      float speed;
      Vec2 path_origin;
      float plane;
      Vec2 position;
      Vec2 direction;
      Vec2 size;
*/
void PreIntroScene::initCreatures()
{
   for(int i=0;i<max_creatures;++i)
   {
      creature_indices[i*6+0]=i*4+0;
      creature_indices[i*6+1]=i*4+2;
      creature_indices[i*6+2]=i*4+1;
      creature_indices[i*6+3]=i*4+1;
      creature_indices[i*6+4]=i*4+2;
      creature_indices[i*6+5]=i*4+3;

      creature_coords[i*8+0]=0.0f;
      creature_coords[i*8+1]=0.0f;
      creature_coords[i*8+2]=1.0f;
      creature_coords[i*8+3]=0.0f;
      creature_coords[i*8+4]=0.0f;
      creature_coords[i*8+5]=1.0f;
      creature_coords[i*8+6]=1.0f;
      creature_coords[i*8+7]=1.0f;

      Creature& c = creatures[i];
      c.path_offset=0;
      c.path_origin=Vec2(-16,0);
      c.plane=0;
      c.size=Vec2(1,1);
      c.type=0;
      c.speed=1;
      c.path_type=0;
   }

   glGenBuffers(1,&creatures_ebo);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,creatures_ebo);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, max_creature_indices*sizeof(GLuint), creature_indices, GL_STATIC_DRAW);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
}

void PreIntroScene::updateCreatures(float ltime)
{
   for(int i=0;i<max_creatures;++i)
   {
      Creature& c=creatures[i];
      const float t=ltime*c.speed+c.path_offset;
      const float teps=1e-3f;
      c.position=evaluateCreaturePath(c.path_type, i, t)+c.path_origin;
      c.direction=normalize(evaluateCreaturePath(c.path_type, i, t+teps) - evaluateCreaturePath(c.path_type, i, t-teps));

      GLfloat* vtx=creature_vertices+i*12;

      Vec2 tgn=Vec2(c.direction.y,-c.direction.x);
      Vec2 vs[4] = { c.position + c.direction * -c.size.y + tgn * -c.size.x,
                     c.position + c.direction * -c.size.y + tgn * +c.size.x,
                     c.position + c.direction * +c.size.y + tgn * -c.size.x,
                     c.position + c.direction * +c.size.y + tgn * +c.size.x };

      for(int j=0;j<4;++j)
      {
         vtx[j*3+0]=vs[j].x;
         vtx[j*3+1]=vs[j].y;
         vtx[j*3+2]=c.plane;
      }
   }
}

void PreIntroScene::renderCreatures(float ltime)
{
   creature_shader.bind();
   glBindBuffer(GL_ARRAY_BUFFER,0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,creatures_ebo);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(2);
   glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,creature_vertices);
   glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,0,creature_coords);
   glDrawElements(GL_TRIANGLES, max_creatures * 6, GL_UNSIGNED_INT, 0);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(2);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
}

void PreIntroScene::initChains()
{
   frand();
   for(int tet=0;tet<max_tetrahedra;++tet)
   {
      tetofs[tet].x=(frand()-0.5)*7;
      tetofs[tet].y=(frand()-0.5)*3;
      tetofs[tet].z=-1.3-tet*1.2;
      tetrahedra_mats[tet]=Mat4::translation(tetofs[tet]);
      tetrahedra_mats_inverse[tet]=tetrahedra_mats[tet].inverse();
      tetrahedra_broken[tet]=false;
   }

   for(int i=0;i<num_chains;++i)
   {
      chains[i].tet_idx=std::max(0,std::min(max_tetrahedra-1,(i*max_tetrahedra)/num_chains));
      float a=frand()*M_PI*2;
      float cx=tetofs[chains[i].tet_idx].x+std::cos(a)*2;
      float cz=tetofs[chains[i].tet_idx].z+std::sin(a)*2;
      int num_p=64;
      chains[i].link_length=3.0/float(num_p);
      for(int j=0;j<num_p;++j)
         chains[i].points.push_back(Vec3(cx,mix(-4.0f,6.0f,float(j)/float(num_p)),cz));
   }

   for(int n=0;n<600;++n)
      for(int i=0;i<num_chains;++i)
      {
         Chain& c = chains[i];

         for(int j=0;j<c.points.size();++j)
         {
            Vec3& p=c.points[j];

            bool outside_all=true;
            const int t=chains[i].tet_idx;
            {
               Vec3 p2=tetrahedra_mats_inverse[t]*p;
               bool outside=false;
               int nearest_plane=-1;
               Real nearest_plane_dist=-1e5;
               for(int pl=0;pl<4;++pl)
               {
                  const Real* pln=tetrahedronPlanes+pl;
                  const Real dist=pln[0]*p2.x+pln[4]*p2.y+pln[8]*p2.z+pln[12];
                  if(dist > 0)
                     outside=true;
                  else if(dist>nearest_plane_dist)
                  {
                     nearest_plane_dist=dist;
                     nearest_plane=pl;
                  }
               }
               if(outside)
               {
                  Real zpl=std::sqrt(p2.x*p2.x+p2.z*p2.z);

                  if(zpl>-1e3)
                  {
                     p2.x-=p2.x/zpl*0.005;
                     p2.z-=p2.z/zpl*0.005;
                  }
                  p=tetrahedra_mats[t]*p2;
               }
               else if(nearest_plane > -1)
               {
                  outside_all=false;
                  const Real e=nearest_plane_dist*-0.9;
                  p2.x+=tetrahedronPlanes[nearest_plane+0]*e;
                  p2.y+=tetrahedronPlanes[nearest_plane+4]*e;
                  p2.z+=tetrahedronPlanes[nearest_plane+8]*e;
                  p=tetrahedra_mats[t]*p2;
               }
            }
         }
      }

   for(int n=0;n<50;++n)
      moveChains();
}

void PreIntroScene::initRandomGrid()
{
   for(int y=0;y<random_grid_h;++y)
      for(int x=0;x<random_grid_w;++x)
      {
         random_grid[x+y*random_grid_w]=(frand()-0.5)*2.0;
      }
}

float PreIntroScene::randomGridLookup(float u, float v) const
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

void PreIntroScene::pushPrevChains()
{
   for(int n=2;n>=0;--n)
      for(int i=0;i<num_chains;++i)
         prev_chains[n+1][i]=prev_chains[n][i];
   for(int i=0;i<num_chains;++i)
      prev_chains[0][i]=chains[i];
}

void PreIntroScene::initialize()
{
   if(initialized)
      return;

   initialized = true;

   initializeTextures();
   initializeBuffers();

   cube_mesh.generateCube();
   tetrahedron_mesh.generateTetrahedron();
   tetrahedron_mesh.transform(Mat4::scale(Vec3(0.999)));
   tetrahedron_mesh.generateNormals();

   {
      Ran lrnd(777);
      for(int i=0;i<32;++i)
      {
         ruin_mesh.addInstance(cube_mesh, Mat4::translation(Vec3(-32+lrnd.doub()*64,0,-20-lrnd.doub()*100)) * Mat4::rotation(mix(-0.2,+0.2,lrnd.doub()),Vec3(0,0,1)) * Mat4::scale(Vec3(0.2-lrnd.doub()*0.1,100,0.2)));
      }
   }

   ruin_mesh.separate();
   ruin_mesh.generateNormals();
   //ruin_mesh.transform(Mat4::scale(Vec3(0.01)));

   initRandomGrid();
   initChains();
   for(int n=0;n<4;++n)
      for(int i=0;i<num_chains;++i)
         prev_chains[n][i]=chains[i];

   initMountain();
   initCreatures();
}

void PreIntroScene::initMountain()
{
   static const GLfloat width_scale=150.0;
   static const GLfloat height_scale=150.0;
   static const GLfloat depth_scale=1.0;

   mountain_num_vertices = 0;
   mountain_num_indices = 0;

   for(int y=0;y<mountains_h;++y)
      for(int x=0;x<mountains_w;++x)
      {
         GLfloat u=GLfloat(x)/GLfloat(mountains_w);
         GLfloat v=GLfloat(y)/GLfloat(mountains_h);
         assert(mountain_num_vertices < mountains_max_vertices);
         mountain_vertices[mountain_num_vertices * 3 + 0] = (u - 0.5) * 2 * width_scale;
         mountain_vertices[mountain_num_vertices * 3 + 1] = evaluateMountainHeight(u, v) * depth_scale;
         mountain_vertices[mountain_num_vertices * 3 + 2] = (v - 0.5) * 2 * height_scale;
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
}

GLfloat PreIntroScene::evaluateMountainHeight(GLfloat u, GLfloat v)
{
   GLfloat n=0.0f;
   GLfloat scale=16.0f,mag=4.0f;

   for(int i=0;i<10;++i)
   {
      n += randomGridLookup(u * scale, v * scale) * mag;
      mag *= 0.5f;
      scale *= 2.0f;
   }

   return n-5.0;
}

void PreIntroScene::moveChains(int tet)
{

   const Real damp=0.1;

   for(int i=0;i<num_chains;++i)
   {
      Chain& c = chains[i];

      if(tet>-1 && tet!=c.tet_idx)
         continue;

      for(int j=1;j<c.points.size();++j)
      {
         Vec3 d=c.points[j] - c.points[j-1];
         Real dl=std::sqrt(d.lengthSquared());
         Real diff=c.link_length-dl;
         if(std::abs(dl) > 1e-3 && std::abs(diff) > 1e-3)
         {
            c.points[j-1]-=d*diff/dl*0.5*damp;
            c.points[j]+=d*diff/dl*0.5*damp;
         }
      }

      for(int j=0;j<c.points.size();++j)
      {
         Vec3& p=c.points[j];

         p.x-=0.001*randomGridLookup(g_ltime*0.01,i/2.0+j*0.1);
         p.z-=0.001*randomGridLookup(g_ltime*0.02,i/2.0+j*0.1);
         p.y-=0.002*(2.0+randomGridLookup(0,i));

         bool outside_all=true;
         const int t=chains[i].tet_idx;
         {
            Vec3 p2=tetrahedra_mats_inverse[t]*p;
            bool outside=false;
            int nearest_plane=-1;
            Real nearest_plane_dist=-1e4;
            for(int pl=0;pl<4;++pl)
            {
               const Real* pln=tetrahedronPlanes+pl;
               const Real dist=pln[0]*p2.x+pln[4]*p2.y+pln[8]*p2.z+pln[12];
               if(dist > 0)
                  outside=true;
               else if(dist>nearest_plane_dist)
               {
                  nearest_plane_dist=dist;
                  nearest_plane=pl;
               }
            }
            Real zpl=std::sqrt(p2.x*p2.x+p2.z*p2.z);

            if(zpl>-1e3)
            {
               p2.x+=p2.x/zpl*0.005;
               p2.z+=p2.z/zpl*0.005;
            }
            if(!outside && (nearest_plane > -1))
            {
               outside_all=false;

               const Real e=nearest_plane_dist*-0.9;
               p2.x+=tetrahedronPlanes[nearest_plane+0]*e;
               p2.y+=tetrahedronPlanes[nearest_plane+4]*e;
               p2.z+=tetrahedronPlanes[nearest_plane+8]*e;
               p=tetrahedra_mats[t]*p2;
            }
         }

      }

   }
}

void PreIntroScene::update()
{
   for(int tet=0;tet<max_tetrahedra;++tet)
   {
      const float rt=release_time+5*tet;
      if(g_ltime > (rt*100))
      {
         Vec3 o=tetofs[tet]+Vec3(0.0,std::pow((g_ltime-(rt*100))*0.0015,2.5),0.0);
         tetrahedra_mats[tet]=Mat4::translation(o)*Mat4::rotation(std::pow((g_ltime-(rt*100))*0.00074,3.0)*tet,
                                    Vec3(1.0,0.0,0.0))*Mat4::rotation(std::pow((g_ltime-(rt*100))*0.00075,3.0)*(tet-2),Vec3(0.0,0.0,1.0));
         tetrahedra_mats_inverse[tet]=tetrahedra_mats[tet].inverse();

         if(!tetrahedra_broken[tet])
         {
            tetrahedra_broken[tet]=true;
            for(int i=0;i<num_chains;++i)
            {
               Chain& c = chains[i];

               if(tet==c.tet_idx)
               {
                  for(int j=0;j<16;++j)
                  {
                     assert(num_particles < max_particles);
                     Particle& p=particles[num_particles++];
                     p.tet_idx = tet;
                     p.px=tetofs[tet].x;
                     p.py=(pow(frand(),0.3)-0.5)*8.0;
                     p.pz=tetofs[tet].z;
                     p.ppx=p.px;
                     p.ppy=p.py;
                     p.ppz=p.pz;
                     const float ang=frand()*M_PI*2;
                     p.vx=std::cos(ang)*0.001;
                     p.vy=0;
                     p.vz=std::sin(ang)*0.001;
                     p.w=mix(0.4,1.0,frand());
                  }
               }
            }
         }

         moveChains(tet);
      }
   }

   for(int i=0;i<num_particles;++i)
   {
      Particle& p=particles[i];
      p.px+=p.vx;
      p.py+=p.vy;
      p.pz+=p.vz;
      p.vx*=0.999*mix(0.8,1.0,p.w);
      p.vy+=0.00001*p.w;
      p.vz*=0.999*mix(0.8,1.0,p.w);

      const int t=p.tet_idx;
      {
         Vec3 p2=tetrahedra_mats_inverse[t]*Vec3(p.px,p.py,p.pz);
         bool outside=false;
         int nearest_plane=-1;
         Real nearest_plane_dist=-1e4;
         for(int pl=0;pl<4;++pl)
         {
            const Real* pln=tetrahedronPlanes+pl;
            const Real dist=pln[0]*p2.x+pln[4]*p2.y+pln[8]*p2.z+pln[12];
            if(dist > 0)
               outside=true;
            else if(dist>nearest_plane_dist)
            {
               nearest_plane_dist=dist;
               nearest_plane=pl;
            }
         }
         if(!outside)
         {
            p2.x-=tetrahedronPlanes[0+nearest_plane]*nearest_plane_dist;
            p2.y-=tetrahedronPlanes[4+nearest_plane]*nearest_plane_dist;
            p2.z-=tetrahedronPlanes[8+nearest_plane]*nearest_plane_dist;
            p2=tetrahedra_mats[t]*p2;
            p.px=p2.x;
            p.py=p2.y;
            p.pz=p2.z;
         }
      }
   }

   ++g_ltime;
}

void PreIntroScene::drawParticles()
{
   screendisplay_shader.uniform4f("colour", 0.1, 0.5, 0.5, 1);

   for(int i=0;i<num_particles;++i)
   {
      const Particle& p=particles[i];

      if(p.py>8.0)
         continue;

      spiral_num_vertices = 0;
      spiral_num_indices = 0;

      screendisplay_shader.uniform2f("radii", 0.001, 0.001);


      spiral_indices[spiral_num_indices++] = spiral_num_vertices;

      spiral_vertices[spiral_num_vertices * 3 + 0] = mix(p.ppx, p.px, sub_frame_time);
      spiral_vertices[spiral_num_vertices * 3 + 1] = mix(p.ppy, p.py, sub_frame_time);
      spiral_vertices[spiral_num_vertices * 3 + 2] = mix(p.ppz, p.pz, sub_frame_time);

      spiral_coords[spiral_num_vertices * 2 + 0] = 1.0;
      spiral_coords[spiral_num_vertices * 2 + 1] = 1.0;

      ++spiral_num_vertices;


      spiral_indices[spiral_num_indices++] = spiral_num_vertices;

      spiral_vertices[spiral_num_vertices * 3 + 0] = mix(p.ppx, p.px, sub_frame_time)-p.vx*2;
      spiral_vertices[spiral_num_vertices * 3 + 1] = mix(p.ppy, p.py, sub_frame_time)-p.vy*2;
      spiral_vertices[spiral_num_vertices * 3 + 2] = mix(p.ppz, p.pz, sub_frame_time)-p.vz*2;

      spiral_coords[spiral_num_vertices * 2 + 0] = 1.0;
      spiral_coords[spiral_num_vertices * 2 + 1] = 1.0;

      ++spiral_num_vertices;

      glDrawRangeElements(GL_LINE_STRIP, 0, spiral_num_vertices - 1, spiral_num_indices, GL_UNSIGNED_SHORT, spiral_indices);
   }

}

void PreIntroScene::createSpiral()
{
   screendisplay_shader.uniform4f("colour", 0.0, 0.0, 0.0, 1);

   for(int i=0;i<num_chains;++i)
   {
      spiral_num_vertices = 0;
      spiral_num_indices = 0;

      screendisplay_shader.uniform2f("radii", 0.0025, 0.0025);

      int j=0;
      for(std::vector<Vec3>::const_iterator it = chains[i].points.begin(); it != chains[i].points.end(); ++it,++j)
      {
         if(it->y > -6)
         {
            spiral_indices[spiral_num_indices++] = spiral_num_vertices;

            spiral_vertices[spiral_num_vertices * 3 + 0] = mix(it->x, prev_chains[3][i].points[j].x, sub_frame_time);
            spiral_vertices[spiral_num_vertices * 3 + 1] = mix(it->y, prev_chains[3][i].points[j].y, sub_frame_time);
            spiral_vertices[spiral_num_vertices * 3 + 2] = mix(it->z, prev_chains[3][i].points[j].z, sub_frame_time);

            spiral_coords[spiral_num_vertices * 2 + 0] = 1.0;
            spiral_coords[spiral_num_vertices * 2 + 1] = 1.0;

            ++spiral_num_vertices;
         }
      }

      glDrawRangeElements(GL_LINE_STRIP, 0, spiral_num_vertices - 1, spiral_num_indices, GL_UNSIGNED_SHORT, spiral_indices);
   }
}

void PreIntroScene::drawView(float ltime)
{
   static const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f };
   static const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
   static const GLubyte indices[] = { 3, 0, 2, 1 };

   fade=cubic(clamp((ltime-10.0)/11.0,0.0,1.0));
   bg_colour=mix(Vec3(0.0,0.005,0.002),Vec3(0.0,0.01,0.02),mix(0.4,0.6,0.5+0.5*std::sin(ltime*0.14)*std::cos(ltime)))*1.2*1.7*fade;

   lltime = ltime;

   const float aspect_ratio = float(window_height) / float(window_width);

   const float jitter_x = frand() / window_width * 3;
   const float jitter_y = frand() / window_height * aspect_ratio * 3;

   Mat4 modelview = Mat4::identity(), projection = Mat4::identity();

   const float fov = M_PI * 0.3f;

   const float znear = 0.5f / tan(fov * 0.5f), zfar = 1024.0f;

   const float frustum[4] = { -1.0f + jitter_x, +1.0f + jitter_x, -aspect_ratio + jitter_y, +aspect_ratio + jitter_y };

   projection = Mat4::frustum(frustum[0], frustum[1], frustum[2], frustum[3], znear, zfar) * projection;

   const float movement=cubic(std::min(1.0f, std::max(0.0f, ltime-10.0f) * 0.021f));
   modelview = modelview * Mat4::translation(Vec3(16-movement*16+1.6, -1.5, -3.5));

   modelview = modelview * Mat4::rotation(std::max(-0.7,0.2-std::pow(std::max(0.0f,ltime-release_time)*0.0045,2.2)), Vec3(1.0f, 0.0f, 0.0f));
   modelview = modelview * Mat4::rotation(-0.1+ltime*0.008, Vec3(0.0f, 1.0f, 0.0f));

   modelview = Mat4::rotation(-0.3 * cubic(std::min(1.0f, std::max(0.0f, ltime - (1 * 60 + 10.0f)) * 0.1f)), Vec3(1.0f, 0.0f, 0.0f)) * modelview;
   modelview = Mat4::rotation(-0.1 * cubic(std::min(1.0f, std::max(0.0f, ltime - (1 * 60 + 11.0f)) * 0.2f)), Vec3(0.0f, 1.0f, 0.0f)) * modelview;

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
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

   glass_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);
   screendisplay_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);
   mountain_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);


   CHECK_FOR_ERRORS;


   glUseProgram(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

   glDisable(GL_BLEND);


   glDepthMask(GL_FALSE);
   glEnable(GL_DEPTH_TEST);
   if(!drawing_view_for_background)
   {
      background_shader.bind();
      background_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
      background_shader.uniform4f("frustum", frustum[0], frustum[1], frustum[2], frustum[3]);
      background_shader.uniform1f("znear", znear);
      background_shader.uniform3f("colour", bg_colour.x, bg_colour.y, bg_colour.z);

      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, noise_tex_2d);

      glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);

      glDisableVertexAttribArray(0);
      glDisableVertexAttribArray(1);

      CHECK_FOR_ERRORS;

   }

   glDepthMask(GL_TRUE);
   glEnable(GL_DEPTH_TEST);
   if(!drawing_view_for_background)
   {
      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(2);

      glass_shader.bind();
      glass_shader.uniform1f("background",drawing_view_for_background?1.0f:0.0f);
      glass_shader.uniform1f("rayjitter",frand());
      glass_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
      glass_shader.uniformMatrix4fv("modelview_inv", 1, GL_FALSE, modelview.inverse().e);
      glass_shader.uniform4f("frustum", frustum[0], frustum[1], frustum[2], frustum[3]);
      glass_shader.uniform1f("znear", znear);

      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D,tet_tex);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_3D,noise_tex);

      Mesh* mesh=&tetrahedron_mesh;

      mesh->bind();

      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
      glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(mesh->getVertexCount() * sizeof(GLfloat) * 3));

      glass_shader.uniformMatrix4fv("tetrahedra_mats[0]", 1, GL_FALSE, tetrahedra_mats[0].e);
      glass_shader.uniformMatrix4fv("tetrahedra_mats_inv[0]", 1, GL_FALSE, tetrahedra_mats_inverse[0].e);
      glass_shader.uniformMatrix4fv("tetrahedra_mats[1]", 1, GL_FALSE, tetrahedra_mats[1].e);
      glass_shader.uniformMatrix4fv("tetrahedra_mats_inv[1]", 1, GL_FALSE, tetrahedra_mats_inverse[1].e);
      glass_shader.uniformMatrix4fv("tetrahedra_mats[2]", 1, GL_FALSE, tetrahedra_mats[2].e);
      glass_shader.uniformMatrix4fv("tetrahedra_mats_inv[2]", 1, GL_FALSE, tetrahedra_mats_inverse[2].e);
      glass_shader.uniformMatrix4fv("tetrahedra_mats[3]", 1, GL_FALSE, tetrahedra_mats[3].e);
      glass_shader.uniformMatrix4fv("tetrahedra_mats_inv[3]", 1, GL_FALSE, tetrahedra_mats_inverse[3].e);

      for(int tet=0;tet<max_tetrahedra;++tet)
      {
         glass_shader.uniformMatrix4fv("object", 1, GL_FALSE, tetrahedra_mats[tet].e);
         glDrawRangeElements(GL_TRIANGLES, 0, mesh->getVertexCount() - 1, mesh->getTriangleCount() * 3, GL_UNSIGNED_INT, 0);
      }

      CHECK_FOR_ERRORS;


      glDisableVertexAttribArray(0);
      glDisableVertexAttribArray(2);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
   }

   if(!drawing_view_for_background)
   {
      glEnable(GL_DEPTH_TEST);
      glDepthMask(GL_TRUE);
      mountain_shader.bind();
      mountain_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
      mountain_shader.uniform3f("colour", bg_colour.x, bg_colour.y, bg_colour.z);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_3D,noise_tex);

      glBindBuffer(GL_ARRAY_BUFFER, mountain_vbo);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mountain_ebo);

      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
      glEnableVertexAttribArray(0);

      for(int y=0;y<mountains_h - 1;++y)
      {
         glDrawRangeElements(GL_TRIANGLE_STRIP, y * mountains_w, (y + 2) * mountains_w - 1, mountains_w * 2, GL_UNSIGNED_SHORT, (GLvoid*)(y * mountains_w * 2 * sizeof(GLushort)));
      }

      {
         Mesh* mesh = &ruin_mesh;
         mesh->bind();
         glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
         glDrawRangeElements(GL_TRIANGLES, 0, mesh->getVertexCount() - 1, mesh->getTriangleCount() * 3, GL_UNSIGNED_INT, 0);
      }

      CHECK_FOR_ERRORS;

      glDisableVertexAttribArray(0);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
   }

   if(!drawing_view_for_background)
   {
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_BLEND);
      glEnable(GL_DEPTH_TEST);
      glDisable(GL_CULL_FACE);
      screendisplay_shader.bind();
      screendisplay_shader.uniform4f("colour", 0.0, 0.0, 0.0, 1);
      screendisplay_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
      glDepthMask(GL_FALSE);

      CHECK_FOR_ERRORS;

      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, spiral_vertices);
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, spiral_coords);
      drawParticles();
      createSpiral();
      glDisableVertexAttribArray(0);
      glDisableVertexAttribArray(1);

      CHECK_FOR_ERRORS;

      glDisable(GL_BLEND);

      CHECK_FOR_ERRORS;
   }

   if(!drawing_view_for_background)
   {
      creature_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
      creature_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);
      renderCreatures(ltime);
      CHECK_FOR_ERRORS;
   }

   glDepthMask(GL_TRUE);

   CHECK_FOR_ERRORS;
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void PreIntroScene::gaussianBlur(Real tint_r, Real tint_g, Real tint_b, Real radx, Real rady, GLuint fbo0, GLuint fbo1, GLuint src_tex, GLuint tex0)
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


void PreIntroScene::render()
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
         // draw the view

         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[6]);

         glDisable(GL_BLEND);

         drawView(time + subframe_scale * float(subframe) / float(num_subframes));


         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[3]);

         sub_frame_time=float(subframe) / float(num_subframes);

         mb_accum_shader.bind();
         mb_accum_shader.uniform1f("time", sub_frame_time);
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

         const float CoC = 0.02;// + sin(time) * 0.05;

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
   gaussianBlur(1, 1, 1, 0.2, 0.2, fbos[5], fbos[4], texs[4], texs[5]);

   CHECK_FOR_ERRORS;


   // ----- composite ------


   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
   glViewport(0, 0, window_width, window_height);


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

   pushPrevChains();

   for(int i=0;i<num_particles;++i)
   {
      Particle& p=particles[i];

      p.ppx=p.px;
      p.ppy=p.py;
      p.ppz=p.pz;
   }
}


void PreIntroScene::free()
{
   glDeleteBuffers(1,&mountain_ebo);
   glDeleteBuffers(1,&mountain_vbo);
   glDeleteBuffers(1,&creatures_ebo);
   glDeleteTextures(1,&tet_tex);
   glDeleteTextures(num_texs, texs);
   glDeleteFramebuffers(num_fbos, fbos);
}

