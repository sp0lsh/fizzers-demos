
#include "Engine.hpp"

#include <IL/il.h>

static bool devil_not_initialised = true;

GLuint Scene::loadTexture(const char *file_name)
{
   if(devil_not_initialised)
   {
      ilInit();

      ilEnable(IL_ORIGIN_SET);
      ilOriginFunc(IL_ORIGIN_LOWER_LEFT);

      ilEnable(IL_TYPE_SET);
      ilTypeFunc(IL_UNSIGNED_BYTE);

      devil_not_initialised = false;
   }

   ILuint ilimg = 0;
   GLuint tex = 0;

   ilGenImages(1, &ilimg);
   ilBindImage(ilimg);

   if(ilLoadImage(file_name) == IL_FALSE)
   {
      ilDeleteImages(1, &ilimg);
      return 0;
   }

   glGenTextures(1, &tex);
   glBindTexture(GL_TEXTURE_2D, tex);

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT));

   glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT),
                              ilGetInteger(IL_IMAGE_FORMAT), GL_UNSIGNED_BYTE, ilGetData());

   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_REPEAT);

   ilDeleteImages(1, &ilimg);

   return tex;
}


void Scene::createWindowTexture(GLuint tex, GLenum format, uint divisor)
{
   assert(divisor != 0);
   assert(window_width != 0);
   assert(window_height != 0);

   glBindTexture(GL_TEXTURE_2D, tex);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
   glTexStorage2D(GL_TEXTURE_2D, 1, format, window_width / divisor, window_height / divisor);

   CHECK_FOR_ERRORS;
}

void Scene::downsample8x(GLuint srctex, GLuint fbo0, GLuint tex0, GLuint target_fbo,
                         GLsizei vpw, GLsizei vph)
{
   const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f,
                                +1.0f, +1.0f, -1.0f, +1.0f };

   const GLfloat vertices_downsample1[] = { -1.0f, -1.0f, +0.0f, -1.0f,
                                            +0.0f, +0.0f, -1.0f, +0.0f };

   const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f,
                              1.0f, 1.0f, 0.0f, 1.0f };

   const GLfloat coords_downsample1[8] = { 0.0f, 0.0f, 0.5f, 0.0f,
                                           0.5f, 0.5f, 0.0f, 0.5f };

   const GLubyte indices[] = { 3, 0, 2, 1 };



   // downsample pass 1

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo0);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   glViewport(0, 0, window_width, window_height);

   CHECK_FOR_ERRORS;

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);

   if(!downsample_shader.isLoaded())
   {
      downsample_shader.load(SHADERS_PATH "downsample_v.glsl", NULL, SHADERS_PATH "downsample_f.glsl");
      downsample_shader.uniform1i("tex0", 0);
   }

   downsample_shader.bind();

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, srctex);

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices_downsample1);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);



   // downsample pass 2

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target_fbo);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   glViewport(0, 0, window_width / 4, window_height / 4);

   CHECK_FOR_ERRORS;

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);


   downsample_shader.bind();

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, tex0);

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords_downsample1);
   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

}
