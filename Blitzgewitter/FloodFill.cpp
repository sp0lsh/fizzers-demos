
#include <IL/il.h>
#include <cassert>
#include "Ran.hpp"
#include <vector>
#include <cstring>

#include "floodfill.h"

using namespace std;

static long int lrand()
{
   static Ran rnd(630);
   return rnd.int32();
}

struct Seed
{
   Seed(int x_,int y_): x(x_), y(y_)
   {
   }

   int operator[](size_t idx) const
   {
      return idx ? y : x;
   }

   int x,y;
};

void preprocessLoadingBar()
{
   ilInit();

   ilEnable(IL_ORIGIN_SET);
   ilOriginFunc(IL_ORIGIN_LOWER_LEFT);

   ilEnable(IL_TYPE_SET);
   ilTypeFunc(IL_UNSIGNED_BYTE);

   ILuint ilimg = 0;

   ilGenImages(1, &ilimg);
   ilBindImage(ilimg);

   assert(ilLoadImage("images/lines.png") == IL_TRUE);

   const int width=ilGetInteger(IL_IMAGE_WIDTH);
   const int height=ilGetInteger(IL_IMAGE_HEIGHT);

   assert(ilGetInteger(IL_IMAGE_FORMAT) == IL_RGBA);

   vector<Seed> seeds;
   bool* occupied=new bool[width*height];
   unsigned long int* result=new unsigned long int[width*height];

   memset(occupied, 0, width*height*sizeof(occupied[0]));
   memset(result, 0xff, width*height*sizeof(result[0]));

   static const int first_seed_x=84;//77;
   static const int first_seed_y=height-520;//511;

   seeds.push_back(Seed(first_seed_x,first_seed_y));

   const unsigned char* pix=ilGetData();
   unsigned long int count=1;

   while(!seeds.empty())
   {
      vector<Seed> new_seeds;
      for(vector<Seed>::const_iterator it=seeds.begin();it!=seeds.end();++it)
      {
         const Seed& s=*it;

         if((lrand()&255) < 200)
         {
           new_seeds.push_back(s);
           continue;
         }

         int ou=s[0],ov=s[1];
         for(int y=-1;y<2;++y)
           for(int x=-1;x<2;++x)
           {
             int u=ou+x;
             int v=ov+y;
             if(u >= 0 && v >= 0 && u < width && v < height && !occupied[u+v*width])
             {
               float a = pix[(u+v*width)*4+3]/255.0f;
               if(a>0.01)
               {
                 //stroke(255,0,0,a);
                 //point(u,v);
                 occupied[u+v*width]=true;
                 result[u+v*width]=count++;
                 new_seeds.push_back(Seed(u,v));
               }
             }
           }
      }
      seeds=new_seeds;
   }

   FILE* out=fopen("floodfill.h","w");

   assert(out);

   fprintf(out,"// count = %d\r\n",count);
   fprintf(out,"unsigned short loading_bar_flood_fill_image[%d*%d]={\r\n",width,height);
   for(int y=0;y<height;++y)
   {
      for(int x=0;x<width;++x)
      {
         fprintf(out,"%d, ",(result[x+y*width]>>1) & 0xffff);
      }
      fprintf(out,"\r\n");
   }

   fprintf(out,"};");
   fclose(out);

   delete[] result;
   delete[] occupied;


   ilDeleteImages(1, &ilimg);
}

