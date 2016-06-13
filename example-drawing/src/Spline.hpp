//
//  Spline.hpp
//  Draw3D
//
//  Created by etienne cella on 2016-06-11.
//
//

#pragma once

#include <cstring>

// T0 is used for curve point type, T1 is used for arc length parameterization
// (you might want to use lower dimensions for length)
template<typename T0, typename T1>
class Spline
{
public:
    void reset();
    float getLength() const noexcept;
    void pushPoint(T0 pt);
    T0 getSampleAtLength(float length);
    
private:
    
    T0 interpolate(T0 p0, T0 p1, T0 p2, T0 p3, float t);
    float computeArcLength(T1 p0, T1 p1, T1 p2, T1 p3, size_t steps);
    std::vector<T0> points;
    std::vector<float> arcLengthLookup;
    float curveLength{.0f};
};

template<typename T0, typename T1>
void Spline<T0, T1>::reset()
{
    points.clear();
    arcLengthLookup.clear();
    curveLength = .0f;
}

template<typename T0, typename T1>
float Spline<T0, T1>::getLength() const noexcept { return curveLength; }

template<typename T0, typename T1>
void Spline<T0, T1>::pushPoint(T0 pt)
{
    points.push_back(pt);
    
    if (points.size() > 3)
    {
        // update arc length lookup table
        arcLengthLookup.push_back(computeArcLength(
           T1(points[points.size() - 4]),
           T1(points[points.size() - 3]),
           T1(points[points.size() - 2]),
           T1(points[points.size() - 1]), 50));
        
        // recompute curve length
        curveLength = .0f;
        for (auto& d : arcLengthLookup )
        {
            curveLength += d;
        }
    }
}

template<typename T0, typename T1>
T0 Spline<T0, T1>::getSampleAtLength(float length)
{
    if (arcLengthLookup.size() == 0) return T0();
    
    float l = .0f;
    auto arcLength = .0f;
    auto index = 0;
    for (auto& d : arcLengthLookup)
    {
        ++index;
        arcLength = d;
        if (l + arcLength > length) break;
        l += arcLength;
    }
    
    float t = (length - l) / arcLength;
    return interpolate(points[index - 1],
                       points[index],
                       points[index + 1],
                       points[index + 2], t);
}

template<typename T0, typename T1>
T0 Spline<T0, T1>::interpolate(T0 p0, T0 p1, T0 p2, T0 p3, float t)
{
    return 0.5f * ((p1 * 2.0f) + (p2 - p0) * t + \
                   (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t * t + \
                   (p1 * 3.0f + p3 - p0 - p2 * 3.0f) * t * t * t);
}

template<typename T0, typename T1>
float Spline<T0, T1>::computeArcLength(T1 p0, T1 p1, T1 p2, T1 p3, size_t steps)
{
    auto l = .0f;
    auto p = p1;
    for (size_t i = 0; i < steps; ++i)
    {
        auto pn = interpolate(p0, p1, p2, p3, (float)i / (float)(steps - 1));
        l += (pn - p).length();
        p = pn;
    }
    return l;
}

