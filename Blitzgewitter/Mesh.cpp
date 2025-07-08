
#include "Engine.hpp"

#include <set>

Mesh::Mesh(const uint max_vcount, const uint max_tcount):
         vertices(NULL), normals(NULL), indices(NULL), vcount(0),
         tcount(0), vbo(0), ebo(0), dirty(false), max_vcount(max_vcount),
         max_tcount(max_tcount)
{
   vertices = new GLfloat[max_vcount * 3];
   normals = new GLfloat[max_vcount * 3];
   indices = new GLuint[max_tcount * 3];
   tris = NULL;
   vertex_adj_tris = NULL;
   analysed = false;
}

Mesh::~Mesh()
{
   free();
}

Vec3 Mesh::getPointInTriangle(const GLuint tri, Real u, Real v) const
{
   GLuint v0 = indices[tri * 3 + 0] * 3,
          v1 = indices[tri * 3 + 1] * 3,
          v2 = indices[tri * 3 + 2] * 3;

   GLfloat *verts[3] = { vertices + v0,
                         vertices + v1,
                         vertices + v2 };

   if(u + v > 1.0f)
   {
      u = 1.0f - u;
      v = 1.0f - v;
   }

   const float w = 1.0f - u - v;

   assert(u >= 0.0f);
   assert(u <= 1.0f);
   assert(v >= 0.0f);
   assert(v <= 1.0f);
   assert(w >= 0.0f);
   assert(w <= 1.0f);
   assert(fabsf((u + v + w) - 1.0f) < 0.0001f);

   Vec3 out(0, 0, 0);

   for(int k = 0; k < 3; ++k)
      out[k] = verts[0][k] * u + verts[1][k] * v + verts[2][k] * w;

   return out;
}

void Mesh::getTriangleAreas(GLfloat* areas) const
{
   assert(areas != NULL);

   for(GLuint t = 0; t < tcount; ++t)
   {
      GLuint v0 = indices[t * 3 + 0] * 3,
             v1 = indices[t * 3 + 1] * 3,
             v2 = indices[t * 3 + 2] * 3;

      assert(v0 < (vcount * 3));
      assert(v1 < (vcount * 3));
      assert(v2 < (vcount * 3));

      GLfloat *v[3] = { vertices + v0, vertices + v1, vertices + v2 };

      GLfloat d0[3] = { (v[1][0] - v[0][0]),
                        (v[1][1] - v[0][1]),
                        (v[1][2] - v[0][2]) };

      GLfloat d1[3] = { (v[2][0] - v[0][0]),
                        (v[2][1] - v[0][1]),
                        (v[2][2] - v[0][2]) };

      GLfloat tri_norm[3];

      tri_norm[0] = (d0[1] * d1[2] - d0[2] * d1[1]);
      tri_norm[1] = (d0[2] * d1[0] - d0[0] * d1[2]);
      tri_norm[2] = (d0[0] * d1[1] - d0[1] * d1[0]);

      GLfloat area = sqrtf(tri_norm[0] * tri_norm[0] +
                           tri_norm[1] * tri_norm[1] +
                           tri_norm[2] * tri_norm[2]) * 0.5f;

      GLfloat mid[3] = { v[0][0] + (d0[0] + d1[0]) * 0.5f,
                         v[0][1] + (d0[1] + d1[1]) * 0.5f,
                         v[0][2] + (d0[2] + d1[2]) * 0.5f };

      areas[t] = area;
   }
}

void Mesh::addHeightNoise()
{
   dirty = true;

   assert(indices != NULL);
   assert(vertices != NULL);
   assert(normals != NULL);

   for(int v = 0; v < vcount; ++v)
   {
      vertices[v * 3 + 1] += cosf(float(v)) * 0.05f;
   }
}

const Vec3& Mesh::getTriangleNorm(GLuint t) const
{
   GLuint v0 = indices[t * 3 + 0] * 3,
          v1 = indices[t * 3 + 1] * 3,
          v2 = indices[t * 3 + 2] * 3;

   assert(v0 < (vcount * 3));
   assert(v1 < (vcount * 3));
   assert(v2 < (vcount * 3));

   GLfloat tri_norm[3];

   GLfloat *v[3] = { vertices + v0,
                     vertices + v1,
                     vertices + v2 };

   GLfloat d0[3] = { v[1][0] - v[0][0], v[1][1] - v[0][1], v[1][2] - v[0][2] };
   GLfloat d1[3] = { v[2][0] - v[0][0], v[2][1] - v[0][1], v[2][2] - v[0][2] };

   tri_norm[0] = (d0[1] * d1[2] - d0[2] * d1[1]);
   tri_norm[1] = (d0[2] * d1[0] - d0[0] * d1[2]);
   tri_norm[2] = (d0[0] * d1[1] - d0[1] * d1[0]);

   const float len = sqrtf(tri_norm[0] * tri_norm[0] +
                           tri_norm[1] * tri_norm[1] +
                           tri_norm[2] * tri_norm[2]);

   for(int k = 0; k < 3; ++k)
      tri_norm[k] *= 1.0f / len;

   return Vec3(tri_norm[0],tri_norm[1],tri_norm[2]);
}

void Mesh::generateNormals()
{
   dirty = true;

   assert(indices != NULL);
   assert(vertices != NULL);
   assert(normals != NULL);

   for(int v = 0; v < vcount; ++v)
   {
      normals[v * 3 + 0] = 0.0f;
      normals[v * 3 + 1] = 0.0f;
      normals[v * 3 + 2] = 0.0f;
   }

   for(int t = 0; t < tcount; ++t)
   {
      GLuint v0 = indices[t * 3 + 0] * 3,
             v1 = indices[t * 3 + 1] * 3,
             v2 = indices[t * 3 + 2] * 3;

      assert(v0 < (vcount * 3));
      assert(v1 < (vcount * 3));
      assert(v2 < (vcount * 3));

      GLfloat tri_norm[3];

      GLfloat *v[3] = { vertices + v0,
                        vertices + v1,
                        vertices + v2 };

      GLfloat d0[3] = { v[1][0] - v[0][0], v[1][1] - v[0][1], v[1][2] - v[0][2] };
      GLfloat d1[3] = { v[2][0] - v[0][0], v[2][1] - v[0][1], v[2][2] - v[0][2] };

      tri_norm[0] = (d0[1] * d1[2] - d0[2] * d1[1]);
      tri_norm[1] = (d0[2] * d1[0] - d0[0] * d1[2]);
      tri_norm[2] = (d0[0] * d1[1] - d0[1] * d1[0]);

      const float len = sqrtf(tri_norm[0] * tri_norm[0] +
                              tri_norm[1] * tri_norm[1] +
                              tri_norm[2] * tri_norm[2]);

      for(int k = 0; k < 3; ++k)
         tri_norm[k] *= 1.0f / len;

      //log("%f, %f, %f, %f\n", tri_norm[0], tri_norm[1], tri_norm[2],tri_norm[0] * vertices[v0 + 0] + tri_norm[1] * vertices[v0 + 1] + tri_norm[2] * vertices[v0 + 2]);

      for(int k = 0; k < 3; ++k)
      {
         normals[v0 + k] += tri_norm[k];
         normals[v1 + k] += tri_norm[k];
         normals[v2 + k] += tri_norm[k];
      }
   }

   for(int v = 0; v < vcount; ++v)
   {
      GLfloat* norm = normals + v * 3;

      const float len = sqrtf(norm[0] * norm[0] +
                              norm[1] * norm[1] +
                              norm[2] * norm[2]);

      for(int k = 0; k < 3; ++k)
         norm[k] *= 1.0f / len;
   }
}

void Mesh::free()
{
   if(vbo)
      glDeleteBuffers(1, &vbo);

   if(ebo)
      glDeleteBuffers(1, &ebo);

   vbo = ebo = 0;

   delete[] vertices;
   delete[] normals;
   delete[] indices;

   delete[] tris;
   delete[] vertex_adj_tris;

   tris = NULL;
   vertex_adj_tris = NULL;
   vertices = NULL;
   normals = NULL;
   indices = NULL;
}

void Mesh::bind()
{
   if(vbo == 0)
      glGenBuffers(1, &vbo);

   if(ebo == 0)
      glGenBuffers(1, &ebo);

   glBindBuffer(GL_ARRAY_BUFFER, vbo);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

   if(dirty)
   {
      dirty = false;

      assert(indices != NULL);
      assert(vertices != NULL);
      assert(normals != NULL);

      glBufferData(GL_ELEMENT_ARRAY_BUFFER, tcount * sizeof(GLuint) * 3, indices, GL_STATIC_DRAW);
      glBufferData(GL_ARRAY_BUFFER, vcount * sizeof(GLfloat) * 3 * 2, NULL, GL_STATIC_DRAW);
      glBufferSubData(GL_ARRAY_BUFFER, 0, vcount * sizeof(GLfloat) * 3, vertices);
      glBufferSubData(GL_ARRAY_BUFFER, vcount * sizeof(GLfloat) * 3, vcount * sizeof(GLfloat) * 3, normals);
   }
}

void Mesh::makeVertex(int i, GLfloat x, GLfloat y, GLfloat z)
{
   dirty = true;

   assert(i >= 0);
   assert(i < max_vcount);
   vertices[i * 3 + 0] = x;
   vertices[i * 3 + 1] = y;
   vertices[i * 3 + 2] = z;
}

void Mesh::makeTriangle(int i, int a, int b, int c)
{
   dirty = true;

   assert(i >= 0);
   assert(i < max_tcount);
   indices[i * 3 + 0] = a;
   indices[i * 3 + 1] = b;
   indices[i * 3 + 2] = c;
}

void Mesh::addPoly(int a, int b, int c)
{
   dirty = true;

   assert(tcount < max_tcount);
   indices[tcount * 3 + 0] = a;
   indices[tcount * 3 + 1] = b;
   indices[tcount * 3 + 2] = c;
   ++tcount;
}

void Mesh::addPoly(int a, int b, int c, int d)
{
   dirty = true;

   assert(tcount < max_tcount);
   indices[tcount * 3 + 0] = a;
   indices[tcount * 3 + 1] = b;
   indices[tcount * 3 + 2] = c;
   ++tcount;

   assert(tcount < max_tcount);
   indices[tcount * 3 + 0] = c;
   indices[tcount * 3 + 1] = d;
   indices[tcount * 3 + 2] = a;
   ++tcount;
}

void Mesh::generateOctahedron()
{
   dirty = true;

   assert(indices != NULL);
   assert(vertices != NULL);
   assert(normals != NULL);

   vcount = 6;
   tcount = 8;

	makeVertex(0,-1, 0.0, 0.0);
	makeVertex(1, 0.0, 0.0,-1);
	makeVertex(2, 1, 0.0, 0.0);
	makeVertex(3, 0.0, 0.0, 1);
	makeVertex(4, 0.0, 1, 0.0);
	makeVertex(5, 0.0,-1, 0.0);

	makeTriangle(0,4,1,0);
	makeTriangle(1,4,2,1);
	makeTriangle(2,4,3,2);
	makeTriangle(3,4,0,3);
	makeTriangle(4,5,0,1);
	makeTriangle(5,5,1,2);
	makeTriangle(6,5,2,3);
	makeTriangle(7,5,3,0);
}

void Mesh::generateDodecahedron()
{
   dirty = true;

   assert(indices != NULL);
   assert(vertices != NULL);
   assert(normals != NULL);

   vcount = 20;
   tcount = 0; // 24

	GLfloat b  = 4.0/(std::sqrt(3.0)+std::sqrt(15.0));
	GLfloat a  = 2.0*b/(std::sqrt(5.0)-1.0);
	GLfloat hh = b*b-a*a/4.0;
	GLfloat d1 = 0.5*(a-b);
	GLfloat h  = std::sqrt(hh-d1*d1);
	GLfloat b2 = b*0.5;
	GLfloat a2 = a*0.5;

	makeVertex( 0,0.0,+b2,-a2-h);
	makeVertex( 1,0.0,-b2,-a2-h);
	makeVertex( 2,+a2+h,0.0,-b2);
	makeVertex( 3,+a2+h,0.0,+b2);
	makeVertex( 4,0.0,+b2,+a2+h);
	makeVertex( 5,0.0,-b2,+a2+h);
	makeVertex( 6,-a2-h,0.0,+b2);
	makeVertex( 7,-a2-h,0.0,-b2);
	makeVertex( 8,-b2,+a2+h,0.0);
	makeVertex( 9,+b2,+a2+h,0.0);
	makeVertex(10,+b2,-a2-h,0.0);
	makeVertex(11,-b2,-a2-h,0.0);
	makeVertex(12,+a2,-a2,-a2);
	makeVertex(13,+a2,-a2,+a2);
	makeVertex(14,-a2,-a2,+a2);
	makeVertex(15,-a2,-a2,-a2);
	makeVertex(16,+a2,+a2,-a2);
	makeVertex(17,+a2,+a2,+a2);
	makeVertex(18,-a2,+a2,+a2);
	makeVertex(19,-a2,+a2,-a2);

	addPoly(19,16,0);
	addPoly(12,15,1);
	addPoly(12,16,2);
	addPoly(17,13,3);
	addPoly(17,18,4);
	addPoly(14,13,5);
	addPoly(14,18,6);
	addPoly(19,15,7);
	addPoly(19,18,8);
	addPoly(17,16,9);
	addPoly(12,13,10);
	addPoly(14,15,11);
	addPoly(16,12,1,0);
	addPoly(15,19,0,1);
	addPoly(16,17,3,2);
	addPoly(13,12,2,3);
	addPoly(18,14,5,4);
	addPoly(13,17,4,5);
	addPoly(18,19,7,6);
	addPoly(15,14,6,7);
	addPoly(18,17,9,8);
	addPoly(16,19,8,9);
	addPoly(13,14,11,10);
	addPoly(15,12,10,11);
}

void Mesh::generateTetrahedron()
{
   dirty = true;

   assert(indices != NULL);
   assert(vertices != NULL);
   assert(normals != NULL);

   vcount = 4;
   tcount = 4;

   makeVertex(0, 0.0000, 153.2110, -94.2809);
   makeVertex(1, 81.6497, 153.2110, 47.1405);
   makeVertex(2, -81.6497, 153.2110, 47.1405);
   makeVertex(3, 0.0000, 286.5443, -0.0000);

   float min_x=1e4;
   float min_y=1e4;
   float min_z=1e4;
   float max_x=-1e4;
   float max_y=-1e4;
   float max_z=-1e4;
   for(int i=0;i<4;++i)
   {
      float x=vertices[i*3+0];
      float y=vertices[i*3+1];
      float z=vertices[i*3+2];
      if(x<min_x)
         min_x=x;
      if(y<min_y)
         min_y=y;
      if(z<min_z)
         min_z=z;
      if(x>max_x)
         max_x=x;
      if(y>max_y)
         max_y=y;
      if(z>max_z)
         max_z=z;
   }

   for(int i=0;i<4;++i)
   {
      vertices[i*3+0]-=(max_x+min_x)*0.5;
      vertices[i*3+1]-=(max_y+min_y)*0.5;
      vertices[i*3+2]-=(max_z+min_z)*0.5;
   }

	makeTriangle(0,0,1,2);
	makeTriangle(1,1,3,2);
	makeTriangle(2,0,3,1);
	makeTriangle(3,2,3,0);

   transform(Mat4::scale(Vec3(0.01)));
}


void Mesh::generateTetrahedron2()
{
   dirty = true;

   assert(indices != NULL);
   assert(vertices != NULL);
   assert(normals != NULL);

   vcount = 4;
   tcount = 4;

	Vec3 d=Vec3(1.0,std::sqrt(2.0/3.0)*0.5,std::sqrt(3.0)/2.0);

	makeVertex( 0,-d.x,-d.y,-d.z);
	makeVertex( 1, d.x,-d.y,-d.z);
	makeVertex( 2, 0.0,-d.y, d.z);
	makeVertex( 3, 0.0,+d.y,-1/12.0*std::sqrt(3.0));

	makeTriangle(0,0,1,2);
	makeTriangle(1,0,3,1);
	makeTriangle(2,1,3,2);
	makeTriangle(3,2,3,0);
}

void Mesh::generateIcosahedron()
{
   dirty = true;

   assert(indices != NULL);
   assert(vertices != NULL);
   assert(normals != NULL);

   vcount = 12;
   tcount = 20;

	GLfloat a2 = 0.5;
	GLfloat h  = (1.0+std::sqrt(5.0))/4.0;

	makeVertex( 0, 0.0,+a2,-h);
	makeVertex( 1,0.0,-a2,-h);
	makeVertex( 2,+h,0.0,-a2);
	makeVertex( 3,+h,0.0,+a2);
	makeVertex( 4,0.0,+a2,+h);
	makeVertex( 5,0.0,-a2,+h);
	makeVertex( 6,-h,0.0,+a2);
	makeVertex( 7,-h,0.0,-a2);
	makeVertex( 8,+a2,+h,0.0);
	makeVertex( 9,-a2,+h,0.0);
	makeVertex(10,-a2,-h,0.0);
	makeVertex(11,+a2,-h,0.0);

	makeTriangle( 0, 9,8,0);
	makeTriangle( 1,8,2,0);
	makeTriangle( 2,8,3,2);
	makeTriangle( 3,8,4,3);
	makeTriangle( 4,8,9,4);
	makeTriangle( 5,4,9,6);
	makeTriangle( 6,6,9,7);
	makeTriangle( 7,9,0,7);

	makeTriangle( 8,11,10,1);
	makeTriangle( 9,2 ,11,1);
	makeTriangle(10,2 ,3 ,11);
	makeTriangle(11,3 ,5 ,11);
	makeTriangle(12,5 ,10,11);
	makeTriangle(13,5 ,6 ,10);
	makeTriangle(14,6 ,7 ,10);
	makeTriangle(15,7 ,1 ,10);

	makeTriangle(16,1,7,0);
	makeTriangle(17,2,1,0);
	makeTriangle(18,4,5,3);
	makeTriangle(19,4,6,5);
}

void Mesh::generateCube()
{
   dirty = true;

   assert(indices != NULL);
   assert(vertices != NULL);
   assert(normals != NULL);

   vcount = 0;
   tcount = 0;

   for(GLuint z = 0; z < 2; ++z)
      for(GLuint y = 0; y < 2; ++y)
         for(GLuint x = 0; x < 2; ++x)
         {
            assert(vcount < max_vcount);
            vertices[vcount * 3 + 1] = y ? +1.0f : -1.0f;
            vertices[vcount * 3 + 0] = x ? +1.0f : -1.0f;
            vertices[vcount * 3 + 2] = z ? +1.0f : -1.0f;

            ++vcount;
         }

   GLuint i2 = 0;

   for(GLuint axis = 0; axis < 3; ++axis)
      for(GLuint side = 0; side < 2; ++side)
      {
         assert(vcount < max_vcount);

         vertices[vcount * 3 + 0] = 0.0f;
         vertices[vcount * 3 + 1] = 0.0f;
         vertices[vcount * 3 + 2] = 0.0f;

         vertices[vcount * 3 + axis] = side ? +1.0f : -1.0f;

         GLuint s_step = 1 << ((axis + 1) % 3),
               t_step = 1 << ((axis + 2) % 3),
               u_step = side << axis;

         int j = i2;

         indices[i2++] = 1 * s_step + 0 * t_step + u_step;
         indices[i2++] = 0 * s_step + 0 * t_step + u_step;
         indices[i2++] = vcount;

         indices[i2++] = 0 * s_step + 0 * t_step + u_step;
         indices[i2++] = 0 * s_step + 1 * t_step + u_step;
         indices[i2++] = vcount;

         indices[i2++] = 0 * s_step + 1 * t_step + u_step;
         indices[i2++] = 1 * s_step + 1 * t_step + u_step;
         indices[i2++] = vcount;

         indices[i2++] = 1 * s_step + 1 * t_step + u_step;
         indices[i2++] = 1 * s_step + 0 * t_step + u_step;
         indices[i2++] = vcount;

         if(side)
         {
            int k = i2;
            for(; j < i2 - 6; ++j)
               std::swap(indices[j], indices[--k]);
         }

         ++vcount;
      }

   tcount = i2 / 3;
}



void Mesh::generateSimpleCube()
{
   dirty = true;

   assert(indices != NULL);
   assert(vertices != NULL);
   assert(normals != NULL);

   generateSimpleCubeMesh(vertices, indices, vcount, tcount, max_vcount, max_tcount);
}

void Mesh::generateSimpleCubeMesh(GLfloat* vertices, GLuint* indices, GLuint& vcount, GLuint& tcount, const GLuint max_vcount, const GLuint max_tcount)
{
   vcount = 0;
   tcount = 0;

   for(GLuint z = 0; z < 2; ++z)
      for(GLuint y = 0; y < 2; ++y)
         for(GLuint x = 0; x < 2; ++x)
         {
            assert(vcount < max_vcount);
            vertices[vcount * 3 + 1] = y ? +1.0f : -1.0f;
            vertices[vcount * 3 + 0] = x ? +1.0f : -1.0f;
            vertices[vcount * 3 + 2] = z ? +1.0f : -1.0f;
            ++vcount;
         }

   GLuint i2 = 0;

   for(GLuint axis = 0; axis < 3; ++axis)
      for(GLuint side = 0; side < 2; ++side)
      {
         GLuint s_step = 1 << ((axis + 1) % 3),
               t_step = 1 << ((axis + 2) % 3),
               u_step = side << axis;

         int j = i2;

         assert(i2 / 3 < max_tcount);
         indices[i2++] = 1 * s_step + 0 * t_step + u_step;
         indices[i2++] = 0 * s_step + 0 * t_step + u_step;
         indices[i2++] = 0 * s_step + 1 * t_step + u_step;

         assert(i2 / 3 < max_tcount);
         indices[i2++] = 0 * s_step + 1 * t_step + u_step;
         indices[i2++] = 1 * s_step + 1 * t_step + u_step;
         indices[i2++] = 1 * s_step + 0 * t_step + u_step;

         if(side)
         {
            int k = i2;
            for(; j < i2 - 3; ++j)
               std::swap(indices[j], indices[--k]);
         }
      }

   tcount = i2 / 3;
}

void Mesh::generateCylinder(GLuint sides, bool cap0, bool cap1, GLuint segments)
{
   dirty = true;

   assert(indices != NULL);
   assert(vertices != NULL);
   assert(normals != NULL);

   vcount = 0;
   tcount = 0;

   for(GLuint s = 0; s < sides; ++s)
   {
      const GLfloat ang = GLfloat(s) / GLfloat(sides) * M_PI * 2;

      const GLuint v0 = ((s + 1) % sides) * (segments+1), v1 = s * (segments+1);

      for(GLuint t=0;t<=segments;++t)
      {
         const GLfloat tf=GLfloat(t)/GLfloat(segments);

         assert(vcount < max_vcount);
         vertices[vcount * 3 + 0] = std::cos(ang);
         vertices[vcount * 3 + 1] = std::sin(ang);
         vertices[vcount * 3 + 2] = -1+2*tf;
         ++vcount;

         if(t < segments)
         {
            assert(tcount < max_tcount);
            indices[tcount * 3 + 2] = t + v0;
            indices[tcount * 3 + 1] = t + v1;
            indices[tcount * 3 + 0] = t + 1 + v1;
            ++tcount;

            assert(tcount < max_tcount);
            indices[tcount * 3 + 2] = t + 1 + v1;
            indices[tcount * 3 + 1] = t + 1 + v0;
            indices[tcount * 3 + 0] = t + v0;
            ++tcount;
         }
      }

      if(s > 1)
      {
         const GLuint v0 = s * (segments+1), v1 = (s - 1) * (segments+1);

         if(cap0)
         {
            assert(tcount < max_tcount);
            indices[tcount * 3 + 0] = v0;
            indices[tcount * 3 + 1] = v1;
            indices[tcount * 3 + 2] = 0;
            ++tcount;
         }

         if(cap1)
         {
            assert(tcount < max_tcount);
            indices[tcount * 3 + 0] = segments + 0;
            indices[tcount * 3 + 1] = segments + v1;
            indices[tcount * 3 + 2] = segments + v0;
            ++tcount;
         }
      }
   }

}

void Mesh::generateGrid(const uint w, const uint h)
{
   dirty = true;

   assert(indices != NULL);
   assert(vertices != NULL);
   assert(normals != NULL);

   vcount = 0;
   tcount = 0;

   for(GLuint z = 0; z < h; ++z)
      for(GLuint x = 0; x < w; ++x)
      {
         assert(vcount < max_vcount);
         vertices[vcount * 3 + 0] = (float(x) - float(w) * 0.5f);
         vertices[vcount * 3 + 1] = 0.0f;
         vertices[vcount * 3 + 2] = (float(z) - float(h) * 0.5f);
         ++vcount;


         if(x < (w - 1) && z < (h - 1))
         {
            if((x ^ z) & 1)
            {
               assert(tcount < max_tcount);
               indices[tcount * 3 + 0] = x + z * w;
               indices[tcount * 3 + 1] = x + z * w + w;
               indices[tcount * 3 + 2] = x + z * w + 1;
               ++tcount;

               assert(tcount < max_tcount);
               indices[tcount * 3 + 0] = x + z * w + 1;
               indices[tcount * 3 + 1] = x + z * w + w;
               indices[tcount * 3 + 2] = x + z * w + 1 + w;
               ++tcount;
            }
            else
            {
               assert(tcount < max_tcount);
               indices[tcount * 3 + 0] = x + z * w;
               indices[tcount * 3 + 1] = x + z * w + w;
               indices[tcount * 3 + 2] = x + z * w + 1 + w;
               ++tcount;

               assert(tcount < max_tcount);
               indices[tcount * 3 + 0] = x + z * w;
               indices[tcount * 3 + 1] = x + z * w + 1 + w;
               indices[tcount * 3 + 2] = x + z * w + 1;
               ++tcount;
            }
         }
      }
}

void Mesh::separate()
{
   dirty = true;

   assert(indices != NULL);
   assert(vertices != NULL);
   assert(normals != NULL);

   GLfloat* old_vertices = new GLfloat[vcount * 3];

   memcpy(old_vertices, vertices, sizeof(GLfloat) * vcount * 3);

   vcount = 0;

   for(GLuint t = 0; t < tcount; ++t)
   {
      for(uint i = 0; i < 3; ++i)
      {
         vertices[(vcount + 0) * 3 + i] = old_vertices[indices[t * 3 + 0] * 3 + i];
         vertices[(vcount + 1) * 3 + i] = old_vertices[indices[t * 3 + 1] * 3 + i];
         vertices[(vcount + 2) * 3 + i] = old_vertices[indices[t * 3 + 2] * 3 + i];
      }

      indices[t * 3 + 0] = vcount + 0;
      indices[t * 3 + 1] = vcount + 1;
      indices[t * 3 + 2] = vcount + 2;

      vcount += 3;
   }

   delete[] old_vertices;
}

void Mesh::transform(const Mat4& transform)
{
   dirty = true;

   assert(indices != NULL);
   assert(vertices != NULL);
   assert(normals != NULL);

   for(GLuint v = 0; v < vcount; ++v)
   {
      Vec3 vertex = transform * Vec3(vertices[v * 3 + 0],
                                     vertices[v * 3 + 1],
                                     vertices[v * 3 + 2]);

      vertices[v * 3 + 0] = vertex.x;
      vertices[v * 3 + 1] = vertex.y;
      vertices[v * 3 + 2] = vertex.z;
   }
}

void Mesh::addInstance(const Mesh& mesh, const Mat4& transform)
{
   dirty = true;

   assert(indices != NULL);
   assert(vertices != NULL);
   assert(normals != NULL);

   assert(mesh.indices != NULL);
   assert(mesh.vertices != NULL);
   assert(mesh.normals != NULL);

   const GLuint vofs = vcount;

   for(GLuint v = 0; v < mesh.vcount; ++v)
   {
      assert(vcount < max_vcount);

      Vec3 vertex = transform * Vec3(mesh.vertices[v * 3 + 0],
                                     mesh.vertices[v * 3 + 1],
                                     mesh.vertices[v * 3 + 2]);

      assert(vcount < max_vcount);
      vertices[vcount * 3 + 0] = vertex.x;
      vertices[vcount * 3 + 1] = vertex.y;
      vertices[vcount * 3 + 2] = vertex.z;
      ++vcount;
   }

   for(GLuint t = 0; t < mesh.tcount; ++t)
   {
      assert(tcount < max_tcount);

      indices[tcount * 3 + 0] = mesh.indices[t * 3 + 0] + vofs;
      indices[tcount * 3 + 1] = mesh.indices[t * 3 + 1] + vofs;
      indices[tcount * 3 + 2] = mesh.indices[t * 3 + 2] + vofs;
      ++tcount;
   }
}

void Mesh::reverseWindings()
{
   dirty = true;

   assert(indices != NULL);
   assert(vertices != NULL);
   assert(normals != NULL);

   for(GLuint t = 0; t < tcount; ++t)
      std::swap(indices[t * 3 + 0], indices[t * 3 + 2]);
}

void Mesh::analyseTopology(GLfloat* out_vertices,GLuint* new_vcount)
{
   if(tris)
   {
      delete[] tris;
      tris=NULL;
   }

   if(vertex_adj_tris)
   {
      delete[] vertex_adj_tris;
      vertex_adj_tris=NULL;
   }

   tris = new Triangle[tcount];
   vertex_adj_tris = new Triangle*[vcount];

   for(int i = 0; i < tcount; ++i)
   {
      Triangle& tri = tris[i];

      for(int k = 0; k < 3; ++k)
      {
         tri.vertices[k] = indices[i * 3 + k];
         vertex_adj_tris[tri.vertices[k]] = tris + i;
         tri.adj_tris[k] = NULL;
      }
   }

   edges.clear();

   for(int i = 0; i < tcount; ++i)
   {
      Triangle& tri = tris[i];

      for(int k = 0; k < 3; ++k)
      {
         const Edge e(tri.vertices[k], tri.vertices[(k + 1) % 3], tris + i, k);

         std::set<Edge>::iterator it = edges.find(e);

         if(it == edges.end())
            edges.insert(e);
         else
         {
            assert(tri.adj_tris[k] == NULL);
            assert(it->tri->adj_tris[it->edge_num] == NULL);

            tri.adj_tris[k] = it->tri;
            it->tri->adj_tris[it->edge_num] = tris + i;

            if(new_vcount)
            {
               tri.odd_verts[k] = vcount + *new_vcount;
               it->tri->odd_verts[it->edge_num] = vcount + *new_vcount;
               ++*new_vcount;
            }

            if(out_vertices)
            {
               const int opp0 = tri.vertices[(k + 2) % 3] * 3,
                         opp1 = it->tri->vertices[(it->tri->indexOfVertex(tri.vertices[k]) + 1) % 3] * 3;

               // loop subdivision rules for odd vertices
               for(int j = 0; j < 3; ++j)
                  out_vertices[tri.odd_verts[k] * 3 + j] = 3.0f / 8.0f * (vertices[tri.vertices[k] * 3 + j] +
                                                         vertices[tri.vertices[(k + 1) % 3] * 3 + j]) +
                                                     1.0f / 8.0f * (vertices[opp0 + j] + vertices[opp1 + j]);
            }

            // no longer needed as each edge should only be reference by at most two triangles
            edges.erase(e);
         }
      }
   }

   analysed = true;
}

const Vec3& Mesh::getVertex(GLuint index) const
{
   assert(index < vcount);
   return Vec3(vertices[index * 3 + 0],vertices[index * 3 + 1],vertices[index * 3 + 2]);
}

const Vec3& Mesh::getNormal(GLuint index) const
{
   assert(index < vcount);
   return Vec3(normals[index * 3 + 0],normals[index * 3 + 1],normals[index * 3 + 2]);
}


void Mesh::subdivide()
{
   dirty = true;

   assert(indices != NULL);
   assert(vertices != NULL);
   assert(normals != NULL);

   GLfloat* out_vertices = new GLfloat[vcount * 4 * 3];

   GLuint new_vcount = 0;

   analyseTopology(out_vertices,&new_vcount);


   // allocate unique vertices for border edges
   for(std::set<Edge>::iterator it = edges.begin(); it != edges.end(); ++it)
   {
      const Edge& e = *it;
      const GLuint v = vcount + new_vcount++;
      Triangle *const tri = e.tri;

      // use midpoint of edge
      for(int k = 0; k < 3; ++k)
         out_vertices[v * 3 + k] = 0.5f * (vertices[tri->vertices[e.edge_num] * 3 + k] +
                                           vertices[tri->vertices[(e.edge_num + 1) % 3] * 3 + k]);

      tri->odd_verts[e.edge_num] = v;
   }

   // offset the even vertices
   for(GLuint i = 0; i < vcount; ++i)
   {
      Triangle *const first_tri = vertex_adj_tris[i];
      int valence = 0;

      GLfloat* new_even = out_vertices + i * 3;
      GLfloat* old_even = vertices + i * 3;

      new_even[0] = new_even[1] = new_even[2] = 0.0f;

      Triangle* tri = first_tri;

      do
      {
         ++valence;

         const int index_of_vertex = (tri->indexOfVertex(i) + 2) % 3;

         for(int k = 0; k < 3; ++k)
            new_even[k] += vertices[tri->vertices[index_of_vertex] * 3 + k];

         // proceed in a counter-clockwise direction around the current vertex i
         // (assuming that the triangles have a counter-clockwise winding)
         tri = tri->adj_tris[index_of_vertex];

      } while(tri != first_tri && tri);

      if(!tri)
      {
         // we have found a border edge. go back in the opposite direction to complete
         // the one-ring

         tri = first_tri;

         do
         {
            ++valence;

            const int index_of_vertex = tri->indexOfVertex(i);

            for(int k = 0; k < 3; ++k)
               new_even[k] += vertices[tri->vertices[(index_of_vertex + 1) % 3] * 3 + k];

            tri = tri->adj_tris[index_of_vertex];

            assert(tri != first_tri);

         } while(tri);
      }

      float beta;

      // extended loop subdivision rules
      if(valence == 6)
         beta = 1.0f / 16.0f;
      else if(valence == 3)
         beta = 3.0f / 16.0f;
      else
         beta = 3.0f / (8.0f * float(valence));

      for(int k = 0; k < 3; ++k)
         new_even[k] = old_even[k] * (1.0f - beta * float(valence)) + new_even[k] * beta;
   }


   int new_tcount = 0;

   for(int i = 0; i < tcount; ++i)
   {
      Triangle& tri = tris[i];

      // one triangle at center
      for(int k = 0; k < 3; ++k)
         indices[new_tcount * 3 + k] = tri.odd_verts[k];

      ++new_tcount;

      // one triangle for each corner
      for(int k = 0; k < 3; ++k)
      {
         indices[new_tcount * 3 + 0] = tri.vertices[k];
         indices[new_tcount * 3 + 1] = tri.odd_verts[k];
         indices[new_tcount * 3 + 2] = tri.odd_verts[(k + 2) % 3];

         ++new_tcount;
      }
   }

   vcount = new_vcount + vcount;
   tcount = new_tcount;

   delete[] vertices;

   vertices = out_vertices;
}



void Mesh::extrude(const GLuint tri, const GLfloat* dir, const float s)
{
   dirty = true;

   assert(indices != NULL);
   assert(vertices != NULL);
   assert(normals != NULL);


   const GLuint old_tri_inds[3] = { indices[tri * 3 + 0],
                                   indices[tri * 3 + 1],
                                   indices[tri * 3 + 2] };

   GLfloat *v[3] = { vertices + old_tri_inds[0] * 3,
                     vertices + old_tri_inds[1] * 3,
                     vertices + old_tri_inds[2] * 3 };

   GLfloat tri_norm[3] = { 0.0f, 0.0f, 0.0f };

   if(dir == NULL)
   {
      dir = tri_norm;

      GLfloat d0[3] = { v[1][0] - v[0][0], v[1][1] - v[0][1], v[1][2] - v[0][2] };
      GLfloat d1[3] = { v[2][0] - v[0][0], v[2][1] - v[0][1], v[2][2] - v[0][2] };

      tri_norm[0] = (d0[1] * d1[2] - d0[2] * d1[1]);
      tri_norm[1] = (d0[2] * d1[0] - d0[0] * d1[2]);
      tri_norm[2] = (d0[0] * d1[1] - d0[1] * d1[0]);

      const float len = sqrtf(tri_norm[0] * tri_norm[0] +
                              tri_norm[1] * tri_norm[1] +
                              tri_norm[2] * tri_norm[2]);

      for(int k = 0; k < 3; ++k)
         tri_norm[k] *= 1.0f / len;
   }

   assert(vcount < max_vcount);
   indices[tri * 3 + 0] = vcount++;
   assert(vcount < max_vcount);
   indices[tri * 3 + 1] = vcount++;
   assert(vcount < max_vcount);
   indices[tri * 3 + 2] = vcount++;

   GLfloat *nv[3] = { vertices + indices[tri * 3 + 0] * 3,
                      vertices + indices[tri * 3 + 1] * 3,
                      vertices + indices[tri * 3 + 2] * 3 };

   for(int k = 0; k < 3; ++k)
   {
      nv[0][k] = v[0][k] + dir[k] * s;
      nv[1][k] = v[1][k] + dir[k] * s;
      nv[2][k] = v[2][k] + dir[k] * s;
   }

   for(int k = 0; k < 3; ++k)
   {
      const int k2 = (k + 1) % 3;
      assert(vcount < max_vcount);
      const GLuint cv = vcount++;
      GLfloat* center = vertices + cv * 3;

      for(int j = 0; j < 3; ++j)
         center[j] = (nv[k][j] + v[k][j] + nv[k2][j] + v[k2][j]) * 0.25f;

      assert(tcount < max_tcount);
      indices[tcount * 3 + 0] = old_tri_inds[k];
      indices[tcount * 3 + 1] = old_tri_inds[k2];
      indices[tcount * 3 + 2] = cv;
      ++tcount;

      assert(tcount < max_tcount);
      indices[tcount * 3 + 0] = old_tri_inds[k2];
      indices[tcount * 3 + 1] = indices[tri * 3 + k2];
      indices[tcount * 3 + 2] = cv;
      ++tcount;

      assert(tcount < max_tcount);
      indices[tcount * 3 + 0] = indices[tri * 3 + k2];
      indices[tcount * 3 + 1] = indices[tri * 3 + k];
      indices[tcount * 3 + 2] = cv;
      ++tcount;

      assert(tcount < max_tcount);
      indices[tcount * 3 + 0] = indices[tri * 3 + k];
      indices[tcount * 3 + 1] = old_tri_inds[k];
      indices[tcount * 3 + 2] = cv;
      ++tcount;
   }
}

