//
//  Drawer3D.cpp
//  Draw3D
//
//  Created by etienne cella on 2016-06-11.
//
//

#include "Drawer3D.hpp"

void Drawer3D::moveTo(const ofVec3f& position)
{
    join = false;
    spline.pushPoint(ofVec4f(position.x, position.y, position.z, radius));
    store();
    needsUpdate = true;
}

void Drawer3D::lineTo(const ofVec3f& position)
{
    spline.pushPoint(ofVec4f(position.x, position.y, position.z, radius));
    if (spline.getLength() >= numPointsPerVbo / curveResolution)
    {
        join = true;
        store();
    }
    needsUpdate = true;
}

void Drawer3D::setRadius(float v)
{
    radius = v;
}

void Drawer3D::setCurveResolution(float v)
{
    curveResolution = v;
}

void Drawer3D::setRadialResolution(int n)
{
    radialResolution = n;
    // trigger buffers resize and indices update
    setNumPointsPerVbo(numPointsPerVbo);
}

void Drawer3D::setNumPointsPerVbo(int n)
{
    numPointsPerVbo = n;
    vertices.resize(numPointsPerVbo * radialResolution);
    normals.resize(vertices.size());
    computeIndices();
}

void Drawer3D::update()
{
    if (!needsUpdate) return;
    
    // compute vertices and normals
    const auto dAngle = 360.0f / (float)radialResolution;
    const auto numPoints = numPointsPerVbo - (join ? 1 : 0);//std::min((int)ceil(spline.getLength() * curveResolution), numPointsPerVbo);
    auto nVertex = join ? radialResolution : 0;
    prevNormal = vboPrevNormal;
    prevTangent = vboPrevTangent;
    
    for (int i = 0; i < numPoints; ++i)
    {
        constexpr auto dt = .01f;
        const auto t = (float)i / (float)(numPoints - 1);
        const auto a = spline.getSampleAtLength(spline.getLength() * std::max(.0f, t - dt));
        const auto b = spline.getSampleAtLength(spline.getLength() * std::min(1.0f, t + dt));
        auto tangent = ofVec3f(a) - ofVec3f(b);
        tangent.normalize();
        
        ofVec3f normal;
        if (i == 0 && !join)
        {
            normal = tangent;
            normal.cross(ofVec3f(0, 0, 1));
        }
        else
        {
            auto perp = prevTangent;
            perp.cross(tangent);
            float cosBetweenTangents = tangent.dot(prevTangent);
            float angle = atan2(perp.length(), cosBetweenTangents);
            normal = ofQuaternion(angle * RAD_TO_DEG, perp) * prevNormal;
        }
        
        normal.normalize();
        prevNormal = normal;
        prevTangent = tangent;
        
        const auto position = spline.getSampleAtLength(spline.getLength() * t);
        
        for (size_t j = 0; j < radialResolution; ++j)
        {
            vertices[nVertex] = ofVec3f(position.x, position.y, position.z) +
                ofQuaternion((float)j * dAngle, tangent) * normal * position.w;
            
            auto n = vertices[nVertex] - position;
            n.normalize();
            normals[nVertex] = n;
            
            ++nVertex;
        }
    }
    
    usedVertices = nVertex;

    // sync current vbo
    if (vbos.size() > 0)
    {
        vbos.back().updateVertexData(&vertices[0], vertices.size());
        vbos.back().updateNormalData(&normals[0], normals.size());
    }
    
    needsUpdate = false;
}

void Drawer3D::draw()
{
    for (auto i = 0; i < vbos.size() - 1; ++i)
    {
        vbos[i].drawElements(GL_TRIANGLES, indicesMemory[i]);
    }
    if (vbos.size() > 0 && spline.getLength() > .01f) vbos.back().drawElements(GL_TRIANGLES, indices.size());
}

void Drawer3D::computeIndices()
{
    const auto indicesCount = (numPointsPerVbo - 1) * radialResolution * 2 * 3;
    indices.resize(indicesCount);
    auto index = 0;
    
    for(auto i = 0; i < numPointsPerVbo - 1; ++i)
    {
        const auto n = i * radialResolution;
        const auto m = n + radialResolution;
        
        for (size_t j = 0; j < radialResolution; ++j)
        {
            // Tri 1
            indices[index++] = n + j;
            indices[index++] = n + ((j + 1) %  radialResolution);
            indices[index++] = m + j;
            // Tri2
            indices[index++] = n + ((j + 1) %  radialResolution);
            indices[index++] = m + ((j + 1) %  radialResolution);
            indices[index++] = m + j;
        }
    }
    if (vbos.size() > 0)
    {
        vbos.back().setIndexData(&indices[0], usedVertices * 3, GL_STATIC_DRAW);
    }
}

void Drawer3D::store()
{
    if (vbos.size() > 0)
    {
        //if (needsUpdate) update();
        vbos.back().clear();
        auto nIndices = usedVertices * 2 * 3;
        vbos.back().setIndexData(&indices[0], nIndices, GL_STATIC_DRAW);
        vbos.back().setVertexData(&vertices[0], usedVertices, GL_STATIC_DRAW);
        vbos.back().setNormalData(&normals[0], usedVertices, GL_STATIC_DRAW);
        // remember indices count for draw calls
        indicesMemory.push_back(nIndices);
    }
    
    vboPrevTangent = prevTangent;
    vboPrevNormal = prevNormal;
    
    spline.reset();
    usedVertices = 0;
    
    if (join)
    {
        for (auto i = 0; i < radialResolution; ++i)
        {
            vertices[i] = vertices[vertices.size() - radialResolution + i];
            normals[i] = normals[normals.size() - radialResolution + i];
        }
    }
    
    std::fill(vertices.begin() + (join ? radialResolution : 0), vertices.end(), ofVec3f::zero());
    std::fill(normals.begin() + (join ? radialResolution : 0), normals.end(), ofVec3f::zero());
    
    vbos.push_back(ofVbo());
    vbos.back().setIndexData(&indices[0], indices.size(), GL_STATIC_DRAW);
    vbos.back().setVertexData(&vertices[0], vertices.size(), GL_DYNAMIC_DRAW);
    vbos.back().setNormalData(&normals[0], normals.size(), GL_DYNAMIC_DRAW);
}