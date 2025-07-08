
#include "Engine.hpp"

#include <cctype>

static const char *const g_letters[] = { "A0227755334", "B05577443", "C200557", "D35577234", "E0557022443", "F053402", "G0334022775",
         "H053447", "I16", "J2445", "K053237", "L0557", "M05162702", "N050447", "O05025727", "P05022443",
         "Q03023427", "R0502", "S0203344775", "T0216", "U052757", "V033772", "W05162757", "X0725", "Y03342775",
         "Z022557", "116", "20224433557", "302273457", "4270334", "50203344775", "60502344775", "70227",
         "80502275734", "90203342757", "00205275725", "[121667", "]011656", ".66", ",85", "!1866", "-34", "_57" };

static const Vec2 g_letterPoints[] = {  Vec2(0.0f, 0.0f), Vec2(0.5f, 0.0f), Vec2(1.0f, 0.0f),
                                        Vec2(0.0f, 0.5f), Vec2(1.0f, 0.5f), Vec2(0.0f, 1.0f),
                                        Vec2(0.5f, 1.0f), Vec2(1.0f, 1.0f), Vec2(0.5f, 0.5f),
                                        Vec2(0.0f, 0.0f) };

static const int g_letterCount = sizeof(g_letters) / sizeof(g_letters[0]);

static GLushort text_indices[1024*2];
static GLfloat text_vertices[1024*2*3];
static int num_text_indices=0, num_text_vertices=0;

class TVScene: public Scene
{
   float dilate;

   Shader screen_shader, gaussblur_shader, composite_shader;
   Shader screen2_shader, screendisplay_shader, zoomblur_shader;
   Shader particles_shader;

   Spline<Real> cube_dist_spline, zoom_spline, tint_spline;
   Spline<Real> logo_disappear_spline;

   static const int num_fbos = 16, num_texs = 16;

   GLuint fbos[num_fbos], texs[num_texs], renderbuffers[num_fbos];

   GLuint screen_fbo, screen_rbo, screen_tex;
   GLuint ds_fbo, ds_tex; // downsampling
   GLuint blur_fbo0, blur_fbo1, blur_tex0, blur_tex1;
   GLuint noise_tex;
   GLuint display_fbo, display_tex; // what is displayed onscreen

   GLuint rcm_logo_tex;

   void initializeTextures();
   void initializeShaders();
   void initializeBuffers();

   void gaussianBlur(GLuint srctex);
   void renderDisplay();
   void renderDisplayWires(const float t, const Mat4& projection);

   public:
      TVScene()
      {
      }

      void initialize();
      void render();
      void update();
      void free();
};

Scene* tvScene = new TVScene();

const int gauss_divisor = 4;

void TVScene::update()
{
}

void TVScene::initializeTextures()
{
   glGenTextures(num_texs, texs);

   screen_tex = texs[0];
   ds_tex = texs[1];
   blur_tex0 = texs[2];
   blur_tex1 = texs[3];
   display_tex = texs[7];
   noise_tex = texs[8];
   rcm_logo_tex = loadTexture("images/RCM.png");

   createWindowTexture(screen_tex, GL_RGBA8);
   createWindowTexture(ds_tex, GL_RGBA8);
   createWindowTexture(blur_tex0, GL_RGBA16, gauss_divisor);
   createWindowTexture(blur_tex1, GL_RGBA16, gauss_divisor);



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
   glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, window_width, window_height);
   CHECK_FOR_ERRORS;
}

void TVScene::initializeBuffers()
{
   glGenFramebuffers(num_fbos, fbos);
   glGenRenderbuffers(num_fbos, renderbuffers);

   screen_fbo = fbos[0];
   screen_rbo = renderbuffers[0];

   ds_fbo = fbos[1];

   blur_fbo0 = fbos[2];
   blur_fbo1 = fbos[3];

   display_fbo = fbos[4];

   glBindRenderbuffer(GL_RENDERBUFFER, screen_rbo);
   glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, window_width, window_height);
   CHECK_FOR_ERRORS;
   glBindRenderbuffer(GL_RENDERBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, screen_fbo);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screen_tex, 0);
   glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, screen_rbo);
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
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   CHECK_FOR_ERRORS;
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void TVScene::initializeShaders()
{
   screen_shader.load(SHADERS_PATH "screen_v.glsl", NULL, SHADERS_PATH "screen_f.glsl");
   screen_shader.uniform1i("reveal", 0);
   screen_shader.uniform1i("pal", 1);
   screen_shader.uniform1f("aspect_ratio", GLfloat(window_width) / GLfloat(window_height));

   screen2_shader.load(SHADERS_PATH "screen2_v.glsl", NULL, SHADERS_PATH "screen2_f.glsl");
   screen2_shader.uniform1i("display", 0);
   screen2_shader.uniform1i("pal", 1);
   screen2_shader.uniform1i("noise_tex", 2);
   screen2_shader.uniform1f("aspect_ratio", GLfloat(window_width) / GLfloat(window_height));

   gaussblur_shader.load(SHADERS_PATH "gaussblur2_v.glsl", NULL, SHADERS_PATH "gaussblur2_f.glsl");
   gaussblur_shader.uniform1i("tex0", 0);

   composite_shader.load(SHADERS_PATH "composite_v.glsl", NULL, SHADERS_PATH "composite_f.glsl");
   composite_shader.uniform1i("tex0", 0);
   composite_shader.uniform1i("tex1", 1);

   screendisplay_shader.load(SHADERS_PATH "lines_v.glsl", SHADERS_PATH "lines_g.glsl", SHADERS_PATH "lines_f.glsl");

   particles_shader.load(SHADERS_PATH "particles_v.glsl", SHADERS_PATH "particles_g.glsl", SHADERS_PATH "particles_f.glsl");
   particles_shader.uniform1i("logo_tex", 0);

   zoomblur_shader.load(SHADERS_PATH "zoomblur_v.glsl", NULL, SHADERS_PATH "zoomblur_f.glsl");
   zoomblur_shader.uniform1i("tex0", 0);
   zoomblur_shader.uniform1i("noise_tex", 1);
}

void TVScene::initialize()
{
   if(initialized)
      return;

   initialized = true;

   initializeShaders();
   initializeTextures();
   initializeBuffers();

   cube_dist_spline.insert(-40, 0, 600);
   cube_dist_spline.insert(-3, 0, 1000);
   cube_dist_spline.insert(-3, 0, 2200);
   cube_dist_spline.insert(-40, 0, 2600);
   cube_dist_spline.initialize();

   zoom_spline.insert(1, 0, 1000);
   //zoom_spline.insert(50, 0, 4000);
   zoom_spline.initialize();

   logo_disappear_spline.insert(0, 0, 3000);
   logo_disappear_spline.insert(1, 0, 3500);
   logo_disappear_spline.initialize();

   tint_spline.insert(0, 0, 0);
   tint_spline.insert(1, 0, 400);
   tint_spline.insert(1, 0, 4250);
   tint_spline.insert(0, 0, 4400);
   tint_spline.initialize();
}

void TVScene::free()
{
}

void TVScene::gaussianBlur(GLuint srctex)
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

static void addTextLine(const Vec2& p0, const Vec2& p1)
{
   text_indices[num_text_indices++]=num_text_vertices;
   text_vertices[num_text_vertices*3+0]=p0.x;
   text_vertices[num_text_vertices*3+1]=p0.y;
   text_vertices[num_text_vertices*3+2]=0;
   num_text_vertices++;

   text_indices[num_text_indices++]=num_text_vertices;
   text_vertices[num_text_vertices*3+0]=p1.x;
   text_vertices[num_text_vertices*3+1]=p1.y;
   text_vertices[num_text_vertices*3+2]=0;
   num_text_vertices++;
}

static void addText(const char* str, Vec2 pos, const Vec2 step, const Vec2 size)
{
    const Real ox = pos.x;
    for(; *str; ++str)
    {
        const char c = toupper(*str);

        if(c == '\n')
        {
            pos.x = ox;
            pos.y += step.y;
            continue;
        }

        for(int i = 0; i < g_letterCount; ++i)
        {
            const char* l = g_letters[i];

            if(l[0] == c)
            {
                ++l;

                for(; *l; l += 2)
                {
                    addTextLine(pos + g_letterPoints[l[0] - '0'] * size,
                            pos + g_letterPoints[l[1] - '0'] * size);
                }
                break;
            }
        }

        pos.x += step.x;
    }
}


void TVScene::renderDisplayWires(const float t, const Mat4& projection)
{
   screendisplay_shader.bind();

   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_BLEND);


   // grid
   for(int g=0;g<2;++g)
   {
      Mat4 modelview = Mat4::identity();

      const double dis = std::max(0.0f, t - 24) * 0.4;
      const double c=cubic(clamp((t-7),0.0,1.0));

      modelview = modelview * Mat4::translation(Vec3(0, 0, -20 - dis*80));

      if(t > 24)
         modelview = modelview * Mat4::rotation(t - 24, Vec3(1.0f, 0.0f, 0.0f));

      modelview = modelview * Mat4::rotation(t * 0.33, Vec3(0.0f, 1.0f, 0.0f));

      screendisplay_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
      screendisplay_shader.uniform1f("brightness", (mix(20.0, 0.05, c) + dis) * (1.0 - clamp((t - 26), 0.0, 1.0)) );


      float last_arp=0.0f;

      for(int i=0;i<num_alk_arps;++i)
      {
         if(music_time > alk_arps[i])
            last_arp=alk_arps[i];
         else
            break;
      }

      particles_shader.uniform1f("arp", 30.0f / (1.0f + (music_time - last_arp) * 5.0f));


      float h=mix(90,4,c)*(1.0-g*2.0);
      float hwobble = std::max(0.0f, 1.0f - (music_time - last_arp) * 0.9f);
      float s=3;
      int grid_width=8, grid_height=8;

      GLushort indices[grid_width * grid_height * 8], num_indices = 0;
      GLfloat vertices[grid_width * grid_height * 3];

      for(int y=0;y<grid_height;++y)
         for(int x=0;x<grid_width;++x)
         {
            int i=x+y*grid_width;
            vertices[i*3+0]=(-(grid_width-1)*0.5f + x)*s;
            vertices[i*3+1]=h + hwobble * cos(x+time*4*2) * sin(y+time*5.2*2);
            vertices[i*3+2]=(-(grid_height-1)*0.5f + y)*s;

            if(x < (grid_width-1))
            {
               indices[num_indices++]=i;
               indices[num_indices++]=i+1;
            }

            if(y < (grid_height-1))
            {
               indices[num_indices++]=i;
               indices[num_indices++]=i+grid_width;
            }

            if(x > 0)
            {
               indices[num_indices++]=i;
               indices[num_indices++]=i-1;
            }

            if(y > 0)
            {
               indices[num_indices++]=i;
               indices[num_indices++]=i-grid_width;
            }
         }


      glEnableVertexAttribArray(0);

      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices);

      glDisable(GL_DEPTH_TEST);
      glDepthMask(GL_FALSE);

      glDrawRangeElements(GL_LINES, 0, grid_width * grid_height, num_indices, GL_UNSIGNED_SHORT, indices);

      glDisableVertexAttribArray(0);

/*
      GLushort indices[(grid_width+1) * 2 + (grid_height+1) * 2];
      GLfloat vertices[((grid_width+1) * 2 + (grid_height+1) * 2)*3];

      for(int i=0;i<=grid_width;++i)
      {
         vertices[i*6+0]=(-grid_width*0.5f + i)*s;
         vertices[i*6+1]=h;
         vertices[i*6+2]=(-grid_height*0.5f)*s;

         vertices[i*6+3]=(-grid_width*0.5f + i)*s;
         vertices[i*6+4]=h;
         vertices[i*6+5]=(+grid_height*0.5f)*s;

         indices[i*2+0]=i*2+0;
         indices[i*2+1]=i*2+1;
      }
      for(int i=0;i<=grid_height;++i)
      {
         vertices[(grid_width+1)*6+i*6+0]=(-grid_width*0.5f)*s;
         vertices[(grid_width+1)*6+i*6+1]=h;
         vertices[(grid_width+1)*6+i*6+2]=(-grid_height*0.5f + i)*s;

         vertices[(grid_width+1)*6+i*6+3]=(+grid_width*0.5f)*s;
         vertices[(grid_width+1)*6+i*6+4]=h;
         vertices[(grid_width+1)*6+i*6+5]=(-grid_height*0.5f + i)*s;

         indices[(grid_width+1)*2+i*2+0]=(grid_width+1+i)*2+0;
         indices[(grid_width+1)*2+i*2+1]=(grid_width+1+i)*2+1;
      }

      glEnableVertexAttribArray(0);

      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices);

      glDisable(GL_DEPTH_TEST);
      glDepthMask(GL_FALSE);

      glDrawRangeElements(GL_LINES, 0, (grid_width+1) * 2 + (grid_height+1) * 2, (grid_width+1) * 2 + (grid_height+1) * 2, GL_UNSIGNED_SHORT, indices);

      glDisableVertexAttribArray(0);
*/
   }

   // text
   {
      Real transit_t = 2.0, wait_t = 2.0;
      Real t_per_line = transit_t * 2.0 + wait_t;

      int line = int(floor(t / t_per_line));
      Real tf = t - line * t_per_line;
      Real x = cubic(1.0 - tf / transit_t);

      if(tf > (wait_t + transit_t))
         x = cubic((tf - (wait_t + transit_t)) / transit_t) * -1;
      else if(tf > transit_t)
         x = 0;

      static const int num_texts=4,num_rows=2;
      static const std::string texts[num_texts][num_rows] = { {"", ""}, {"please", "accept"}, {"this", "invitation"}, {"for", ""} };

      if(line < num_texts)
      {
         const Mat4 modelview = Mat4::translation(Vec3(0, sin(t*2.2)*0.2+0.5, -10+sin(t)*0.1));

         screendisplay_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
         screendisplay_shader.uniform1f("brightness", 1);

         glDisable(GL_DEPTH_TEST);
         glDepthMask(GL_FALSE);
         glEnableVertexAttribArray(0);
         glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, text_vertices);

         num_text_indices=0;
         num_text_vertices=0;

         for(int row=0;row<num_rows;++row)
         {
            const std::string& text = texts[line][row];
            const Vec2 text_size = Vec2(1.5, -1.0), text_step = Vec2(2.0, -1.6);

            addText(text.c_str(), Vec2(x * 30 - text_step.x * 0.5 * text.size(), texts[line][1].empty() ? 0.0 : (-0.5 + row) * text_step.y ), text_step, text_size);
         }

         glDrawRangeElements(GL_LINES, 0, num_text_vertices, num_text_indices, GL_UNSIGNED_SHORT, text_indices);
         glDisableVertexAttribArray(0);
      }


      // TEXT UNDER THE LOGOO
      {
         const Mat4 modelview = Mat4::translation(Vec3(0, -10, -22));

         screendisplay_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
         screendisplay_shader.uniform1f("brightness", std::min(1.0f, std::max(0.0f, (t - 27) * 0.3f)));

         glDisable(GL_DEPTH_TEST);
         glDepthMask(GL_FALSE);
         glEnableVertexAttribArray(0);
         glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, text_vertices);

         num_text_indices=0;
         num_text_vertices=0;


         const std::string& text = "retro computer museum";
         const Vec2 text_size = Vec2(1.0, -1.0), text_step = Vec2(1.3, -1.6);

         addText(text.c_str(), Vec2(-text_step.x * 0.5 * text.size(), 0.0), text_step, text_size);


         glDrawRangeElements(GL_LINES, 0, num_text_vertices, num_text_indices, GL_UNSIGNED_SHORT, text_indices);
         glDisableVertexAttribArray(0);
      }

   }

   particles_shader.bind();
   particles_shader.uniform1f("time", time);
   particles_shader.uniform1f("logo_bias", 1);
   particles_shader.uniform1f("radius", 0.02 + 0.07 * logo_disappear_spline.evaluate(frame_num));
   particles_shader.uniform1f("disappear", logo_disappear_spline.evaluate(frame_num));
   screen2_shader.uniform1f("disappear", cubic(1.0 - clamp(music_time / alk_boops[0], 0.0, 1.0)) * 0.2 + logo_disappear_spline.evaluate(frame_num));
   particles_shader.uniform1f("aspect_ratio", float(window_height) / float(window_width));

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, rcm_logo_tex);


   // logo
   if(t > 26)
   {
      Mat4 modelview = Mat4::identity();

      modelview = modelview * Mat4::translation(Vec3(0, 0, -320 - 1200 * (1.0 - cubic(clamp((t - 26) * 0.4, 0.0, 1.0))) )) * Mat4::scale(Vec3(2, 2, 2));
      //modelview = modelview * Mat4::rotation(t, Vec3(1.0f, 0.0f, 0.0f));
      // modelview = modelview * Mat4::rotation(t * 0.33, Vec3(0.0f, 1.0f, 0.0f));

      particles_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
      particles_shader.uniform1f("brightness", 0.15);

      glDisable(GL_DEPTH_TEST);
      glDepthMask(GL_FALSE);

      const float aspect=815.0f/450.0f;
      const int w=128,h=128;

      GLfloat vertices[w*h*3];
      GLfloat coords[w*h*2];
      GLushort indices[w*h];

      for(int y=0;y<h;++y)
         for(int x=0;x<w;++x)
         {
            const int vi=(x+y*w)*3;
            const int ci=(x+y*w)*2;

            vertices[vi+0]=(x-w*0.5)*aspect;
            vertices[vi+1]=y-h*0.5;
            vertices[vi+2]=0;

            coords[ci+0]=float(x)/float(w);
            coords[ci+1]=float(y)/float(h);

            indices[x+y*w]=x+y*w;
         }

      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);
      glDrawRangeElements(GL_POINTS, 0, w*h, w*h, GL_UNSIGNED_SHORT, indices);
      glDisableVertexAttribArray(0);
      glDisableVertexAttribArray(1);
   }



   CHECK_FOR_ERRORS;
}

void TVScene::renderDisplay()
{
   const float aspect_ratio = float(window_height) / float(window_width);

   Mat4 projection = Mat4::identity();

   const float fov = M_PI * 0.7f;
   const float znear = 1.0f / tan(fov * 0.5f), zfar = 100.0f;

   projection = Mat4::frustum(-1.0f, +1.0f, -aspect_ratio, +aspect_ratio, znear, zfar) * projection;

   screendisplay_shader.uniform2f("radii", 0.01f+dilate*0.01f, 0.01f+dilate*0.01f);

   screendisplay_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);
   particles_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, display_fbo);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   glViewport(0, 0, window_width, window_height);

   glClear(GL_COLOR_BUFFER_BIT);

   for(int i = 0; i < 8; ++i)
   {
      GLfloat c = std::pow(GLfloat(0.25) + GLfloat(i) / GLfloat(8), 2.0);
      screendisplay_shader.uniform4f("colour", c, c, c, c);
      particles_shader.uniform4f("colour", c, c, c, c);
      renderDisplayWires(time + GLfloat(i) * 0.1, projection);
   }
}



void TVScene::render()
{
   const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f,
                                +1.0f, +1.0f, -1.0f, +1.0f };

   const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f,
                              1.0f, 1.0f, 0.0f, 1.0f };

   const GLubyte indices[] = { 3, 0, 2, 1 };

   const GLfloat zoom = zoom_spline.evaluate(frame_num);

   const GLfloat zoomed_vertices[] = { -zoom, -zoom, +zoom, -zoom,
                                       +zoom, +zoom, -zoom, +zoom };


   {
      float last_boop=0.0f;
      int j=0;
      for(int i=0;i<num_alk_boops;++i)
      {
         if(music_time > alk_boops[i])
         {
            last_boop=alk_boops[i];
            j=i;
         }
         else
            break;
      }

      float rate=1;

      if(j < (num_alk_boops - 1) && (alk_boops[j+1]-alk_boops[j]) < 1)
         rate=3;

      //dilate=2.1-2.0*std::min(1.0,fmod(time,2.0));//+cos(time*4);
      dilate=((last_boop > 0.0f) ? ((1.0 - std::min(1.0f, (music_time - last_boop) * rate * 0.6f)) * 2) : 0.0) + 0.1;

      //if(music_time < alk_boops[0])
        // dilate += music_time / alk_boops[0] * 0.3;
   }


   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, screen_fbo);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   glViewport(0, 0, window_width, window_height);

   // outer screen
   screen_shader.bind();
   screen_shader.uniform1f("time", Real(uint(time * 20)) * Real(0.05));
   screen_shader.uniform1f("tint", tint_spline.evaluate(frame_num) + 0.001f);

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_1D, 0);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, 0);

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, zoomed_vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   CHECK_FOR_ERRORS;


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
   screen2_shader.uniform1f("dilate", dilate);

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, noise_tex);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_1D, 0);
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
   composite_shader.uniform1f("darken",0.0f);
   composite_shader.uniform1i("frame_num", frame_num);

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


   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);

   zoomblur_shader.bind();
   zoomblur_shader.uniform2f("scale", -0.7f, -0.7f);

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, noise_tex);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, screen_tex);

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   glDisable(GL_BLEND);
}

