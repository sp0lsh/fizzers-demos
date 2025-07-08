#pragma once

#include "Engine.hpp"
#include <map>
#include <string>
#include <vector>

namespace tower
{

struct ParameterRange
{
   ParameterRange()
   {
   }

   ParameterRange(Real x0, Real x1)
   {
      this->x0 = x0;
      this->x1 = x1;
   }

   ParameterRange(Real x)
   {
      this->x0 = x;
      this->x1 = x;
   }

   Real x0, x1;
};

struct PieceType
{
   std::map<std::string, ParameterRange> parameters;
   Real weight, cumulative_weight;
};

struct Section
{
   std::vector<PieceType> types;
   Real height0, height1, radius;
   unsigned int num_instances;
};

struct PieceInstance
{
   Section* section;
   std::map<std::string, Real> parameters;
};

struct Tower
{
   std::vector<Section> sections;
   std::vector<PieceInstance> instances;

   void genInstances();
};

}
