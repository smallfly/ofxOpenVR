//
//  Drawer3D.hpp
//  Draw3D
//
//  Created by etienne cella on 2016-06-11.
//
//

#pragma once

#include <vector>
#include <algorithm>
#include "ofMath.h"
#include "ofQuaternion.h"
#include "ofVbo.h"
#include "Spline.hpp"

class Drawer3D
{
public:
    
    void moveTo(const ofVec3f& position);
    void lineTo(const ofVec3f& position);
    void setRadius(float v);
    void setCurveResolution(float v);
    void setRadialResolution(int n);
    void setNumPointsPerVbo(int n);
    void update();
    void draw();

private:
    
    void store();
    void computeIndices();
    
    float curveResolution{1.0f};
    float radius{1.0f};
    int radialResolution{6};
    int numPointsPerVbo{128};
    int usedVertices{0};
    Spline<ofVec4f, ofVec3f> spline;
    std::vector<ofVbo> vbos;
    std::vector<int> indicesMemory;
    std::vector<ofIndexType> indices;
    std::vector<ofVec3f> vertices;
    std::vector<ofVec3f> normals;
    ofVec3f prevNormal;
    ofVec3f prevTangent;
    ofVec3f vboPrevNormal;
    ofVec3f vboPrevTangent;
    bool needsUpdate{false};
    bool join{false};
};