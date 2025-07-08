
#include "Engine.hpp"

#include <cmath>

static const unsigned long int light_texture_w = 64;
static const unsigned long int light_texture_h = 64;
static const unsigned long int light_texture_channels = 1;

typedef unsigned long int TableEntry;

TableEntry sumArea(const unsigned char* light_texture, unsigned long int x0, unsigned long int y0, unsigned long int x1, unsigned long int y1, unsigned long int c)
{
   TableEntry sum = 0;

   double plane0[3] = { +y0, -x0, 0 };
   double plane1[3] = { -y1, +x1, 0 };
   double plane2[3] = { (y1 - y0), (x0 - x1), 0 };

   plane2[2] = -(x0*plane2[0] + y0*plane2[1]);

   if(plane2[2] < 0)
   {
      plane2[0] = -plane2[0];
      plane2[1] = -plane2[1];
      plane2[2] = -plane2[2];
   }

   for(unsigned long int y = 0; y < light_texture_h; ++y)
   {
      for(unsigned long int x = 0; x < light_texture_w; ++x)
      {
         double u = x, v = y;

         double d0 = u * plane0[0] + v * plane0[1] + plane0[2];
         double d1 = u * plane1[0] + v * plane1[1] + plane1[2];
         double d2 = u * plane2[0] + v * plane2[1] + plane2[2];

         if(d0 < 0 && d1 < 0 && d2 < 0)
            sum += light_texture[(x + y * light_texture_w) * light_texture_channels + c];

      }
   }
   return sum;
}

void generateAreaLightTables()
{
   unsigned char* light_texture = new unsigned char[light_texture_w * light_texture_h * light_texture_channels];
   TableEntry* lookup_table = new TableEntry[light_texture_w * light_texture_h * light_texture_w * light_texture_h * light_texture_channels];

   const unsigned long int pitch0 = light_texture_channels;
   const unsigned long int pitch1 = pitch0 * light_texture_w;
   const unsigned long int pitch2 = pitch1 * light_texture_h;
   const unsigned long int pitch3 = pitch2 * light_texture_w;

   for(unsigned long int y = 0; y < light_texture_h; ++y)
   {
      for(unsigned long int x = 0; x < light_texture_w; ++x)
      {
         double u = x, v = y;

         double du = (u - light_texture_w * 0.5) / (light_texture_w * 0.5);
         double dv = (v - light_texture_h * 0.5) / (light_texture_h * 0.5);

         double l = std::sqrt(du * du + dv * dv);

         light_texture[x + y * light_texture_w] = std::max(0.0, 1.0 - l);
      }
   }

   for(unsigned long int u0 = 0; u0 < light_texture_w; ++u0)
   {
      for(unsigned long int u1 = 0; u1 < light_texture_h; ++u1)
      {
         for(unsigned long int u2 = 0; u2 < light_texture_w; ++u2)
         {
            for(unsigned long int u3 = 0; u3 < light_texture_h; ++u3)
            {
               for(unsigned long int c = 0; c < light_texture_channels; ++c)
               {
                  lookup_table[u0 * pitch0 + u1 * pitch1 + u2 * pitch2 + u3 * pitch3 + c] = sumArea(light_texture, u0, u1, u2, u3, c);
               }
            }
         }
      }
   }

   delete[] lookup_table;
   delete[] light_texture;
}

