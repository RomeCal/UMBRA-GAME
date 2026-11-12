#pragma once
// UMBRA - shared includes and constants.  Owner: R1 (Technical Director)
#ifdef _WIN32
#define NOMINMAX
#endif
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>
#include <fstream>
#include <cstdio>
#include <cmath>
#include <algorithm>

#ifdef _MSC_VER
#pragma comment(lib, "opengl32.lib")
#endif

const int   MAX_POINT_LIGHTS = 3;
const int   MAX_SPOT_LIGHTS  = 3;
const float TO_RADIANS       = 3.14159265358979f / 180.0f;

// Texture unit ledger (see README / T2).  Unit 0 is reserved for the sky cube map.
const int UNIT_SKY        = 0;
const int UNIT_DIFFUSE    = 1;   // theTexture / hudTex
const int UNIT_DIR_SHADOW = 2;   // directionalShadowMap
const int UNIT_OMNI_FIRST = 3;   // omniShadowMaps[0..2] -> units 3,4,5

// B14: no absolute paths.  Looks for the file next to the working directory and
// up to four folders above it, so it works whether Visual Studio starts the
// program from the project folder or from x64/Debug.
inline std::string FindAsset(const std::string& rel)
{
    static const char* prefixes[] = { "", "../", "../../", "../../../", "../../../../" };
    for (const char* p : prefixes)
    {
        std::string full = std::string(p) + rel;
        std::ifstream f(full.c_str(), std::ios::binary);
        if (f.good()) return full;
    }
    return rel;
}
