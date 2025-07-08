

template<uint i>
struct Factorial
{
   static const uint value = i * Factorial<i - 1>::value;
};


template<>
struct Factorial<0>
{
   static const uint value = 1;
};


template<uint n, uint i>
struct BinomialCoefficient
{
   static const uint value = Factorial<n>::value / (Factorial<i>::value * Factorial<n - i>::value);
};


template<uint n, uint i>
struct BernsteinPolynomial
{
   static Real value(const Real t)
   {
      return Real(BinomialCoefficient<n, i>::value) * std::pow(1.0f - t, n - i) * std::pow(t, i);
   }
};


template<typename T, uint n, uint i>
struct BezierCurveEvaluate
{
   static T evaluate(const Real t, const T *const control_points)
   {
      return control_points[i] * BernsteinPolynomial<n, i>::value(t) +
                  BezierCurveEvaluate<T, n, i - 1>::evaluate(t, control_points);
   }
};


template<typename T, uint n>
struct BezierCurveEvaluate<T, n, 0>
{
   static T evaluate(const Real t, const T *const control_points)
   {
      return control_points[0] * BernsteinPolynomial<n, 0>::value(t);
   }
};

template<typename T, uint n>
struct BezierCurve
{
   typedef T ControlPoint;
   static const uint control_point_count = n + 1;

   T control_points[control_point_count];

   T operator()(const Real t) const
   {
      return BezierCurveEvaluate<T, n, n>::evaluate(t, control_points);
   }
};







