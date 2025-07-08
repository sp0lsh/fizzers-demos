
#pragma once

//#define NDEBUG

#include <vector>
#include "gl3w/include/GL3/gl3w.h"
#include <cassert>
#include <algorithm>
#include <set>

#include <cstdio>

#include "Mat.hpp"

#ifdef NDEBUG
#define CHECK_FOR_ERRORS
#else
#define CHECK_FOR_ERRORS checkForErrors(__FILE__, __LINE__);
#endif

#define SHADERS_PATH "shaders/"
#define IMAGES_PATH  "images/"
#define MESHES_PATH  "meshes/"

typedef float Real;
typedef unsigned int uint;
typedef TMat4<Real> Mat4;
typedef TVec3<Real> Vec3;
typedef TVec2<Real> Vec2;
typedef Real Vec1;

#include  "Model.hpp"

template<typename T>
static T mix(T a, T b, double c)
{
   return a * (1.0 - c) + b * c;
}

static double clamp(double x, double l, double u)
{
   return std::min(u, std::max(l, x));
}

// music
struct ChannelState
{
   int type; // 0 = noise only, 1 = sawtooth + noise, 2 = square + noise

   bool use_cut_env, use_amp_env, invert_filter;

   double amp, amp_env_t0, amp_env_t1, amp_env_t2;
   double cut, cut_env_t0, cut_env_t1, cut_env_t2;
   double res;

   double freq, freq_zoom; // in oscillations per sample
   double t_offset;

   double pan; // -1 = left, 0 = middle, +1 = right

   double noise;

   ChannelState()
   {
      memset(this, 0, sizeof(*this));
   }

};


template<typename T>
class Spline
{
    struct Node
    {
        T point, tangent;
        uint frame_num;

        bool operator<(const Node& n) const
        {
            return frame_num < n.frame_num;
        }
    };

    mutable struct
    {
        T point;
        uint frame_num;

    } cache;

    std::vector<Node> nodes;
    uint nodes_size;
    bool initialized;

    public:
        Spline() : nodes_size(0), initialized(false)
        {
        }

        void insert(const T& point, const T& tangent, const uint frame_num)
        {
            Node node;
            node.point = point;
            node.tangent = tangent;
            node.frame_num = frame_num;
            nodes.push_back(node);
        }

        void initialize()
        {
            std::sort(nodes.begin(), nodes.end());
            nodes_size = nodes.size();
            initialized = true;
        }

        void repeat(const uint start, const uint end, const uint offset)
        {
            nodes_size = nodes.size();
            for(uint i = 0; i < nodes.size(); ++i)
            {
               if(nodes[i].frame_num >= start && nodes[i].frame_num <= end)
               {
                  Node n = nodes[i];
                  n.frame_num += offset;
                  nodes.push_back(n);
               }
            }
        }

        T evaluate(const uint frame_num) const
        {
            assert(initialized == true);
            assert(nodes_size > 0);

            if(cache.frame_num == frame_num)
               return cache.point;

            int start = 0, end = nodes_size - 1;

            do
            {
                const int mid = (start + end) / 2;
                const Node& node = nodes[mid];
                if(frame_num > node.frame_num)
                    start = mid;
                else
                    end = mid;

            } while(start < end - 1);

            const Node &n0 = nodes[start], &n1 = nodes[end];

            if(start == end)
                return n0.point;

            if(n1.frame_num <= frame_num)
                return n1.point;

            if(n0.frame_num >= frame_num)
                return n0.point;

            uint ofs = frame_num - n0.frame_num, len = n1.frame_num - n0.frame_num;

            // cubic hermite curve
            Real t = Real(ofs) / Real(len),
                c0 = (Real(2) * t - Real(3)) * t * t + Real(1), // 2t^3 - 3t^2 +1
                c1 = (t - Real(2)) * t * t + t,                 // t^3 - 2t^2 + t
                c2 = (Real(-2) * t + Real(3)) * t * t,          // -2t^3 + 3t^2
                c3 = t * t * t - t * t;                         // t^3 - t^2

            const T point = n0.point * c0 + n0.tangent * c1 + n1.point * c2 + n1.tangent * c3;

            cache.point = point;
            cache.frame_num = frame_num;

            return point;
        }
};


class Shader
{
   GLuint program;
   bool loaded;

   public:
      Shader() : program(0), loaded(false) { }

      void uniform1i(const char* name, GLint x);
      void uniform2i(const char* name, GLint x, GLint y);

      void uniform4f(const char* name, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
      void uniform3f(const char* name, GLfloat x, GLfloat y, GLfloat z);
      void uniform2f(const char* name, GLfloat x, GLfloat y);
      void uniform1f(const char* name, GLfloat x);

      void uniformMatrix4fv(const char* name, GLsizei count, GLboolean transpose, const GLfloat* value);

      void bind();
      bool isLoaded() const { return loaded; }
      void load(const char* vsh_filename, const char* gsh_filename,
                            const char* fsh_filename);
      void free();

      ~Shader() { free(); }
};

extern GLuint createProgram(const char* vsh_filename, const char* gsh_filename,
                            const char* fsh_filename);

class Mesh
{
   public:
   GLfloat* vertices;
   GLfloat* normals;
   GLuint* indices;
   GLuint vcount, tcount;
   GLuint vbo, ebo;
   bool dirty;
   const GLuint max_vcount, max_tcount;

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

      Triangle*  tris;
      Triangle** vertex_adj_tris;
      std::set<Edge> edges;
      bool analysed;

      Mesh(const uint max_vcount = 4096, const uint max_tcount = 4096);
      ~Mesh();

      uint getTriangleCount() const { return tcount; }
      uint getVertexCount() const { return vcount; }

      bool isAnalysed() const { return analysed; }

      void analyseTopology(GLfloat* out_vertices=NULL,GLuint* new_vcount=NULL);
      void addPoly(int a, int b, int c);
      void addPoly(int a, int b, int c, int d);
      void makeVertex(int i, GLfloat x, GLfloat y, GLfloat z);
      void makeTriangle(int i, int a, int b, int c);
      Vec3 getPointInTriangle(const GLuint tri, Real u, Real v) const;
      void getTriangleAreas(GLfloat* areas) const;
      void generateNormals();
      void subdivide();
      void extrude(const GLuint tri, const GLfloat* dir, const float s);
      void addHeightNoise();
      void generateGrid(const uint w, const uint h);
      void generateSimpleCube();
      void generateCube();
      void generateIcosahedron();
      void generateTetrahedron();
      void generateTetrahedron2();
      void generateDodecahedron();
      void generateOctahedron();
      void generateCylinder(GLuint sides, bool cap0, bool cap1, GLuint segments);
      void addInstance(const Mesh& mesh, const Mat4& transform);
      void separate();
      void transform(const Mat4& transform);
      void reverseWindings();
      void bind();
      void free();
      const Vec3& getVertex(GLuint index) const;
      const Vec3& getNormal(GLuint index) const;
      const Vec3& getTriangleNorm(GLuint t) const;

      static void generateSimpleCubeMesh(GLfloat* vertices, GLuint* indices,
                                          GLuint& vcount, GLuint& tcount, const GLuint max_vcount, const GLuint max_tcount);
};


class Scene
{
   Shader downsample_shader;

   public:
      void downsample8x(GLuint srctex, GLuint fbo0, GLuint tex0, GLuint target_fbo,
                        GLsizei vpw, GLsizei vph);

      void createWindowTexture(GLuint tex, GLenum format, uint divisor = 1);

      static GLuint loadTexture(const char *file_name,bool mipmaps=false);

      uint frame_num;
      uint window_width, window_height;
      float time, music_time;
      float scene_start_time;

      bool initialized;

      Scene(): frame_num(0), window_width(0), window_height(0), time(0.0f)
      {
         initialized = false;
      }

      virtual ~Scene()
      {
      }

      virtual void slowInitialize() = 0;
      virtual void initialize() = 0;
      virtual void render() = 0;
      virtual void update() = 0;
      virtual void free() = 0;
};

extern FILE* g_logfile;

static void checkForErrors(const char* file_name, int line_num)
{
   const GLenum error = glGetError();
   if(error)
   {
      if(g_logfile)
      {
         fprintf(g_logfile, "*** File '%s', Line %05d: OpenGL Error Code 0x%08x ***\n", file_name, line_num, error);
         fclose(g_logfile);
      }
      exit(0);
   }
}

static void log(const char* str, ...)
{
   if(!g_logfile)
      return;

   va_list l;
   va_start(l, str);
   vfprintf(g_logfile, str, l);
   va_end(l);

   fflush(g_logfile);
}

static inline double cubic(double x)
{
   return (3.0 - 2.0 * x) * x * x;
}


