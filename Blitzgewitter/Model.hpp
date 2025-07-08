
#pragma once

#include <cstring>
#include <cctype>
#include <cmath>
#include <string>
#include <map>
#include <vector>
#include <cassert>

namespace model
{

struct Material;
struct Mesh;
class MaterialLib;
struct Colour;
struct Texture;

struct Texture
{
    GLuint handle;

    static Texture* fromFilename(const std::string&);
};

struct Colour: Vec3
{
   ElementType luminance() const
   {
      return std::max(x,std::max(y,z));
   };

   Colour() { }

   Colour(const Vec3& v)
   {
      x=v.x;
      y=v.y;
      z=v.z;
   }
};

static Colour saturate(const Colour& c)
{
   typedef Colour::ElementType T;

   return Colour( Vec3( ( c.x > T(1) ) ? T(1) : ( ( c.x < T(0) ) ? T(0) : c.x ),
                    ( c.y > T(1) ) ? T(1) : ( ( c.y < T(0) ) ? T(0) : c.y ),
                    ( c.z > T(1) ) ? T(1) : ( ( c.z < T(0) ) ? T(0) : c.z ) ) );
}

struct Material
{
    const Texture *texture_diffuse, *texture_specular;
    const Texture *texture_emission;

    Colour diffuse, specular, emission;
    Real   reflectivity, glossiness;
    bool   refractive;
    Real   rindex_inner, rindex_outer; // refractive indices

    Material(): texture_diffuse(NULL), texture_specular(NULL),
             texture_emission(NULL)
    {
    }
};

struct Mesh
{
   struct SubObject
   {
      std::string name;
      long unsigned int first_triangle, triangle_count;
   };

   struct Triangle
   {
      uint a, b, c;
      Vec2 texcoords[3];
      const Material* material;

      bool operator<(const Triangle& tri) const
      {
         return material < tri.material;
      }
   };

   // this is to make sure dynamically allocated MaterialLibs are free'd
   // at static destruction. it is okay to subclass std::map because
   // the only reference is to the subclass. it is okay to delete elements from
   // the destructor because it will be called before ~std::map()
   static struct MaterialLibMap: std::map<std::string, MaterialLib*>
   {
      ~MaterialLibMap();

   } material_libs;

   bool has_own_materials;

   std::vector<Triangle>  triangles;
   std::vector<Vec3>      vertices;

   std::vector<SubObject> subobjects;

   int loadFile(const char *const file_name);

   void getAABB(Vec3& box_min, Vec3& box_max);
   void centerOrigin();

   uint triangleCount();
   void triangle(uint id, Vec3&, Vec3&, Vec3&, uint& tag);

   int loadOFFFile(FILE *const);
   int loadOBJFile(FILE *const);

   Mesh(): has_own_materials(false) { }
};


class MaterialLib
{
   public:
      typedef std::map<std::string, Material*> MaterialMap;
      typedef std::map<std::string, Texture*>  TextureMap;

   private:
      MaterialMap materials;

      // textures are allocated externally
      TextureMap  textures;

      void addMaterial(const std::string& name, const Vec3& mat_a,
                           const Vec3& mat_d,  const Vec3& mat_s,
                           Texture* mat_d_tex, Texture* mat_s_tex);

   public:
      MaterialLib() { }

      const Material* operator[](MaterialMap::key_type k) const
      {
         MaterialMap::const_iterator it;
         if((it = materials.find(k)) != materials.end())
            return it->second;
         return NULL;
      }

      ~MaterialLib()
      {
         for(MaterialMap::iterator it=materials.begin();
                        it!=materials.end(); ++it)
            delete it->second;
      }

      int loadMTLFile(FILE *const);
      int loadFile(const char *const file_name);
};

}
