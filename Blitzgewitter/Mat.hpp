/* Copyright(c) Edd Biddulph */

// matrices

#pragma once

#include "Vec.hpp"
#include <cmath>
#include <cassert>

template<typename T>
struct TMat4
{
   typedef T ElementType;

   T e[16];

   const T& operator[](const int index) const { return e[index]; }
   T& operator[](const int index) { return e[index]; }

   TMat4( T e00, T e04, T e08, T e12,
          T e01, T e05, T e09, T e13,
          T e02, T e06, T e10, T e14,
          T e03, T e07, T e11, T e15 )
   {
      e[ 0]=e00; e[ 4]=e04; e[ 8]=e08; e[12]=e12;
      e[ 1]=e01; e[ 5]=e05; e[ 9]=e09; e[13]=e13;
      e[ 2]=e02; e[ 6]=e06; e[10]=e10; e[14]=e14;
      e[ 3]=e03; e[ 7]=e07; e[11]=e11; e[15]=e15;
   }

   TMat4()
   {
   }

   TVec3<T> column(const int i) const
   {
      const int j = i << 2;
      return TVec3<T>( e[j+0], e[j+1], e[j+2] );
   }

   static TMat4 identity()
   {
      TMat4 m;

      m[ 0]=1; m[ 1]=0; m[ 2]=0; m[ 3]=0;
      m[ 4]=0; m[ 5]=1; m[ 6]=0; m[ 7]=0;
      m[ 8]=0; m[ 9]=0; m[10]=1; m[11]=0;
      m[12]=0; m[13]=0; m[14]=0; m[15]=1;

      return m;
   }

   template<typename U, typename V>
   static TMat4 rotation(const V angle, const TVec3<U> axis)
   {
      TMat4 m;

      const U &x=axis.x, &y=axis.y, &z=axis.z;
      const T c=std::cos(angle), s=std::sin(angle);

      m[ 0]=x*x*(T(1)-c)+c;      m[ 4]=x*y*(T(1)-c)-z*s;   m[ 8]=x*z*(T(1)-c)+y*s;    m[12]=0;
      m[ 1]=y*x*(T(1)-c)+z*s;    m[ 5]=y*y*(T(1)-c)+c;     m[ 9]=y*z*(T(1)-c)-x*s;    m[13]=0;
      m[ 2]=z*x*(T(1)-c)-y*s;    m[ 6]=z*y*(T(1)-c)+x*s;   m[10]=z*z*(T(1)-c)+c;      m[14]=0;
      m[ 3]=0;                   m[ 7]=0;                  m[11]=0;                   m[15]=1;

      return m;
   }

   template<typename U>
   static TMat4 frustum(U left, U right,
               U bottom, U top, U nearVal, U farVal)
   {
      TMat4 m;

      const T A = (right+left)/(right-left), B = (top+bottom)/(top-bottom),
              C = -(farVal+nearVal)/(farVal-nearVal), D = -(T(2)*farVal*nearVal)/(farVal-nearVal);

      m[ 0]=(T(2)*nearVal)/(right-left);   m[ 4]=0;                           m[ 8]=A;    m[12]=0;
      m[ 1]=0;                             m[ 5]=(T(2)*nearVal)/(top-bottom); m[ 9]=B;    m[13]=0;
      m[ 2]=0;                             m[ 6]=0;                           m[10]=C;    m[14]=D;
      m[ 3]=0;                             m[ 7]=0;                           m[11]=-1;   m[15]=0;

      return m;
   }

   template<typename U>
   static TMat4 scale(const TVec3<U> vec)
   {
      TMat4 m=identity();

      m[ 0]=vec.x;
      m[ 5]=vec.y;
      m[10]=vec.z;

      return m;
   }

   template<typename U>
   static TMat4 translation(const TVec3<U> vec)
   {
      TMat4 m=identity();

      m[12]=vec.x;
      m[13]=vec.y;
      m[14]=vec.z;

      return m;
   }

   TMat4 transpose() const
   {
      TMat4 m;

      m[ 0]=e[ 0]; m[ 1]=e[ 4]; m[ 2]=e[ 8]; m[ 3]=e[12];
      m[ 4]=e[ 1]; m[ 5]=e[ 5]; m[ 6]=e[ 9]; m[ 7]=e[13];
      m[ 8]=e[ 2]; m[ 9]=e[ 6]; m[10]=e[10]; m[11]=e[14];
      m[12]=e[ 3]; m[13]=e[ 7]; m[14]=e[11]; m[15]=e[15];

      return m;
   }

   template<typename U>
   static U determinant3x3(U e0, U e3, U e6,
                           U e1, U e4, U e7,
                           U e2, U e5, U e8)
   {
      return e0 * (e4*e8 - e5*e7) - e3 * (e1*e8 - e2*e7) + e6 * (e1*e5 - e2*e4);
   }

   const T& get(const int c, const int r) const
   {
      return e[((c & 3) << 2) + (r & 3)];
   }

   T& get(const int c, const int r)
   {
      return e[((c & 3) << 2) + (r & 3)];
   }

   TMat4 inverse() const
   {
      TMat4 m;

      const T d=determinant();

      assert(d!=T(0));

      const T inv_d = T(1) / d;

      for(int c=0; c<4; ++c)
         for(int r=0; r<4; ++r)
         {
            const T s = ((r^c) & 1) ? T(-1) : T(+1);

            m.get(c,r) = inv_d * determinant3x3( get(c+1, r+1), get(c+2, r+1), get(c+3, r+1),
                                                 get(c+1, r+2), get(c+2, r+2), get(c+3, r+2),
                                                 get(c+1, r+3), get(c+2, r+3), get(c+3, r+3) ) * s;
         }

      return m.transpose();
   }

   //  0  4  8 12
   //  1  5  9 13
   //  2  6 10 14
   //  3  7 11 15

   T determinant() const
   {
      const T c0=+(e[ 5] * (e[10]*e[15] - e[11]*e[14]) -
                   e[ 9] * (e[ 6]*e[15] - e[ 7]*e[14]) +
                   e[13] * (e[ 6]*e[11] - e[ 7]*e[10]) );

      const T c1=-(e[ 1] * (e[10]*e[15] - e[11]*e[14]) -
                   e[ 9] * (e[ 2]*e[15] - e[ 3]*e[14]) +
                   e[13] * (e[ 2]*e[11] - e[ 3]*e[10]) );

      const T c2=+(e[ 1] * (e[ 6]*e[15] - e[ 7]*e[14]) -
                   e[ 5] * (e[ 2]*e[15] - e[ 3]*e[14]) +
                   e[13] * (e[ 2]*e[ 7] - e[ 3]*e[ 6]) );

      const T c3=-(e[ 1] * (e[ 6]*e[11] - e[ 7]*e[10]) -
                   e[ 5] * (e[ 2]*e[11] - e[ 3]*e[10]) +
                   e[ 9] * (e[ 2]*e[ 7] - e[ 3]*e[ 6]) );

      return e[ 0]*c0 + e[ 4]*c1 + e[ 8]*c2 + e[12]*c3;
   }

   template<typename U>
   TMat4 operator*(const TMat4<U> rhs) const
   {
      TMat4 m;

      m[ 0]=e[ 0]*rhs[ 0] + e[ 4]*rhs[ 1] + e[ 8]*rhs[ 2] + e[12]*rhs[ 3];
      m[ 1]=e[ 1]*rhs[ 0] + e[ 5]*rhs[ 1] + e[ 9]*rhs[ 2] + e[13]*rhs[ 3];
      m[ 2]=e[ 2]*rhs[ 0] + e[ 6]*rhs[ 1] + e[10]*rhs[ 2] + e[14]*rhs[ 3];
      m[ 3]=e[ 3]*rhs[ 0] + e[ 7]*rhs[ 1] + e[11]*rhs[ 2] + e[15]*rhs[ 3];

      m[ 4]=e[ 0]*rhs[ 4] + e[ 4]*rhs[ 5] + e[ 8]*rhs[ 6] + e[12]*rhs[ 7];
      m[ 5]=e[ 1]*rhs[ 4] + e[ 5]*rhs[ 5] + e[ 9]*rhs[ 6] + e[13]*rhs[ 7];
      m[ 6]=e[ 2]*rhs[ 4] + e[ 6]*rhs[ 5] + e[10]*rhs[ 6] + e[14]*rhs[ 7];
      m[ 7]=e[ 3]*rhs[ 4] + e[ 7]*rhs[ 5] + e[11]*rhs[ 6] + e[15]*rhs[ 7];

      m[ 8]=e[ 0]*rhs[ 8] + e[ 4]*rhs[ 9] + e[ 8]*rhs[10] + e[12]*rhs[11];
      m[ 9]=e[ 1]*rhs[ 8] + e[ 5]*rhs[ 9] + e[ 9]*rhs[10] + e[13]*rhs[11];
      m[10]=e[ 2]*rhs[ 8] + e[ 6]*rhs[ 9] + e[10]*rhs[10] + e[14]*rhs[11];
      m[11]=e[ 3]*rhs[ 8] + e[ 7]*rhs[ 9] + e[11]*rhs[10] + e[15]*rhs[11];

      m[12]=e[ 0]*rhs[12] + e[ 4]*rhs[13] + e[ 8]*rhs[14] + e[12]*rhs[15];
      m[13]=e[ 1]*rhs[12] + e[ 5]*rhs[13] + e[ 9]*rhs[14] + e[13]*rhs[15];
      m[14]=e[ 2]*rhs[12] + e[ 6]*rhs[13] + e[10]*rhs[14] + e[14]*rhs[15];
      m[15]=e[ 3]*rhs[12] + e[ 7]*rhs[13] + e[11]*rhs[14] + e[15]*rhs[15];

      return m;
   }

   template<typename U>
   void transform(TVec3<U>& vec) const
   {
      vec=operator*(vec);
   }

   template<typename U>
   void transform3x3(TVec3<U>& vec) const
   {
      vec=TVec3<U>( e[0]*vec[0] + e[4]*vec[1] + e[ 8]*vec[2],
                    e[1]*vec[0] + e[5]*vec[1] + e[ 9]*vec[2],
                    e[2]*vec[0] + e[6]*vec[1] + e[10]*vec[2] );
   }

   template<typename U>
   TVec3<U> operator*(const TVec3<U> vec) const
   {
      const U w = U(1) / ( e[3]*vec[0] + e[7]*vec[1] + e[11]*vec[2] + e[15] );

      return TVec3<U>( ( e[0]*vec[0] + e[4]*vec[1] + e[ 8]*vec[2] + e[12] ) * w,
                       ( e[1]*vec[0] + e[5]*vec[1] + e[ 9]*vec[2] + e[13] ) * w,
                       ( e[2]*vec[0] + e[6]*vec[1] + e[10]*vec[2] + e[14] ) * w );
   }
};



