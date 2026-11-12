#pragma once
#include "CommonValues.h"

class Mesh;
struct Material;

// Owner: R1.  Everything about the level lives in a text file (Scenes/*.txt).
// The format is documented in README_SceneFormat.txt.  The loader never crashes on a bad
// file: it prints "scene file '<path>' line N: <what was wrong>", skips the line, and goes on.

struct TextureDef
{
    std::string name, type, path;
    glm::vec3 c1{ 0.5f }, c2{ 0.3f };
    int size = 256;
    bool isFile = false;
    bool missing = false;      // file does not exist: already reported by the loader
    int line = 0;
};

struct MaterialDef
{
    std::string name, texture;
    float specular = 0.2f, shininess = 16.f;
    glm::vec3 emissive{ 0.f };
    int line = 0;
};

struct ObjectDef
{
    std::string shape, material;
    glm::vec3 pos{ 0.f }, scale{ 1.f };
    glm::vec2 tile{ 1.f, 1.f };
    bool solid = false, movable = false, castShadow = true;
    int line = 0;
    // filled in by the renderer when the level is built (not read from the file)
    Mesh* mesh = nullptr;
    Material* mat = nullptr;
};

struct SunDef
{
    glm::vec3 dir{ 0.f, -0.6f, 0.8f }, colour{ 1.f, 0.95f, 0.8f }, centre{ 0.f };
    float ambient = 0.25f, diffuse = 1.0f;
    float halfSize = 20.f, nearPlane = 0.1f, farPlane = 100.f;
    int   resolution = 2048;
    float biasScale = 0.005f, biasMin = 0.0005f;
    float sweepAmplitudeDeg = 0.f, sweepPeriodSec = 60.f;
    bool  follow = false, snap = true;
    float followAhead = 4.f;
    bool  defined = false;
};

struct PointDef
{
    glm::vec3 pos{ 0.f }, colour{ 1.f };
    float ambient = 0.f, diffuse = 1.f, constant = 1.f, linear = 0.09f, exponent = 0.032f;
    float farPlane = 40.f; int resolution = 1024;
    int line = 0;
};

struct SpotDef
{
    glm::vec3 pos{ 0.f }, dir{ 0.f, -1.f, 0.f }, colour{ 1.f };
    float ambient = 0.f, diffuse = 1.f, constant = 1.f, linear = 0.05f, exponent = 0.01f, edgeDeg = 20.f;
    bool attachToCamera = false;
    int line = 0;
};

struct SceneData
{
    std::string path, name = "UNTITLED", nextLevel;
    glm::vec4 bounds{ -20.f, -14.f, 20.f, 14.f };     // minX minZ maxX maxZ
    glm::vec2 playerStart{ 0.f, 0.f };
    float playerYaw = 0.f;
    glm::vec2 goal{ 10.f, 0.f };
    float goalRadius = 1.5f;
    float maxHealth = 100.f, sunDrain = 25.f, shadeRegen = 12.f;
    float walkSpeed = 4.5f, sprintMultiplier = 1.6f, eyeHeight = 1.7f, playerRadius = 0.4f;

    std::vector<std::string> skyFaces;                 // 6 file paths (+X -X +Y -Y +Z -Z), empty = generated sky
    std::vector<std::string> hints;                    // on-screen tips shown during the first seconds
    float maxStamina = 100.f, staminaDrain = 40.f, staminaRegen = 22.f;
    glm::vec3 fogColour{ 0.8f, 0.75f, 0.65f };
    float fogDensity = 0.f;
    float parTime = 60.f;                              // seconds for a 3-star finish
    float omniBias = 0.15f;                            // world units (point-light shadow bias)
    SunDef sun;
    std::vector<TextureDef>  textures;
    std::vector<MaterialDef> materials;
    std::vector<ObjectDef>   objects;
    std::vector<PointDef>    points;
    std::vector<SpotDef>     spots;

    std::vector<std::string> messages;   // every warning/error, also printed to stderr
    int errorCount = 0;
};

// Returns false only if the file cannot be opened or contains no objects at all.
bool LoadScene(const std::string& path, SceneData& out);
