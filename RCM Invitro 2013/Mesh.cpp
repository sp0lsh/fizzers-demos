
#include "Engine.hpp"

#include <set>

struct Triangle
{
   GLuint     vertices[3];
   Triangle* adj_tris[3];
   GLuint     odd_verts[3];

   int indexOfVertex(GLuint vertex)
   {
      for(int i = 0; i < 3; ++i)
         if(vertices[i] == vertex)
            return i;

      assert(0);
   }
};

struct Edge
{
   GLuint v0, v1;
   Triangle *const tri;
   const int edge_num;

   Edge(GLuint v0_, GLuint v1_, Triangle* tri_, int edge_num_):
      v0(v0_), v1(v1_), tri(tri_), edge_num(edge_num_)
   {
      if(v1 < v0)
         std::swap(v0, v1);
   }

   bool operator==(const Edge& e) const
   {
      return v0 == e.v0 && v1 == e.v1;
   }

   bool operator<(const Edge& e) const
   {
      return (v0 == e.v0) ? (v1 < e.v1) : (v0 < e.v0);
   }
};

Mesh::Mesh(const uint max_vcount, const uint max_tcount):
         vertices(NULL), normals(NULL), indices(NULL), vcount(0),
         tcount(0), vbo(0), ebo(0), dirty(false), max_vcount(max_vcount),
         max_tcount(max_tcount)
{
   vertices = new GLfloat[max_vcount * 3];
   normals = new GLfloat[max_vcount * 3];
   indices = new GLuint[max_tcount * 3];
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
            vertices[vcount * 3 + 1] = y ? +1.0f : -1.0f;
            vertices[vcount * 3 + 0] = x ? +1.0f : -1.0f;
            vertices[vcount * 3 + 2] = z ? +1.0f : -1.0f;

            ++vcount;
         }

   GLuint i2 = 0;

   for(GLuint axis = 0; axis < 3; ++axis)
      for(GLuint side = 0; side < 2; ++side)
      {
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

   generateSimpleCubeMesh(vertices, indices, vcount, tcount);
}

void Mesh::generateSimpleCubeMesh(GLfloat* vertices, GLuint* indices, GLuint& vcount, GLuint& tcount)
{
   vcount = 0;
   tcount = 0;

   for(GLuint z = 0; z < 2; ++z)
      for(GLuint y = 0; y < 2; ++y)
         for(GLuint x = 0; x < 2; ++x)
         {
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

         indices[i2++] = 1 * s_step + 0 * t_step + u_step;
         indices[i2++] = 0 * s_step + 0 * t_step + u_step;
         indices[i2++] = 0 * s_step + 1 * t_step + u_step;

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
         vertices[vcount * 3 + 0] = (float(x) - float(w) * 0.5f) * 1.2f;
         vertices[vcount * 3 + 1] = 0.0f;
         vertices[vcount * 3 + 2] = (float(z) - float(h) * 0.5f) * 1.2f;
         ++vcount;


         if(x < (w - 1) && z < (h - 1))
         {
            if((x ^ z) & 1)
            {
               indices[tcount * 3 + 0] = x + z * w;
               indices[tcount * 3 + 1] = x + z * w + w;
               indices[tcount * 3 + 2] = x + z * w + 1;
               ++tcount;

               indices[tcount * 3 + 0] = x + z * w + 1;
               indices[tcount * 3 + 1] = x + z * w + w;
               indices[tcount * 3 + 2] = x + z * w + 1 + w;
               ++tcount;
            }
            else
            {
               indices[tcount * 3 + 0] = x + z * w;
               indices[tcount * 3 + 1] = x + z * w + w;
               indices[tcount * 3 + 2] = x + z * w + 1 + w;
               ++tcount;

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

void Mesh::subdivide()
{
   dirty = true;

   assert(indices != NULL);
   assert(vertices != NULL);
   assert(normals != NULL);

   Triangle*  tris = new Triangle[tcount];
   Triangle** vertex_adj_tris = new Triangle*[vcount];
   GLfloat* out_vertices = new GLfloat[vcount * 4 * 3];

   GLuint new_vcount = 0;

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

   {
      // now we will populate the adjacent triangle pointers
      // of each triangle
      std::set<Edge> edges;

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

               const int opp0 = tri.vertices[(k + 2) % 3] * 3,
                         opp1 = it->tri->vertices[(it->tri->indexOfVertex(tri.vertices[k]) + 1) % 3] * 3;

               tri.odd_verts[k] = vcount + new_vcount;
               it->tri->odd_verts[it->edge_num] = vcount + new_vcount;


               // loop subdivision rules for odd vertices
               for(int j = 0; j < 3; ++j)
                  out_vertices[tri.odd_verts[k] * 3 + j] = 3.0f / 8.0f * (vertices[tri.vertices[k] * 3 + j] +
                                                         vertices[tri.vertices[(k + 1) % 3] * 3 + j]) +
                                                     1.0f / 8.0f * (vertices[opp0 + j] + vertices[opp1 + j]);

               ++new_vcount;

               // no longer needed as each edge should only be reference by at most two triangles
               edges.erase(e);
            }
         }
      }

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

   delete[] tris;
   delete[] vertex_adj_tris;
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

