
#include "Engine.hpp"

#include <cstring>
#include <cctype>
#include <cmath>
#include <string>
#include <map>
#include <vector>
#include <cassert>


using std::min;
using std::max;
using std::strncmp;
using std::isspace;
using std::sscanf;
using std::fgets;
using std::string;
using std::map;
using std::vector;
using std::abs;

using namespace model;


model::Mesh::MaterialLibMap model::Mesh::material_libs;


template<typename T>
static void deleteMapElements(T& map)
{
   for(typename T::iterator it=map.begin(); it!=map.end(); ++it)
      delete it->second;

   map.clear();
}

model::Mesh::MaterialLibMap::~MaterialLibMap()
{
   deleteMapElements(*this);
}

void MaterialLib::addMaterial(const std::string& name, const Vec3& mat_a,
                           const Vec3& mat_d,  const Vec3& mat_s, Texture* mat_d_tex, Texture* mat_s_tex)
{
   Material* material = new Material;

   material->refractive = false;

   material->reflectivity = std::max(mat_s.x, std::max(mat_s.y, mat_s.z));

   material->glossiness = Vec1(0.0); // TODO: should be based on 'Ns' parameter

   material->diffuse = saturate( mat_d );
   material->specular = Colour( (material->reflectivity < Vec1(1e-6)) ? Vec3(Vec1(0.0)) :
                                 ( mat_s / Vec3(material->reflectivity) ) );

   material->texture_diffuse = mat_d_tex;
   material->texture_specular = mat_s_tex;

   material->emission = Vec3(Vec1(0.0));

   if(materials[name])
      delete materials[name];

   materials[name] = material;
}

Texture* Texture::fromFilename(const std::string& fname)
{
   Texture* tx = new Texture;
   tx->handle = Scene::loadTexture(fname.c_str());
   return tx;
}

int model::Mesh::loadOFFFile(FILE *const src)
{
   {
      char str[8];
      str[0] = '\0';
      fgets(str,8,src);

      if(strncmp(str,"OFF",3))
         return -1;
   }

   int num_vertices=0, num_faces=0, num_edges=0;

   if(fscanf(src," %d %d %d",&num_vertices,&num_faces,&num_edges) != 3)
      return -1;

   vertices.reserve(num_vertices);

   for(int i=0;i<num_vertices;++i)
   {
      float x, y, z;

      if(fscanf(src," %f %f %f",&x,&y,&z) != 3)
         return -1;

      vertices[i].x = x;
      vertices[i].y = y;
      vertices[i].z = z;
   }

   for(int i=0;i<num_faces;++i)
   {
      int num, first, prev = -1;
      Triangle tri;

      if(fscanf(src," %d %d",&num,&first) != 2)
         return -1;

      for(;num > 1;--num)
      {
         int i;
         if(fscanf(src," %d",&i) != 1)
            return -1;

         if(prev != -1)
         {
            tri.a=first;
            tri.b=prev;
            tri.c=i;

            tri.texcoords[0] = Vec2(-0.5,-0.5);
            tri.texcoords[1] = Vec2(1,-0.5);
            tri.texcoords[2] = Vec2(1,1);

            triangles.push_back(tri);
         }

         prev=i;
      }
   }

   return 0;
}



// parse an mtl file, which are nearly always complementary
// to an obj mesh
int MaterialLib::loadMTLFile(FILE *const mtl_lib)
{
   uint mats = 0;
   string newmtl("");

   const int max_line_length = 1024;
   char str[max_line_length + 1];

   str[max_line_length] = '\0';

   // default material attributes
   const Vec3 def_mat_a = Vec1(0.0), def_mat_d = Vec1(1.0),
              def_mat_s = Vec1(0.0);

   Vec3 mat_a = def_mat_a, mat_d = def_mat_d, mat_s = def_mat_s;
   Texture *mat_d_tex = NULL, *mat_s_tex = NULL;

   char* c;

   // the following loop replaces anything in str[]
   while(!feof(mtl_lib))
   {
      str[0] = '\0';
      fgets(str, max_line_length, mtl_lib);

      // remove newlines and carriage returns
      for(c = str; *c; ++c)
         if(*c == '\n' || *c == '\r')
         {
            *c = '\0';
            break;
         }

      switch(str[0])
      {
         // textures
         case 'm':
            if(!strncmp(str, "map_Ka", 6)) // ambient
            {
               // ignored, as currently there is no possible way this could be used.
            }
            else if(!strncmp(str, "map_Kd", 6)) // diffuse
            {
               c = str + 6; // skip past 'map_Kd'

               while(*c && isspace(*c)) { ++c; }

               if(!textures[c])
                  textures[c] = Texture::fromFilename(c);

               mat_d_tex = textures[c];

               if(!mat_d_tex)
                  return -1;
            }
            else if(!strncmp(str, "map_Ks", 6)) // specular
            {
               c = str + 6; // skip past 'map_Ks'

               while(*c && isspace(*c)) { ++c; }

               if(!textures[c])
                  textures[c] = Texture::fromFilename(c);

               mat_s_tex = textures[c];

               if(!mat_s_tex)
                  return -1;
            }
            break;

         // new material name start
         case 'n':
            if(strncmp(str, "newmtl", 6))
               return -1;

            c = str + 6; // skip past 'newmtl'

            while(*c && isspace(*c)) { ++c; }

            if(mats > 0)
            {
               // add currently built material to list
               addMaterial(newmtl, mat_a, mat_d, mat_s, mat_d_tex, mat_s_tex);

               // restore default attributes
               mat_a = def_mat_a;
               mat_d = def_mat_d;
               mat_s = def_mat_s;

               mat_d_tex = NULL;
               mat_s_tex = NULL;
            }

            newmtl = c;
            ++mats;
            break;

         // reflectance and transmission factors
         case 'K':
            if(mats == 0)
               return -1;

            if(str[1] == 'a') // ambient
               sscanf(str, "Ka %f %f %f", &mat_a.x, &mat_a.y, &mat_a.z);
            else if(str[1] == 'd') // diffuse
               sscanf(str, "Kd %f %f %f", &mat_d.x, &mat_d.y, &mat_d.z);
            else if(str[1] == 's') // specular
               sscanf(str, "Ks %f %f %f", &mat_s.x, &mat_s.y, &mat_s.z);
            break;
      }
   }

   // add the last material in the file
   if(mats > 0)
      addMaterial(newmtl, mat_a, mat_d, mat_s, mat_d_tex ,mat_s_tex);


   return 0;
}



int MaterialLib::loadFile(const char *const file_name)
{
   FILE* src=fopen(file_name,"r");

   if(!src)
      return -1;

   int err=0;

   char ext[strlen(file_name)+1];

   {
      const char* s=strrchr(file_name,'.');

      if(!s)
         return -1;

      strcpy(ext,s+1);
   }

   for(char* c=ext;*c;++c)
      *c=tolower(*c);

   if(!strcmp(ext,"mtl"))
      err=loadMTLFile(src);
   else
      return -1;

   fclose(src);

   return err;
}


// load a static OBJ mesh from a file. this function will
// open other files when necessary for the purpose of reading materials
int model::Mesh::loadOBJFile(FILE *const src)
{
   using namespace std;

   int has_normals=0, has_texcoords=0;

   uint material_index = 0;

   const int max_line_length = 1024;
   char str[max_line_length + 1];

   str[max_line_length] = '\0';

   const MaterialLib* mtllib = NULL;
   const Material* curr_mat = NULL;

   vector<Vec2> texcoords;
   string object(""), group(""), usemtl("");

   while(!feof(src))
   {
      str[0] = '\0';
      fgets(str,max_line_length,src);

      // remove newlines and carriage returns
      for(char* c = str; *c; ++c)
         if(*c == '\n' || *c == '\r')
         {
            *c = '\0';
            break;
         }

      switch(str[0])
      {
         // parse material lib file
         case 'm':
            {
               if(strncmp(str, "mtllib", 6))
                  return -1;

               const char* mtllib_name = str + 6;

               while(*mtllib_name && isspace(*mtllib_name)) { ++mtllib_name; }

               mtllib = material_libs[mtllib_name];

               if(!mtllib)
               {
                  MaterialLib* new_mtllib = material_libs[mtllib_name] = new MaterialLib;

                  if(new_mtllib->loadFile(mtllib_name) != 0)
                     return -1;

                  mtllib = new_mtllib;
               }
            }
            break;

         // set object name
         case 'o':
            {
               const char *c = str + 1;
               while(*c && isspace(*c)) { ++c; }
               object = c;
            }
            break;

         // set polygon group name
         case 'g':
            {
               const char *c = str + 1;
               while(*c && isspace(*c)) { ++c; }
               group = c;
            }
            break;

         // set current material
         case 'u':
            {
               if(strncmp(str, "usemtl", 6))
                  return -1;

               const char *c = str + 6; // skip past 'usemtl'
               while(*c && isspace(*c)) { ++c; }
               usemtl = c;
               ++material_index;

               if(!mtllib)
                  return -1;

               curr_mat = (*mtllib)[usemtl];

               if(!curr_mat)
                  return -1;
            }
            break;

         // vertex position, texcoord, and normal
         case 'v':
            {
               if(str[1]=='n')
               {
                  has_normals=1;
                  break;
               }
               else if(str[1]=='t')
               {
                  has_texcoords=1;

                  float x,y,z;
                  int num = sscanf(str,"vt %f %f %f",&x,&y,&z);

                  if(num < 2)
                     return -1;

                  texcoords.push_back(Vec2(x,y));

                  break;
               }
               else if(str[1]==' ')
                  ;
               else
                  break;

               float x,y,z,w;
               int num = sscanf(str,"v %f %f %f %f",&x,&y,&z,&w);

               if(num < 3)
                  return -1;

               vertices.push_back(Vec3(x,y,z));
            }
            break;

         // triangle
         case 'f':
            {
               int p0,t0,n0,p1,t1,n1,p2,t2,n2;

               bool face_has_normals = false, face_has_texcoords = false;

               if(has_normals && has_texcoords &&
                  sscanf(str,"f %d / %d / %d %d / %d / %d %d / %d / %d",&p0,&t0,&n0,&p1,&t1,&n1,&p2,&t2,&n2) == 9)
               {
                  face_has_normals = true;
                  face_has_texcoords = true;
               }
               else if(has_texcoords &&
                  sscanf(str,"f %d / %d %d / %d %d / %d",&p0,&t0,&p1,&t1,&p2,&t2) == 6)
               {
                  face_has_texcoords = true;
               }
               else if(has_normals &&
                  sscanf(str,"f %d // %d %d // %d %d // %d",&p0,&n0,&p1,&n1,&p2,&n2) == 6)
               {
                  face_has_normals = true;
               }
               else if(sscanf(str,"f %d %d %d",&p0,&p1,&p2) != 3)
               {
                  return -1;
               }

               if(!p0 || !p1 || !p2)
                  return -1;

               if(p0 < 0) { p0 = vertices.size() + p0; }
               if(p1 < 0) { p1 = vertices.size() + p1; }
               if(p2 < 0) { p2 = vertices.size() + p2; }

               Triangle tri;

               tri.material = curr_mat;

               tri.a = p0-1;
               tri.b = p1-1;
               tri.c = p2-1;

               if(face_has_texcoords)
               {
                  if(!t0 || !t1 || !t2)
                     return -1;

                  if(t0 < 0) { t0 = texcoords.size() + t0; }
                  if(t1 < 0) { t1 = texcoords.size() + t1; }
                  if(t2 < 0) { t2 = texcoords.size() + t2; }

                  tri.texcoords[0] = texcoords[t0 - 1];
                  tri.texcoords[1] = texcoords[t1 - 1];
                  tri.texcoords[2] = texcoords[t2 - 1];
               }

               triangles.push_back(tri);
            }
            break;
      }
   }

   return 0;
}


void model::Mesh::centerOrigin()
{
   Vec3 box_min(1e9, 1e9, 1e9), box_max(-1e9, -1e9, -1e9);

   for(std::vector<Vec3>::iterator it = vertices.begin(); it != vertices.end(); ++it)
   {
      box_min.x = std::min(box_min.x, it->x);
      box_min.y = std::min(box_min.y, it->y);
      box_min.z = std::min(box_min.z, it->z);

      box_max.x = std::max(box_max.x, it->x);
      box_max.y = std::max(box_max.y, it->y);
      box_max.z = std::max(box_max.z, it->z);
   }

   Vec3 mid = (box_min + box_max) * 0.5;

   for(std::vector<Vec3>::iterator it = vertices.begin(); it != vertices.end(); ++it)
      (*it) -= mid;
}

int model::Mesh::loadFile(const char *const file_name)
{
   assert(file_name != NULL);

   FILE* src=fopen(file_name,"r");

   if(!src)
      return -1;

   int err=0;

   char ext[strlen(file_name)+1];

   {
      const char* s=strrchr(file_name,'.');

      if(!s)
      {
         fclose(src);
         return -1;
      }

      strcpy(ext,s+1);
   }

   for(char* c=ext;*c;++c)
      *c=tolower(*c);

   if(!strcmp(ext,"obj"))
      err = loadOBJFile(src);
   else if(!strcmp(ext,"off"))
      err = loadOFFFile(src);
   else
      err = -1;

   fclose(src);

   return err;
}

