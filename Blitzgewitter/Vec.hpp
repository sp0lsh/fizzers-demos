/* Copyright(c) Edd Biddulph */

// geometric vectors

#pragma once


template<typename T>
struct TVec3_NC
{
   typedef T ElementType;

   union
   {
      struct { T x,y,z; };
      T e[3];
   };

   static TVec3_NC<T> cstr(const T x, const T y, const T z)
   {
      TVec3_NC<T> vec;
      vec.x=x; vec.y=y; vec.z=z;
      return vec;
   }

   T lengthSquared() const { return x*x + y*y + z*z; }

   const T& operator[](const int index) const { return e[index]; }
   T& operator[](const int index) { return e[index]; }

   T dot(const TVec3_NC<T> vec) const { return x*vec.x + y*vec.y + z*vec.z; }

   TVec3_NC<T> cross(const TVec3_NC<T> vec) const
   {
      return cstr( y*vec.z - z*vec.y, z*vec.x - x*vec.z, x*vec.y - y*vec.x );
   }

   TVec3_NC<T> operator-() const { return cstr(-x, -y, -z); }

   TVec3_NC<T> operator+(const TVec3_NC<T> vec) const { return cstr(x+vec.x, y+vec.y, z+vec.z); }
   TVec3_NC<T> operator-(const TVec3_NC<T> vec) const { return cstr(x-vec.x, y-vec.y, z-vec.z); }
   TVec3_NC<T> operator*(const TVec3_NC<T> vec) const { return cstr(x*vec.x, y*vec.y, z*vec.z); }
   TVec3_NC<T> operator/(const TVec3_NC<T> vec) const { return cstr(x/vec.x, y/vec.y, z/vec.z); }

   TVec3_NC<T> operator*(const T val) const { return cstr(x*val, y*val, z*val); }
   TVec3_NC<T> operator/(const T val) const { return cstr(x/val, y/val, z/val); }

   void operator+=(const TVec3_NC<T> vec) { x+=vec.x; y+=vec.y; z+=vec.z; }
   void operator-=(const TVec3_NC<T> vec) { x-=vec.x; y-=vec.y; z-=vec.z; }
   void operator*=(const TVec3_NC<T> vec) { x*=vec.x; y*=vec.y; z*=vec.z; }
   void operator/=(const TVec3_NC<T> vec) { x/=vec.x; y/=vec.y; z/=vec.z; }
};

template<typename T> static TVec3_NC<T> operator*(T lhs, TVec3_NC<T> vec) { return vec * lhs; }


template<typename T>
struct TVec3: TVec3_NC<T>
{
   typedef TVec3_NC<T> NoConstructors;

   TVec3() { }

   template<typename U>
   TVec3(const TVec3<U> vec) { this->x=vec.x; this->y=vec.y; this->z=vec.z; }

   template<typename U>
   TVec3(const TVec3_NC<U> vec) { this->x=vec.x; this->y=vec.y; this->z=vec.z; }

   TVec3(const T x, const T y, const T z) { this->x=x; this->y=y; this->z=z; }
   TVec3(const T val) { this->x=this->y=this->z=val; }
};



template<typename T>
struct TVec2_NC
{
   typedef T ElementType;

   union
   {
      struct { T x,y; };
      T e[2];
   };

   static TVec2_NC<T> cstr(const T x, const T y)
   {
      TVec2_NC<T> vec;
      vec.x=x; vec.y=y;
      return vec;
   }

   T lengthSquared() const { return x*x + y*y; }

   const T& operator[](const int index) const { return e[index]; }
   T& operator[](const int index) { return e[index]; }

   T dot(const TVec2_NC<T> vec) const { return x*vec.x + y*vec.y; }

   TVec2_NC<T> operator-() const { return cstr(-x, -y); }

   TVec2_NC<T> operator+(const TVec2_NC<T> vec) const { return cstr(x+vec.x, y+vec.y); }
   TVec2_NC<T> operator-(const TVec2_NC<T> vec) const { return cstr(x-vec.x, y-vec.y); }
   TVec2_NC<T> operator*(const TVec2_NC<T> vec) const { return cstr(x*vec.x, y*vec.y); }
   TVec2_NC<T> operator/(const TVec2_NC<T> vec) const { return cstr(x/vec.x, y/vec.y); }

   TVec2_NC<T> operator*(const T val) const { return cstr(x*val, y*val); }
   TVec2_NC<T> operator/(const T val) const { return cstr(x/val, y/val); }

   void operator+=(const TVec2_NC<T> vec) { x+=vec.x; y+=vec.y; }
   void operator-=(const TVec2_NC<T> vec) { x-=vec.x; y-=vec.y; }
   void operator*=(const TVec2_NC<T> vec) { x*=vec.x; y*=vec.y; }
   void operator/=(const TVec2_NC<T> vec) { x/=vec.x; y/=vec.y; }
};

template<typename T> static TVec2_NC<T> operator*(T lhs, TVec2_NC<T> vec) { return vec * lhs; }

template<typename T>
struct TVec2: TVec2_NC<T>
{
   typedef TVec2_NC<T> NoConstructors;

   TVec2() { }

   template<typename U>
   TVec2(const TVec2<U> vec) { this->x=vec.x; this->y=vec.y; }

   template<typename U>
   TVec2(const TVec2_NC<U> vec) { this->x=vec.x; this->y=vec.y; }

   TVec2(const T x, const T y) { this->x=x; this->y=y; }
   TVec2(const T val) { this->x=this->y=val; }
};



