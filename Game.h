#pragma once
#include "CommonValues.h"
#include "SceneLoader.h"
#include "Window.h"
#include "Camera.h"
#include "HUD.h"
#include <map>

enum class GameState { TITLE, PLAYING, PAUSED, WON, LOST };

// Owner: R4.  The mechanic: sunlight drains health, shade restores it.
// "In sunlight" is NOT a trigger volume: it is the answer the GPU gives when the sun's own
// shadow map is sampled at the player's feet (SunProbe), so a wrong shadow = a wrong game.
class Game
{
public:
    Game();
    void LoadLevel(const SceneData& sd, const std::vector<ObjectDef>& originalObjects, std::vector<ObjectDef>& liveObjects, int levelNumber);
    void Restart(const std::vector<ObjectDef>& originalObjects, std::vector<ObjectDef>& liveObjects);
    void Update(Window& window, Camera& camera, std::vector<ObjectDef>& objs, float dt,
                float sunLitFraction, bool outsideBox);
    void DrawHUD(HUD& hud, int w, int h, const std::vector<std::string>& debugLines, int sceneWarnings,
                 const std::vector<ObjectDef>& objs, const glm::vec3& camFront) const;

    GameState state;
    float health, maxHealth, stamina, maxStamina, timer, burn;
    glm::vec3 feetPos;
    bool freeCam;
    bool wantRestart, wantNextLevel, wantFirstLevel, wantQuit;
    bool movedObjects;        // a movable block moved this frame -> shadows must be redrawn
    float litFraction;
    bool outsideShadowBox;
    int  stars, levelNumber;

    glm::vec2 goal; float goalRadius;
    std::string levelName, nextLevel;

private:
    void MovePlayer(glm::vec2 delta, std::vector<ObjectDef>& objs);
    void ResolveCollisions(std::vector<ObjectDef>& objs);
    bool BoxFree(const std::vector<ObjectDef>& objs, size_t skip, const glm::vec3& newPos) const;

    glm::vec4 bounds;
    float sunDrain, shadeRegen, walkSpeed, sprintMul, eyeHeight, radius;
    float staminaDrain, staminaRegen, parTime;
    bool exhausted, showMap;
    std::vector<std::string> hints;
    std::map<std::string, float> bestTimes;   // kept for the whole session, per level name
    glm::vec2 startPos; float startYaw;
    bool applyStartCamera;
};
