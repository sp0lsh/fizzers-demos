
#include "Particles.hpp"
#include "Ran.hpp"

using namespace particles;

static inline Real frand()
{
   static Ran rnd(332);
   return rnd.doub();
}

static void drawQuad()
{
   static GLuint g_quad_vbo = 0;

   if(g_quad_vbo == 0)
   {
      GLfloat verts[8] = { +1.0f,  -1.0f, +1.0f, +1.0f,  -1.0f,  -1.0f,  -1.0f, +1.0f };
      glGenBuffers(1, &g_quad_vbo);
      glBindBuffer(GL_ARRAY_BUFFER, g_quad_vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
   }

   glBindBuffer(GL_ARRAY_BUFFER, g_quad_vbo);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
   glEnableVertexAttribArray(0);
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
   glDisableVertexAttribArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void drawQuadSection(int section)
{
   static const int num_sections = 2;
   static GLuint vbo = 0;

   section %= num_sections;

   if(vbo == 0)
   {
      GLfloat verts[(2 + 2 * num_sections) * 2] = { +1.0f,  -1.0f, +1.0f, +1.0f,  -1.0f,  -1.0f,  -1.0f, +1.0f };

      for(int i=0;i<=num_sections;++i)
      {
         verts[i * 4 + 0] = +1.0f - 2.0f / num_sections * i;
         verts[i * 4 + 1] = -1.0f;
         verts[i * 4 + 2] = +1.0f - 2.0f / num_sections * i;
         verts[i * 4 + 3] = +1.0f;
      }

      glGenBuffers(1, &vbo);
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
   }

   glBindBuffer(GL_ARRAY_BUFFER, vbo);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
   glEnableVertexAttribArray(0);
   glDrawArrays(GL_TRIANGLE_STRIP, section * 2, 4);
   glDisableVertexAttribArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Camera::Camera()
{
}

const Mat4& Camera::getModelView() const
{
   return modelview;
}

const Mat4& Camera::getProjection() const
{
   return projection;
}

const Mat4& Camera::getInvModelView() const
{
   return inv_modelview;
}

const Mat4& Camera::getInvProjection() const
{
   return inv_projection;
}

void Camera::setModelView(Mat4& m)
{
   modelview = m;
   inv_modelview = modelview.inverse();
}

void Camera::setProjection(Mat4& m)
{
   projection = m;
   inv_projection = projection.inverse();
}


Particles::Particles():
   particle_pos_tex(0),
   particle_vel_tex(0),
   depth_tex(0),
   pm_shader_program(0),
   particle_shader_program(0),
   particle_tex_size(32),
   particle_fbo(0),
   camera(0),
   pm_shader_view_mv_loc(0),
   pm_shader_view_p_loc(0),
   pm_shader_inv_view_mv_loc(0),
   pm_shader_inv_view_p_loc(0),
   particle_shader_modelview_loc(0),
   particle_shader_projection_loc(0),
   frame_width(0),
   frame_height(0),
   particle_scale(1.0f),
   particle_shader_scale_loc(0),
   gravity_scale(1.0f),
   pm_shader_gravity_scale_loc(0),
   initial_spread(1.0f),
   section_counter(0)
{

}

void Particles::resetPositions()
{
   GLfloat* dat = new GLfloat[particle_tex_size * particle_tex_size * 3];

   for(int y = 0; y < particle_tex_size; ++y)
      for(int x = 0; x < particle_tex_size; ++x)
      {
         float px, py, pz, r;

         do
         {
            px = frand() - 0.5f;
            pz = frand() - 0.5f;

            r = px * px + pz * pz;

         } while(r > 0.25f);

         py = frand() * 0.5f;

         dat[(x + y * particle_tex_size) * 3 + 0] = px * initial_spread * 40;
         dat[(x + y * particle_tex_size) * 3 + 1] = py * initial_spread * 256 + 40*2;
         dat[(x + y * particle_tex_size) * 3 + 2] = pz * initial_spread * 40 - 5;
      }

	glBindTexture(GL_TEXTURE_2D, particle_pos_tex);
   glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, particle_tex_size, particle_tex_size, GL_RGB, GL_FLOAT, dat);
	glBindTexture(GL_TEXTURE_2D, 0);

   delete[] dat;
}

void Particles::resetVelocities()
{
   GLfloat* dat = new GLfloat[particle_tex_size * particle_tex_size * 3];

   const float s = 0.00f;

   for(int y = 0; y < particle_tex_size; ++y)
      for(int x = 0; x < particle_tex_size; ++x)
      {
         dat[(x + y * particle_tex_size) * 3 + 0] = (frand() - 0.5f) * s;
         dat[(x + y * particle_tex_size) * 3 + 1] = (frand() - 0.5f) * s;
         dat[(x + y * particle_tex_size) * 3 + 2] = (frand() - 0.5f) * s;
      }

	glBindTexture(GL_TEXTURE_2D, particle_vel_tex);
   glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, particle_tex_size, particle_tex_size, GL_RGB, GL_FLOAT, dat);
	glBindTexture(GL_TEXTURE_2D, 0);
   delete[] dat;
}

int Particles::init()
{
	glGenTextures(1, &particle_pos_tex);
	glBindTexture(GL_TEXTURE_2D, particle_pos_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, particle_tex_size, particle_tex_size);

   resetPositions();

	glBindTexture(GL_TEXTURE_2D, 0);
   CHECK_FOR_ERRORS("creating particle position texture");


	glGenTextures(1, &particle_vel_tex);
	glBindTexture(GL_TEXTURE_2D, particle_vel_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB16F, particle_tex_size, particle_tex_size);

   resetVelocities();

	glBindTexture(GL_TEXTURE_2D, 0);
   CHECK_FOR_ERRORS("creating particle velocity texture");


	glGenFramebuffers(1, &particle_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, particle_fbo);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, particle_vel_tex, 0);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, particle_pos_tex, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
   CHECK_FOR_ERRORS("creating particle FBO");

	pm_shader_program = createProgram(SHADERS_PATH "pm_v.glsl", NULL, SHADERS_PATH "pm_f.glsl");
	glUniform1i(glGetUniformLocation(pm_shader_program, "depth_tex"), 0);
	glUniform1i(glGetUniformLocation(pm_shader_program, "particle_pos_tex"), 1);
	glUniform1i(glGetUniformLocation(pm_shader_program, "particle_vel_tex"), 2);
   pm_shader_view_mv_loc = glGetUniformLocation(pm_shader_program, "view_mv");
   pm_shader_view_p_loc = glGetUniformLocation(pm_shader_program, "view_p");
   pm_shader_inv_view_mv_loc = glGetUniformLocation(pm_shader_program, "inv_view_mv");
   pm_shader_inv_view_p_loc = glGetUniformLocation(pm_shader_program, "inv_view_p");
   pm_shader_gravity_scale_loc = glGetUniformLocation(pm_shader_program, "gravity_scale");
	glUseProgram(0);
   CHECK_FOR_ERRORS("loading shaders");

	particle_shader_program = createProgram(SHADERS_PATH "particle_v.glsl", SHADERS_PATH "particle_g.glsl", SHADERS_PATH "particle_f.glsl");
	glUniform1i(glGetUniformLocation(particle_shader_program, "positions_tex"), 0);
   particle_shader_modelview_loc = glGetUniformLocation(particle_shader_program, "modelview");
   particle_shader_projection_loc = glGetUniformLocation(particle_shader_program, "projection");
   particle_shader_scale_loc = glGetUniformLocation(particle_shader_program, "particle_scale");
	glUseProgram(0);

   CHECK_FOR_ERRORS("loading shaders");

   return 0;
}

void Particles::setInitialSpread(float spread)
{
   initial_spread = spread;
}

void Particles::setGravityScale(float scale)
{
   gravity_scale = scale;
}

void Particles::setParticleScale(float scale)
{
   particle_scale = scale;
}

void Particles::setDepthTex(GLuint tex)
{
   depth_tex = tex;
}

void Particles::setParticleTexSize(GLsizei sz)
{
   particle_tex_size = sz;
}

void Particles::setFrameWidth(int w)
{
   frame_width = w;
}

void Particles::setFrameHeight(int h)
{
   frame_height = h;
}

void Particles::setCamera(Camera* cam)
{
   camera = cam;
}

void Particles::update()
{
	glUseProgram(pm_shader_program);

	glUniformMatrix4fv(pm_shader_view_mv_loc, 1, GL_FALSE, camera->getModelView().e);
	glUniformMatrix4fv(pm_shader_view_p_loc, 1, GL_FALSE, camera->getProjection().e);
   glUniformMatrix4fv(pm_shader_inv_view_mv_loc, 1, GL_FALSE, camera->getInvModelView().e);
   glUniformMatrix4fv(pm_shader_inv_view_p_loc, 1, GL_FALSE, camera->getInvProjection().e);
   glUniform1f(pm_shader_gravity_scale_loc, gravity_scale);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, particle_fbo);

	{
      GLenum b[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
      glDrawBuffers(2, b);
	}

	glViewport(0, 0, particle_tex_size, particle_tex_size);
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, particle_vel_tex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, particle_pos_tex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depth_tex);

   drawQuadSection(section_counter++);

   CHECK_FOR_ERRORS("updating particles");
}

void Particles::render()
{
   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
   glUseProgram(particle_shader_program);

   glUniformMatrix4fv(particle_shader_modelview_loc, 1, GL_FALSE, camera->getModelView().e);
   glUniformMatrix4fv(particle_shader_projection_loc, 1, GL_FALSE, camera->getProjection().e);
   glUniform1f(particle_shader_scale_loc, particle_scale);

   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);

	glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, particle_pos_tex);
   glDrawArrays(GL_POINTS, 0, particle_tex_size * particle_tex_size);
   glBindTexture(GL_TEXTURE_2D, 0);

   glDisable(GL_BLEND);
   glDepthMask(GL_TRUE);

   CHECK_FOR_ERRORS("rendering particles");
}
