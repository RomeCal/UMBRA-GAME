#pragma once
#include "CommonValues.h"

// Owner: R3.  Vertex layout: position(3) uv(2) normal(3).  Every draw is glDrawElements (B2).
class Mesh
{
public:
    Mesh() : vao(0), vbo(0), ibo(0), indexCount(0), vertexCount(0) {}
    ~Mesh() { Clear(); }
    void Create(const std::vector<GLfloat>& vertices, const std::vector<GLuint>& indices);
    void Render() const;
    void Clear();
    int VertexCount() const { return vertexCount; }
    int IndexCount() const { return indexCount; }

    // Generated shapes (unit size, centred on the origin, so scale == size in world units)
    static Mesh* MakeBox();        // 24 vertices: 4 per face, because each face has its own normal
    static Mesh* MakePlane();      // 4 vertices, lies in the XZ plane, normal +Y
    static Mesh* MakeCylinder(int segments = 24);
    static Mesh* MakePyramid();    // 16 vertices: 3 per side face + 4 for the base

private:
    GLuint vao, vbo, ibo;
    int indexCount, vertexCount;
    Mesh(const Mesh&); Mesh& operator=(const Mesh&);
};
