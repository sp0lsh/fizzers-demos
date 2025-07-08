
// midnight platonic

#include "Engine.hpp"
#include "Bezier.hpp"
#include "Ran.hpp"
#include "Particles.hpp"
#include <omp.h>

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

class CaveScene: public Scene
{
   Shader cave_shader;
   Shader gaussbokeh_shader, composite_shader;
   Shader mb_accum_shader;
   Shader bgfg_shader;
   Shader water_shader;
   Shader tet_shader;

   int num_subframes;
   float subframe_scale;

   float lltime;

   Mesh tetrahedron_mesh;
   Mesh tetrahedron2_mesh;

   static const int max_tetrahedra=4;

   Mat4 tetrahedra_mats[max_tetrahedra];


   static const int num_fbos = 16, num_texs = 16;

   static const int cave_mesh_num_u=512;
   static const int cave_mesh_num_v=1024;

   static const GLuint num_cavemesh_indices=(cave_mesh_num_v-1)*cave_mesh_num_u*2;
   static const GLuint num_cavemesh_vertices=cave_mesh_num_u*cave_mesh_num_v;

   GLuint cavemesh_indices[num_cavemesh_indices];
   GLfloat cavemesh_vertices[num_cavemesh_vertices * 3];
   GLfloat cavemesh_coords[num_cavemesh_vertices * 2];
   GLfloat cavemesh_normals[num_cavemesh_vertices * 3];

   GLuint cavemesh_vbo, cavemesh_ebo;

   static const int water_mesh_num_u=512;
   static const int water_mesh_num_v=512;

   static const GLuint num_watermesh_indices=(water_mesh_num_v-1)*water_mesh_num_u*2;
   static const GLuint num_watermesh_vertices=water_mesh_num_u*water_mesh_num_v;

   GLuint watermesh_indices[num_watermesh_indices];
   GLfloat watermesh_vertices[num_watermesh_vertices * 3];

   GLuint watermesh_vbo, watermesh_ebo;

   GLuint fbos[num_fbos], texs[num_texs], renderbuffers[num_fbos];

   GLuint noise_tex;
   GLuint noise_tex_2d;

   GLuint tet_tex;

   static const int random_grid_w = 256;
   static const int random_grid_h = 256;

   float random_grid[random_grid_w*random_grid_h];

   Vec3 tetrahedronPath(int tet,float ltime);

   void initRandomGrid();
   float randomGridLookup(float u, float v, int w=random_grid_w, int h=random_grid_h) const;

   void initCaveMesh();
   void initWaterMesh();

   void drawCave(float ltime);
   void drawTetrahedra(const Mat4& modelview, float ltime);
   void drawWater(float ltime);

   Vec3 evaluateCaveSurface(float u,float v);
   Vec3 evaluateWaterSurface(float u,float v);
   void slowInitialize();
   void initializeTextures();
   void initializeShaders();
   void initializeBuffers();

   void drawView(float ltime);
   void gaussianBlur(Real tint_r, Real tint_g, Real tint_b, Real radx, Real rady, GLuint fbo0, GLuint fbo1, GLuint src_tex, GLuint tex0);


   public:
      CaveScene(): cavemesh_vbo(0), cavemesh_ebo(0), watermesh_vbo(0), watermesh_ebo(0), tetrahedron_mesh(32, 32), tetrahedron2_mesh(64, 64)
      {
      }

      ~CaveScene()
      {
         free();
      }


      void initialize();
      void render();
      void update();
      void free();
};

Scene* caveScene = new CaveScene();

template<typename T>
static T sq(const T& x)
{
   return x*x;
}

void CaveScene::initRandomGrid()
{
   for(int y=0;y<random_grid_h;++y)
      for(int x=0;x<random_grid_w;++x)
      {
         random_grid[x+y*random_grid_w]=(frand()-0.5)*2.0;
      }
}

float CaveScene::randomGridLookup(float u, float v, int w, int h) const
{
   if(w>random_grid_w)
      w=random_grid_w;

   if(h>random_grid_h)
      h=random_grid_h;

   u=fmodf(u,float(w));
   v=fmodf(v,float(h));

   if(u<0)
      u+=w;

   if(v<0)
      v+=h;

   const int iu=int(u);
   const int iv=int(v);

   const float fu=u-float(iu);
   const float fv=v-float(iv);

   const float a=random_grid[iu+iv*w];
   const float b=random_grid[((iu+1)%w)+iv*w];
   const float c=random_grid[((iu+1)%w)+((iv+1)%h)*w];
   const float d=random_grid[iu+((iv+1)%h)*w];

   return (1.0-fv)*((1.0-fu)*a+fu*b) + fv*((1.0-fu)*d+fu*c);
}

Vec3 CaveScene::evaluateWaterSurface(float u,float v)
{
   return Vec3(u*2.0-1.0,0,v*2.0-1.0);
}

Vec3 CaveScene::evaluateCaveSurface(float u,float v)
{
   v*=2.0;

   float l=0.1f+std::pow(cubic(clamp(v/0.6f,0.0f,1.0f)),mix(1.0f,0.2f,std::pow(v,0.15f)));

   float f=0.0f;
   for(int i=0;i<10;++i)
   {
      f+=randomGridLookup((u*64)*std::pow(2.0f,float(i)),(v*8)*std::pow(2.0f,float(i)),64<<i,8<<i)/std::pow(2.0f,float(i+1));
   }

   float y=1.0f-v;

   l-=f*0.18*cubic(clamp(v/0.6f,0.0f,1.0f));
   y-=f*0.1*cubic(clamp(v/0.6f,0.0f,1.0f));

   l+=cubic(clamp((v-0.9f)/0.1f,0.0f,1.0f))*0.075f;
   float a=u*M_PI*2;
   return Vec3(std::cos(a)*l,y,std::sin(a)*l);
}

void CaveScene::initCaveMesh()
{
   Mat4 cave_xfrm=Mat4::identity();

   for(int v=0;v<cave_mesh_num_v;++v)
   {
      for(int u=0;u<cave_mesh_num_u;++u)
      {
         float fu=float(u)/float(cave_mesh_num_u-1);
         float fv=float(v)/float(cave_mesh_num_v-1);

         static const float eps=1e-3;

         Vec3 p0 = cave_xfrm * evaluateCaveSurface(fu,fv);
         Vec3 p1 = cave_xfrm * evaluateCaveSurface(fu+eps,fv);
         Vec3 p2 = cave_xfrm * evaluateCaveSurface(fu,fv+eps);

         Vec3 nrm=(p1-p0).cross(p2-p0);
         float nrml=std::sqrt(nrm.lengthSquared());
         assert(nrml>0.0f);
         nrm=nrm*1.0f/nrml;

         cavemesh_coords[(u+v*cave_mesh_num_u)*2+0]=fu;
         cavemesh_coords[(u+v*cave_mesh_num_u)*2+1]=fv;

         cavemesh_vertices[(u+v*cave_mesh_num_u)*3+0]=p0.x;
         cavemesh_vertices[(u+v*cave_mesh_num_u)*3+1]=p0.y;
         cavemesh_vertices[(u+v*cave_mesh_num_u)*3+2]=p0.z;

         cavemesh_normals[(u+v*cave_mesh_num_u)*3+0]=nrm.x;
         cavemesh_normals[(u+v*cave_mesh_num_u)*3+1]=nrm.y;
         cavemesh_normals[(u+v*cave_mesh_num_u)*3+2]=nrm.z;

         if(v<(cave_mesh_num_v-1))
         {
            cavemesh_indices[(u+v*cave_mesh_num_u)*2+0]=u+(v+0)*cave_mesh_num_u;
            cavemesh_indices[(u+v*cave_mesh_num_u)*2+1]=u+(v+1)*cave_mesh_num_u;
         }
      }
   }

   glGenBuffers(1,&cavemesh_vbo);
   glGenBuffers(1,&cavemesh_ebo);

   glBindBuffer(GL_ARRAY_BUFFER,cavemesh_vbo);
   CHECK_FOR_ERRORS;
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,cavemesh_ebo);
   CHECK_FOR_ERRORS;

   glBufferData(GL_ARRAY_BUFFER,num_cavemesh_vertices*8*sizeof(GLfloat),0,GL_STATIC_DRAW);
   CHECK_FOR_ERRORS;
   glBufferSubData(GL_ARRAY_BUFFER, 0 ,num_cavemesh_vertices*3*sizeof(GLfloat), cavemesh_vertices);
   CHECK_FOR_ERRORS;
   glBufferSubData(GL_ARRAY_BUFFER, num_cavemesh_vertices*3*sizeof(GLfloat) ,num_cavemesh_vertices*2*sizeof(GLfloat), cavemesh_coords);
   CHECK_FOR_ERRORS;
   glBufferSubData(GL_ARRAY_BUFFER, num_cavemesh_vertices*5*sizeof(GLfloat) ,num_cavemesh_vertices*3*sizeof(GLfloat), cavemesh_normals);
   CHECK_FOR_ERRORS;
   glBufferData(GL_ELEMENT_ARRAY_BUFFER,num_cavemesh_indices*sizeof(GLuint),cavemesh_indices,GL_STATIC_DRAW);
   CHECK_FOR_ERRORS;

   glBindBuffer(GL_ARRAY_BUFFER,0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
}

void CaveScene::initWaterMesh()
{
   Mat4 water_xfrm = Mat4::translation(Vec3(0.0,0.0,0.0)) * Mat4::scale(Vec3(2.0,0.0,2.0));

   for(int v=0;v<water_mesh_num_v;++v)
   {
      for(int u=0;u<water_mesh_num_u;++u)
      {
         float fu=float(u)/float(water_mesh_num_u-1);
         float fv=float(v)/float(water_mesh_num_v-1);

         Vec3 p0 = water_xfrm * evaluateWaterSurface(fu,fv);

         watermesh_vertices[(u+v*water_mesh_num_u)*3+0]=p0.x;
         watermesh_vertices[(u+v*water_mesh_num_u)*3+1]=p0.y;
         watermesh_vertices[(u+v*water_mesh_num_u)*3+2]=p0.z;

         if(v<(water_mesh_num_v-1))
         {
            watermesh_indices[(u+v*water_mesh_num_u)*2+0]=u+(v+0)*water_mesh_num_u;
            watermesh_indices[(u+v*water_mesh_num_u)*2+1]=u+(v+1)*water_mesh_num_u;
         }
      }
   }

   glGenBuffers(1,&watermesh_vbo);
   glGenBuffers(1,&watermesh_ebo);

   glBindBuffer(GL_ARRAY_BUFFER,watermesh_vbo);
   CHECK_FOR_ERRORS;
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,watermesh_ebo);
   CHECK_FOR_ERRORS;

   glBufferData(GL_ARRAY_BUFFER,num_watermesh_vertices*3*sizeof(GLfloat),watermesh_vertices,GL_STATIC_DRAW);
   CHECK_FOR_ERRORS;
   glBufferData(GL_ELEMENT_ARRAY_BUFFER,num_watermesh_indices*sizeof(GLuint),watermesh_indices,GL_STATIC_DRAW);
   CHECK_FOR_ERRORS;

   glBindBuffer(GL_ARRAY_BUFFER,0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
}

void CaveScene::slowInitialize()
{
   tet_tex=loadTexture(IMAGES_PATH "tet.png",true);

   initRandomGrid();

   initCaveMesh();
   initWaterMesh();

   tetrahedron_mesh.generateTetrahedron();
   tetrahedron_mesh.generateNormals();

   tetrahedron2_mesh.generateTetrahedron();
   tetrahedron2_mesh.separate();
   tetrahedron2_mesh.generateNormals();

   initializeShaders();
}

void CaveScene::initializeTextures()
{
   glGenTextures(num_texs, texs);

   createWindowTexture(texs[0], GL_RGBA16F);
   createWindowTexture(texs[2], GL_RGBA16F);
   createWindowTexture(texs[4], GL_RGBA16F, 4);
   createWindowTexture(texs[5], GL_RGBA16F, 4);
   createWindowTexture(texs[6], GL_RGBA16F);
   createWindowTexture(texs[9], GL_RGBA16F, 4);

   CHECK_FOR_ERRORS;

   glBindTexture(GL_TEXTURE_2D, 0);

   noise_tex = texs[7];
   noise_tex_2d = texs[11];

   {
      uint s = 32;
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

   CHECK_FOR_ERRORS;
   glBindTexture(GL_TEXTURE_3D, 0);

}

void CaveScene::initializeBuffers()
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
}

void CaveScene::initializeShaders()
{
   tet_shader.load(SHADERS_PATH "cavetet_v.glsl", NULL, SHADERS_PATH "cavetet_f.glsl");
   tet_shader.uniform1i("tex0",0);
   tet_shader.uniform1i("tex1",1);

   water_shader.load(SHADERS_PATH "cavewater_v.glsl", NULL, SHADERS_PATH "cavewater_f.glsl");
   water_shader.uniform1i("tex0",0);

   cave_shader.load(SHADERS_PATH "cave_v.glsl", NULL, SHADERS_PATH "cave_f.glsl");
   cave_shader.uniform1i("tex0",0);
   cave_shader.uniform1i("tex1",1);

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
}



void CaveScene::initialize()
{
   if(initialized)
      return;

   initialized = true;

   initializeTextures();
   initializeBuffers();
}


void CaveScene::update()
{
}

void CaveScene::drawCave(float ltime)
{
   cave_shader.uniform1f("time",ltime);
   cave_shader.bind();
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_3D,noise_tex);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D,noise_tex_2d);
   glBindBuffer(GL_ARRAY_BUFFER,cavemesh_vbo);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,cavemesh_ebo);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glEnableVertexAttribArray(2);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(cave_mesh_num_v*cave_mesh_num_u*3*sizeof(GLfloat)));
   glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(cave_mesh_num_v*cave_mesh_num_u*5*sizeof(GLfloat)));
   for(int v=0;v<cave_mesh_num_v-1;++v)
   {
      glDrawRangeElements(GL_TRIANGLE_STRIP, v*cave_mesh_num_u, (v+2)*cave_mesh_num_u-1, 2*cave_mesh_num_u, GL_UNSIGNED_INT, (GLvoid*)(v*2*cave_mesh_num_u*sizeof(GLuint)));
   }
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
   glDisableVertexAttribArray(2);
   glBindBuffer(GL_ARRAY_BUFFER,0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
}

Vec3 CaveScene::tetrahedronPath(int tet,float ltime)
{
   return Vec3(mix(tet*0.1f,0.0f,ltime*0.2f),-0.5f+1.75f*ltime*0.2f+tet*0.1,0.0f);
}

void CaveScene::drawTetrahedra(const Mat4& modelview, float ltime)
{
   tet_shader.bind();
   tet_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, tet_tex);

   Mesh* mesh=&tetrahedron2_mesh;

   mesh->bind();

   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
   glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(mesh->getVertexCount() * sizeof(GLfloat) * 3));

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(2);

   for(int tet=0;tet<max_tetrahedra;++tet)
   {
      Vec3 p=tetrahedronPath(tet,ltime);
      tetrahedra_mats[tet]=Mat4::translation(p) * Mat4::rotation(ltime*0.03+tet,Vec3(1,0,0)) * Mat4::rotation(ltime*0.04,Vec3(0,1,0)) * Mat4::scale(Vec3(0.025));
   }

   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE,GL_ONE);
   glDisable(GL_CULL_FACE);
   glDepthMask(GL_FALSE);
   for(int tet=0;tet<max_tetrahedra;++tet)
   {
      tet_shader.uniformMatrix4fv("object", 1, GL_FALSE, tetrahedra_mats[tet].e);
      glDrawRangeElements(GL_TRIANGLES, 0, mesh->getVertexCount() - 1, mesh->getTriangleCount() * 3, GL_UNSIGNED_INT, 0);
   }
   glDepthMask(GL_TRUE);
   glEnable(GL_CULL_FACE);
   glDisable(GL_BLEND);

   CHECK_FOR_ERRORS;


   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(2);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void CaveScene::drawWater(float ltime)
{
   Mesh* mesh=&tetrahedron_mesh;

   static GLfloat planes[max_tetrahedra*4*4];
   static GLfloat origins[max_tetrahedra*3];
   const GLfloat stretch=1024;

/*

   assert(mesh->vcount==4);
   assert(mesh->tcount==4);

   for(int tet=0;tet<max_tetrahedra;++tet)
   {
      Vec3 transformed_vertices[4] = { Vec3(0), Vec3(0), Vec3(0), Vec3(0) };
      Vec3 origin=tetrahedra_mats[tet]*Vec3(0.0,0.0,0.0);

      for(int i=0;i<4;++i)
      {
         Vec3 mv=mesh->getVertex(i);
         transformed_vertices[i]=(tetrahedra_mats[tet]*mesh->getVertex(i))-origin;
         transformed_vertices[i].y*=stretch;
      }

      for(int i=0;i<4;++i)
      {
         Vec3 pa=transformed_vertices[mesh->indices[i*3+0]];
         Vec3 pb=transformed_vertices[mesh->indices[i*3+1]];
         Vec3 pc=transformed_vertices[mesh->indices[i*3+2]];

         Vec3 nrm=(pb-pa).cross(pc-pa);
         Real l=std::sqrt(nrm.lengthSquared());
         assert(l>0);
         nrm=nrm*(1.0f/l);
         planes[tet*16+i*4+0]=nrm.x;
         planes[tet*16+i*4+1]=nrm.y;
         planes[tet*16+i*4+2]=nrm.z;
         planes[tet*16+i*4+3]=-nrm.dot(pa);
      }

      origins[tet*3+0]=origin.x;
      origins[tet*3+1]=origin.y;
      origins[tet*3+2]=origin.z;
   }
*/
   water_shader.bind();

   water_shader.uniformMatrix4fv("tetrahedra_planes[0]", 1, GL_TRUE, planes+0*16);
   water_shader.uniformMatrix4fv("tetrahedra_planes[1]", 1, GL_TRUE, planes+1*16);
   water_shader.uniformMatrix4fv("tetrahedra_planes[2]", 1, GL_TRUE, planes+2*16);
   water_shader.uniformMatrix4fv("tetrahedra_planes[3]", 1, GL_TRUE, planes+3*16);
   CHECK_FOR_ERRORS;

   water_shader.uniform3f("tetrahedra_origins[0]",origins[0*3+0],origins[0*3+1],origins[0*3+2]);
   water_shader.uniform3f("tetrahedra_origins[1]",origins[1*3+0],origins[1*3+1],origins[1*3+2]);
   water_shader.uniform3f("tetrahedra_origins[2]",origins[2*3+0],origins[2*3+1],origins[2*3+2]);
   water_shader.uniform3f("tetrahedra_origins[3]",origins[3*3+0],origins[3*3+1],origins[3*3+2]);
   CHECK_FOR_ERRORS;

   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE,GL_ONE);
   water_shader.uniform1f("time",ltime);

   glDepthMask(GL_FALSE);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D,noise_tex_2d);
   glBindBuffer(GL_ARRAY_BUFFER,watermesh_vbo);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,watermesh_ebo);
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
   for(int v=0;v<water_mesh_num_v-1;++v)
   {
      glDrawRangeElements(GL_TRIANGLE_STRIP, v*water_mesh_num_u, (v+2)*water_mesh_num_u-1, 2*water_mesh_num_u, GL_UNSIGNED_INT, (GLvoid*)(v*2*water_mesh_num_u*sizeof(GLuint)));
   }
   glDisableVertexAttribArray(0);
   glBindBuffer(GL_ARRAY_BUFFER,0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
   glDisable(GL_BLEND);
}

void CaveScene::drawView(float ltime)
{
   lltime = ltime;

   const float aspect_ratio = float(window_height) / float(window_width);

   const float jitter_x = frand() / window_width * 1 / 256.0f;
   const float jitter_y = frand() / window_height * aspect_ratio * 1 / 256.0f;

   Mat4 modelview = Mat4::identity(), projection = Mat4::identity();

   const float znear = 1.0f / 256.0f, zfar = 1024.0f;

   const float zoom = 1.0f - cubic(clamp(ltime - 2.5f,0.0f,1.0f)) * 0.5f;

   projection = Mat4::frustum((-znear + jitter_x) * zoom, (+znear + jitter_x) * zoom, (-aspect_ratio*znear + jitter_y) * zoom, (+aspect_ratio*znear + jitter_y) * zoom, znear, zfar) * projection;

   modelview = modelview * Mat4::rotation(mix(0.3f,-0.75f,cubic(clamp(ltime*0.2f,0.0f,1.0f))),Vec3(1,0,0));
   modelview = modelview * Mat4::translation(Vec3(0, -0.2, -0.75));
   modelview = modelview * Mat4::rotation(ltime*0.4*0.5,Vec3(0,1,0));

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

   const Vec3 cc=Vec3(0.5,0.6,1.0)*0.02;
   //glClearColor(cc.x,cc.y,cc.z,0.0f);
   glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
   //glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

   CHECK_FOR_ERRORS;

   cave_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);
   cave_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);

   water_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);
   water_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);

   tet_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);

   CHECK_FOR_ERRORS;

   drawCave(ltime);
   drawTetrahedra(modelview,ltime);
   drawWater(ltime);

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

void CaveScene::gaussianBlur(Real tint_r, Real tint_g, Real tint_b, Real radx, Real rady, GLuint fbo0, GLuint fbo1, GLuint src_tex, GLuint tex0)
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


void CaveScene::render()
{
   static const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f };
   static const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
   static const GLubyte indices[] = { 3, 0, 2, 1 };

   const float aspect_ratio = float(window_height) / float(window_width);

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

   {

      // ----- motion blur ------

      glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);
      glClear(GL_COLOR_BUFFER_BIT);

      num_subframes = 4;
      subframe_scale = 1.0/35.0;

      for(int subframe = 0;subframe<num_subframes;++subframe)
      {
         // draw the view

         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[6]);

         glDisable(GL_BLEND);

         drawView(time + subframe_scale * float(subframe) / float(num_subframes));


         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);

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


void CaveScene::free()
{
   glDeleteTextures(1,&tet_tex);
   glDeleteBuffers(1,&cavemesh_vbo);
   glDeleteBuffers(1,&cavemesh_ebo);
   glDeleteTextures(num_texs, texs);
   glDeleteFramebuffers(num_fbos, fbos);
}

