#version 460
// "Is the player standing in the sun?"  Asks the SAME directional shadow map, with the SAME
// bias formula and the SAME light matrix that shader.frag uses, at the player's feet.
// The game's health is driven by this answer, so a wrong shadow map is a wrong game.
out vec4 colour;

uniform sampler2D directionalShadowMap;      // unit 2
uniform mat4  directionalLightTransform;
uniform vec3  toSun;                         // unit vector towards the sun
uniform vec3  samplePos;                     // player's feet, world space
uniform float radius;                        // player radius: 4 extra samples around the feet
uniform float dirBiasScale;
uniform float dirBiasMin;
uniform int   shadowsEnabled;

bool InShadow(vec3 worldPos, out bool outside)
{
    vec4 ls = directionalLightTransform * vec4(worldPos, 1.0);
    vec3 p = ls.xyz / ls.w * 0.5 + 0.5;
    outside = (p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0 || p.z > 1.0);
    if (p.z > 1.0 || shadowsEnabled == 0) return false;
    float bias = max(dirBiasScale * (1.0 - dot(vec3(0.0, 1.0, 0.0), toSun)), dirBiasMin);
    float closest = texture(directionalShadowMap, p.xy).r;
    return (p.z - bias > closest);
}

void main()
{
    vec3 offsets[5] = vec3[5](vec3(0.0), vec3(radius, 0.0, 0.0), vec3(-radius, 0.0, 0.0),
                              vec3(0.0, 0.0, radius), vec3(0.0, 0.0, -radius));
    float lit = 0.0;
    bool outsideCentre = false;
    for (int i = 0; i < 5; ++i)
    {
        bool outside;
        bool shadowed = InShadow(samplePos + offsets[i], outside);
        if (i == 0) outsideCentre = outside;
        if (!shadowed) lit += 1.0;
    }
    colour = vec4(lit / 5.0, outsideCentre ? 1.0 : 0.0, 0.0, 1.0);
}
