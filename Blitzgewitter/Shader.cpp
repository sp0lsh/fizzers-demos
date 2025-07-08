
#include "Engine.hpp"

void Shader::uniform1i(const char* name, GLint x)
{
   GLint loc=glGetUniformLocation(program, name);
   if(loc<0)
      return;
   glProgramUniform1i(program, loc, x);
   CHECK_FOR_ERRORS;
}

void Shader::uniform2i(const char* name, GLint x, GLint y)
{
   GLint loc=glGetUniformLocation(program, name);
   if(loc<0)
      return;
   glProgramUniform2i(program, loc, x, y);
   CHECK_FOR_ERRORS;
}


void Shader::uniform4f(const char* name, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GLint loc=glGetUniformLocation(program, name);
   if(loc<0)
      return;
   glProgramUniform4f(program, loc, x, y, z, w);
   CHECK_FOR_ERRORS;
}

void Shader::uniform3f(const char* name, GLfloat x, GLfloat y, GLfloat z)
{
   GLint loc=glGetUniformLocation(program, name);
   if(loc<0)
      return;
   glProgramUniform3f(program, loc, x, y, z);
   CHECK_FOR_ERRORS;
}

void Shader::uniform2f(const char* name, GLfloat x, GLfloat y)
{
   GLint loc=glGetUniformLocation(program, name);
   if(loc<0)
      return;
   glProgramUniform2f(program, loc, x, y);
   CHECK_FOR_ERRORS;
}

void Shader::uniform1f(const char* name, GLfloat x)
{
   GLint loc=glGetUniformLocation(program, name);
   if(loc<0)
      return;
   glProgramUniform1f(program, loc, x);
   CHECK_FOR_ERRORS;
}

void Shader::uniformMatrix4fv(const char* name, GLsizei count, GLboolean transpose, const GLfloat* value)
{
   GLint loc=glGetUniformLocation(program, name);
   if(loc<0)
      return;
   glProgramUniformMatrix4fv(program, loc, count, transpose, value);
   CHECK_FOR_ERRORS;
}


void Shader::bind()
{
   glUseProgram(program);
}

static void shaderSourceFromFile(GLuint shader, const char* filename)
{
   FILE* in = fopen(filename, "r");

   if(!in)
   {
      log("Could not open shader file '%s'.", filename);
      return;
   }

   static const int max_lines = 1024;
   static char str_array[max_lines][1024];
   static int len_array[max_lines];

   int num_lines = 0;

   while(num_lines < max_lines && fgets(str_array[num_lines], sizeof(str_array[0]), in))
   {
      len_array[num_lines] = strlen(str_array[num_lines]);
      ++num_lines;
   }

   const GLchar *strp_array[num_lines];

   for(int i = 0; i < num_lines; ++i)
      strp_array[i] = str_array[i];

   glShaderSource(shader, num_lines, strp_array, len_array);

   fclose(in);

   CHECK_FOR_ERRORS;
}

GLuint createProgram(const char* vsh_filename, const char* gsh_filename,
                            const char* fsh_filename)
{
   static char buf[4096];

   GLuint vertex_shader   = glCreateShader(GL_VERTEX_SHADER),
          geometry_shader = glCreateShader(GL_GEOMETRY_SHADER),
          fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

   CHECK_FOR_ERRORS;

   GLuint prog = glCreateProgram();

   if(vsh_filename)
   {
      shaderSourceFromFile(vertex_shader, vsh_filename);

      glCompileShader(vertex_shader);

      CHECK_FOR_ERRORS;

      GLint p = 0;
      glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &p);
      if(p == GL_FALSE)
      {
         log("\nVertex ShaderInfoLog: (%s)\n\n", vsh_filename);
         glGetShaderInfoLog(vertex_shader, sizeof(buf), NULL, buf);
         log(buf);
      }

      glAttachShader(prog, vertex_shader);

      CHECK_FOR_ERRORS;
   }

   if(gsh_filename)
   {
      shaderSourceFromFile(geometry_shader, gsh_filename);

      glCompileShader(geometry_shader);

      CHECK_FOR_ERRORS;

      GLint p = 0;
      glGetShaderiv(geometry_shader, GL_COMPILE_STATUS, &p);
      if(p == GL_FALSE)
      {
         log("\nGeometry ShaderInfoLog: (%s)\n\n", gsh_filename);
         glGetShaderInfoLog(geometry_shader, sizeof(buf), NULL, buf);
         log(buf);
      }

      glAttachShader(prog, geometry_shader);

      CHECK_FOR_ERRORS;
   }

   if(fsh_filename)
   {
      shaderSourceFromFile(fragment_shader, fsh_filename);

      glCompileShader(fragment_shader);

      CHECK_FOR_ERRORS;

      GLint p = 0;
      glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &p);
      if(p == GL_FALSE)
      {
         log("\nFragment ShaderInfoLog: (%s)\n\n", fsh_filename);
         glGetShaderInfoLog(fragment_shader, sizeof(buf), NULL, buf);
         log(buf);
      }

      glAttachShader(prog, fragment_shader);

      CHECK_FOR_ERRORS;
   }

   // we should be okay to do this since the specification doesn't mention
   // any errors that may be reported if the named symbol isn't found.

   glBindAttribLocation(prog, 0, "vertex");
   glBindAttribLocation(prog, 1, "coord");
   glBindAttribLocation(prog, 2, "normal");

   glBindFragDataLocation(prog, 0, "output_colour");
   glBindFragDataLocation(prog, 0, "output0");
   glBindFragDataLocation(prog, 1, "output1");
   glBindFragDataLocation(prog, 0, "output_colour0");
   glBindFragDataLocation(prog, 1, "output_colour1");

   CHECK_FOR_ERRORS;

   glLinkProgram(prog);

   {
      GLint p = 0;
      glGetProgramiv(prog, GL_LINK_STATUS, &p);
      if(p == GL_FALSE)
      {
         log("\nProgramInfoLog: \n\n");
         glGetProgramInfoLog(prog, sizeof(buf), NULL, buf);
         log(buf);
      }
   }

   if(vsh_filename)
      glDetachShader(prog, vertex_shader);

   if(gsh_filename)
      glDetachShader(prog, geometry_shader);

   if(fsh_filename)
      glDetachShader(prog, fragment_shader);

   if(vsh_filename)
      glDeleteShader(vertex_shader);

   if(gsh_filename)
      glDeleteShader(geometry_shader);

   if(fsh_filename)
      glDeleteShader(fragment_shader);

   glUseProgram(prog);

   CHECK_FOR_ERRORS;

   return prog;
}

void Shader::free()
{
   glDeleteProgram(program);
   program = 0;
}

void Shader::load(const char* vsh_filename, const char* gsh_filename,
                            const char* fsh_filename)
{
   program = createProgram(vsh_filename, gsh_filename, fsh_filename);
   loaded = true;
}

