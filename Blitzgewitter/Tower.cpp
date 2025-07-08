#include "Tower.hpp"
#include "Ran.hpp"

using namespace tower;
using namespace std;

static Real frand()
{
   static Ran rnd(998);
   return rnd.doub();
}

static Real randRange(const Real x0, const Real x1)
{
   return mix(x0,x1,frand());
}

void Tower::genInstances()
{
   instances.clear();

   for(vector<Section>::iterator s=sections.begin();s!=sections.end();++s)
   {
      Real cw=0;
      for(vector<PieceType>::iterator t=s->types.begin();t!=s->types.end();++t)
      {
         cw+=t->weight;
         t->cumulative_weight=cw;
      }

      for(unsigned int i=0;i<s->num_instances;++i)
      {
         PieceInstance instance;
         const Real r=frand()*cw;
         for(vector<PieceType>::iterator t=s->types.begin();t!=s->types.end();++t)
         {
            if(t->cumulative_weight >= r)
            {
               for(map<std::string, ParameterRange>::iterator p=t->parameters.begin();p!=t->parameters.end();++p)
               {
                  instance.parameters[p->first]=randRange(p->second.x0, p->second.x1);
               }
               break;
            }
         }

         instance.parameters["height"]=mix(s->height0,s->height1,instance.parameters["height"]);
         instance.parameters["distance"]*=s->radius;
         instance.section=&*s;

         instances.push_back(instance);
      }
   }
}
