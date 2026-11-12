#pragma once
#include "CommonValues.h"

// Periodic value noise helpers (shared by procedural textures and the sky).  period 0 = no wrap.
float Noise2D(float x, float y, int period = 0);
float Fbm2D(float x, float y, int period = 0);

// Owner: R3.  Textures come either from an uncompressed .tga file or from a
// procedural recipe named in the scene file, so the project needs no image library.
class Texture
{
public:
    Texture() : id(0), width(0), height(0) {}
    ~Texture() { Clear(); }
    bool LoadTGA(const std::string& path, std::string& error);
    // Decodes an uncompressed TGA into RGBA bytes. bottomRowFirst = true for 2D textures (OpenGL's row 0 is the
    // bottom); false for cube-map faces (row 0 is the TOP of the picture, the cube-map convention).
    static bool DecodeTGA(const std::string& path, std::vector<unsigned char>& rgba, int& w, int& h,
                          bool bottomRowFirst, std::string& error);
    void GenerateProcedural(const std::string& type, glm::vec3 c1, glm::vec3 c2, int size);
    void GenerateMissing();                // magenta/black checker: "an asset is missing" is visible
    void Use(int unit) const;
    void Clear();
    int Width() const { return width; }
    int Height() const { return height; }
private:
    void Upload(const unsigned char* rgba, int w, int h);
    GLuint id; int width, height;
    Texture(const Texture&); Texture& operator=(const Texture&);
};
