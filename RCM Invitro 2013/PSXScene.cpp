
#include "Engine.hpp"

#include <cctype>

class PSXScene: public Scene
{
   model::Mesh amiga_mesh, atari_mesh, c64_mesh, gameboy_mesh, torus_mesh;

   Shader screen_shader, gaussblur_shader, composite_shader;
   Shader screen2_shader, screendisplay_shader, zoomblur_shader;
   Shader mesh_shader, torus_shader, text_shader;

   Spline<Real> zoom_spline, tint_spline;

   static const int num_fbos = 16, num_texs = 16;

   GLuint fbos[num_fbos], texs[num_texs], renderbuffers[num_fbos];

   GLuint screen_fbo, display_rbo, screen_tex;
   GLuint ds_fbo, ds_tex; // downsampling
   GLuint blur_fbo0, blur_fbo1, blur_tex0, blur_tex1;
   GLuint noise_tex;
   GLuint display_fbo, display_tex; // what is displayed onscreen

   GLuint rcm_logo_tex;
   GLuint gb_tex;
   GLuint font_tex;
   GLuint credits_tex;

   GLfloat text_coords[8192 * 2];
   GLfloat text_vertices[8192 * 2];
   GLushort text_indices[8192], num_text_vertices, num_text_indices;

   void initializeTextures();
   void initializeShaders();
   void initializeBuffers();
   void initialiseMeshes();

   void gaussianBlur(GLuint srctex);
   void renderDisplay();
   void renderDisplayWires(float t, const Mat4& projection);

   void bindMaterialForMesh(const model::Material *const mat);
   void renderModel(const model::Mesh *const m);

   void addTextQuad(Vec2 p0, Vec2 p1, int c = 0);
   void addText(Vec2 org, const std::string& text);

   public:
      PSXScene()
      {
         num_text_vertices = 0;
         num_text_indices = 0;
      }

      void initialize();
      void render();
      void update();
      void free();
};

Scene* psxScene = new PSXScene();

const int gauss_divisor = 4;

void PSXScene::addTextQuad(Vec2 p0, Vec2 p1, int c)
{
   if(c < 0)
      return;

   GLushort i0 = num_text_vertices, i1 = i0 + 1, i2 = i0 + 2, i3 = i0 + 3;

   const int gw = 16, gh = 3;
   const int x = c % gw, y = gh - ((c / gw) % gh) - 1;

   const Vec2 c0 = Vec2(Real(x + 0) / Real(gw), Real(y + 0) / Real(gh));
   const Vec2 c1 = Vec2(Real(x + 1) / Real(gw), Real(y + 1) / Real(gh));

   text_vertices[i0 * 2 + 0] = p0.x;
   text_vertices[i0 * 2 + 1] = p0.y;

   text_vertices[i1 * 2 + 0] = p1.x;
   text_vertices[i1 * 2 + 1] = p0.y;

   text_vertices[i2 * 2 + 0] = p0.x;
   text_vertices[i2 * 2 + 1] = p1.y;

   text_vertices[i3 * 2 + 0] = p1.x;
   text_vertices[i3 * 2 + 1] = p1.y;

   text_coords[i0 * 2 + 0] = c0.x;
   text_coords[i0 * 2 + 1] = c0.y;

   text_coords[i1 * 2 + 0] = c1.x;
   text_coords[i1 * 2 + 1] = c0.y;

   text_coords[i2 * 2 + 0] = c0.x;
   text_coords[i2 * 2 + 1] = c1.y;

   text_coords[i3 * 2 + 0] = c1.x;
   text_coords[i3 * 2 + 1] = c1.y;

   text_indices[num_text_indices + 0] = i0;
   text_indices[num_text_indices + 1] = i1;
   text_indices[num_text_indices + 2] = i2;

   text_indices[num_text_indices + 3] = i1;
   text_indices[num_text_indices + 4] = i3;
   text_indices[num_text_indices + 5] = i2;

   num_text_vertices += 4;
   num_text_indices += 6;
}

void PSXScene::addText(Vec2 org, const std::string& text)
{
   const Vec2 sz(0.15, 0.1);

   for(std::string::const_iterator it = text.begin(); it != text.end(); ++it)
   {
      if(!isspace(*it))
      {
         if(*it == '!')
            addTextQuad(org, org + sz, 36);
         else if(*it == '.')
            addTextQuad(org, org + sz, 38);
         else if(isalpha(*it))
            addTextQuad(org, org + sz, tolower(*it) - 'a');
         else if(isdigit(*it))
            addTextQuad(org, org + sz, 26 + *it - '0');
      }
      if(*it == 'w' || *it == 'm')
         org.x += sz.x;
      else if(*it == 'i')
         org.x += sz.x * 0.4;
      else
         org.x += sz.x * 0.7;
   }
}


void PSXScene::update()
{
}


void PSXScene::initialiseMeshes()
{
   amiga_mesh.loadFile("meshes/amiga500.obj");
   atari_mesh.loadFile("meshes/atarist.obj");
   c64_mesh.loadFile("meshes/commodore64.obj");
   gameboy_mesh.loadFile("meshes/gameboy.obj");
   torus_mesh.loadFile("meshes/torus.obj");

   amiga_mesh.centerOrigin();
   atari_mesh.centerOrigin();
   c64_mesh.centerOrigin();
   gameboy_mesh.centerOrigin();
   torus_mesh.centerOrigin();
}

void PSXScene::initializeTextures()
{
   glGenTextures(num_texs, texs);

   screen_tex = texs[0];
   ds_tex = texs[1];
   blur_tex0 = texs[2];
   blur_tex1 = texs[3];
   display_tex = texs[7];
   noise_tex = texs[8];
   rcm_logo_tex = loadTexture("images/RCM.png");
   gb_tex = loadTexture("images/seeyou.png");
   font_tex = loadTexture("images/font2.png");
   credits_tex = loadTexture("images/credits.png");

   glBindTexture(GL_TEXTURE_2D, credits_tex);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
   glBindTexture(GL_TEXTURE_2D, 0);

   createWindowTexture(screen_tex, GL_RGB16F);
   createWindowTexture(ds_tex, GL_RGBA8);
   createWindowTexture(blur_tex0, GL_RGB16F, gauss_divisor);
   createWindowTexture(blur_tex1, GL_RGB16F, gauss_divisor);

   {
      uint w = 128, h = 128;
      uint pix[w*h];

      for(uint y = 0; y < h; ++y)
         for(uint x=0;x<w;++x)
         {
            pix[x+y*w]=(rand()&255) | ((rand()&255) <<8)|((rand()&255))<<16;
         }

      glBindTexture(GL_TEXTURE_2D, noise_tex);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pix);
      CHECK_FOR_ERRORS;
   }


   glBindTexture(GL_TEXTURE_2D, display_tex);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
   glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB16F, window_width, window_height);
   CHECK_FOR_ERRORS;
}

void PSXScene::initializeBuffers()
{
   glGenFramebuffers(num_fbos, fbos);
   glGenRenderbuffers(num_fbos, renderbuffers);

   screen_fbo = fbos[0];
   display_rbo = renderbuffers[0];

   ds_fbo = fbos[1];

   blur_fbo0 = fbos[2];
   blur_fbo1 = fbos[3];

   display_fbo = fbos[4];

   glBindRenderbuffer(GL_RENDERBUFFER, display_rbo);
   glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, window_width, window_height);
   CHECK_FOR_ERRORS;
   glBindRenderbuffer(GL_RENDERBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, screen_fbo);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screen_tex, 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   CHECK_FOR_ERRORS;
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ds_fbo);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ds_tex, 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   CHECK_FOR_ERRORS;
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, blur_fbo0);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blur_tex0, 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   CHECK_FOR_ERRORS;
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, blur_fbo1);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blur_tex1, 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   CHECK_FOR_ERRORS;
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, display_fbo);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, display_tex, 0);
   glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, display_rbo);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   CHECK_FOR_ERRORS;
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void PSXScene::initializeShaders()
{
   screen_shader.load(SHADERS_PATH "psx_screen_v.glsl", NULL, SHADERS_PATH "psx_screen_f.glsl");
   screen_shader.uniform1i("reveal", 0);
   screen_shader.uniform1i("pal", 1);
   screen_shader.uniform1i("logo_tex", 2);
   screen_shader.uniform1f("aspect_ratio", GLfloat(window_width) / GLfloat(window_height));

   screen2_shader.load(SHADERS_PATH "psx_screen2_v.glsl", NULL, SHADERS_PATH "psx_screen2_f.glsl");
   screen2_shader.uniform1i("display", 0);
   screen2_shader.uniform1i("credits_tex", 1);
   screen2_shader.uniform1i("noise_tex", 2);
   screen2_shader.uniform1f("aspect_ratio", GLfloat(window_width) / GLfloat(window_height));
   screen2_shader.uniform1i("gbtex", 3);

   gaussblur_shader.load(SHADERS_PATH "psx_gaussblur2_v.glsl", NULL, SHADERS_PATH "psx_gaussblur2_f.glsl");
   gaussblur_shader.uniform1i("tex0", 0);

   composite_shader.load(SHADERS_PATH "psx_composite_v.glsl", NULL, SHADERS_PATH "psx_composite_f.glsl");
   composite_shader.uniform1i("tex0", 0);
   composite_shader.uniform1i("tex1", 1);

   screendisplay_shader.load(SHADERS_PATH "lines_v.glsl", SHADERS_PATH "lines_g.glsl", SHADERS_PATH "lines_f.glsl");

   zoomblur_shader.load(SHADERS_PATH "zoomblur_v.glsl", NULL, SHADERS_PATH "zoomblur_f.glsl");
   zoomblur_shader.uniform1i("tex0", 0);

   mesh_shader.load(SHADERS_PATH "mesh_v.glsl", NULL, SHADERS_PATH "mesh_f.glsl");
   mesh_shader.uniform1i("tex0", 0);

   torus_shader.load(SHADERS_PATH "torus_v.glsl", NULL, SHADERS_PATH "torus_f.glsl");

   text_shader.load(SHADERS_PATH "psx_text_v.glsl", NULL, SHADERS_PATH "psx_text_f.glsl");
   text_shader.uniform1i("tex", 0);
}

void PSXScene::initialize()
{
   if(initialized)
      return;

   initialized = true;

   initializeShaders();
   initializeTextures();
   initializeBuffers();
   initialiseMeshes();

   zoom_spline.insert(1, 0, 1000);
   //zoom_spline.insert(50, 0, 4000);
   zoom_spline.initialize();

   tint_spline.insert(0, 0, 0);
   tint_spline.insert(1, 0, 400);
   tint_spline.insert(1, 0, 4250);
   tint_spline.insert(0, 0, 4400);
   tint_spline.initialize();
}

void PSXScene::free()
{
}

void PSXScene::gaussianBlur(GLuint srctex)
{
   const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f,
                                +1.0f, +1.0f, -1.0f, +1.0f };


   const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f,
                              1.0f, 1.0f, 0.0f, 1.0f };

   const GLubyte indices[] = { 3, 0, 2, 1 };

   // gaussian blur pass 1

   const float gauss_rad = 0.3f;

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, blur_fbo0);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   glViewport(0, 0, window_width / gauss_divisor, window_height / gauss_divisor);

   CHECK_FOR_ERRORS;

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);

   gaussblur_shader.bind();
   gaussblur_shader.uniform2f("direction", gauss_rad, 0.0f);

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, srctex);

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);



   // gaussian blur pass 2

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, blur_fbo1);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   glViewport(0, 0, window_width / gauss_divisor, window_height / gauss_divisor);


   CHECK_FOR_ERRORS;

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);

   gaussblur_shader.bind();
   gaussblur_shader.uniform2f("direction", 0.0f, gauss_rad);

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, blur_tex0);

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   CHECK_FOR_ERRORS;
}

void PSXScene::bindMaterialForMesh(const model::Material *const mat)
{
   glActiveTexture(GL_TEXTURE0);

   if(!mat)
   {
      glBindTexture(GL_TEXTURE_2D, 0);
      return;
   }

   if(mat->texture_diffuse)
   {
      glBindTexture(GL_TEXTURE_2D, mat->texture_diffuse->handle);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      mesh_shader.uniform1f("tbias", 1);
   }
   else
   {
      glBindTexture(GL_TEXTURE_2D, 0);
      mesh_shader.uniform1f("tbias", 0);
   }

   mesh_shader.uniform4f("colour", mat->diffuse.x, mat->diffuse.y, mat->diffuse.z, 1);
}

void PSXScene::renderModel(const model::Mesh *const m)
{
   const model::Material* mat = 0;

   static GLushort indices[1024*4];
   static GLfloat vertices[1024*4*3];
   static GLfloat coords[1024*4*2];
   int num_vertices=0;

   glDisable(GL_CULL_FACE);

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);

   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);

   for(int j=0;j<m->triangles.size();++j)
   {
      const model::Mesh::Triangle& t = m->triangles[j];

      if(t.material != mat)
      {
         if(mat)
         {
            bindMaterialForMesh(mat);
            glDrawRangeElements(GL_TRIANGLES, 0, num_vertices, num_vertices, GL_UNSIGNED_SHORT, indices);
         }

         mat = t.material;
         num_vertices=0;
      }

      indices[num_vertices] = num_vertices;
      vertices[num_vertices*3+0]=m->vertices[t.a].x;
      vertices[num_vertices*3+1]=m->vertices[t.a].y;
      vertices[num_vertices*3+2]=m->vertices[t.a].z;
      coords[num_vertices*2+0]=t.texcoords[0].x;
      coords[num_vertices*2+1]=t.texcoords[0].y;
      ++num_vertices;

      indices[num_vertices] = num_vertices;
      vertices[num_vertices*3+0]=m->vertices[t.b].x;
      vertices[num_vertices*3+1]=m->vertices[t.b].y;
      vertices[num_vertices*3+2]=m->vertices[t.b].z;
      coords[num_vertices*2+0]=t.texcoords[1].x;
      coords[num_vertices*2+1]=t.texcoords[1].y;
      ++num_vertices;

      indices[num_vertices] = num_vertices;
      vertices[num_vertices*3+0]=m->vertices[t.c].x;
      vertices[num_vertices*3+1]=m->vertices[t.c].y;
      vertices[num_vertices*3+2]=m->vertices[t.c].z;
      coords[num_vertices*2+0]=t.texcoords[2].x;
      coords[num_vertices*2+1]=t.texcoords[2].y;
      ++num_vertices;

      if(j == (m->triangles.size()-1))
      {
         bindMaterialForMesh(mat);
         glDrawRangeElements(GL_TRIANGLES, 0, num_vertices, num_vertices, GL_UNSIGNED_SHORT, indices);
      }

   }

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
}

void PSXScene::renderDisplayWires(float t, const Mat4& projection)
{
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);

   mesh_shader.bind();

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, 0);


   const Real wait_t = 3, transit_t = 2, t_per_item = wait_t + transit_t;
   const int num_items = 4;

   const Real dr = (2.0 * M_PI) / num_items;
   const Real item_r_rate = 0.33;

   Real item_r = t * item_r_rate, gameboy_r = item_r;

   const Real bounce = std::pow(std::min(1.0, std::max(0.0, (t - 2.0) / -3.0)), 4.0);

   t = std::max(0.0f, t);

   Real r = (floor(t / t_per_item) + cubic(std::max(0.0, fmod(t, t_per_item) - wait_t) / transit_t)) * dr;
   Real cd = -10;
   Real y = -4, z = -20;
   Real vrr = 0.3;

   if((t / t_per_item) > (num_items - 1))
   {
      r = M_PI * 1.5;
      double a = cubic(std::min(1.0, (t - t_per_item * (num_items - 1)) / 2.5)), er = t_per_item * (num_items - 1) * item_r_rate, b = 1.0 - a;
      gameboy_r = (floor(er / M_PI) - 1.2 + 2) * M_PI * a + er * b;

      cd = cd * b + a * -17;
      y = y * b + a * -2;
      vrr = vrr * b + a * 0.5;

      screen2_shader.uniform1f("gb_appear", a);
   }

   y += bounce * (-cos(time * 10) * (10 + std::pow(bounce, 10.0f) * 10));

   if(music_time < 84)
   {
      const float f = fmod(((music_time / 60.0) * 143.0 - 1.5) * 0.5, 1.0);
      y += std::min(std::pow(1.0 - f, 10.0) * 0.2, f * 10.0);
   }

   // TORUS

   {
      float last_arp=0.0f;

      for(int i=0;i<num_alk_arps;++i)
      {
         if(music_time > alk_arps[i])
            last_arp=alk_arps[i];
         else
            break;
      }

      const float rate = 1.0f;

      torus_shader.uniform1f("arp", std::pow(1.0f - std::min(1.0f, (music_time - last_arp) * rate), 2.0f));
   }

   torus_shader.bind();
   {
      const float ms = 10;
      Mat4 modelview = Mat4::translation(Vec3(cos(time) * ms, sin(time) * ms, 0.0)) * Mat4::rotation(cos(time * 0.6) * 2, Vec3(0, 0, 1)) * Mat4::rotation(M_PI * 0.5, Vec3(0, 1, 0)) *
               Mat4::scale(Vec3(300, 200, 100)) * Mat4::translation(Vec3(0, 0, -1)) * Mat4::rotation(-time, Vec3(0, 1, 0));
      torus_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
      renderModel(&torus_mesh);
   }

   mesh_shader.bind();


   if(bounce < 1.0)
      for(int n =0; n<2;++n)
      {
         const Mat4 rotm = Mat4::rotation(item_r, Vec3(0.0f, 1.0f, 0.0f)) * Mat4::rotation(1.0, Vec3(1.0f, 0.0f, 0.0f)) * Mat4::scale(Vec3(1.2, 1.2, 1.2));
         const Mat4 vr = Mat4::rotation(vrr, Vec3(1, 0, 0));
         Mat4 outline_scaler = Mat4::identity();

         if(n)
         {
            mesh_shader.uniform1f("brightness", 1);
            glClear(GL_DEPTH_BUFFER_BIT);
         }
         else
         {
            mesh_shader.uniform1f("brightness", 0);
            const float s = 1.04;
            outline_scaler = outline_scaler * Mat4::scale(Vec3(s, s, s));
         }

         {
            Mat4 modelview = vr * Mat4::translation(Vec3(0, y, z)) * Mat4::rotation(r + dr * 0, Vec3(0.0f, 1.0f, 0.0f)) * Mat4::translation(Vec3(0, 0, cd)) * rotm * outline_scaler;
            mesh_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
            renderModel(&amiga_mesh);
         }

         {
            Mat4 modelview = vr * Mat4::translation(Vec3(0, y, z)) * Mat4::rotation(r + dr * 1, Vec3(0.0f, 1.0f, 0.0f)) * Mat4::translation(Vec3(0, 0, cd)) * rotm * outline_scaler;
            mesh_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
            renderModel(&atari_mesh);
         }

         {
            Mat4 modelview = vr * Mat4::translation(Vec3(0, y, z)) * Mat4::rotation(r + dr * 2, Vec3(0.0f, 1.0f, 0.0f)) * Mat4::translation(Vec3(0, 0, cd)) * Mat4::rotation(M_PI, Vec3(0.0f, 1.0f, 0.0f)) *
                     rotm * outline_scaler;
            mesh_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
            renderModel(&c64_mesh);
         }

         {
            Mat4 modelview = vr * Mat4::translation(Vec3(0, y, z)) * Mat4::rotation(r + dr * 3, Vec3(0.0f, 1.0f, 0.0f)) * Mat4::translation(Vec3(0, 0, cd)) * Mat4::rotation(M_PI * 1.2, Vec3(0.0f, 1.0f, 0.0f)) *
                                          Mat4::rotation(gameboy_r, Vec3(0.0f, 1.0f, 0.0f)) * Mat4::rotation(1.0, Vec3(1.0f, 0.0f, 0.0f)) * Mat4::scale(Vec3(1.2, 1.2, 1.2)) * outline_scaler;
            mesh_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
            renderModel(&gameboy_mesh);
         }
      }


   // TEXT
   {
      Mat4 modelview = Mat4::translation(Vec3(2.0 - time, 0.52, -1.8));

      num_text_indices = 0;
      num_text_vertices = 0;

      static const std::string texts[] = {
         "relive the classics!.. sinclair .. amstrad .. sega .. acorn .. "
         "commodore .. nintendo .. apple .. tandy .. sony .. philips .."
         " and much more!"
         };

      addText(Vec2(0, 0), texts[0]);

      glDisable(GL_DEPTH_TEST);
      glDepthMask(GL_FALSE);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      if(num_text_indices > 0)
      {
         glEnableVertexAttribArray(0);
         glEnableVertexAttribArray(1);

         glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, text_vertices);
         glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, text_coords);

         text_shader.bind();
         text_shader.uniform1f("time", time);

         glActiveTexture(GL_TEXTURE0);
         glBindTexture(GL_TEXTURE_2D, font_tex);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


         text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (modelview * Mat4::translation(Vec3(-0.01, -0.01, 0.0))).e);
         text_shader.uniform3f("colour", 0, 0, 0);
         glDrawRangeElements(GL_TRIANGLES, 0, num_text_vertices, num_text_indices, GL_UNSIGNED_SHORT, text_indices);

         text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
         text_shader.uniform3f("colour", 1, 1, 1);
         glDrawRangeElements(GL_TRIANGLES, 0, num_text_vertices, num_text_indices, GL_UNSIGNED_SHORT, text_indices);



         glDisableVertexAttribArray(0);
         glDisableVertexAttribArray(1);
      }
   }


   CHECK_FOR_ERRORS;
}

void PSXScene::renderDisplay()
{
   const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f,
                                +1.0f, +1.0f, -1.0f, +1.0f };

   const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f,
                              1.0f, 1.0f, 0.0f, 1.0f };

   const GLubyte indices[] = { 3, 0, 2, 1 };

   const float aspect_ratio = float(window_height) / float(window_width);

   Mat4 projection = Mat4::identity();

   const float fov = M_PI * 0.4f;
   const float znear = 0.001f / tan(fov * 0.5f), zfar = 400.0f;

   projection = Mat4::frustum(-1.0f * znear, +1.0f * znear, -aspect_ratio * znear, +aspect_ratio * znear, znear, zfar) * projection;

   screendisplay_shader.uniform2f("radii", 0.01f, 0.01f);

   screendisplay_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);
   mesh_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);
   torus_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);
   torus_shader.uniform1f("time", time);
   text_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, display_fbo);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   glViewport(0, 0, window_width, window_height);

   glDepthMask(GL_TRUE);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glDepthMask(GL_FALSE);

   // outer screen
   screen_shader.bind();
   screen_shader.uniform1f("time", time);
   screen_shader.uniform1f("tint", tint_spline.evaluate(frame_num) + 0.001f);

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, rcm_logo_tex);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_1D, 0);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, 0);

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   CHECK_FOR_ERRORS;

   for(int i = 0; i < 1; ++i)
   {
      GLfloat c = std::pow(GLfloat(0.25) + GLfloat(i) / GLfloat(8), 2.0);
      screendisplay_shader.uniform4f("colour", c, c, c, c);
      renderDisplayWires(time + GLfloat(i) * 0.1, projection);
   }
}



void PSXScene::render()
{
   const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f,
                                +1.0f, +1.0f, -1.0f, +1.0f };

   const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f,
                              1.0f, 1.0f, 0.0f, 1.0f };

   const GLubyte indices[] = { 3, 0, 2, 1 };

   const GLfloat zoom = zoom_spline.evaluate(frame_num);

   const GLfloat zoomed_vertices[] = { -zoom, -zoom, +zoom, -zoom,
                                       +zoom, +zoom, -zoom, +zoom };


   time -= 3;


   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, screen_fbo);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   glViewport(0, 0, window_width, window_height);

   glClear(GL_COLOR_BUFFER_BIT);




   renderDisplay();


   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, screen_fbo);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   glViewport(0, 0, window_width, window_height);

   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);

   // inner screen
   screen2_shader.bind();
   screen2_shader.uniform1f("time", time);
   screen2_shader.uniform1f("tint", tint_spline.evaluate(frame_num));
   screen2_shader.uniform1f("dilate", 0);

   glActiveTexture(GL_TEXTURE3);
   glBindTexture(GL_TEXTURE_2D, gb_tex);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, noise_tex);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, credits_tex);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, display_tex);

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, zoomed_vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   CHECK_FOR_ERRORS;

   glDisable(GL_BLEND);


   downsample8x(screen_tex, ds_fbo, ds_tex, blur_fbo1, window_width, window_height);

   gaussianBlur(screen_tex);

   glBindFramebuffer(GL_READ_FRAMEBUFFER, screen_fbo);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glViewport(0, 0, window_width, window_height);

   glBlitFramebuffer(0, 0, window_width, window_height,
                     0, 0, window_width, window_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

   glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);



   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);

   composite_shader.bind();
   composite_shader.uniform1f("time",time+3);
   composite_shader.uniform1f("darken",0.0f);
   composite_shader.uniform1i("frame_num", frame_num);
   composite_shader.uniform1f("aspect_ratio", float(window_width) / float(window_height));

   {
      float s = 1.0f + cosf(time * 30.0f) * 0.03f;
      composite_shader.uniform4f("tint", s * 1.3f, s * 1.2f, s * 1.0f, 1.0f);
   }

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, noise_tex);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, blur_tex1);

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   glDisable(GL_BLEND);


}

