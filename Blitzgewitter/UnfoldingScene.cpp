
#include "Engine.hpp"
#include "Bezier.hpp"
#include "Ran.hpp"
#include "Particles.hpp"
#include "Tower.hpp"

#include <list>

#define SHADERS_SUBPATH SHADERS_PATH "unfolding/"

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

struct QuadGeometry
{
   Vec3 points[4];  // The vertices of the quad.
};

static const Vec2 credit_size(135,265);

// A quad in the unfolding model.
struct Quad
{
   float start_time;    // The time at which this quad will appear.
   float end_time;      // The time at which the pivot ends.
   int   edge;          // The edge of the quad that it will pivot around.
   float start_angle;   // The starting angle for the pivot.
   float end_angle;     // The ending angle for the pivot.
   int   material;      // A material identifier.

   QuadGeometry geometry;

   // Extrude an edge to create a new quad, and set up the properties of the target
   // appropriately for an unfold to occur at the given time.
   void build(Quad* target, int edge, int face, float angle, float time, float speed) const;

   static void rotateQuadGeometry(const QuadGeometry& in, const int edge, const float angle, QuadGeometry& out);
};

class UnfoldingScene: public Scene
{
   bool drawing_view_for_background;
   int num_subframes;
   float subframe_scale;

   float lltime;

   static const int num_fbos = 16, num_texs = 16;

   Shader logo_shader, mb_accum_shader, composite_shader;
   Shader credit_shader;

   GLuint fbos[num_fbos], texs[num_texs], renderbuffers[num_fbos];

   GLuint noise_tex_2d;

   Quad quads[1024];
   Quad* next_quad;

   void initializeTextures();
   void initializeShaders();
   void initializeBuffers();

   static const int num_credits=3;

   GLuint credit_texs[num_credits];
   Vec2 credit_pos[num_credits];
   float credit_appear_times[num_credits];

   void drawView(float ltime);

   void renderQuad(const Quad& quad, const float time);
   void buildStrip(const Quad* base, float time, const char*& c);
   void createStrip(const char* cmds, const Vec3& offset, const float time, const Mat4& transform);

   public:
      UnfoldingScene()
      {
         for(int i=0;i<num_credits;++i)
            credit_texs[i]=0;
         next_quad = quads;
      }

      ~UnfoldingScene()
      {
         free();
      }

      void slowInitialize();
      void initialize();
      void render();
      void update();
      void free();
};

Scene* unfoldingScene = new UnfoldingScene();

void UnfoldingScene::slowInitialize()
{
   credit_texs[0] = loadTexture(IMAGES_PATH "gfx.png");
   credit_texs[1] = loadTexture(IMAGES_PATH "code.png");
   credit_texs[2] = loadTexture(IMAGES_PATH "music.png");

   for(int i=0;i<num_credits;++i)
   {
      credit_pos[i] = Vec2(mix(-0.75f,+0.75f,float(i)/float(num_credits-1)),-0.7f);
   }

   credit_appear_times[2]=5.0f;
   credit_appear_times[0]=5.5f;
   credit_appear_times[1]=6.0f;

   const Mat4 transform = Mat4::rotation(M_PI * 0.25f, Vec3(0.0f, 1.0f, 0.0f));

   createStrip("{B0d1C1}A0c1D1", Vec3(0, 0, 0), 0.5f, transform);
   createStrip("D0b1D0", Vec3(5, 0, 0), 1.5f, transform);
   createStrip("D0b1{a1}C1", Vec3(10, 0, 0), 2.5f, transform);
   createStrip("D0{c2a1B1d0}a1C0A1c0A1", Vec3(15, 0, 2), 3.5f, transform);
   createStrip("D0b1d0b1d0b1", Vec3(20, 0, 0), 4.5f, transform);

  initializeShaders();
 }


void Quad::rotateQuadGeometry(const QuadGeometry& in, const int edge, const float angle, QuadGeometry& out)
{
   static const int emap[7] = { 0, 1, 3, 2, 0, 1, 3 };

   const Vec3& v0 = in.points[emap[edge + 0]];
   const Vec3& v1 = in.points[emap[edge + 1]];
   const Vec3& v2 = in.points[emap[edge + 3]];

   Vec3 e0 = normalize(v1 - v0);
   Vec3 e1 = v2 - v0;
   Vec3 e2 = normalize(e0.cross(e1));

   e1 = normalize(e0.cross(e2));

   for(int i = 0; i < 4; ++i)
   {
      Vec3& v = out.points[i];

      v.x = (in.points[i] - v0).dot(e0);
      v.y = (in.points[i] - v0).dot(e1);
      v.z = (in.points[i] - v0).dot(e2);
   }

   const Vec3 re1 = e1 * std::cos(angle) + e2 * std::sin(angle);
   const Vec3 re2 = e2 * std::cos(angle) - e1 * std::sin(angle);

   for(int i = 0; i < 4; ++i)
   {
      Vec3& v = out.points[i];

      v = v0 + v.x * e0 + v.y * re1 + v.z * re2;
   }
}

void Quad::build(Quad* target, int edge, int face, float angle, float time, float speed) const
{
   rotateQuadGeometry(geometry, edge, angle, target->geometry);

   target->start_angle = face ? -angle : (M_PI * 2.0f - angle);
   target->end_angle = 0.0f;

   const float duration = std::abs(target->start_angle - target->end_angle) / speed;

   target->start_time = time;
   target->end_time = time + duration;
   target->edge = (edge + 0) % 4;
}



void UnfoldingScene::renderQuad(const Quad& quad, const float time)
{
   if(time < quad.start_time)
      return;

   QuadGeometry rotated_geom;

   const float bias = std::min(1.0f, (time - quad.start_time) / (quad.end_time - quad.start_time));
   const float angle = quad.start_angle + bias * (quad.end_angle - quad.start_angle);

   if(quad.edge == -1 || time >= quad.end_time)
      rotated_geom = quad.geometry;
   else
      Quad::rotateQuadGeometry(quad.geometry, quad.edge, angle, rotated_geom);

   GLfloat verts[4 * 3] = { 0 };
   GLfloat norms[4 * 3] = { 0 };
   GLfloat colours[4 * 3] = { 0 };

   for(int i = 0; i < 4; ++i)
   {
      verts[i * 3 + 0] = rotated_geom.points[i].x;
      verts[i * 3 + 1] = rotated_geom.points[i].y;
      verts[i * 3 + 2] = rotated_geom.points[i].z;
   }

   const Vec3 normal = normalize((rotated_geom.points[1] - rotated_geom.points[0]).cross(rotated_geom.points[2] - rotated_geom.points[0]));
   const Vec3 base_normal = normalize((quad.geometry.points[1] - quad.geometry.points[0]).cross(quad.geometry.points[2] - quad.geometry.points[0]));

   for(int i = 0; i < 4; ++i)
   {
      norms[i * 3 + 0] = normal.x;
      norms[i * 3 + 1] = normal.y;
      norms[i * 3 + 2] = normal.z;
   }

   for(int i = 0; i < 4; ++i)
   {
      colours[i * 3 + 0] = 0.3f;
      colours[i * 3 + 1] = 0.3f;
      colours[i * 3 + 2] = 0.3f;
   }

   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, verts);
   glVertexAttribPointer(1, 3, GL_FLOAT, false, 0, norms);
   glVertexAttribPointer(2, 3, GL_FLOAT, false, 0, colours);

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glEnableVertexAttribArray(2);
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
   glDisableVertexAttribArray(2);
}


// Edges are labelled ABCD, and the number indicates the 90-degree multiple.
// Braces indicate a branch out of the current quad. An upper-case letter indicates
// the font face should unfold, and a lower-case letter undicates the back face
// should unfold.
void UnfoldingScene::buildStrip(const Quad* base, float time, const char*& c)
{
   const Quad* current_quad = base;
   const float speed = M_PI * 2.0f;

   for(; c && c[0];)
   {
      if(c[0] == '}')
      {
         ++c;
         return;
      }
      else if(c[0] == '{')
      {
         ++c;
         buildStrip(current_quad, time + 4.0f / speed, c);
         continue;
      }

      const char uc = std::toupper(c[0]);
      const int face = (uc == c[0]);
      int edge = 0;
      float angle = 0.0f;

      switch(uc)
      {
         case 'A': edge = 0; break;
         case 'B': edge = 1; break;
         case 'C': edge = 2; break;
         case 'D': edge = 3; break;
      }

      switch(c[1])
      {
         case '0': angle = M_PI * 0.5f; break;
         case '1': angle = M_PI * 1.0f; break;
         case '2': angle = M_PI * 1.5f; break;
      }

      Quad* target_quad = next_quad++;

      current_quad->build(target_quad, edge, face, angle, time, speed);
      current_quad = target_quad;

      time = target_quad->end_time;

      c += 2;
   }
}

void UnfoldingScene::initializeShaders()
{
   credit_shader.load(SHADERS_PATH "credit_v.glsl", NULL, SHADERS_PATH "credit_f.glsl");
   credit_shader.uniform1i("tex0", 0);

   logo_shader.load(SHADERS_SUBPATH "vertex.glsl", NULL, SHADERS_SUBPATH "fragment.glsl");
   logo_shader.uniform1i("tex0", 0);

   mb_accum_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "mb_accum_f.glsl");
   mb_accum_shader.uniform1i("tex0", 0);

   composite_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "cubes_composite3_f.glsl");
   composite_shader.uniform1i("tex0", 0);
   composite_shader.uniform1i("tex1", 1);
}

void UnfoldingScene::initializeTextures()
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
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pix);
      CHECK_FOR_ERRORS;
   }

   glBindTexture(GL_TEXTURE_2D, 0);
}


void UnfoldingScene::initializeBuffers()
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


   glBindBuffer(GL_ARRAY_BUFFER, 0);

   CHECK_FOR_ERRORS;
}

void UnfoldingScene::createStrip(const char* cmds, const Vec3& offset, const float time, const Mat4& transform)
{
   QuadGeometry geom;

   geom.points[0] = transform * Vec3(-1.0f, 0.0f, -1.0f) + offset;
   geom.points[1] = transform * Vec3(+1.0f, 0.0f, -1.0f) + offset;
   geom.points[2] = transform * Vec3(-1.0f, 0.0f, +1.0f) + offset;
   geom.points[3] = transform * Vec3(+1.0f, 0.0f, +1.0f) + offset;

   Quad* base = next_quad++;

   base->geometry = geom;
   base->edge = -1;
   base->start_time = time;

   buildStrip(base, time, cmds);
}

void UnfoldingScene::initialize()
{
   if(initialized)
      return;

   initialized = true;

   initializeTextures();
   initializeBuffers();
}

void UnfoldingScene::update()
{
}

void UnfoldingScene::free()
{
}

void UnfoldingScene::drawView(float ltime)
{
   glDisable(GL_STENCIL_TEST);
   glDisable(GL_SCISSOR_TEST);
   glEnable(GL_CULL_FACE);
   glDisable(GL_BLEND);
   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);
   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
   glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

   const float aspect_ratio = float(window_height) / float(window_width);

   Mat4 modelview = Mat4::identity(), projection = Mat4::identity();

   const float jitter_x = frand() / window_width * 3;
   const float jitter_y = frand() / window_height * aspect_ratio * 3;

   projection = Mat4::translation(Vec3(jitter_x,jitter_y,0)) * Mat4::scale(Vec3(aspect_ratio,1,1)) * Mat4::rotation(-M_PI/6.0f, Vec3(1.0f, 0.0f, 0.0f));


   modelview = Mat4::translation(Vec3(-1.1,0,-0.3)) * Mat4::scale(Vec3(0.1));

   logo_shader.bind();

   logo_shader.uniformMatrix4fv("modelview",1,false,modelview.e);
   logo_shader.uniformMatrix4fv("projection",1,false,projection.e);


   glViewport(0, 0, window_width, window_height);

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glDisable(GL_CULL_FACE);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, noise_tex_2d);


   for(Quad* q = quads; q < next_quad; ++q)
   {
      renderQuad(q[0], ltime*1.5);
   }

   {
      glDisable(GL_DEPTH_TEST);
      glDepthMask(GL_FALSE);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
      static const GLfloat coords[8]={0.0f,0.0f,1.0f,0.0f,0.0f,1.0f,1.0f,1.0f};
      credit_shader.bind();
      GLfloat vertices[8];
      glBindBuffer(GL_ARRAY_BUFFER,0);
      glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,vertices);
      glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,0,coords);
      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);
      for(int i=0;i<num_credits;++i)
      {
         credit_shader.uniform1f("alpha",cubic(clamp((ltime-credit_appear_times[i])*1.5f,0.0f,1.0f)));
         static const GLfloat base_vertices[8]={-1.0f,-1.0f,+1.0f,-1.0f,-1.0f,+1.0f,+1.0f,+1.0f};
         const float xs=credit_size.x/720.0f;
         const float ys=credit_size.y/720.0f;
         for(int j=0;j<4;++j)
         {
            vertices[j * 2 + 0] = (credit_pos[i].x + base_vertices[j * 2 + 0] * xs)*float(window_height)/float(window_width);
            vertices[j * 2 + 1] = credit_pos[i].y + base_vertices[j * 2 + 1] * ys;
         }
         glActiveTexture(GL_TEXTURE0);
         glBindTexture(GL_TEXTURE_2D,credit_texs[i]);
         glDrawArrays(GL_TRIANGLE_STRIP,0,4);
      }
      glDisableVertexAttribArray(0);
      glDisableVertexAttribArray(1);
      CHECK_FOR_ERRORS;
   }
}

void UnfoldingScene::render()
{

   static const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f };
   static const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
   static const GLubyte indices[] = { 3, 0, 2, 1 };

   const float aspect_ratio = float(window_height) / float(window_width);

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);


   // ----- motion blur ------

   glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);
   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }
   glClear(GL_COLOR_BUFFER_BIT);

   num_subframes = 8;
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

      drawView(time + subframe_scale * float(subframe) / float(num_subframes));


      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);
      {
         GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
         glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
      }
      glViewport(0, 0, window_width, window_height);

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
   glDrawBuffer(GL_BACK);
   glViewport(0, 0, window_width, window_height);

   glDisable(GL_BLEND);

   composite_shader.bind();
   composite_shader.uniform1i("frame_num", frame_num);
   composite_shader.uniform1f("fade", 1.0-cubic(clamp((time-8.0)/3.0,0.0,1.0)));
   {
      const float l=std::max(0.0f,1.0f-std::pow(time*2.0f,0.5f));
      composite_shader.uniform3f("addcolour", l,l,l);
   }

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

