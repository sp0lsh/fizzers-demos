
#pragma once

#include "Engine.hpp"

namespace particles
{

class Camera
{
   public:
      Camera();

      const Mat4& getModelView() const;
      const Mat4& getProjection() const;
      const Mat4& getInvModelView() const;
      const Mat4& getInvProjection() const;

      void setModelView(Mat4&);
      void setProjection(Mat4&);

   private:
      Mat4 modelview;
      Mat4 projection;
      Mat4 inv_modelview;
      Mat4 inv_projection;
};


class Particles
{
   public:
      Particles();

      int  init();
      void setDepthTex(GLuint);
      void setParticleTexSize(GLsizei);
      void setCamera(Camera*);
      void setParticleScale(float);
      void setGravityScale(float);
      void setInitialSpread(float);
      void update();
      void render();
      void setFrameWidth(int);
      void setFrameHeight(int);
      void resetPositions();
      void resetVelocities();

   private:
      GLuint  particle_pos_tex;
      GLuint  particle_vel_tex;
      GLuint  depth_tex;
      GLuint  pm_shader_program;
      GLuint  particle_shader_program;
      GLsizei particle_tex_size;
      GLuint  particle_fbo;
      Camera* camera;
      GLint   pm_shader_view_mv_loc;
      GLint   pm_shader_view_p_loc;
      GLint   pm_shader_inv_view_mv_loc;
      GLint   pm_shader_inv_view_p_loc;
      GLint   particle_shader_modelview_loc;
      GLint   particle_shader_projection_loc;
      int     frame_width;
      int     frame_height;
      float   particle_scale;
      GLint   particle_shader_scale_loc;
      float   gravity_scale;
      GLint   pm_shader_gravity_scale_loc;
      float   initial_spread;
      int     section_counter;
};

}
