
// midnight platonic

#include "Engine.hpp"
#include "Bezier.hpp"
#include "Ran.hpp"
#include "Particles.hpp"
#include "Tower.hpp"

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

class PlatonicScene: public Scene
{
   typedef std::pair<Vec2,Vec2> Edge;

   struct VoronoiCell
   {
      std::vector<Edge> edges;
      Vec2 center;
      float time0,time1;
   };

   tower::Tower twr0;

   Shader forrest_shader, bokeh_shader, bokeh2_shader;
   Shader gaussbokeh_shader, composite_shader;
   Shader mb_accum_shader;
   Shader bgfg_shader;
   Shader text_shader;
   Shader room_logo_shader;

   bool drawing_view_for_background;
   int num_subframes;
   float subframe_scale;

   float lltime;

   Mesh icosahedron_mesh;
   Mesh tetrahedron_mesh;
   Mesh octahedron_mesh;


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

   model::Mesh text_mesh;

   static const int num_fbos = 16, num_texs = 16;

   GLuint fbos[num_fbos], texs[num_texs], renderbuffers[num_fbos];

   GLuint bokeh_temp_fbo;
   GLuint bokeh_temp_tex_1, bokeh_temp_tex_2;

   GLuint noise_tex_2d;

   GLuint logo_tex;

   GLuint text_mesh_num_indices;
   GLuint text_mesh_num_vertices;
   GLfloat* text_mesh_vertices;
   GLuint* text_mesh_indices;

   GLuint text_vbo, text_ebo;

   float original_time;

   void initVoronoi();

   void initializeTextures();
   void initializeShaders();
   void initializeBuffers();

   void drawLogo(float ltime);
   void drawText(const std::string& str,bool gold=false,bool pink=false,bool bright=false);
   void drawTower(tower::Tower& twr, const Mat4& modelview, Real camheight);
   void drawView(float ltime);
   void gaussianBlur(Real tint_r, Real tint_g, Real tint_b, Real radx, Real rady, GLuint fbo0, GLuint fbo1, GLuint src_tex, GLuint tex0);


   public:
      PlatonicScene(): icosahedron_mesh(32, 32), tetrahedron_mesh(16, 16), octahedron_mesh(16, 16)
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

      ~PlatonicScene()
      {
         free();
      }

      void slowInitialize();
      void initialize();
      void render();
      void update();
      void free();
};

Scene* platonicScene = new PlatonicScene();


void PlatonicScene::initVoronoi()
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

void PlatonicScene::slowInitialize()
{
   initVoronoi();

   logo_tex = loadTexture(IMAGES_PATH "logo.png");

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

   initializeShaders();
}

void PlatonicScene::drawText(const std::string& str,bool gold,bool pink,bool bright)
{
   if(gold)
      text_shader.uniform3f("colour",1.0,0.9,0.7);
   else if(pink)
      text_shader.uniform3f("colour",1.0,0.65,0.85);
   else if(bright)
      text_shader.uniform3f("colour",2.0,2.0,2.0);
   else
      text_shader.uniform3f("colour",1.0,1.0,1.0);

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

void PlatonicScene::initializeTextures()
{
   glGenTextures(num_texs, texs);

   createWindowTexture(texs[0], GL_RGBA16F);
   createWindowTexture(texs[2], GL_RGBA16F);
   createWindowTexture(texs[4], GL_RGBA16F, 4);
   createWindowTexture(texs[5], GL_RGBA16F, 4);
   createWindowTexture(texs[6], GL_RGBA16F);
   createWindowTexture(texs[9], GL_RGBA16F, 4);
   createWindowTexture(texs[12], GL_RGBA16F, 2);
   createWindowTexture(texs[13], GL_RGBA16F, 2);

   CHECK_FOR_ERRORS;

   glBindTexture(GL_TEXTURE_2D, 0);

   noise_tex_2d = texs[11];
   bokeh_temp_tex_1 = texs[12];
   bokeh_temp_tex_2 = texs[13];

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

}

void PlatonicScene::initializeBuffers()
{
   glGenFramebuffers(num_fbos, fbos);
   glGenRenderbuffers(num_fbos, renderbuffers);

   bokeh_temp_fbo = fbos[12];

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

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, bokeh_temp_fbo);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bokeh_temp_tex_1, 0);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, bokeh_temp_tex_2, 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   CHECK_FOR_ERRORS;
}

void PlatonicScene::initializeShaders()
{
   room_logo_shader.load(SHADERS_PATH "room_logo_v.glsl", NULL, SHADERS_PATH "room_logo_f.glsl");
   room_logo_shader.uniform1i("tex0",0);

   text_shader.load(SHADERS_PATH "text_v.glsl", NULL, SHADERS_PATH "text_f.glsl");

   forrest_shader.load(SHADERS_PATH "forrest_v.glsl", NULL, SHADERS_PATH "forrest_f.glsl");


   bokeh_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "bokeh_f.glsl");
   bokeh_shader.uniform1i("tex0", 0);
   bokeh_shader.uniform1f("aspect_ratio", float(window_height) / float(window_width));

   bokeh2_shader.load(SHADERS_PATH "generic_v.glsl", NULL,SHADERS_PATH "bokeh2_f.glsl");
   bokeh2_shader.uniform1i("tex1", 0);
   bokeh2_shader.uniform1i("tex2", 1);
   bokeh2_shader.uniform1f("aspect_ratio", float(window_height) / float(window_width));

   gaussbokeh_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "gaussblur_f.glsl");
   gaussbokeh_shader.uniform1i("tex0", 0);

   mb_accum_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "mb_accum_f.glsl");
   mb_accum_shader.uniform1i("tex0", 0);


   composite_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "cubes_composite_f.glsl");
   composite_shader.uniform1i("tex0", 0);
   composite_shader.uniform1i("tex1", 1);
   composite_shader.uniform1i("bloom_tex", 4);
   composite_shader.uniform1i("bloom2_tex", 5);

   bgfg_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "bgfg_f.glsl");
   bgfg_shader.uniform1i("tex0", 0);
   bgfg_shader.uniform1i("tex1", 1);
}



void PlatonicScene::initialize()
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


/*
distance
angle
height
time_scale
time_offset
ptn_thickness
ptn_brightness
ptn_scroll
ptn_frequency
shimmer
l0_scroll
l0_brightness
l1_scroll
l1_brightness
l2_scroll
l2_brightness
rot_x
rot_y
rot_z
sca_x
sca_y
sca_z
rot_spd_x
rot_spd_y
rot_spd_z
geom
background
ambient
section_rot_spd
*/


   {
      tower::Section section;
      section.height0 = 0;
      section.height1 = -8;
      section.radius = 5;
      section.num_instances = 15;

      {
         tower::PieceType piece_type;
         piece_type.weight = 1;
         piece_type.parameters["distance"] = tower::ParameterRange(0, 0.1);
         piece_type.parameters["angle"] = tower::ParameterRange(0, 2*M_PI);
         piece_type.parameters["height"] = tower::ParameterRange(0,1);
         piece_type.parameters["sca_x"] = tower::ParameterRange(1);
         piece_type.parameters["sca_y"] = tower::ParameterRange(1);
         piece_type.parameters["sca_z"] = tower::ParameterRange(1);
         piece_type.parameters["rot_x"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_y"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["l1_brightness"] = tower::ParameterRange(0.6);
         piece_type.parameters["background"] = tower::ParameterRange(0);
         piece_type.parameters["section_rot_spd"] = tower::ParameterRange(1);
         section.types.push_back(piece_type);
      }

      {
         tower::PieceType piece_type;
         piece_type.weight = 2;
         piece_type.parameters["distance"] = tower::ParameterRange(0.0, 0.2);
         piece_type.parameters["angle"] = tower::ParameterRange(0, 2*M_PI);
         piece_type.parameters["height"] = tower::ParameterRange(0,1);
         piece_type.parameters["l1_brightness"] = tower::ParameterRange(0.05);
         piece_type.parameters["l2_brightness"] = tower::ParameterRange(2.0);
         piece_type.parameters["l2_scroll"] = tower::ParameterRange(0.1,0.2);
         piece_type.parameters["l0_brightness"] = tower::ParameterRange(1);
         piece_type.parameters["l0_scroll"] = tower::ParameterRange(0.1,0.2);
         piece_type.parameters["sca_x"] = tower::ParameterRange(0.4,0.6);
         piece_type.parameters["sca_y"] = tower::ParameterRange(0.4,0.6);
         piece_type.parameters["sca_z"] = tower::ParameterRange(0.4,0.6);
         piece_type.parameters["rot_x"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_y"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["geom"] = tower::ParameterRange(0);
         piece_type.parameters["time_scale"] = tower::ParameterRange(1);
         piece_type.parameters["background"] = tower::ParameterRange(0);
         piece_type.parameters["section_rot_spd"] = tower::ParameterRange(1);
         section.types.push_back(piece_type);
      }

      twr0.sections.push_back(section);
   }

   {
      tower::Section section;
      section.height0 = -8;
      section.height1 = -32;
      section.radius = 10;
      section.num_instances = 60;

      {
         tower::PieceType piece_type;
         piece_type.weight = 1;
         piece_type.parameters["distance"] = tower::ParameterRange(0, 0.1);
         piece_type.parameters["angle"] = tower::ParameterRange(0, 2*M_PI);
         piece_type.parameters["height"] = tower::ParameterRange(0,1);
         piece_type.parameters["sca_x"] = tower::ParameterRange(1);
         piece_type.parameters["sca_y"] = tower::ParameterRange(1);
         piece_type.parameters["sca_z"] = tower::ParameterRange(1);
         piece_type.parameters["rot_x"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_y"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["l1_brightness"] = tower::ParameterRange(0.6);
         piece_type.parameters["geom"] = tower::ParameterRange(0);
         piece_type.parameters["background"] = tower::ParameterRange(0);
         piece_type.parameters["section_rot_spd"] = tower::ParameterRange(1);
         section.types.push_back(piece_type);
      }

      {
         tower::PieceType piece_type;
         piece_type.weight = 4;
         piece_type.parameters["distance"] = tower::ParameterRange(0.0, 0.2);
         piece_type.parameters["angle"] = tower::ParameterRange(0, 2*M_PI);
         piece_type.parameters["height"] = tower::ParameterRange(0,1);
         piece_type.parameters["l1_brightness"] = tower::ParameterRange(0.05);
         piece_type.parameters["l2_brightness"] = tower::ParameterRange(2.0);
         piece_type.parameters["l2_scroll"] = tower::ParameterRange(0.1,0.2);
         piece_type.parameters["l0_brightness"] = tower::ParameterRange(1);
         piece_type.parameters["l0_scroll"] = tower::ParameterRange(0.1,0.2);
         piece_type.parameters["sca_x"] = tower::ParameterRange(0.4,0.6);
         piece_type.parameters["sca_y"] = tower::ParameterRange(0.4,0.6);
         piece_type.parameters["sca_z"] = tower::ParameterRange(0.4,0.6);
         piece_type.parameters["rot_x"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_y"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["geom"] = tower::ParameterRange(0);
         piece_type.parameters["time_scale"] = tower::ParameterRange(1);
         piece_type.parameters["background"] = tower::ParameterRange(0);
         piece_type.parameters["section_rot_spd"] = tower::ParameterRange(1);
         section.types.push_back(piece_type);
      }

      twr0.sections.push_back(section);
   }


   {
      tower::Section section;
      section.height0 = -32;
      section.height1 = -64;
      section.radius = 15;
      section.num_instances = 60;

      {
         tower::PieceType piece_type;
         piece_type.weight = 1;
         piece_type.parameters["distance"] = tower::ParameterRange(0, 0.1);
         piece_type.parameters["angle"] = tower::ParameterRange(0, 2*M_PI);
         piece_type.parameters["height"] = tower::ParameterRange(0,1);
         piece_type.parameters["sca_x"] = tower::ParameterRange(1);
         piece_type.parameters["sca_y"] = tower::ParameterRange(1);
         piece_type.parameters["sca_z"] = tower::ParameterRange(5);
         piece_type.parameters["rot_x"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_y"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["l1_brightness"] = tower::ParameterRange(0.6);
         piece_type.parameters["geom"] = tower::ParameterRange(0);
         piece_type.parameters["background"] = tower::ParameterRange(0);
         section.types.push_back(piece_type);
      }

      {
         tower::PieceType piece_type;
         piece_type.weight = 4;
         piece_type.parameters["distance"] = tower::ParameterRange(0.0, 0.2);
         piece_type.parameters["angle"] = tower::ParameterRange(0, 2*M_PI);
         piece_type.parameters["height"] = tower::ParameterRange(0,1);
         piece_type.parameters["l1_brightness"] = tower::ParameterRange(0.05);
         piece_type.parameters["l2_brightness"] = tower::ParameterRange(2.0);
         piece_type.parameters["l2_scroll"] = tower::ParameterRange(0.1,0.2);
         piece_type.parameters["l0_brightness"] = tower::ParameterRange(1);
         piece_type.parameters["l0_scroll"] = tower::ParameterRange(0.1,0.2);
         piece_type.parameters["sca_x"] = tower::ParameterRange(0.4,0.6);
         piece_type.parameters["sca_y"] = tower::ParameterRange(0.4,1.6);
         piece_type.parameters["sca_z"] = tower::ParameterRange(0.4,0.6);
         piece_type.parameters["rot_x"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_y"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["geom"] = tower::ParameterRange(2);
         piece_type.parameters["time_scale"] = tower::ParameterRange(10);
         piece_type.parameters["background"] = tower::ParameterRange(0);
         section.types.push_back(piece_type);
      }

      twr0.sections.push_back(section);
   }

   {
      tower::Section section;
      section.height0 = -55;
      section.height1 = -96;
      section.radius = 30;
      section.num_instances = 60;

      {
         tower::PieceType piece_type;
         piece_type.weight = 1;
         piece_type.parameters["distance"] = tower::ParameterRange(0, 0.1);
         piece_type.parameters["angle"] = tower::ParameterRange(M_PI*0.5, M_PI);
         piece_type.parameters["height"] = tower::ParameterRange(0,1);
         piece_type.parameters["sca_x"] = tower::ParameterRange(0.5,0.6);
         piece_type.parameters["sca_y"] = tower::ParameterRange(0.5,0.6);
         piece_type.parameters["sca_z"] = tower::ParameterRange(0.5,0.6);
         piece_type.parameters["rot_x"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_y"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_spd_x"] = tower::ParameterRange(3.0,4.1);
         piece_type.parameters["rot_spd_y"] = tower::ParameterRange(3.0,4.1);
         piece_type.parameters["l1_brightness"] = tower::ParameterRange(0.05);
         piece_type.parameters["l2_brightness"] = tower::ParameterRange(2.0);
         piece_type.parameters["l2_scroll"] = tower::ParameterRange(0.1,0.2);
         piece_type.parameters["l0_brightness"] = tower::ParameterRange(1);
         piece_type.parameters["l0_scroll"] = tower::ParameterRange(0.1,0.2);
         piece_type.parameters["time_scale"] = tower::ParameterRange(10);
         piece_type.parameters["geom"] = tower::ParameterRange(0);
         piece_type.parameters["background"] = tower::ParameterRange(0);
         section.types.push_back(piece_type);
      }

      twr0.sections.push_back(section);
   }

   {
      tower::Section section;
      section.height0 = -80;
      section.height1 = -128;
      section.radius = 25;
      section.num_instances = 60;

      {
         tower::PieceType piece_type;
         piece_type.weight = 2;
         piece_type.parameters["distance"] = tower::ParameterRange(0.9, 0.1);
         piece_type.parameters["angle"] = tower::ParameterRange(0, M_PI*2);
         piece_type.parameters["height"] = tower::ParameterRange(0,1);
         piece_type.parameters["sca_x"] = tower::ParameterRange(1.0);
         piece_type.parameters["sca_y"] = tower::ParameterRange(1.0);
         piece_type.parameters["sca_z"] = tower::ParameterRange(1.0);
         piece_type.parameters["rot_x"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_y"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_spd_x"] = tower::ParameterRange(-0.3,0.3);
         piece_type.parameters["rot_spd_y"] = tower::ParameterRange(-0.3,0.3);
         piece_type.parameters["time_scale"] = tower::ParameterRange(3);
         piece_type.parameters["geom"] = tower::ParameterRange(1);
         piece_type.parameters["background"] = tower::ParameterRange(0);
         piece_type.parameters["ptn_thickness"] = tower::ParameterRange(0.01,0.02);
         piece_type.parameters["ptn_brightness"] = tower::ParameterRange(0.1,0.3);
         piece_type.parameters["shmmer"] = tower::ParameterRange(1);
         section.types.push_back(piece_type);
      }

      {
         tower::PieceType piece_type;
         piece_type.weight = 1;
         piece_type.parameters["distance"] = tower::ParameterRange(0.0, 0.2);
         piece_type.parameters["angle"] = tower::ParameterRange(0, M_PI*2);
         piece_type.parameters["height"] = tower::ParameterRange(0,1);
         piece_type.parameters["sca_x"] = tower::ParameterRange(1.3);
         piece_type.parameters["sca_y"] = tower::ParameterRange(1.3);
         piece_type.parameters["sca_z"] = tower::ParameterRange(1.3);
         piece_type.parameters["rot_x"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_y"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_spd_x"] = tower::ParameterRange(-0.1,0.1);
         piece_type.parameters["rot_spd_y"] = tower::ParameterRange(-0.1,0.1);
         piece_type.parameters["l1_brightness"] = tower::ParameterRange(0.05);
         piece_type.parameters["l1_scroll"] = tower::ParameterRange(0.1,0.2);
         piece_type.parameters["l2_brightness"] = tower::ParameterRange(2.0);
         piece_type.parameters["l2_scroll"] = tower::ParameterRange(0.1,0.2);
         piece_type.parameters["l0_brightness"] = tower::ParameterRange(1);
         piece_type.parameters["l0_scroll"] = tower::ParameterRange(0.1,0.2);
         piece_type.parameters["time_scale"] = tower::ParameterRange(3);
         piece_type.parameters["geom"] = tower::ParameterRange(0);
         piece_type.parameters["background"] = tower::ParameterRange(0);
         piece_type.parameters["shmmer"] = tower::ParameterRange(1);
         piece_type.parameters["ambient"] = tower::ParameterRange(0.1,0.2);
         section.types.push_back(piece_type);
      }

      twr0.sections.push_back(section);
   }



   {
      tower::Section section;
      section.height0 = -100;
      section.height1 = -256;
      section.radius = 10;
      section.num_instances = 100;

      {
         tower::PieceType piece_type;
         piece_type.weight = 1;
         piece_type.parameters["distance"] = tower::ParameterRange(0.0, 0.6);
         piece_type.parameters["angle"] = tower::ParameterRange(0, M_PI*2);
         piece_type.parameters["height"] = tower::ParameterRange(0,1);
         piece_type.parameters["sca_x"] = tower::ParameterRange(0.3,0.4);
         piece_type.parameters["sca_y"] = tower::ParameterRange(0.3,0.4);
         piece_type.parameters["sca_z"] = tower::ParameterRange(0.3,0.4);
         piece_type.parameters["rot_x"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_y"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_spd_x"] = tower::ParameterRange(-0.6,0.6);
         piece_type.parameters["rot_spd_y"] = tower::ParameterRange(-0.6,0.6);
         piece_type.parameters["time_scale"] = tower::ParameterRange(3);
         piece_type.parameters["geom"] = tower::ParameterRange(1);
         piece_type.parameters["background"] = tower::ParameterRange(0);
         piece_type.parameters["ptn_thickness"] = tower::ParameterRange(0.01,0.02);
         piece_type.parameters["ptn_brightness"] = tower::ParameterRange(0.05,0.2);
         piece_type.parameters["shmmer"] = tower::ParameterRange(1);
         section.types.push_back(piece_type);
      }

      twr0.sections.push_back(section);
   }

   {
      tower::Section section;
      section.height0 = -140;
      section.height1 = -200;
      section.radius = 1;
      section.num_instances = 30;

      {
         tower::PieceType piece_type;
         piece_type.weight = 1;
         piece_type.parameters["distance"] = tower::ParameterRange(0.0, 0.6);
         piece_type.parameters["angle"] = tower::ParameterRange(0, M_PI*2);
         piece_type.parameters["height"] = tower::ParameterRange(0,1);
         piece_type.parameters["sca_x"] = tower::ParameterRange(1.5,2.0);
         piece_type.parameters["sca_y"] = tower::ParameterRange(1.5,2.0);
         piece_type.parameters["sca_z"] = tower::ParameterRange(1.5,2.0);
         //piece_type.parameters["rot_x"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_y"] = tower::ParameterRange(-0.4,0.4);
         //piece_type.parameters["rot_spd_x"] = tower::ParameterRange(-0.6,0.6);
         //piece_type.parameters["rot_spd_y"] = tower::ParameterRange(-0.6,0.6);
         piece_type.parameters["time_scale"] = tower::ParameterRange(3);
         piece_type.parameters["geom"] = tower::ParameterRange(2);
         piece_type.parameters["background"] = tower::ParameterRange(0);
         piece_type.parameters["l1_brightness"] = tower::ParameterRange(1.0);
         piece_type.parameters["shmmer"] = tower::ParameterRange(1);
         piece_type.parameters["section_rot_spd"] = tower::ParameterRange(8);
         section.types.push_back(piece_type);
      }

      twr0.sections.push_back(section);
   }

   {
      tower::Section section;
      section.height0 = -200;
      section.height1 = -256;
      section.radius = 1;
      section.num_instances = 60;

      {
         tower::PieceType piece_type;
         piece_type.weight = 1;
         piece_type.parameters["distance"] = tower::ParameterRange(0.0, 0.6);
         piece_type.parameters["angle"] = tower::ParameterRange(0, M_PI*2);
         piece_type.parameters["height"] = tower::ParameterRange(0,1);
         piece_type.parameters["sca_x"] = tower::ParameterRange(1.5,2.0);
         piece_type.parameters["sca_y"] = tower::ParameterRange(1.5,2.0);
         piece_type.parameters["sca_z"] = tower::ParameterRange(1.5,2.0);
         //piece_type.parameters["rot_x"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_y"] = tower::ParameterRange(-0.4,0.4);
         //piece_type.parameters["rot_spd_x"] = tower::ParameterRange(-0.6,0.6);
         //piece_type.parameters["rot_spd_y"] = tower::ParameterRange(-0.6,0.6);
         piece_type.parameters["time_scale"] = tower::ParameterRange(3);
         piece_type.parameters["geom"] = tower::ParameterRange(2);
         piece_type.parameters["background"] = tower::ParameterRange(0);
         piece_type.parameters["l1_brightness"] = tower::ParameterRange(1.0);
         piece_type.parameters["shmmer"] = tower::ParameterRange(1);
         piece_type.parameters["section_rot_spd"] = tower::ParameterRange(8);
         section.types.push_back(piece_type);
      }

      twr0.sections.push_back(section);
   }

   {
      tower::Section section;
      section.height0 = -200;
      section.height1 = -256;
      section.radius = 1;
      section.num_instances = 50;

      {
         tower::PieceType piece_type;
         piece_type.weight = 1;
         piece_type.parameters["distance"] = tower::ParameterRange(0.0, 0.6);
         piece_type.parameters["angle"] = tower::ParameterRange(0, M_PI*2);
         piece_type.parameters["height"] = tower::ParameterRange(0,1);
         piece_type.parameters["sca_x"] = tower::ParameterRange(0.5,1.0);
         piece_type.parameters["sca_y"] = tower::ParameterRange(0.5,1.0);
         piece_type.parameters["sca_z"] = tower::ParameterRange(0.5,1.0);
         piece_type.parameters["rot_x"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_y"] = tower::ParameterRange(0,2*M_PI);
         //piece_type.parameters["rot_spd_x"] = tower::ParameterRange(-0.6,0.6);
         //piece_type.parameters["rot_spd_y"] = tower::ParameterRange(-0.6,0.6);
         piece_type.parameters["time_scale"] = tower::ParameterRange(3);
         piece_type.parameters["geom"] = tower::ParameterRange(2);
         piece_type.parameters["background"] = tower::ParameterRange(0);
         //piece_type.parameters["l1_brightness"] = tower::ParameterRange(1.0);
         piece_type.parameters["ambient"] = tower::ParameterRange(0.8);
         piece_type.parameters["section_rot_spd"] = tower::ParameterRange(8);
         section.types.push_back(piece_type);
      }

      twr0.sections.push_back(section);
   }

   {
      tower::Section section;
      section.height0 = -10;
      section.height1 = +4;
      section.radius = 5;
      section.num_instances = 10;

      {
         tower::PieceType piece_type;
         piece_type.weight = 1;
         piece_type.parameters["distance"] = tower::ParameterRange(0.8, 1.0);
         piece_type.parameters["angle"] = tower::ParameterRange(0, 2*M_PI);
         piece_type.parameters["height"] = tower::ParameterRange(0.0,1);
         piece_type.parameters["sca_x"] = tower::ParameterRange(0.8);
         piece_type.parameters["sca_y"] = tower::ParameterRange(0.8);
         piece_type.parameters["sca_z"] = tower::ParameterRange(0.8);
         piece_type.parameters["rot_x"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_y"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["l1_brightness"] = tower::ParameterRange(1.5);
         piece_type.parameters["geom"] = tower::ParameterRange(1);
         piece_type.parameters["background"] = tower::ParameterRange(1);
         section.types.push_back(piece_type);
      }

      twr0.sections.push_back(section);
   }

   {
      tower::Section section;
      section.height0 = -50;
      section.height1 = -128;
      section.radius = 3;
      section.num_instances = 30;

      {
         tower::PieceType piece_type;
         piece_type.weight = 1;
         piece_type.parameters["distance"] = tower::ParameterRange(0.0, 1.0);
         piece_type.parameters["angle"] = tower::ParameterRange(0, 2*M_PI);
         piece_type.parameters["height"] = tower::ParameterRange(0.0,1);
         piece_type.parameters["sca_x"] = tower::ParameterRange(1.8);
         piece_type.parameters["sca_y"] = tower::ParameterRange(1.8);
         piece_type.parameters["sca_z"] = tower::ParameterRange(1.8);
         piece_type.parameters["rot_x"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_y"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_spd_x"] = tower::ParameterRange(1.0,2.1);
         piece_type.parameters["rot_spd_y"] = tower::ParameterRange(1.0,2.1);
         piece_type.parameters["l1_brightness"] = tower::ParameterRange(2.5);
         piece_type.parameters["geom"] = tower::ParameterRange(1);
         piece_type.parameters["background"] = tower::ParameterRange(1);
         section.types.push_back(piece_type);
      }

      twr0.sections.push_back(section);
   }

   {
      tower::Section section;
      section.height0 = -32;
      section.height1 = -64;
      section.radius = 30;
      section.num_instances = 60;

      {
         tower::PieceType piece_type;
         piece_type.weight = 3;
         piece_type.parameters["distance"] = tower::ParameterRange(0.0, 0.2);
         piece_type.parameters["angle"] = tower::ParameterRange(0, 2*M_PI);
         piece_type.parameters["height"] = tower::ParameterRange(0,1);
         piece_type.parameters["l1_brightness"] = tower::ParameterRange(0.2);
         piece_type.parameters["l2_brightness"] = tower::ParameterRange(2.0);
         piece_type.parameters["l2_scroll"] = tower::ParameterRange(0.1,0.2);
         piece_type.parameters["l0_brightness"] = tower::ParameterRange(1);
         piece_type.parameters["l0_scroll"] = tower::ParameterRange(0.1,0.2);
         piece_type.parameters["sca_x"] = tower::ParameterRange(0.4,0.6);
         piece_type.parameters["sca_y"] = tower::ParameterRange(0.4,1.6);
         piece_type.parameters["sca_z"] = tower::ParameterRange(0.4,0.6);
         piece_type.parameters["rot_x"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_y"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["geom"] = tower::ParameterRange(1);
         piece_type.parameters["time_scale"] = tower::ParameterRange(10);
         piece_type.parameters["background"] = tower::ParameterRange(1);
         section.types.push_back(piece_type);
      }

      twr0.sections.push_back(section);
   }

   {
      tower::Section section;
      section.height0 = -170;
      section.height1 = -276;
      section.radius = 10;
      section.num_instances = 40;

      {
         tower::PieceType piece_type;
         piece_type.weight = 1;
         piece_type.parameters["distance"] = tower::ParameterRange(0.0, 1.0);
         piece_type.parameters["angle"] = tower::ParameterRange(0, M_PI*2);
         piece_type.parameters["height"] = tower::ParameterRange(0,1);
         piece_type.parameters["sca_x"] = tower::ParameterRange(1.5,2.0);
         piece_type.parameters["sca_y"] = tower::ParameterRange(1.5,2.0);
         piece_type.parameters["sca_z"] = tower::ParameterRange(1.5,2.0);
         //piece_type.parameters["rot_x"] = tower::ParameterRange(0,2*M_PI);
         piece_type.parameters["rot_y"] = tower::ParameterRange(-0.4,0.4);
         //piece_type.parameters["rot_spd_x"] = tower::ParameterRange(-0.6,0.6);
         //piece_type.parameters["rot_spd_y"] = tower::ParameterRange(-0.6,0.6);
         piece_type.parameters["time_scale"] = tower::ParameterRange(3);
         piece_type.parameters["geom"] = tower::ParameterRange(2);
         piece_type.parameters["background"] = tower::ParameterRange(1);
         piece_type.parameters["l1_brightness"] = tower::ParameterRange(1.0);
         piece_type.parameters["shmmer"] = tower::ParameterRange(1);
         section.types.push_back(piece_type);
      }

      twr0.sections.push_back(section);
   }

   twr0.genInstances();


   icosahedron_mesh.generateIcosahedron();
   tetrahedron_mesh.generateTetrahedron();
   octahedron_mesh.generateOctahedron();

}


void PlatonicScene::update()
{
}

static Vec2 rotate(float angle,const Vec2& vec)
{
   return Vec2(std::cos(angle)*vec.x + std::sin(angle)*vec.y,
               std::cos(angle)*vec.y - std::sin(angle)*vec.x);
}

void PlatonicScene::drawLogo(float ltime)
{
   float ltime2=ltime;
   ltime=4.0f-ltime;

   ltime*=1.0f;

   static const float s=0.35f;
   static const float logo_aspect=1420.0f/300.0f;
   static const float target_x0=-s*logo_aspect*float(window_height)/float(window_width);
   static const float target_x1=+s*logo_aspect*float(window_height)/float(window_width);
   static const float target_y0=-1*s;
   static const float target_y1=+1*s;
   static const int edge_subdivisions=2;

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
      const Vec2 p=Vec2(vc.center*1.5f+Vec2(-ltime2*0.2f,ltime2*2.5f));
      const Vec2 center=mix(p,vc.center,mix(t/(t+0.15f),1.0f,std::min(1.0f,ltime*0.05f)));
      float rot=ltime2*std::cos(i)*0.5f;

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

            const Vec2 rp0=rotate(rot,p0-vc.center);
            const Vec2 rp1=rotate(rot,p1-vc.center);

            assert(num_voronoi_indices<max_voronoi_indices);
            voronoi_indices[num_voronoi_indices++]=num_voronoi_vertices;
            assert(num_voronoi_indices<max_voronoi_indices);
            voronoi_indices[num_voronoi_indices++]=num_voronoi_vertices+1;
            assert(num_voronoi_indices<max_voronoi_indices);
            voronoi_indices[num_voronoi_indices++]=first_index;

            float x0=mix(target_x0,target_x1,(rp0.x)*zz+center.x);
            float y0=mix(target_y0,target_y1,(rp0.y)*zz+center.y);

            float x1=mix(target_x0,target_x1,(rp1.x)*zz+center.x);
            float y1=mix(target_y0,target_y1,(rp1.y)*zz+center.y);

/*
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
*/

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
   room_logo_shader.uniform1f("time",1.0f+ltime);

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

void PlatonicScene::drawTower(tower::Tower& twr, const Mat4& modelview, Real camheight)
{
   const Real camheightrange = drawing_view_for_background ? 30 : 10;

   forrest_shader.bind();

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(2);

   for(std::vector<tower::PieceInstance>::iterator p=twr.instances.begin();p!=twr.instances.end();++p)
   {
      std::map<std::string,Real>& ps=p->parameters;

      if((ps["background"]>0.5)!=drawing_view_for_background)
         continue;

      if(camheight < (p->section->height1 - camheightrange) || camheight > (p->section->height0 + camheightrange))
         continue;

      Mesh* mesh=&icosahedron_mesh;

      switch(int(ps["geom"]+0.5))
      {
         case 1:
            mesh = &tetrahedron_mesh;
            break;

         case 2:
            mesh = &octahedron_mesh;
            break;
      }

      mesh->bind();

      forrest_shader.uniform1f("time", lltime*ps["time_scale"]+ps["time_offset"]);

      forrest_shader.uniform1f("ptn_thickness",ps["ptn_thickness"]);
      forrest_shader.uniform1f("ptn_brightness",ps["ptn_brightness"]);
      forrest_shader.uniform1f("ptn_scroll",ps["ptn_scroll"]);
      forrest_shader.uniform1f("ptn_frequency",ps["ptn_frequency"]);
      forrest_shader.uniform1f("shimmer",ps["shimmer"]);
      forrest_shader.uniform1f("l0_scroll",ps["l0_scroll"]);
      forrest_shader.uniform1f("l0_brightness",ps["l0_brightness"]);
      forrest_shader.uniform1f("l1_scroll",ps["l1_scroll"]);
      forrest_shader.uniform1f("l1_brightness",ps["l1_brightness"]);
      forrest_shader.uniform1f("l2_scroll",ps["l2_scroll"]);
      forrest_shader.uniform1f("l2_brightness",ps["l2_brightness"]);
      forrest_shader.uniform1f("ambient",ps["ambient"]);

      Mat4 modelview2 = modelview * Mat4::rotation(lltime * ps["section_rot_spd"], Vec3(0,1,0)) *
                        Mat4::translation(Vec3(std::cos(ps["angle"])*ps["distance"], ps["height"], std::sin(ps["angle"])*ps["distance"])) *
                        Mat4::rotation(ps["rot_z"]+lltime*ps["rot_spd_z"],Vec3(0,0,1)) * Mat4::rotation(ps["rot_y"]+lltime*ps["rot_spd_y"],Vec3(0,1,0)) * Mat4::rotation(ps["rot_x"]+lltime*ps["rot_spd_x"],Vec3(1,0,0)) *
                        Mat4::scale(Vec3(ps["sca_x"],ps["sca_y"],ps["sca_z"]));

      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
      glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(mesh->getVertexCount() * sizeof(GLfloat) * 3));

      forrest_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview2.e);

      glDrawRangeElements(GL_TRIANGLES, 0, mesh->getVertexCount() - 1, mesh->getTriangleCount() * 3, GL_UNSIGNED_INT, 0);

      CHECK_FOR_ERRORS;
   }

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(2);
}

void PlatonicScene::drawView(float ltime)
{
   lltime = ltime;

   const float aspect_ratio = float(window_height) / float(window_width);

   const float jitter_x = frand() / window_width * 3;
   const float jitter_y = frand() / window_height * aspect_ratio * 3;

   Mat4 modelview = Mat4::identity(), projection = Mat4::identity();

   const float fov = M_PI * 0.3f;

   const float znear = 1.0f / tan(fov * 0.5f), zfar = 1024.0f;

   projection = Mat4::frustum(-1.0f + jitter_x, +1.0f + jitter_x, -aspect_ratio + jitter_y, +aspect_ratio + jitter_y, znear, zfar) * projection;

   const float move_back = drawing_view_for_background ? 10 : 0;

   Real camheight = 7 - ltime * 6;

   // 11:59 <@SunnyREV> first hit: 4:39:579 second hit: 4:39:569 third hit: 4:39:840 fourth hit: 4:40:028 fifth hit: 4:40:121 sixth hit: 4:40:206
   // 4:39:579, 4:39:665, 4:39:840, 4:40:025, 4:40:117, 4:40:215

   //static const float drum_hits[6] = { (4 * 60 + 39) + 0.569, (4 * 60 + 39) + 0.579, (4 * 60 + 39) + 0.840, (4 * 60 + 40) + 0.028, (4 * 60 + 40) + 0.121, (4 * 60 + 40) + 0.206 };
   static const float drum_hits[6] = { (4 * 60 + 39) + 0.579, (4 * 60 + 39) + 0.665, (4 * 60 + 39) + 0.840, (4 * 60 + 40) + 0.025, (4 * 60 + 40) + 0.117, (4 * 60 + 40) + 0.215 };
   static const float firsthitheight = 7 - (((4 * 60 + 39) + 0.579) - scene_start_time) * 6;
   static const float drum_hit_heights[6] = { firsthitheight, firsthitheight + 10, firsthitheight, firsthitheight + 10, firsthitheight, firsthitheight - 4 };

   {
      const float rtime=original_time+scene_start_time;
      for(int i=0;i<5;++i)
      {
         if(rtime > drum_hits[i])
         {
            camheight=mix(drum_hit_heights[i],drum_hit_heights[i+1],(rtime-drum_hits[i])/(drum_hits[i+1]-drum_hits[i]));
         }
      }
/*
      if(rtime > drum_hits[5])
      {
         camheight=drum_hit_heights[5];
      }
*/
   }

   modelview = modelview * Mat4::translation(Vec3(0, -camheight, -20 - move_back));

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

   forrest_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);
   text_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);


   CHECK_FOR_ERRORS;





   drawTower(twr0, modelview, camheight);

   glUseProgram(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);

   if(!drawing_view_for_background)
   {
      glBindBuffer(GL_ARRAY_BUFFER,text_vbo);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,text_ebo);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
      static const float sca=2;
      static const float zp=-7;
      static const float line_height=0.6;
      static const float second_set_y=-80;
      static const Mat4 scale_matrix=Mat4::scale(Vec3(1.25));
      static const Mat4 rot_correction=Mat4::rotation(M_PI*0.5f,Vec3(1,0,0));
      text_shader.bind();
      text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (Mat4::translation(Vec3(-8 * sca * 0.6,-10,zp * sca)) * modelview * scale_matrix * rot_correction).e);
      drawText("party_hall");
      text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (Mat4::translation(Vec3(6.5 * sca * 0.6,(-10-line_height*12),zp * sca)) * modelview * scale_matrix * rot_correction).e);
      drawText("kickass");
      text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (Mat4::translation(Vec3(-6 * sca * 0.6,(-10-line_height*24),zp * sca)) * modelview * scale_matrix * rot_correction).e);
      drawText("bigscreen");
      text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (Mat4::translation(Vec3(9 * sca * 0.6,(-10-line_height*36),zp * sca)) * modelview * scale_matrix * rot_correction).e);
      drawText("in-house");
      text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (Mat4::translation(Vec3(-8 * sca * 0.6,(-10-line_height*48),zp * sca)) * modelview * scale_matrix * rot_correction).e);
      drawText("bbq");
      text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (Mat4::translation(Vec3(8 * sca * 0.6,(-10-line_height*60),zp * sca)) * modelview * scale_matrix * rot_correction).e);
      drawText("large_outdoor");
      text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (Mat4::translation(Vec3(-8 * sca * 0.6,(-10-line_height*110),zp * sca)) * modelview * scale_matrix * rot_correction).e);
      drawText("a_team_that_cares",false,true);

      text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (Mat4::translation(Vec3(7 * sca * 0.6,second_set_y-10-line_height*96,zp * sca)) * modelview * scale_matrix * rot_correction).e);
      drawText("old_&_newschool_competitions",true);
      text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (Mat4::translation(Vec3(7 * sca * 0.6,second_set_y-10-line_height*104,zp * sca)) * modelview * scale_matrix * rot_correction).e);
      drawText("graphics");
      text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (Mat4::translation(Vec3(7 * sca * 0.6,second_set_y-10-line_height*109,zp * sca)) * modelview * scale_matrix * rot_correction).e);
      drawText("photo");
      text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (Mat4::translation(Vec3(7 * sca * 0.6,second_set_y-10-line_height*114,zp * sca)) * modelview * scale_matrix * rot_correction).e);
      drawText("music");
      text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (Mat4::translation(Vec3(7 * sca * 0.6,second_set_y-10-line_height*119,zp * sca)) * modelview * scale_matrix * rot_correction).e);
      drawText("demo");
      text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (Mat4::translation(Vec3(7 * sca * 0.6,second_set_y-10-line_height*124,zp * sca)) * modelview * scale_matrix * rot_correction).e);
      drawText("gravedigger");
      text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (Mat4::translation(Vec3(7 * sca * 0.6,second_set_y-10-line_height*129,zp * sca)) * modelview * scale_matrix * rot_correction).e);
      drawText("wild");

      text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (Mat4::translation(Vec3(-7 * sca * 0.6,second_set_y-10-line_height*(58+98),zp * sca)) * modelview * scale_matrix * rot_correction).e);
      drawText("live_events",true);
      text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (Mat4::translation(Vec3(-7 * sca * 0.6,second_set_y-10-line_height*(58+103),zp * sca)) * modelview * scale_matrix * rot_correction).e);
      drawText("choco");
      text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (Mat4::translation(Vec3(-7 * sca * 0.6,second_set_y-10-line_height*(58+109),zp * sca)) * modelview * scale_matrix * rot_correction).e);
      drawText("computadora");
      text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (Mat4::translation(Vec3(-7 * sca * 0.6,second_set_y-10-line_height*(58+115),zp * sca)) * modelview * scale_matrix * rot_correction).e);
      drawText("datucker");
      text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (Mat4::translation(Vec3(-7 * sca * 0.6,second_set_y-10-line_height*(104+109),zp * sca)) * modelview * scale_matrix * rot_correction).e);
      drawText("and_more");

      text_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (Mat4::translation(Vec3(7 * sca * 0.6,second_set_y-10-line_height*(126+143),zp * sca)) * modelview * scale_matrix * rot_correction).e);
      drawText("join_us",false,false,true);

      glDisableVertexAttribArray(0);
      glBindBuffer(GL_ARRAY_BUFFER,0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);

      if(ltime < 10)
         drawLogo(ltime);
   }

   glDepthMask(GL_TRUE);

   CHECK_FOR_ERRORS;
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void PlatonicScene::gaussianBlur(Real tint_r, Real tint_g, Real tint_b, Real radx, Real rady, GLuint fbo0, GLuint fbo1, GLuint src_tex, GLuint tex0)
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


extern unsigned long int makeMS(int minutes,int seconds,int frames);

void PlatonicScene::render()
{
   //time+=(makeMS(4,40,16)/1000.0f - 4) - scene_start_time;

   const float adjust_seconds = 1.75;
   const float offset_seconds = 1;

   original_time=time;

   if(time < offset_seconds)
   {
      time *= adjust_seconds / offset_seconds;
   }
   else
   {
      time = adjust_seconds + (time - offset_seconds);
   }

   static const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f };
   static const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
   static const GLubyte indices[] = { 3, 0, 2, 1 };

   const float aspect_ratio = float(window_height) / float(window_width);

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

   for(int nn = 0; nn < 2; ++nn)
   {
//if(drawing_view_for_background) continue; // **********************************************************************************

      drawing_view_for_background = !nn;

      // ----- motion blur ------

      glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[3]);
      glClear(GL_COLOR_BUFFER_BIT);

      num_subframes = drawing_view_for_background ? 1 : 7;
      subframe_scale = 1.0/35.0;

      for(int subframe = 0;subframe<num_subframes;++subframe)
      {
         // draw the view

         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[6]);

         glDisable(GL_BLEND);

         drawView(time + subframe_scale * float(subframe) / float(num_subframes));


         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[3]);

         mb_accum_shader.bind();
         mb_accum_shader.uniform1f("time", float(subframe) / float(num_subframes));
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

      if(drawing_view_for_background)
      {
         // ----- aperture ------

         const float CoC = 0.02;// + sin(time) * 0.05;

         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, bokeh_temp_fbo);

         {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
            glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
         }

         glViewport(0, 0, window_width / 2, window_height / 2);

         bokeh_shader.bind();
         bokeh_shader.uniform2f("CoC", CoC / aspect_ratio, CoC);

         glDepthMask(GL_FALSE);
         glDisable(GL_DEPTH_TEST);
         glDisable(GL_BLEND);

         glActiveTexture(GL_TEXTURE0);
         glBindTexture(GL_TEXTURE_2D, texs[2]);
         glEnableVertexAttribArray(0);
         glEnableVertexAttribArray(1);
         glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
         glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
         glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
         glDisableVertexAttribArray(0);
         glDisableVertexAttribArray(1);

         CHECK_FOR_ERRORS;


         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);

         glViewport(0, 0, window_width, window_height);

         {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
         }

         bokeh2_shader.bind();
         bokeh2_shader.uniform2f("CoC", CoC / aspect_ratio, CoC);
         bokeh2_shader.uniform1f("brightness", 0.2);

         glActiveTexture(GL_TEXTURE1);
         glBindTexture(GL_TEXTURE_2D, bokeh_temp_tex_2);
         glActiveTexture(GL_TEXTURE0);
         glBindTexture(GL_TEXTURE_2D, bokeh_temp_tex_1);
         glEnableVertexAttribArray(0);
         glEnableVertexAttribArray(1);
         glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
         glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
         glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
         glDisableVertexAttribArray(0);
         glDisableVertexAttribArray(1);
      }
      else
      {
         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);
         glViewport(0, 0, window_width, window_height);
         glDisable(GL_BLEND);
         //glEnable(GL_BLEND);
         //glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);

         bgfg_shader.bind();

         glActiveTexture(GL_TEXTURE1);
         glBindTexture(GL_TEXTURE_2D, texs[2]);
         glActiveTexture(GL_TEXTURE0);
         glBindTexture(GL_TEXTURE_2D, texs[0]);
         glEnableVertexAttribArray(0);
         glEnableVertexAttribArray(1);
         glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
         glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
         glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
         glDisableVertexAttribArray(0);
         glDisableVertexAttribArray(1);

         //glBindFramebuffer(GL_READ_FRAMEBUFFER, fbos[0]);
         //glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);
         //glViewport(0, 0, window_width, window_height);
         //glBlitFramebuffer(0, 0, window_width, window_height, 0, 0, window_width, window_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
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

   composite_shader.uniform4f("tint", 1, 1, 1, 1);

   glActiveTexture(GL_TEXTURE5);
   glBindTexture(GL_TEXTURE_2D, texs[9]);
   glActiveTexture(GL_TEXTURE4);
   glBindTexture(GL_TEXTURE_2D, texs[4]);
   glActiveTexture(GL_TEXTURE3);
   glBindTexture(GL_TEXTURE_2D, texs[13]);
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


void PlatonicScene::free()
{
   glDeleteTextures(1,&logo_tex);
   delete[] text_mesh_vertices;
   text_mesh_vertices=0;
   delete[] text_mesh_indices;
   text_mesh_indices=0;
   glDeleteTextures(num_texs, texs);
   glDeleteFramebuffers(num_fbos, fbos);
   glDeleteBuffers(1,&text_vbo);
   glDeleteBuffers(1,&text_ebo);
}

