
// midnight platonic

#include "Engine.hpp"
#include "Bezier.hpp"
#include "Ran.hpp"
#include "Particles.hpp"
#include <omp.h>

static inline Real frand()
{
   static Ran rnd(643);
   return rnd.doub();
}

static long int lrand()
{
   static Ran rnd(112);
   return rnd.int32();
}

static const char synth_pattern[]=".X..X.XX.X.XXXXX" ".X..X.XX.XXXXXXX";

class RoomScene: public Scene
{
   typedef std::pair<Vec2,Vec2> Edge;

   struct VoronoiCell
   {
      std::vector<Edge> edges;
      Vec2 center;
      float time0,time1;
   };

   Shader gaussbokeh_shader, composite_shader;
   Shader mb_accum_shader;
   Shader room_shader;
   Shader room_object_shader;
   Shader room_logo_shader;
   Shader text_shader;

   Mat4 object_transform;

   int num_subframes;
   float subframe_scale;

   float lltime;
   int current_material_colour_set;

   Mesh icosahedron_mesh;

   model::Mesh text_mesh;

   static const int max_voronoi_vertices=1024*6;
   GLfloat voronoi_vertices[max_voronoi_vertices*2];
   GLfloat voronoi_coords[max_voronoi_vertices*2];
   int num_voronoi_vertices;

   static const int max_voronoi_indices=1024*6;
   GLushort voronoi_indices[max_voronoi_indices];
   int num_voronoi_indices;

   static const int max_voronoi_cells=128;
   VoronoiCell voronoi_cells[max_voronoi_cells];
   int num_voronoi_cells;

   static const int volume_size_exp = 6;
   static const int volume_size = (1<<volume_size_exp);

   Vec3 volume_min;
   Vec3 volume_max;

   model::Mesh volume_mesh;
   unsigned char* volume_data;

   static const int num_fbos = 16, num_texs = 16;

   static const GLuint max_materials=16;
   static const GLuint max_submesh_indices=4096;
   static const GLuint max_submesh_vertices=4096;

   GLuint num_materials, num_submesh_indices, num_submesh_vertices;

   GLushort submesh_offsets[max_materials];
   GLushort submesh_indices[max_submesh_indices];
   GLfloat submesh_vertices[max_submesh_vertices * 3];
   GLfloat submesh_normals[max_submesh_vertices * 3];
   Vec3 submesh_material_colours[max_materials];

   static const int max_synth_hits=256;
   double synth_hit_times[max_synth_hits];

   GLuint mesh_vbo, mesh_ebo;

   GLuint fbos[num_fbos], texs[num_texs], renderbuffers[num_fbos];

   GLuint noise_tex;
   GLuint noise_tex_2d;

   GLuint volume_tex;

   GLuint logo_tex;

   GLuint text_mesh_num_indices;
   GLuint text_mesh_num_vertices;
   GLfloat* text_mesh_vertices;
   GLuint* text_mesh_indices;

   GLuint text_vbo, text_ebo;

   void initVoronoi();
   void initVolume();
   void slowInitialize();
   void initializeTextures();
   void initializeShaders();
   void initializeBuffers();

   void drawText(const std::string& str,float br=1.0f);
   void drawLogo(float ltime);
   void drawRoom(const Mat4& modelview);
   void drawView(float ltime);
   void gaussianBlur(Real tint_r, Real tint_g, Real tint_b, Real radx, Real rady, GLuint fbo0, GLuint fbo1, GLuint src_tex, GLuint tex0);


   public:
      RoomScene(): num_materials(0), num_submesh_indices(0), num_submesh_vertices(0), mesh_vbo(0), mesh_ebo(0), volume_data(0), icosahedron_mesh(32, 32)
      {
         num_voronoi_vertices = 0;
         num_voronoi_cells = 0;
         num_voronoi_indices = 0;
         text_mesh_num_indices=0;
         text_mesh_num_vertices=0;
         text_mesh_vertices=0;
         text_mesh_indices=0;
         text_vbo=0;
         text_ebo=0;
      }

      ~RoomScene()
      {
         free();
      }


      void initialize();
      void render();
      void update();
      void free();
};

Scene* roomScene = new RoomScene();

static const float logo_appear_time = (3.0f * 60.0f + 47.0f) - (3.0f * 60.0f + 15.5f);

template<typename T>
static T sq(const T& x)
{
   return x*x;
}

void RoomScene::initVoronoi()
{
   static const int voronoi_num_points=32;
   Ran lrnd(1582);

   std::vector<Vec2> nodes;

   for(int i=0;i<voronoi_num_points;++i)
   {
      nodes.push_back(Vec2(lrnd.doub(),lrnd.doub()));
   }

   for(std::vector<Vec2>::iterator n0=nodes.begin();n0!=nodes.end();++n0)
   {
      std::vector<Edge> edges;
      for(std::vector<Vec2>::iterator n1=nodes.begin();n1!=nodes.end();++n1)
      {
         if(n0==n1)
            continue;

         Vec2 d=(*n1)-(*n0);
         Vec2 pd=Vec2(-d.y,d.x);

         Edge e;

         e.first = *n0 + (d * 0.5) - (pd * 100.0);
         e.second = *n0 + (d * 0.5) + (pd * 100.0);

         edges.push_back(e);
      }
      edges.push_back(Edge(Vec2(0,0),Vec2(1,0)));
      edges.push_back(Edge(Vec2(1,0),Vec2(1,1)));
      edges.push_back(Edge(Vec2(1,1),Vec2(0,1)));
      edges.push_back(Edge(Vec2(0,1),Vec2(0,0)));

      log("%d edges\n",edges.size());

      std::vector<Edge> clipped_edges;
      for(std::vector<Edge>::iterator e0=edges.begin();e0!=edges.end();++e0)
      {
         Edge e=*e0;
         bool removed=false;
         for(std::vector<Edge>::iterator e1=edges.begin();e1!=edges.end();++e1)
         {
            if(e0==e1)
               continue;

            Vec2 p=e1->second-e1->first;
            p=Vec2(p.y,-p.x);
            p=p*(Real(1)/std::sqrt(p.lengthSquared()));

            Real d0=(e.first-e1->first).dot(p);
            Real d1=(e.second-e1->first).dot(p);

            if(d0 < 0 && d1 < 0)
               continue;

            if(d0 > 0 && d1 > 0)
            {
               removed=true;
               break;
            }

            if(d0 < 0)
            {
               Real t=d0/(d0-d1);
               e.second=mix(e.first,e.second,t);
            }
            else
            {
               Real t=d1/(d1-d0);
               e.first=mix(e.second,e.first,t);
            }
         }
         if(!removed && (e.second-e.first).lengthSquared() > 1e-8)
         {
            clipped_edges.push_back(e);
         }
      }
      if(!clipped_edges.empty())
      {
         log("%d edges\n",clipped_edges.size());
         log("%f %f\n",n0->x,n0->y);
         log("%f %f\n",clipped_edges[0].first.x,clipped_edges[0].first.y);
         assert(num_voronoi_cells<max_voronoi_cells);
         VoronoiCell& vc=voronoi_cells[num_voronoi_cells];
         vc.edges=clipped_edges;
         vc.center=*n0;
         vc.time0=float(num_voronoi_cells)/float(voronoi_num_points)*3.0f;
         vc.time1=vc.time0+1.0f;;
         ++num_voronoi_cells;
      }
   }
}

void RoomScene::initVolume()
{
   volume_data = new unsigned char[volume_size * volume_size * volume_size * 4];

   volume_mesh.getAABB(volume_min, volume_max);

   Vec3 volume_box_size=volume_max-volume_min;

   assert(volume_box_size.x>0);
   assert(volume_box_size.y>0);
   assert(volume_box_size.z>0);

   const Vec3 volume_center=(volume_max+volume_min)*Real(0.5);

   const Real thickness=std::max(volume_box_size.x,std::max(volume_box_size.y,volume_box_size.z))/Real(volume_size)*0.5;
   const Real expansion=thickness*10;

   volume_min-=Vec3(expansion);
   volume_max+=Vec3(expansion);

   volume_box_size=volume_max-volume_min;

   struct TriangleBox
   {
      Vec3 box_min, box_max;
   };

   TriangleBox* triangle_boxes = new TriangleBox[volume_mesh.triangles.size()];

   {
      int ti=0;
      for(std::vector<model::Mesh::Triangle>::const_iterator it=volume_mesh.triangles.begin();it!=volume_mesh.triangles.end();++it,++ti)
      {
         triangle_boxes[ti].box_min=Vec3(+1e4);
         triangle_boxes[ti].box_max=Vec3(-1e4);

         triangle_boxes[ti].box_min.x=std::min(triangle_boxes[ti].box_min.x,volume_mesh.vertices[it->a].x);
         triangle_boxes[ti].box_min.y=std::min(triangle_boxes[ti].box_min.y,volume_mesh.vertices[it->a].y);
         triangle_boxes[ti].box_min.z=std::min(triangle_boxes[ti].box_min.z,volume_mesh.vertices[it->a].z);

         triangle_boxes[ti].box_min.x=std::min(triangle_boxes[ti].box_min.x,volume_mesh.vertices[it->b].x);
         triangle_boxes[ti].box_min.y=std::min(triangle_boxes[ti].box_min.y,volume_mesh.vertices[it->b].y);
         triangle_boxes[ti].box_min.z=std::min(triangle_boxes[ti].box_min.z,volume_mesh.vertices[it->b].z);

         triangle_boxes[ti].box_min.x=std::min(triangle_boxes[ti].box_min.x,volume_mesh.vertices[it->c].x);
         triangle_boxes[ti].box_min.y=std::min(triangle_boxes[ti].box_min.y,volume_mesh.vertices[it->c].y);
         triangle_boxes[ti].box_min.z=std::min(triangle_boxes[ti].box_min.z,volume_mesh.vertices[it->c].z);

         triangle_boxes[ti].box_max.x=std::max(triangle_boxes[ti].box_max.x,volume_mesh.vertices[it->a].x);
         triangle_boxes[ti].box_max.y=std::max(triangle_boxes[ti].box_max.y,volume_mesh.vertices[it->a].y);
         triangle_boxes[ti].box_max.z=std::max(triangle_boxes[ti].box_max.z,volume_mesh.vertices[it->a].z);

         triangle_boxes[ti].box_max.x=std::max(triangle_boxes[ti].box_max.x,volume_mesh.vertices[it->b].x);
         triangle_boxes[ti].box_max.y=std::max(triangle_boxes[ti].box_max.y,volume_mesh.vertices[it->b].y);
         triangle_boxes[ti].box_max.z=std::max(triangle_boxes[ti].box_max.z,volume_mesh.vertices[it->b].z);

         triangle_boxes[ti].box_max.x=std::max(triangle_boxes[ti].box_max.x,volume_mesh.vertices[it->c].x);
         triangle_boxes[ti].box_max.y=std::max(triangle_boxes[ti].box_max.y,volume_mesh.vertices[it->c].y);
         triangle_boxes[ti].box_max.z=std::max(triangle_boxes[ti].box_max.z,volume_mesh.vertices[it->c].z);
      }
   }

#pragma omp parallel for
   for(int z=0;z<volume_size;++z)
      for(int y=0;y<volume_size;++y)
         for(int x=0;x<volume_size;++x)
         {
            const Vec3 pnt=Vec3(volume_min.x+Real(x+0.5)/Real(volume_size)*(volume_max.x-volume_min.x),
                                volume_min.y+Real(y+0.5)/Real(volume_size)*(volume_max.y-volume_min.y),
                                volume_min.z+Real(z+0.5)/Real(volume_size)*(volume_max.z-volume_min.z));

            Real nearest_dist=1e4;
            const model::Mesh::Triangle* nearest_triangle=0;

            Vec3 accumulated_colour=Vec3(0.0);
            int accumulated_count=0;

            int ti=0;
            for(std::vector<model::Mesh::Triangle>::const_iterator it=volume_mesh.triangles.begin();it!=volume_mesh.triangles.end();++it,++ti)
            {
               const TriangleBox& tbox=triangle_boxes[ti];

               if(pnt.x < tbox.box_min.x || pnt.y < tbox.box_min.y || pnt.z < tbox.box_min.z ||
                  pnt.x > tbox.box_max.x || pnt.y > tbox.box_max.y || pnt.z > tbox.box_max.z)
                  continue;

               const Vec3 va=volume_mesh.vertices[it->a];
               const Vec3 vb=volume_mesh.vertices[it->b];
               const Vec3 vc=volume_mesh.vertices[it->c];

               const Vec3 ea=vb-va;
               const Vec3 eb=vc-vb;
               const Vec3 ec=va-vc;

               const Vec3 n=ea.cross(eb);

               const Vec3 ena=ea.cross(n);
               const Vec3 enb=eb.cross(n);
               const Vec3 enc=ec.cross(n);

               const Real da=ena.dot(pnt-va);
               const Real db=enb.dot(pnt-vb);
               const Real dc=enc.dot(pnt-vc);

               Real dist=1e4;

               const Real nl=std::sqrt(n.lengthSquared());
               assert(nl > 1e-4);
               const Real ndist=std::abs(n.dot(pnt-va)/nl);

               // interior
               if(da < 0 && db < 0 && dc < 0)
               {
                  dist=ndist;
               }

               // edge a
               if(da >= 0 && db < 0 && dc < 0)
               {
                  const Real l=std::sqrt(ena.lengthSquared());
                  assert(l > 1e-4);
                  dist=std::sqrt(sq(da/l)+ndist*ndist);
               }

               // edge b
               if(da < 0 && db >= 0 && dc < 0)
               {
                  const Real l=std::sqrt(enb.lengthSquared());
                  assert(l > 1e-4);
                  dist=std::sqrt(sq(db/l)+ndist*ndist);
               }

               // edge c
               if(da < 0 && db < 0 && dc >= 0)
               {
                  const Real l=std::sqrt(enc.lengthSquared());
                  assert(l > 1e-4);
                  dist=std::sqrt(sq(dc/l)+ndist*ndist);
               }

               // vertex a
               if(da < 0 && db >= 0 && dc >= 0)
               {
                  dist=std::sqrt((pnt-vc).lengthSquared());
               }

               // vertex b
               if(da >= 0 && db < 0 && dc >= 0)
               {
                  dist=std::sqrt((pnt-va).lengthSquared());
               }

               // vertex c
               if(da >= 0 && db >= 0 && dc < 0)
               {
                  dist=std::sqrt((pnt-vb).lengthSquared());
               }

               assert(dist>=0);

               if(dist<thickness)
               {
                  assert(it->material);
                  accumulated_colour+=it->material->diffuse;
                  accumulated_count+=1;
               }

               if(dist<nearest_dist)
               {
                  nearest_triangle=&*it;
                  nearest_dist=dist;
               }
            }

            unsigned char* texel=volume_data+(x+(y+z*volume_size)*volume_size)*4;

            texel[3]=(nearest_dist<=thickness) ? 255 : 0;

            if(!nearest_triangle)
               continue;

            if(accumulated_count > 0)
            {
               const Vec3 c=accumulated_colour*(1.0/Real(accumulated_count));
               texel[0]=clamp(c.x, Real(0), Real(1))*255;
               texel[1]=clamp(c.y, Real(0), Real(1))*255;
               texel[2]=clamp(c.z, Real(0), Real(1))*255;
            }
            else
            {
               const model::Material* mtl=nearest_triangle->material;

               if(mtl)
               {
                  texel[0]=clamp(mtl->diffuse.x, Real(0), Real(1))*255;
                  texel[1]=clamp(mtl->diffuse.y, Real(0), Real(1))*255;
                  texel[2]=clamp(mtl->diffuse.z, Real(0), Real(1))*255;
               }
               else
               {
                  texel[0]=texel[1]=texel[2]=128;
               }
            }

         }

   unsigned char* volume_data2=new unsigned char[volume_size*volume_size*volume_size*4];

   for(int m=0;m<2;++m)
   {
      memcpy(volume_data2,volume_data,volume_size*volume_size*volume_size*4);

#pragma omp parallel for
      for(int z=1;z<volume_size-1;++z)
         for(int y=1;y<volume_size-1;++y)
            for(int x=1;x<volume_size-1;++x)
            {
               if(volume_data[((x)+((y)+(z)*volume_size)*volume_size)*4 + 3] == 0)
               {
                  unsigned int r=0;
                  unsigned int g=0;
                  unsigned int b=0;
                  unsigned int n=0;
                  for(int k=-1;k<2;++k)
                     for(int j=-1;j<2;++j)
                        for(int i=-1;i<2;++i)
                        {
                           unsigned char* texel=volume_data2+((x+i)+((y+j)+(z+k)*volume_size)*volume_size)*4;
                           if(texel[3] > 0)
                           {
                              r+=texel[0];
                              g+=texel[1];
                              b+=texel[2];
                              ++n;
                           }
                        }

                  if(n>0)
                  {
                     volume_data[((x)+((y)+(z)*volume_size)*volume_size)*4 + 0] = r/n;
                     volume_data[((x)+((y)+(z)*volume_size)*volume_size)*4 + 1] = g/n;
                     volume_data[((x)+((y)+(z)*volume_size)*volume_size)*4 + 2] = b/n;
                     volume_data[((x)+((y)+(z)*volume_size)*volume_size)*4 + 3] = 128;
                  }
               }
            }
   }

   delete[] triangle_boxes;
   delete[] volume_data2;
}

void RoomScene::slowInitialize()
{
   initVoronoi();

   logo_tex = loadTexture(IMAGES_PATH "logo.png");

   {
      static const double bpm=330;
      int j=0;
      for(int i=0;i<max_synth_hits;++i)
      {
         while(synth_pattern[j%strlen(synth_pattern)]!='X')
            ++j;

         synth_hit_times[i]=j*(60.0/bpm)*1.0;
         ++j;
      }
   }

   {
      int err=text_mesh.loadFile(MESHES_PATH "blitzgewitter_text.obj");

      assert(err==0);
      assert(text_mesh.subobjects.size() > 5);

      text_mesh_num_indices=text_mesh.triangles.size()*3;
      text_mesh_num_vertices=text_mesh.vertices.size();
      text_mesh_vertices=new GLfloat[text_mesh_num_vertices*3];
      text_mesh_indices=new GLuint[text_mesh_num_indices];

      {
         int i=0;
         for(std::vector<model::Mesh::Triangle>::const_iterator it=text_mesh.triangles.begin();it!=text_mesh.triangles.end();++it,++i)
         {
            text_mesh_indices[i*3+0]=it->a;
            text_mesh_indices[i*3+1]=it->b;
            text_mesh_indices[i*3+2]=it->c;
         }
      }

      {
         int i=0;
         for(std::vector<Vec3>::const_iterator it=text_mesh.vertices.begin();it!=text_mesh.vertices.end();++it,++i)
         {
            text_mesh_vertices[i*3+0]=it->x;
            text_mesh_vertices[i*3+1]=it->y;
            text_mesh_vertices[i*3+2]=it->z;
         }
      }
   }

   int err=volume_mesh.loadFile(MESHES_PATH "cube.obj");

   assert(err==0);

   volume_mesh.centerOrigin();

   initVolume();

   num_materials=0;
   const model::Material* prev_material=0;
   for(std::vector<model::Mesh::Triangle>::const_iterator it=volume_mesh.triangles.begin();it!=volume_mesh.triangles.end();++it)
   {
      if(it->material != prev_material)
      {
         assert(it->material);
         assert(num_materials<max_materials);
         submesh_offsets[num_materials]=num_submesh_indices;
         submesh_material_colours[num_materials]=it->material->diffuse;
         num_materials++;
         prev_material=it->material;
      }
      assert(num_submesh_indices<max_submesh_indices);
      submesh_indices[num_submesh_indices++]=it->a;
      assert(num_submesh_indices<max_submesh_indices);
      submesh_indices[num_submesh_indices++]=it->b;
      assert(num_submesh_indices<max_submesh_indices);
      submesh_indices[num_submesh_indices++]=it->c;
   }

   assert(num_materials<max_materials);
   submesh_offsets[num_materials]=num_submesh_indices;

   num_submesh_vertices=0;
   for(std::vector<Vec3>::const_iterator it=volume_mesh.vertices.begin();it!=volume_mesh.vertices.end();++it)
   {
      assert(num_submesh_vertices<max_submesh_vertices);

      submesh_vertices[num_submesh_vertices*3+0]=it->x;
      submesh_vertices[num_submesh_vertices*3+1]=it->y;
      submesh_vertices[num_submesh_vertices*3+2]=it->z;

      submesh_normals[num_submesh_vertices*3+0]=0;
      submesh_normals[num_submesh_vertices*3+1]=0;
      submesh_normals[num_submesh_vertices*3+2]=0;

      num_submesh_vertices++;
   }

   for(std::vector<model::Mesh::Triangle>::const_iterator it=volume_mesh.triangles.begin();it!=volume_mesh.triangles.end();++it)
   {
      const Vec3 va=volume_mesh.vertices[it->a];
      const Vec3 vb=volume_mesh.vertices[it->b];
      const Vec3 vc=volume_mesh.vertices[it->c];

      Vec3 tri_norm=(vb-va).cross(vc-va);

      Real l=std::sqrt(tri_norm.lengthSquared());

      assert(l>1e-4);

      tri_norm=tri_norm*Real(1)/l;

      submesh_normals[it->a*3+0]+=tri_norm.x;
      submesh_normals[it->a*3+1]+=tri_norm.y;
      submesh_normals[it->a*3+2]+=tri_norm.z;

      submesh_normals[it->b*3+0]+=tri_norm.x;
      submesh_normals[it->b*3+1]+=tri_norm.y;
      submesh_normals[it->b*3+2]+=tri_norm.z;

      submesh_normals[it->c*3+0]+=tri_norm.x;
      submesh_normals[it->c*3+1]+=tri_norm.y;
      submesh_normals[it->c*3+2]+=tri_norm.z;
   }

   for(GLuint i=0;i<num_submesh_vertices;++i)
   {
      GLfloat* n=submesh_normals+i*3;
      GLfloat lsq=sq(n[0])+sq(n[1])+sq(n[2]);
      assert(lsq>1e-7);
      lsq=1.0f/std::sqrt(lsq);
      n[0]*=lsq;
      n[1]*=lsq;
      n[2]*=lsq;
   }

   //log("num_submesh_vertices = %d\n",num_submesh_vertices);
   //log("num_submesh_indices = %d\n",num_submesh_indices);

   glGenBuffers(1,&mesh_vbo);
   glGenBuffers(1,&mesh_ebo);

   glBindBuffer(GL_ARRAY_BUFFER,mesh_vbo);
   CHECK_FOR_ERRORS;
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh_vbo);
   CHECK_FOR_ERRORS;

   glBufferData(GL_ARRAY_BUFFER,num_submesh_vertices*3*sizeof(GLfloat),submesh_vertices,GL_STATIC_DRAW);
   CHECK_FOR_ERRORS;
   glBufferData(GL_ELEMENT_ARRAY_BUFFER,num_submesh_indices*sizeof(GLushort),submesh_indices,GL_STATIC_DRAW);
   CHECK_FOR_ERRORS;

   glBindBuffer(GL_ARRAY_BUFFER,0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);

   initializeShaders();
}

void RoomScene::initializeTextures()
{
   glGenTextures(num_texs, texs);

   createWindowTexture(texs[0], GL_RGBA16F);
   createWindowTexture(texs[2], GL_RGBA16F);
   createWindowTexture(texs[4], GL_RGBA16F, 4);
   createWindowTexture(texs[5], GL_RGBA16F, 4);
   createWindowTexture(texs[6], GL_RGBA16F);
   createWindowTexture(texs[9], GL_RGBA16F, 4);

   CHECK_FOR_ERRORS;

   glBindTexture(GL_TEXTURE_2D, 0);

   noise_tex = texs[7];
   noise_tex_2d = texs[11];
   volume_tex = texs[14];

   {
      uint s = 16;
      GLubyte* pix = new GLubyte[s * s * s * 3];
      for(uint i = 0; i < s * s * s * 3; ++i)
         pix[i] = lrand();

      glBindTexture(GL_TEXTURE_3D, noise_tex);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

      glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGB8, s, s, s);
      glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, s, s, s, GL_RGB, GL_UNSIGNED_BYTE, pix);

      delete[] pix;
      CHECK_FOR_ERRORS;
      glBindTexture(GL_TEXTURE_3D, 0);
   }

   {
      uint w = 128, h = 128;
      uint pix[w*h];

      for(uint y = 0; y < h; ++y)
         for(uint x=0;x<w;++x)
         {
            pix[x+y*w]=(lrand()&255) | ((lrand()&255) <<8)|((lrand()&255))<<16;
         }

      glBindTexture(GL_TEXTURE_2D, noise_tex_2d);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pix);
      CHECK_FOR_ERRORS;
   }

   glBindTexture(GL_TEXTURE_3D, volume_tex);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
   assert(volume_data);

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, volume_size, volume_size, volume_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, volume_data);

   const int num_levels=volume_size_exp+1;
   for(int level=1;level<num_levels;++level)
   {
      const int volume_size1=volume_size>>(level-1);
      const int volume_size2=volume_size>>level;

      assert(volume_size1>0);
      assert(volume_size2>0);

      for(int z=0;z<volume_size2;++z)
         for(int y=0;y<volume_size2;++y)
            for(int x=0;x<volume_size2;++x)
            {
               int c[4]={0,0,0,0};
               for(int k=0;k<2;++k)
                  for(int j=0;j<2;++j)
                     for(int i=0;i<2;++i)
                     {
                        unsigned char* texel=volume_data+((x*2+i)+((y*2+j)+(z*2+k)*volume_size1)*volume_size1)*4;
                        c[0]+=texel[0];
                        c[1]+=texel[1];
                        c[2]+=texel[2];
                        c[3]+=texel[3];
                     }

               unsigned char* texel=volume_data+(x+(y+z*volume_size2)*volume_size2)*4;
               texel[0]=c[0]/8;
               texel[1]=c[1]/8;
               texel[2]=c[2]/8;
               texel[3]=c[3]/8;
            }

      glTexImage3D(GL_TEXTURE_3D, level, GL_RGBA8, volume_size2, volume_size2, volume_size2, 0, GL_RGBA, GL_UNSIGNED_BYTE, volume_data);
   }

   CHECK_FOR_ERRORS;
   glBindTexture(GL_TEXTURE_3D, 0);

}

void RoomScene::initializeBuffers()
{
   glGenFramebuffers(num_fbos, fbos);
   glGenRenderbuffers(num_fbos, renderbuffers);

   glBindRenderbuffer(GL_RENDERBUFFER, renderbuffers[1]);
   glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, window_width, window_height);
   CHECK_FOR_ERRORS;
   glBindRenderbuffer(GL_RENDERBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texs[0], 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[3]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texs[2], 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[4]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texs[4], 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[5]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texs[5], 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[6]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texs[6], 0);
   glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffers[1]);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[9]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texs[9], 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   CHECK_FOR_ERRORS;
}

void RoomScene::initializeShaders()
{
   text_shader.load(SHADERS_PATH "text_v.glsl", NULL, SHADERS_PATH "text_f.glsl");

   room_logo_shader.load(SHADERS_PATH "room_logo_v.glsl", NULL, SHADERS_PATH "room_logo_f.glsl");
   room_logo_shader.uniform1i("tex0",0);

   room_shader.load(SHADERS_PATH "room_v.glsl", SHADERS_PATH "room_g.glsl", SHADERS_PATH "room_f.glsl");
   room_shader.uniform1i("volume_tex", 0);
   room_shader.uniform1i("noise_tex", 1);

   room_object_shader.load(SHADERS_PATH "room_object_v.glsl", SHADERS_PATH "room_object_g.glsl", SHADERS_PATH "room_object_f.glsl");
   room_object_shader.uniform1i("noise_tex", 0);

   gaussbokeh_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "gaussblur_f.glsl");
   gaussbokeh_shader.uniform1i("tex0", 0);

   mb_accum_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "mb_accum3_f.glsl");
   mb_accum_shader.uniform1i("tex0", 0);

   composite_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "cubes_composite_f.glsl");
   composite_shader.uniform1i("tex0", 0);
   composite_shader.uniform1i("tex1", 1);
   composite_shader.uniform1i("bloom_tex", 4);
   composite_shader.uniform1i("bloom2_tex", 5);
}



void RoomScene::initialize()
{
   if(initialized)
      return;

   initialized = true;

   glGenBuffers(1,&text_vbo);
   glGenBuffers(1,&text_ebo);

   glBindBuffer(GL_ARRAY_BUFFER,text_vbo);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,text_ebo);

   glBufferData(GL_ARRAY_BUFFER,text_mesh_num_vertices*sizeof(GLfloat)*3, text_mesh_vertices, GL_STATIC_DRAW);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER,text_mesh_num_indices*sizeof(GLuint), text_mesh_indices, GL_STATIC_DRAW);
   CHECK_FOR_ERRORS;

   glBindBuffer(GL_ARRAY_BUFFER,0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);

   initializeTextures();
   initializeBuffers();

   icosahedron_mesh.generateIcosahedron();
}


void RoomScene::update()
{
}

void RoomScene::drawRoom(const Mat4& modelview)
{
   double last_flash_time[3]={0.0,0.0,0.0};

   for(int i=0;i<max_synth_hits;++i)
   {
      last_flash_time[i%3]=synth_hit_times[i];

      if(synth_hit_times[i]>time)
         break;
   }

   const float brightness=mix(1.0f,0.1f,cubic(clamp((time-logo_appear_time)*0.8f,0.0f,1.0f)))*mix(1.0f,0.0f,cubic(clamp((time-33.0f)*0.25f,0.0f,1.0f)));

   const Vec3 material_colours_set[6][3]= {
         {
      Vec3(1.0,1.0,0.8)*2.9*brightness*cubic(1.0-clamp((time-last_flash_time[0])*3,0.0,1.0)),
      Vec3(1.0,1.0,0.8)*2.9*brightness*cubic(1.0-clamp((time-last_flash_time[1])*3,0.0,1.0)),
      Vec3(1.0,1.0,0.8)*2.9*brightness*cubic(1.0-clamp((time-last_flash_time[2])*3,0.0,1.0))
         },
         {
      Vec3(1.0,1.0,0.8)*2.9*brightness*cubic(1.0-clamp((time-last_flash_time[0])*3,0.0,1.0)),
      Vec3(1.0,1.0,0.8)*2.9*brightness*cubic(1.0-clamp((time-last_flash_time[1])*3,0.0,1.0)),
      Vec3(1.0,1.0,0.8)*2.9*brightness*cubic(1.0-clamp((time-last_flash_time[2])*3,0.0,1.0))
         },
         {
      Vec3(0.5,0.6,1.0)*2.9*brightness*cubic(1.0-clamp((time-last_flash_time[0])*3,0.0,1.0)),
      Vec3(1.0,1.0,0.8)*2.9*brightness*cubic(1.0-clamp((time-last_flash_time[1])*3,0.0,1.0)),
      Vec3(1.0,0.8,0.5)*2.9*brightness*cubic(1.0-clamp((time-last_flash_time[2])*3,0.0,1.0))
         },
         {
      Vec3(1.0,0.8,0.5)*2.9*brightness*cubic(1.0-clamp((time-last_flash_time[0])*3,0.0,1.0)),
      Vec3(0.5,0.6,1.0)*2.9*brightness*cubic(1.0-clamp((time-last_flash_time[1])*3,0.0,1.0)),
      Vec3(1.0,1.0,0.8)*2.9*brightness*cubic(1.0-clamp((time-last_flash_time[2])*3,0.0,1.0))
         },
         {
      Vec3(1.0,1.0,0.8)*2.9*brightness*cubic(1.0-clamp((time-last_flash_time[0])*3,0.0,1.0)),
      Vec3(1.0,1.0,0.8)*2.9*brightness*cubic(1.0-clamp((time-last_flash_time[1])*3,0.0,1.0)),
      Vec3(0.3,0.4,0.8)*2.9*brightness*cubic(1.0-clamp((time-last_flash_time[2])*3,0.0,1.0))
         },
         {
      Vec3(1.0,1.0,0.8)*2.9*brightness*cubic(1.0-clamp((time-last_flash_time[0])*3,0.0,1.0)),
      Vec3(1.0,1.0,0.8)*2.9*brightness*cubic(1.0-clamp((time-last_flash_time[1])*3,0.0,1.0)),
      Vec3(1.0,1.0,0.8)*2.9*brightness*cubic(1.0-clamp((time-last_flash_time[2])*3,0.0,1.0))
         }
      };

   const Vec3* material_colours=material_colours_set[current_material_colour_set];

   const Vec3 object_origin=Vec3(0.0,0.0,-1.4);//Vec3(std::cos(time)*0.25,0.0,-1.3+std::sin(time*0.5)*0.2);

   room_shader.bind();

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_3D, noise_tex);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_3D, volume_tex);

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(2);

   Mesh* mesh=&icosahedron_mesh;

   mesh->bind();

   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
   glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(mesh->getVertexCount() * sizeof(GLfloat) * 3));

   room_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
   room_shader.uniform3f("box_min",volume_min.x,volume_min.y,volume_min.z);
   room_shader.uniform3f("box_max",volume_max.x,volume_max.y,volume_max.z);

   {
      Mat4 m = (modelview * object_transform).inverse();
      room_shader.uniformMatrix4fv("inv_object", 1, GL_FALSE, m.e);
   }

   room_shader.uniform3f("material_colours[0]",material_colours[0].x,material_colours[0].y,material_colours[0].z);
   room_shader.uniform3f("material_colours[1]",material_colours[1].x,material_colours[1].y,material_colours[1].z);
   room_shader.uniform3f("material_colours[2]",material_colours[2].x,material_colours[2].y,material_colours[2].z);

   glFrontFace(GL_CW);
   glDrawRangeElements(GL_TRIANGLES, 0, mesh->getVertexCount() - 1, mesh->getTriangleCount() * 3, GL_UNSIGNED_INT, 0);
   glFrontFace(GL_CCW);

   CHECK_FOR_ERRORS;


   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(2);



   room_object_shader.bind();
   CHECK_FOR_ERRORS;

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_3D, noise_tex);

   glBindBuffer(GL_ARRAY_BUFFER,0);
   CHECK_FOR_ERRORS;
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
   CHECK_FOR_ERRORS;

   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, submesh_vertices);
   CHECK_FOR_ERRORS;

   glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, submesh_normals);
   CHECK_FOR_ERRORS;

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(3);

   room_object_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (modelview * object_transform * Mat4::scale(Vec3(0.333))).e);

   CHECK_FOR_ERRORS;

   for(GLuint i = 0; i < num_materials; ++i)
   {
      Vec3 c = submesh_material_colours[i];

      c = material_colours[0] * c.x + material_colours[1] * c.y + material_colours[2] * c.z;

      room_object_shader.uniform3f("material_colour",c.x,c.y,c.z);
      glDrawRangeElements(GL_TRIANGLES, 0, num_submesh_vertices - 1, submesh_offsets[i+1]-submesh_offsets[i], GL_UNSIGNED_SHORT, submesh_indices+submesh_offsets[i]);
   }

   CHECK_FOR_ERRORS;


   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(3);
   glBindBuffer(GL_ARRAY_BUFFER,0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
}

void RoomScene::drawLogo(float ltime)
{
   ltime*=4.0f;

   static const float s=0.35f;
   static const float logo_aspect=1420.0f/300.0f;
   static const float target_x0=-s*logo_aspect*float(window_height)/float(window_width);
   static const float target_x1=+s*logo_aspect*float(window_height)/float(window_width);
   static const float target_y0=-1*s;
   static const float target_y1=+1*s;
   static const int edge_subdivisions=8;

   num_voronoi_vertices=0;
   num_voronoi_indices=0;

   const float zz=mix(0.9f,1.0f,std::min(1.0f,ltime*0.5f));

   for(int i=0;i<num_voronoi_cells;++i)
   {
      const VoronoiCell& vc=voronoi_cells[i];

      const GLushort first_index=num_voronoi_vertices;

      float ca=std::atan2(vc.center.y,vc.center.x);

      //const Vec2 center=mix(Vec2(vc.center.x,2.0f),vc.center,std::pow(clamp((ltime-vc.time0)/(vc.time1-vc.time0),0.0f,1.0f),0.1f));
      const float t=std::max(0.0f,ltime-vc.time0);
      //const Vec2 center=mix(Vec2(vc.center.x*1.2f,4.0f),vc.center,mix(t/(t+0.15f),1.0f,std::min(1.0f,ltime*0.2f)));
      const Vec2 center=mix(Vec2(std::cos(ca)*4.0f,std::sin(ca)*4.0f),vc.center,mix(t/(t+0.15f),1.0f,std::min(1.0f,ltime*0.05f)));

      assert(num_voronoi_vertices<max_voronoi_vertices);
      voronoi_vertices[num_voronoi_vertices*2+0]=mix(target_x0,target_x1,center.x);
      voronoi_vertices[num_voronoi_vertices*2+1]=mix(target_y0,target_y1,center.y);
      voronoi_coords[num_voronoi_vertices*2+0]=vc.center.x;
      voronoi_coords[num_voronoi_vertices*2+1]=vc.center.y;
      ++num_voronoi_vertices;

      for(std::vector<Edge>::const_iterator e=vc.edges.begin();e!=vc.edges.end();++e)
      {
         for(int j=0;j<edge_subdivisions;++j)
         {
            const float t0=float(j+0)/float(edge_subdivisions);
            const float t1=float(j+1)/float(edge_subdivisions);

            const Vec2 p0=mix(e->first,e->second,t0);
            const Vec2 p1=mix(e->first,e->second,t1);

            assert(num_voronoi_indices<max_voronoi_indices);
            voronoi_indices[num_voronoi_indices++]=num_voronoi_vertices;
            assert(num_voronoi_indices<max_voronoi_indices);
            voronoi_indices[num_voronoi_indices++]=num_voronoi_vertices+1;
            assert(num_voronoi_indices<max_voronoi_indices);
            voronoi_indices[num_voronoi_indices++]=first_index;

            float x0=mix(target_x0,target_x1,(p0.x-vc.center.x)*zz+center.x);
            float y0=mix(target_y0,target_y1,(p0.y-vc.center.y)*zz+center.y);

            float x1=mix(target_x0,target_x1,(p1.x-vc.center.x)*zz+center.x);
            float y1=mix(target_y0,target_y1,(p1.y-vc.center.y)*zz+center.y);

            const float exp=mix(2.0f,1.0f,std::pow(std::min(1.0f,ltime*0.4f),0.1f));
            const float scale=mix(3.0f,1.0f,std::pow(std::min(1.0f,ltime*0.2f),0.1f));

            {
               float l=std::pow(std::sqrt(x0*x0+y0*y0),exp)*scale;
               float a=std::atan2(y0,x0);
               x0=std::cos(a)*l;
               y0=std::sin(a)*l;
            }

            {
               float l=std::pow(std::sqrt(x1*x1+y1*y1),exp)*scale;
               float a=std::atan2(y1,x1);
               x1=std::cos(a)*l;
               y1=std::sin(a)*l;
            }

            assert(num_voronoi_vertices<max_voronoi_vertices);
            voronoi_vertices[num_voronoi_vertices*2+0]=x0;
            voronoi_vertices[num_voronoi_vertices*2+1]=y0;
            voronoi_coords[num_voronoi_vertices*2+0]=p0.x;
            voronoi_coords[num_voronoi_vertices*2+1]=p0.y;
            ++num_voronoi_vertices;

            assert(num_voronoi_vertices<max_voronoi_vertices);
            voronoi_vertices[num_voronoi_vertices*2+0]=x1;
            voronoi_vertices[num_voronoi_vertices*2+1]=y1;
            voronoi_coords[num_voronoi_vertices*2+0]=p1.x;
            voronoi_coords[num_voronoi_vertices*2+1]=p1.y;
            ++num_voronoi_vertices;
         }
      }
   }

   Mat4 modelview = Mat4::identity(), projection = Mat4::identity();

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D,logo_tex);

   room_logo_shader.bind();
   room_logo_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
   room_logo_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);
   room_logo_shader.uniform1f("time",ltime);

   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

   glBindBuffer(GL_ARRAY_BUFFER,0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, voronoi_vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, voronoi_coords);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glDrawRangeElements(GL_TRIANGLES, 0, num_voronoi_vertices - 1, num_voronoi_indices, GL_UNSIGNED_SHORT, voronoi_indices);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D,0);
}

void RoomScene::drawView(float ltime)
{
   lltime = ltime;

   const float aspect_ratio = float(window_height) / float(window_width);

   const float jitter_x = frand() / window_width * 1 / 256.0f;
   const float jitter_y = frand() / window_height * aspect_ratio * 1 / 256.0f;

   Mat4 modelview = Mat4::identity(), projection = Mat4::identity();

   const float fov = M_PI * 0.6f;

   const float znear = 1.0f / 256.0f, zfar = 1024.0f;

   projection = Mat4::frustum(-znear + jitter_x, +znear + jitter_x, -aspect_ratio*znear + jitter_y, +aspect_ratio*znear + jitter_y, znear, zfar) * projection;

   modelview = modelview * Mat4::rotation(cos(ltime*3) * 0.3 / ltime * 0.03, Vec3(0.0f, 0.0f, 1.0f));
   modelview = modelview * Mat4::rotation(cos(ltime * 15.0) * 0.01 / 128, Vec3(0.0f, 1.0f, 0.0f));
   modelview = modelview * Mat4::rotation(sin(ltime * 17.0) * 0.01 / 128  + cos(ltime*7) * 0.3 / ltime * 0.06, Vec3(1.0f, 0.0f, 0.0f));

   modelview = modelview * Mat4::translation(Vec3(0, 0, -1));

   Mat4 modelview2 = modelview;

   // 12:08 <@SunnyREV> 3:04:658, 3:16:294, 3:27:931, 3:39:566
   //static const float switch_times[2] = { (3 * 60 + 27) + 0.931, (3 * 60 + 39) + 0.566 };
   static const float switch_times[5] = { (3 * 60 + 16) + 0.249 + 11.635 * 0.5, (3 * 60 + 16) + 0.249 + 11.635 * 1, (3 * 60 + 16) + 0.249 + 11.635 * 1.5,
            (3 * 60 + 16) + 0.249 + 11.635 * 2, (3 * 60 + 16) + 0.249 + 11.635 * 2.5 };

   current_material_colour_set=0;
   float nearest_switch_dist=100.0f;
   for(int i=0;i<5;++i)
   {
      if(ltime > (switch_times[i]-scene_start_time))
      {
         current_material_colour_set=i+1;
      }

      float d=std::abs((switch_times[i]-scene_start_time)-ltime);
      if(d<nearest_switch_dist)
      {
         nearest_switch_dist=d;
      }
   }

   {
      float b=std::pow(clamp(nearest_switch_dist*2.0f,0.0f,1.0f),0.5f);
      mb_accum_shader.uniform1f("brightness", b);
   }

   mb_accum_shader.uniform1f("kal", 0);

   if(ltime > (switch_times[4]-scene_start_time))
   {
      const float tt=ltime-(switch_times[4]-scene_start_time);
      modelview = modelview * Mat4::translation(Vec3(0, 0, 0.2f));
      modelview = Mat4::rotation(M_PI*0.5f,Vec3(0,0,1)) * Mat4::rotation(mix(float(M_PI*0.5f),0.0f,tt/(tt+0.5f)),Vec3(1,0,0)) * modelview;
   }
   else if(ltime > (switch_times[3]-scene_start_time))
   {
      mb_accum_shader.uniform1f("kal", 3);
      const float tt=ltime-(switch_times[3]-scene_start_time);
      modelview = modelview * Mat4::rotation(tt*0.5*0.5,Vec3(1,0,0));
      modelview = modelview * Mat4::rotation(tt*0.4*0.5,Vec3(0,1,0));
   }
   else if(ltime > (switch_times[2]-scene_start_time))
   {
      mb_accum_shader.uniform1f("kal", 2);
      const float tt=ltime-(switch_times[2]-scene_start_time);
      modelview = modelview * Mat4::translation(Vec3(0, 0, 0.75-tt*0.05));
      modelview = modelview * Mat4::rotation(tt*0.05,Vec3(0,0,1));
   }
   else if(ltime > (switch_times[1]-scene_start_time))
   {
      mb_accum_shader.uniform1f("kal", 1);
      const float tt=ltime-(switch_times[1]-scene_start_time);
      modelview = modelview * Mat4::translation(Vec3(0, 0, 0.75-tt*0.05));
      modelview = modelview * Mat4::rotation(tt*0.05,Vec3(0,0,1));
   }
   else if(ltime > (switch_times[0]-scene_start_time))
   {
      const float tt=ltime-(switch_times[0]-scene_start_time);
      modelview = modelview * Mat4::translation(Vec3(0, 0, 0.75-tt*0.05));
      modelview = modelview * Mat4::rotation(tt*0.05,Vec3(0,0,1));
   }
   else
   {
      const float tt=ltime;
      modelview = modelview * Mat4::rotation(tt*0.5*0.5,Vec3(1,0,0));
      modelview = modelview * Mat4::rotation(tt*0.4*0.5,Vec3(0,1,0));
   }

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }

   glViewport(0, 0, window_width, window_height);

   CHECK_FOR_ERRORS;

   glDepthMask(GL_TRUE);
   glEnable(GL_CULL_FACE);
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LEQUAL);

   glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

   CHECK_FOR_ERRORS;

   room_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);
   room_object_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);
   text_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);

   CHECK_FOR_ERRORS;

   drawRoom(modelview);
   if(ltime > logo_appear_time)
      drawLogo(ltime - logo_appear_time);

   glBindBuffer(GL_ARRAY_BUFFER,text_vbo);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,text_ebo);
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

   static const float text0_appear_time=logo_appear_time-1.5f;
   static const float text1_appear_time=logo_appear_time+4.0f;

   if(ltime > text0_appear_time)
   {
      float ti=ltime-text0_appear_time;
      Vec3 p0=Vec3(1.0f,-0.5f,1.5f);
      Vec3 p1=Vec3(-.6f,0.2f,0.0f);
      Mat4 animrot=Mat4::rotation(mix(1.0f,0.0f,cubic(clamp(ti*0.33f,0.0f,1.0f))),Vec3(0,1,0));
      static const Mat4 scale_matrix=Mat4::scale(Vec3(0.05f));
      static const Mat4 rot_correction=Mat4::rotation(M_PI*0.5f,Vec3(1,0,0));
      text_shader.bind();
      text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (Mat4::translation(Vec3(mix(p0,p1,cubic(clamp(ti*0.25f,0.0f,1.0f))))) * animrot * modelview2 * scale_matrix * rot_correction).e);
      drawText("titan_invites");
   }

   if(ltime > text1_appear_time)
   {
      static const Mat4 scale_matrix=Mat4::scale(Vec3(0.06f));
      static const Mat4 rot_correction=Mat4::rotation(M_PI*0.5f,Vec3(1,0,0));
      text_shader.bind();
      text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (Mat4::translation(Vec3(0,-0.33,0)) * modelview2 * scale_matrix * rot_correction).e);
      drawText("nordlicht.",cubic(clamp(ltime-text1_appear_time,0.0f,1.0f)));
   }

   glDisableVertexAttribArray(0);
   glBindBuffer(GL_ARRAY_BUFFER,0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);

   glUseProgram(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);

   glDepthMask(GL_TRUE);

   CHECK_FOR_ERRORS;
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void RoomScene::gaussianBlur(Real tint_r, Real tint_g, Real tint_b, Real radx, Real rady, GLuint fbo0, GLuint fbo1, GLuint src_tex, GLuint tex0)
{
   static const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f };
   static const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
   static const GLubyte indices[] = { 3, 0, 2, 1 };

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);

   glViewport(0, 0, window_width / 4, window_height / 4);

   // gaussian blur pass 1

   {
      gaussbokeh_shader.uniform4f("tint", tint_r, tint_g, tint_b, 1.0f);
   }

   const float gauss_radx = radx;
   const float gauss_rady = rady;

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo0);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }


   CHECK_FOR_ERRORS;

   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);

   gaussbokeh_shader.bind();
   gaussbokeh_shader.uniform2f("direction", gauss_radx * float(window_height) / float(window_width), 0.0f);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, src_tex);

   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);



   // gaussian blur pass 2

   gaussbokeh_shader.uniform4f("tint", 1, 1, 1, 1);


   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo1);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }


   CHECK_FOR_ERRORS;

   gaussbokeh_shader.uniform2f("direction", 0.0f, gauss_rady);

   glBindTexture(GL_TEXTURE_2D, tex0);

   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   CHECK_FOR_ERRORS;
}


void RoomScene::drawText(const std::string& str,float br)
{
   const float l=0.75f*br;
   text_shader.uniform3f("colour",l,l,l);

   for(std::vector<model::Mesh::SubObject>::const_iterator it=text_mesh.subobjects.begin();it!=text_mesh.subobjects.end();++it)
   {
      if(it->name.find(str)==0)
      {
         glDrawElements(GL_TRIANGLES,it->triangle_count*3,GL_UNSIGNED_INT,(GLvoid*)(it->first_triangle*3*sizeof(GLuint)));
         return;
      }
   }
   assert(false);
}

void RoomScene::render()
{
   object_transform = Mat4::translation(Vec3(0,0,-0.25));

   static const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f };
   static const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
   static const GLubyte indices[] = { 3, 0, 2, 1 };

   const float aspect_ratio = float(window_height) / float(window_width);

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

   {
      // ----- motion blur ------

      glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);
      glClear(GL_COLOR_BUFFER_BIT);

      num_subframes = 2;
      subframe_scale = 1.0/35.0;

      for(int subframe = 0;subframe<num_subframes;++subframe)
      {
         // draw the view

         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[6]);

         glDisable(GL_BLEND);

         const float ltime=time + subframe_scale * float(subframe) / float(num_subframes);
         drawView(ltime);


         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);

         mb_accum_shader.bind();
         mb_accum_shader.uniform1f("time", float(subframe) / float(num_subframes));
         mb_accum_shader.uniform1f("ltime", ltime);
         mb_accum_shader.uniform1f("scale", 1.0f / float(num_subframes));

         glDepthMask(GL_FALSE);
         glDisable(GL_DEPTH_TEST);
         glEnable(GL_BLEND);
         glBlendFunc(GL_ONE, GL_ONE);

         glActiveTexture(GL_TEXTURE0);
         glBindTexture(GL_TEXTURE_2D, texs[6]);
         glEnableVertexAttribArray(0);
         glEnableVertexAttribArray(1);
         glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
         glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
         glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
         glDisableVertexAttribArray(0);
         glDisableVertexAttribArray(1);
      }

      CHECK_FOR_ERRORS;
   }

   // ----- bloom ------

   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_BLEND);

   downsample8x(texs[0], fbos[3], texs[2], fbos[4], window_width, window_height);

   gaussianBlur(1, 1, 1, 0.5, 0.5, fbos[5], fbos[9], texs[4], texs[5]);
   gaussianBlur(1, 1, 1, 0.1, 0.1, fbos[5], fbos[4], texs[4], texs[5]);

   CHECK_FOR_ERRORS;


   // ----- composite ------


   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
   glViewport(0, 0, window_width, window_height);


   composite_shader.bind();
   composite_shader.uniform1f("time", time);
   composite_shader.uniform1i("frame_num", frame_num);
   {
      float s=std::pow(1.0f-clamp(time,0.0f,1.0f),2.0f);
      composite_shader.uniform3f("addcolour", s,s,s);
   }
   composite_shader.uniform4f("tint", 1, 1, 1, 1);

   glActiveTexture(GL_TEXTURE5);
   glBindTexture(GL_TEXTURE_2D, texs[9]);
   glActiveTexture(GL_TEXTURE4);
   glBindTexture(GL_TEXTURE_2D, texs[4]);
   glActiveTexture(GL_TEXTURE3);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, noise_tex_2d);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, texs[0]);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   CHECK_FOR_ERRORS;
}


void RoomScene::free()
{
   glDeleteTextures(1,&logo_tex);
   delete[] volume_data;
   volume_data=0;
   glDeleteTextures(num_texs, texs);
   glDeleteFramebuffers(num_fbos, fbos);
}

