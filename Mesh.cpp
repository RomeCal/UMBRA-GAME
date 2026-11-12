#include "Mesh.h"

void Mesh::Create(const std::vector<GLfloat>& v, const std::vector<GLuint>& idx)
{
    Clear();
    indexCount = (int)idx.size();
    vertexCount = (int)(v.size() / 8);
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(GLuint), idx.data(), GL_STATIC_DRAW);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(GLfloat), v.data(), GL_STATIC_DRAW);
    const GLsizei stride = 8 * sizeof(GLfloat);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Mesh::Render() const
{
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
}

void Mesh::Clear()
{
    if (ibo) glDeleteBuffers(1, &ibo);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    vao = vbo = ibo = 0; indexCount = vertexCount = 0;
}

static void PushVertex(std::vector<GLfloat>& v, glm::vec3 p, glm::vec2 uv, glm::vec3 n)
{
    v.push_back(p.x);  v.push_back(p.y);  v.push_back(p.z);
    v.push_back(uv.x); v.push_back(uv.y);
    v.push_back(n.x);  v.push_back(n.y);  v.push_back(n.z);
}

Mesh* Mesh::MakeBox()
{
    // face normal, then the two in-face axes (u x v == normal, so the winding is counter-clockwise)
    const glm::vec3 F[6][3] = {
        { { 1, 0, 0}, { 0, 0,-1}, { 0, 1, 0} },
        { {-1, 0, 0}, { 0, 0, 1}, { 0, 1, 0} },
        { { 0, 1, 0}, { 1, 0, 0}, { 0, 0,-1} },
        { { 0,-1, 0}, { 1, 0, 0}, { 0, 0, 1} },
        { { 0, 0, 1}, { 1, 0, 0}, { 0, 1, 0} },
        { { 0, 0,-1}, {-1, 0, 0}, { 0, 1, 0} } };
    const float S[4][2] = { {-1,-1}, {1,-1}, {1,1}, {-1,1} };
    std::vector<GLfloat> v; std::vector<GLuint> idx;
    for (int f = 0; f < 6; ++f)
    {
        GLuint base = (GLuint)(v.size() / 8);
        for (int c = 0; c < 4; ++c)
        {
            glm::vec3 p = F[f][0] * 0.5f + F[f][1] * (S[c][0] * 0.5f) + F[f][2] * (S[c][1] * 0.5f);
            PushVertex(v, p, glm::vec2((S[c][0] + 1) * 0.5f, (S[c][1] + 1) * 0.5f), F[f][0]);
        }
        idx.push_back(base); idx.push_back(base + 1); idx.push_back(base + 2);
        idx.push_back(base); idx.push_back(base + 2); idx.push_back(base + 3);
    }
    Mesh* m = new Mesh(); m->Create(v, idx); return m;
}

Mesh* Mesh::MakePlane()
{
    std::vector<GLfloat> v; std::vector<GLuint> idx;
    glm::vec3 n(0, 1, 0);
    PushVertex(v, glm::vec3(-0.5f, 0,  0.5f), glm::vec2(0, 0), n);
    PushVertex(v, glm::vec3( 0.5f, 0,  0.5f), glm::vec2(1, 0), n);
    PushVertex(v, glm::vec3( 0.5f, 0, -0.5f), glm::vec2(1, 1), n);
    PushVertex(v, glm::vec3(-0.5f, 0, -0.5f), glm::vec2(0, 1), n);
    idx = { 0, 1, 2, 0, 2, 3 };
    Mesh* m = new Mesh(); m->Create(v, idx); return m;
}

Mesh* Mesh::MakeCylinder(int seg)
{
    std::vector<GLfloat> v; std::vector<GLuint> idx;
    // side: two vertices per step around, with a sideways normal (+1 extra pair closes the uv seam)
    for (int i = 0; i <= seg; ++i)
    {
        float a = 2.0f * 3.14159265f * (float)i / (float)seg;
        float x = std::cos(a), z = std::sin(a);
        PushVertex(v, glm::vec3(0.5f * x, -0.5f, 0.5f * z), glm::vec2((float)i / seg, 0), glm::vec3(x, 0, z));
        PushVertex(v, glm::vec3(0.5f * x,  0.5f, 0.5f * z), glm::vec2((float)i / seg, 1), glm::vec3(x, 0, z));
    }
    for (int i = 0; i < seg; ++i)
    {
        GLuint a = 2 * i, b = 2 * i + 1, c = 2 * i + 2, d = 2 * i + 3;
        idx.push_back(a); idx.push_back(b); idx.push_back(c);
        idx.push_back(b); idx.push_back(d); idx.push_back(c);
    }
    // caps: own centre + ring so the normal is straight up / down
    for (int cap = 0; cap < 2; ++cap)
    {
        float y = cap == 0 ? 0.5f : -0.5f;
        glm::vec3 n(0, cap == 0 ? 1.f : -1.f, 0);
        GLuint centre = (GLuint)(v.size() / 8);
        PushVertex(v, glm::vec3(0, y, 0), glm::vec2(0.5f, 0.5f), n);
        GLuint ring = (GLuint)(v.size() / 8);
        for (int i = 0; i <= seg; ++i)
        {
            float a = 2.0f * 3.14159265f * (float)i / (float)seg;
            PushVertex(v, glm::vec3(0.5f * std::cos(a), y, 0.5f * std::sin(a)),
                       glm::vec2(0.5f + 0.5f * std::cos(a), 0.5f + 0.5f * std::sin(a)), n);
        }
        for (int i = 0; i < seg; ++i)
        {
            if (cap == 0) { idx.push_back(centre); idx.push_back(ring + i + 1); idx.push_back(ring + i); }
            else          { idx.push_back(centre); idx.push_back(ring + i);     idx.push_back(ring + i + 1); }
        }
    }
    Mesh* m = new Mesh(); m->Create(v, idx); return m;
}

Mesh* Mesh::MakePyramid()
{
    std::vector<GLfloat> v; std::vector<GLuint> idx;
    glm::vec3 A(0, 0.5f, 0);
    glm::vec3 P[4] = { {-0.5f,-0.5f, 0.5f}, {0.5f,-0.5f, 0.5f}, {0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f} };
    for (int i = 0; i < 4; ++i)
    {
        glm::vec3 p0 = P[i], p1 = P[(i + 1) % 4];
        glm::vec3 n = glm::normalize(glm::cross(p1 - p0, A - p0));
        GLuint base = (GLuint)(v.size() / 8);
        PushVertex(v, p0, glm::vec2(0, 0), n);
        PushVertex(v, p1, glm::vec2(1, 0), n);
        PushVertex(v, A,  glm::vec2(0.5f, 1), n);
        idx.push_back(base); idx.push_back(base + 1); idx.push_back(base + 2);
    }
    GLuint base = (GLuint)(v.size() / 8);
    glm::vec3 dn(0, -1, 0);
    PushVertex(v, P[0], glm::vec2(0, 0), dn);
    PushVertex(v, P[3], glm::vec2(0, 1), dn);
    PushVertex(v, P[2], glm::vec2(1, 1), dn);
    PushVertex(v, P[1], glm::vec2(1, 0), dn);
    idx.push_back(base); idx.push_back(base + 1); idx.push_back(base + 2);
    idx.push_back(base); idx.push_back(base + 2); idx.push_back(base + 3);
    Mesh* m = new Mesh(); m->Create(v, idx); return m;
}
