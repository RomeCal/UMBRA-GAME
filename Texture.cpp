#include "Texture.h"

static float Hash2(int x, int y)
{
    unsigned int h = (unsigned int)x * 374761393u + (unsigned int)y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= (h >> 16);
    return (float)(h & 0xFFFFFF) / (float)0xFFFFFF;
}

float Noise2D(float x, float y, int period)
{
    int ix = (int)std::floor(x), iy = (int)std::floor(y);
    float fx = x - (float)ix, fy = y - (float)iy;
    fx = fx * fx * (3.f - 2.f * fx);
    fy = fy * fy * (3.f - 2.f * fy);
    auto h = [&](int a, int b) {
        if (period > 0) { a = ((a % period) + period) % period; b = ((b % period) + period) % period; }
        return Hash2(a, b);
    };
    float a = h(ix, iy), b = h(ix + 1, iy), c = h(ix, iy + 1), d = h(ix + 1, iy + 1);
    return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
}

float Fbm2D(float x, float y, int period)
{
    float sum = 0.f, amp = 0.5f, norm = 0.f;
    for (int o = 0; o < 4; ++o)
    {
        sum += amp * Noise2D(x, y, period);
        norm += amp;
        x *= 2.f; y *= 2.f; amp *= 0.5f;
        if (period > 0) period *= 2;
    }
    return sum / norm;
}

static glm::vec3 Pattern(const std::string& type, int px, int py, int size, glm::vec3 c1, glm::vec3 c2)
{
    float u = (float)px / (float)size, v = (float)py / (float)size;
    float n = Fbm2D(u * 8.f, v * 8.f, 8);
    glm::vec3 c;
    if (type == "checker")
    {
        int cx = (int)(u * 8.f), cy = (int)(v * 8.f);
        c = ((cx + cy) & 1) ? c2 : c1;
        c *= 0.92f + 0.16f * n;
    }
    else if (type == "brick")
    {
        const int rows = 8; const float cols = 4.f;
        float rv = v * rows; int r = (int)rv; float vv = rv - (float)r;
        float uu = u * cols + ((r & 1) ? 0.5f : 0.f);
        float uf = uu - std::floor(uu);
        bool mortar = vv < 0.09f || uf < 0.04f;
        c = mortar ? c2 * (0.9f + 0.1f * n) : c1 * (0.75f + 0.5f * n);
    }
    else if (type == "stripes")
    {
        int s = ((int)(u * 12.f)) & 1;
        float grain = Noise2D(u * 40.f, v * 40.f, 40);
        c = (s ? c2 : c1) * (0.8f + 0.4f * grain);
    }
    else if (type == "noise")
    {
        c = glm::mix(c1, c2, n) * (0.9f + 0.2f * Noise2D(u * 64.f, v * 64.f, 64));
    }
    else if (type == "stone")
    {
        float n2 = Fbm2D(u * 6.f, v * 6.f, 6);
        c = glm::mix(c1, c2, n2);
        float crack = std::fabs(Noise2D(u * 10.f, v * 10.f, 10) - 0.5f);
        if (crack < 0.02f) c *= 0.6f;
    }
    else if (type == "grid")
    {
        float gu = u * 8.f - std::floor(u * 8.f), gv = v * 8.f - std::floor(v * 8.f);
        bool line = gu < 0.06f || gv < 0.06f;
        c = (line ? c2 : c1) * (0.95f + 0.1f * n);
    }
    else   // "plain" and anything unknown
    {
        c = c1 * (0.95f + 0.1f * n);
    }
    return glm::clamp(c, glm::vec3(0.f), glm::vec3(1.f));
}

void Texture::GenerateProcedural(const std::string& type, glm::vec3 c1, glm::vec3 c2, int size)
{
    size = std::max(8, std::min(size, 2048));
    std::vector<unsigned char> px((size_t)size * size * 4);
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x)
        {
            glm::vec3 c = Pattern(type, x, y, size, c1, c2);
            unsigned char* p = &px[((size_t)y * size + x) * 4];
            p[0] = (unsigned char)(c.r * 255.f); p[1] = (unsigned char)(c.g * 255.f);
            p[2] = (unsigned char)(c.b * 255.f); p[3] = 255;
        }
    Upload(px.data(), size, size);
}

void Texture::GenerateMissing()
{
    const int size = 64;
    std::vector<unsigned char> px(size * size * 4);
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x)
        {
            bool on = (((x / 8) + (y / 8)) & 1) != 0;
            unsigned char* p = &px[(y * size + x) * 4];
            p[0] = on ? 255 : 0; p[1] = 0; p[2] = on ? 255 : 0; p[3] = 255;
        }
    Upload(px.data(), size, size);
}

bool Texture::DecodeTGA(const std::string& rel, std::vector<unsigned char>& rgba, int& w, int& hgt,
                        bool bottomRowFirst, std::string& error)
{
    std::string path = FindAsset(rel);
    std::ifstream f(path.c_str(), std::ios::binary);
    if (!f.good()) { error = "file could not be opened"; return false; }
    unsigned char h[18];
    f.read((char*)h, 18);
    if (!f) { error = "file is too short to be a TGA"; return false; }
    int type = h[2], bpp = h[16];
    w = h[12] | (h[13] << 8); hgt = h[14] | (h[15] << 8);
    if (type != 2) { error = "only uncompressed true-colour TGA (type 2) is supported"; return false; }
    if (bpp != 24 && bpp != 32) { error = "TGA must be 24 or 32 bits per pixel"; return false; }
    if (w <= 0 || hgt <= 0 || w > 8192 || hgt > 8192) { error = "TGA has an impossible size"; return false; }
    f.seekg(18 + h[0], std::ios::beg);
    int bytes = bpp / 8;
    std::vector<unsigned char> src((size_t)w * hgt * bytes);
    f.read((char*)src.data(), (std::streamsize)src.size());
    if (!f) { error = "TGA pixel data is truncated"; return false; }
    bool fileTopDown = (h[17] & 0x20) != 0;
    rgba.assign((size_t)w * hgt * 4, 255);
    for (int y = 0; y < hgt; ++y)
    {
        int sy = (bottomRowFirst == fileTopDown) ? (hgt - 1 - y) : y;
        for (int x = 0; x < w; ++x)
        {
            const unsigned char* s = &src[((size_t)sy * w + x) * bytes];
            unsigned char* d = &rgba[((size_t)y * w + x) * 4];
            d[0] = s[2]; d[1] = s[1]; d[2] = s[0]; d[3] = bytes == 4 ? s[3] : 255;
        }
    }
    return true;
}

bool Texture::LoadTGA(const std::string& rel, std::string& error)
{
    std::vector<unsigned char> rgba; int w = 0, h = 0;
    if (!DecodeTGA(rel, rgba, w, h, true, error)) return false;
    Upload(rgba.data(), w, h);
    return true;
}

void Texture::Upload(const unsigned char* rgba, int w, int h)
{
    Clear();
    width = w; height = h;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glGenerateMipmap(GL_TEXTURE_2D);       // memory budget: x1.33 for the mip chain
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (GLEW_EXT_texture_filter_anisotropic)
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
}

void Texture::Use(int unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);
}

void Texture::Clear()
{
    if (id) glDeleteTextures(1, &id);
    id = 0; width = height = 0;
}
