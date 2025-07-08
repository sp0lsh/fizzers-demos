
// midnight platonic

#include "Engine.hpp"
#include "Bezier.hpp"
#include "Ran.hpp"
#include "Particles.hpp"
#include "Tower.hpp"

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

tower::Tower* platonic2_tower=0;

class Platonic2Scene: public Scene
{
   tower::Tower twr1;

   particles::Particles prt;
   particles::Camera prt_camera;

   Shader forrest_shader, bokeh_shader, bokeh2_shader;
   Shader gaussbokeh_shader, composite_shader;
   Shader mb_accum_shader;
   Shader screendisplay_shader;
   Shader bgfg_shader;

   bool drawing_view_for_background;
   int num_subframes;
   float subframe_scale;

   float lltime;

   Mesh icosahedron_mesh;

   static const int num_fbos = 16, num_texs = 16;

   GLuint fbos[num_fbos], texs[num_texs], renderbuffers[num_fbos];

   GLuint bokeh_temp_fbo;
   GLuint bokeh_temp_tex_1, bokeh_temp_tex_2;

   GLuint depth_tex;
   GLuint noise_tex_2d, noise_tex;

   void slowInitialize();
   void initializeTextures();
   void initializeShaders();
   void initializeBuffers();

   void drawTower(tower::Tower& twr, const Mat4& modelview, Real camheight);
   void drawView(float ltime);
   void gaussianBlur(Real tint_r, Real tint_g, Real tint_b, Real radx, Real rady, GLuint fbo0, GLuint fbo1, GLuint src_tex, GLuint tex0);


   public:
      Platonic2Scene(): icosahedron_mesh(64, 64)
      {
      }

      ~Platonic2Scene()
      {
         free();
      }


      void initialize();
      void render();
      void update();
      void free();
};

Scene* platonic2Scene = new Platonic2Scene();


void Platonic2Scene::slowInitialize()
{
   initializeShaders();


/*
distance
angle
height
time_scale
time_offset
ptn_thickness
ptn_brightness
ptn_scroll
ptn_frequency
shimmer
l0_scroll
l0_brightness
l1_scroll
l1_brightness
l2_scroll
l2_brightness
rot_x
rot_y
rot_z
sca_x
sca_y
sca_z
rot_spd_x
rot_spd_y
rot_spd_z
geom
background
ambient
section_rot_spd
*/


   {
      tower::Section section;
      section.height0 = -4;
      section.height1 = +4;
      section.radius = 5;
      section.num_instances = 40;

      {
         tower::PieceType piece_type;
         piece_type.weight = 1;
         piece_type.parameters["distance"] = tower::ParameterRange(2.0, 4.0);
         piece_type.parameters["angle"] = tower::ParameterRange(0, 2*M_PI);
         piece_type.parameters["height"] = tower::ParameterRange(0.0,1);
         piece_type.parameters["sca_x"] = tower::ParameterRange(2.3);
         piece_type.parameters["sca_y"] = tower::ParameterRange(2.3);
         piece_type.parameters["sca_z"] = tower::ParameterRange(2.3);
         piece_type.parameters["rot_spd_x"] = tower::ParameterRange(0.1*0.5,0.2*0.5);
         piece_type.parameters["rot_spd_y"] = tower::ParameterRange(0.2*0.5,0.3*0.5);
         piece_type.parameters["rot_x"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_y"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["l0_scroll"] = tower::ParameterRange(0.02*2);
         piece_type.parameters["l0_brightness"] = tower::ParameterRange(2);
         piece_type.parameters["l1_scroll"] = tower::ParameterRange(0.01*2);
         piece_type.parameters["l1_brightness"] = tower::ParameterRange(2);
         piece_type.parameters["l2_scroll"] = tower::ParameterRange(0.03*2);
         piece_type.parameters["l2_brightness"] = tower::ParameterRange(2);
         piece_type.parameters["geom"] = tower::ParameterRange(0);
         piece_type.parameters["background"] = tower::ParameterRange(1);
         piece_type.parameters["time_scale"] = tower::ParameterRange(1);
         piece_type.parameters["time_offset"] = tower::ParameterRange(0,15);
         piece_type.parameters["shimmer"] = tower::ParameterRange(0.7);
         piece_type.parameters["ptn_brightness"] = tower::ParameterRange(0.7);
         piece_type.parameters["ptn_thickness"] = tower::ParameterRange(0.1);
         piece_type.parameters["ptn_frequency"] = tower::ParameterRange(2.0);
         section.types.push_back(piece_type);
      }

      {
         tower::PieceType piece_type;
         piece_type.weight = 3;
         piece_type.parameters["distance"] = tower::ParameterRange(0.4, 3.0);
         piece_type.parameters["angle"] = tower::ParameterRange(0, 2*M_PI);
         piece_type.parameters["height"] = tower::ParameterRange(-1,3);
         piece_type.parameters["sca_x"] = tower::ParameterRange(2.3);
         piece_type.parameters["sca_y"] = tower::ParameterRange(2.3);
         piece_type.parameters["sca_z"] = tower::ParameterRange(2.3);
         piece_type.parameters["rot_spd_x"] = tower::ParameterRange(0.1*0.5,0.2*0.5);
         piece_type.parameters["rot_spd_y"] = tower::ParameterRange(0.2*0.5,0.3*0.5);
         piece_type.parameters["rot_x"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_y"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["l0_scroll"] = tower::ParameterRange(0.02*2);
         piece_type.parameters["l0_brightness"] = tower::ParameterRange(2);
         piece_type.parameters["l1_scroll"] = tower::ParameterRange(0.01*2);
         piece_type.parameters["l1_brightness"] = tower::ParameterRange(2);
         piece_type.parameters["l2_scroll"] = tower::ParameterRange(0.03*2);
         piece_type.parameters["l2_brightness"] = tower::ParameterRange(2);
         piece_type.parameters["geom"] = tower::ParameterRange(0);
         piece_type.parameters["background"] = tower::ParameterRange(0);
         piece_type.parameters["time_scale"] = tower::ParameterRange(1);
         piece_type.parameters["time_offset"] = tower::ParameterRange(0,15);
         piece_type.parameters["shimmer"] = tower::ParameterRange(0.7);
         section.types.push_back(piece_type);
      }

      twr1.sections.push_back(section);
   }


   twr1.genInstances();

   platonic2_tower=&twr1;

   icosahedron_mesh.generateIcosahedron();
   icosahedron_mesh.separate();
   icosahedron_mesh.generateNormals();
}

void Platonic2Scene::initializeTextures()
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

   glBindTexture(GL_TEXTURE_2D, depth_tex);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, window_width, window_height);
   CHECK_FOR_ERRORS;
   glBindTexture(GL_TEXTURE_2D, 0);

}

void Platonic2Scene::initializeBuffers()
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

void Platonic2Scene::initializeShaders()
{
   forrest_shader.load(SHADERS_PATH "forrest2_v.glsl", NULL, SHADERS_PATH "forrest2_f.glsl");
   forrest_shader.uniform1i("tex0",0);

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

   screendisplay_shader.load(SHADERS_PATH "lines_v.glsl", SHADERS_PATH "lines_g.glsl", SHADERS_PATH "lines_f.glsl");

   bgfg_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "bgfg_f.glsl");
   bgfg_shader.uniform1i("tex0", 0);
   bgfg_shader.uniform1i("tex1", 1);

   prt.setFrameWidth(window_width);
   prt.setFrameHeight(window_height);
   prt.setCamera(&prt_camera);
   prt.setParticleTexSize(512);
   prt.init();
}



void Platonic2Scene::initialize()
{
   if(initialized)
      return;

   initialized = true;

   initializeTextures();
   initializeBuffers();
}


void Platonic2Scene::update()
{
/*
  static int nn = 0;
   int lln = 16;
   if((int(time) & (lln-1)) == 0 && int(time)/lln != nn)
   {
      nn = int(time)/lln;
      prt.resetPositions();
      prt.resetVelocities();
   }
*/
}

void Platonic2Scene::drawTower(tower::Tower& twr, const Mat4& modelview, Real camheight)
{
   const Real camheightrange = drawing_view_for_background ? 40 : 10;

   forrest_shader.bind();

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(2);

   static const float glow_time=0.0f;

   if(lltime>glow_time)
      forrest_shader.uniform1f("glow",std::exp(-(lltime-glow_time)*4.0f));
   else
      forrest_shader.uniform1f("glow",0.0f);

   unsigned int piece_index=0;
   for(std::vector<tower::PieceInstance>::iterator p=twr.instances.begin();p!=twr.instances.end();++p,++piece_index)
   {
      std::map<std::string,Real>& ps=p->parameters;

      if((ps["background"]>0.5)!=drawing_view_for_background)
         continue;

      if(camheight < (p->section->height1 - camheightrange) || camheight > (p->section->height0 + camheightrange))
         continue;

      Mesh* mesh=&icosahedron_mesh;


      mesh->bind();

      const float b=drawing_view_for_background ? (cubic(clamp(lltime*0.5f,0.0f,1.0f))) : 1;

      forrest_shader.uniform1f("ptn_thickness",ps["ptn_thickness"]);
      forrest_shader.uniform1f("ptn_brightness",ps["ptn_brightness"]);
      forrest_shader.uniform1f("ptn_scroll",ps["ptn_scroll"]);
      forrest_shader.uniform1f("ptn_frequency",ps["ptn_frequency"]);
      forrest_shader.uniform1f("shimmer",ps["shimmer"]);
      forrest_shader.uniform1f("l0_scroll",ps["l0_scroll"]);
      forrest_shader.uniform1f("l0_brightness",ps["l0_brightness"]*b);
      forrest_shader.uniform1f("l1_scroll",ps["l1_scroll"]);
      forrest_shader.uniform1f("l1_brightness",ps["l1_brightness"]*b);
      forrest_shader.uniform1f("l2_scroll",ps["l2_scroll"]);
      forrest_shader.uniform1f("l2_brightness",ps["l2_brightness"]*b);
      forrest_shader.uniform1f("ambient",ps["ambient"]);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_3D,noise_tex);

      const float upwards=(piece_index == 27) ? 0.0f : std::pow(std::max(0.0f,(lltime-15.0f-piece_index*0.1f)),2.0f)*0.9f;
      const float rs=(piece_index == 27) ? 0.96f : 1.0f;

      Mat4 modelview2 = modelview * Mat4::rotation(lltime * ps["section_rot_spd"], Vec3(0,1,0)) *
                        Mat4::translation(Vec3(std::cos(ps["angle"])*ps["distance"], ps["height"]-upwards, std::sin(ps["angle"])*ps["distance"])) *
                        Mat4::rotation(ps["rot_z"]+lltime*ps["rot_spd_z"]*rs,Vec3(0,0,1)) * Mat4::rotation(ps["rot_y"]+lltime*ps["rot_spd_y"]*rs,Vec3(0,1,0)) * Mat4::rotation(ps["rot_x"]+lltime*ps["rot_spd_x"]*rs,Vec3(1,0,0)) *
                        Mat4::scale(Vec3(ps["sca_x"],ps["sca_y"],ps["sca_z"]));

      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
      glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(mesh->getVertexCount() * sizeof(GLfloat) * 3));

      forrest_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview2.e);

//// 011011
//if(piece_index==27)
      glDrawRangeElements(GL_TRIANGLES, 0, mesh->getVertexCount() - 1, mesh->getTriangleCount() * 3, GL_UNSIGNED_INT, 0);

      CHECK_FOR_ERRORS;
   }

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(2);
}

void Platonic2Scene::drawView(float ltime)
{
   lltime = ltime;

   const float aspect_ratio = float(window_height) / float(window_width);

   const float jitter_x = frand() / window_width * 3;
   const float jitter_y = frand() / window_height * aspect_ratio * 3;

   const float fov = M_PI * 0.3f;

   const float znear = 1.0f / tan(fov * 0.5f), zfar = 256.0f;

   const float frustum_scale = 1.0f/512.0f;

   Mat4 projection = Mat4::frustum((-1.0f + jitter_x)*frustum_scale, (+1.0f + jitter_x)*frustum_scale, (-aspect_ratio + jitter_y)*frustum_scale, (+aspect_ratio + jitter_y)*frustum_scale, znear*frustum_scale, zfar);

   const float move_back = drawing_view_for_background ? 10 : 0;

   Mat4 modelview = Mat4::rotation(cos(ltime * 15.0) * 0.01 / 128, Vec3(0.0f, 1.0f, 0.0f)) * Mat4::rotation(sin(ltime * 17.0) * 0.01 / 128, Vec3(1.0f, 0.0f, 0.0f)) *
                  Mat4::translation(Vec3(0, 0, -45 - move_back + ltime * 0.2));

   if(ltime > 6.0)
   {
      modelview=Mat4::rotation(-0.1,Vec3(0,1,0))*Mat4::translation(Vec3(0,-0.25,15))*modelview;
   }

   if(ltime > 10.0)
   {
      modelview=Mat4::rotation(-0.175*cubic(clamp((ltime-10.0f)*0.1f,0.0f,1.0f)),Vec3(0,1,0))*modelview;
      modelview=Mat4::translation(Vec3(0,0,std::pow(std::max(0.0f,ltime-10.0f)*0.29f,2.0f)))*modelview;
   }

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

   forrest_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);
   screendisplay_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);
   prt_camera.setProjection(projection);


   CHECK_FOR_ERRORS;


   drawTower(twr1, modelview, 0);


   glUseProgram(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);

   if(!drawing_view_for_background)
   {
      glDepthMask(GL_FALSE);
      glDisable(GL_BLEND);

      prt_camera.setModelView(modelview);


      prt.setParticleScale(8.0f);
      prt.setGravityScale(0.01f * 260.0 * 4.0f / float(num_subframes));
      prt.setInitialSpread(1.0f);

      prt.setDepthTex(depth_tex);


      prt.update();
         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[6]);
      {
         GLenum b[1] = { GL_COLOR_ATTACHMENT0 };
         glDrawBuffers(1, b);
      }

      glEnable(GL_DEPTH_TEST);

      glViewport(0, 0, window_width, window_height);

      prt.render();

   }


   glDepthMask(GL_TRUE);

   CHECK_FOR_ERRORS;
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Platonic2Scene::gaussianBlur(Real tint_r, Real tint_g, Real tint_b, Real radx, Real rady, GLuint fbo0, GLuint fbo1, GLuint src_tex, GLuint tex0)
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


void Platonic2Scene::render()
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
         //glEnable(GL_BLEND);
         //glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);

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

         //glBindFramebuffer(GL_READ_FRAMEBUFFER, fbos[0]);
         //glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);
         //glViewport(0, 0, window_width, window_height);
         //glBlitFramebuffer(0, 0, window_width, window_height, 0, 0, window_width, window_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
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
      float s=std::pow(clamp(time-22.4f,0.0f,1.0f),2.0f);
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


void Platonic2Scene::free()
{
   glDeleteTextures(num_texs, texs);
   glDeleteFramebuffers(num_fbos, fbos);
}

