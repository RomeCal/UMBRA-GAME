#version 460
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec4 DirectionalLightSpacePos;

out vec4 colour;

const int MAX_POINT_LIGHTS = 3;
const int MAX_SPOT_LIGHTS  = 3;

struct Light            { vec3 colour; float ambientIntensity; float diffuseIntensity; };
struct DirectionalLight { Light base; vec3 direction; };
struct PointLight       { Light base; vec3 position; float constant; float linear; float exponent; };
struct SpotLight        { PointLight base; vec3 direction; float edge; };
struct Material         { float specularIntensity; float shininess; vec3 emissive; };

uniform int pointLightCount;
uniform int spotLightCount;
uniform DirectionalLight directionalLight;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform SpotLight  spotLights[MAX_SPOT_LIGHTS];

uniform sampler2D   theTexture;                          // unit 1
uniform sampler2D   directionalShadowMap;                // unit 2
uniform samplerCube omniShadowMaps[MAX_POINT_LIGHTS];    // units 3, 4, 5
uniform float       omniFarPlane[MAX_POINT_LIGHTS];

uniform Material material;
uniform vec3  eyePosition;

// Shadow biases.  Different units on purpose:
//   dirBias*  : normalised depth (0..1 across the light's near..far)
//   omniBias  : WORLD units (the stored depth is multiplied back by farPlane before comparing)
uniform float dirBiasScale;
uniform float dirBiasMin;
uniform float omniBias;
uniform int   pcfRadius;      // 0 = hard edge (what the game logic uses), 1 = 3x3, 2 = 5x5
uniform int   shadowsEnabled; // demo key '0': 0 = pretend nothing blocks the sun (the game becomes unwinnable)

uniform vec3  fogColour;
uniform float fogDensity;     // 0 = no fog

// ---- directional (sun) shadow ----
float CalcDirectionalShadowFactor(vec3 n, vec3 toLight)
{
    if (shadowsEnabled == 0) return 0.0;
    vec3 p = DirectionalLightSpacePos.xyz / DirectionalLightSpacePos.w;
    p = p * 0.5 + 0.5;
    if (p.z > 1.0) return 0.0;                              // beyond the far plane: treat as lit

    float bias = max(dirBiasScale * (1.0 - dot(n, toLight)), dirBiasMin);

    vec2 texel = 1.0 / vec2(textureSize(directionalShadowMap, 0));
    float shadow = 0.0, count = 0.0;
    for (int x = -pcfRadius; x <= pcfRadius; ++x)
        for (int y = -pcfRadius; y <= pcfRadius; ++y)
        {
            float closest = texture(directionalShadowMap, p.xy + vec2(x, y) * texel).r;
            shadow += (p.z - bias > closest) ? 1.0 : 0.0;
            count += 1.0;
        }
    return shadow / count;
}

// ---- omnidirectional (point light) shadow ----
// Portable "index a sampler array" without relying on dynamic sampler indexing.
float SampleOmni(int i, vec3 d)
{
    if (i == 0) return texture(omniShadowMaps[0], d).r;
    if (i == 1) return texture(omniShadowMaps[1], d).r;
    return texture(omniShadowMaps[2], d).r;
}

float CalcOmniShadowFactor(int i, vec3 lightPos)
{
    vec3 fragToLight = FragPos - lightPos;
    float current = length(fragToLight);
    float closest = SampleOmni(i, fragToLight) * omniFarPlane[i];
    return (current - omniBias > closest) ? 1.0 : 0.0;
}

// ---- Phong: ambient + diffuse + specular ----
vec4 CalcLightByDirection(Light light, vec3 direction, float shadowFactor, vec3 n, vec3 viewDir)
{
    vec4 ambient = vec4(light.colour, 1.0) * light.ambientIntensity;

    float diffuseFactor = max(dot(n, normalize(-direction)), 0.0);
    vec4 diffuse = vec4(light.colour, 1.0) * light.diffuseIntensity * diffuseFactor;

    vec4 specular = vec4(0.0);
    if (diffuseFactor > 0.0)
    {
        vec3 reflected = normalize(reflect(direction, n));
        float sf = dot(viewDir, reflected);
        if (sf > 0.0)
            specular = vec4(light.colour * material.specularIntensity * pow(sf, material.shininess), 1.0);
    }
    return ambient + (1.0 - shadowFactor) * (diffuse + specular);
}

vec4 CalcPointLight(PointLight pl, int shadowIndex, vec3 n, vec3 viewDir)
{
    vec3 direction = FragPos - pl.position;
    float dist = length(direction);
    direction = normalize(direction);
    float shadow = CalcOmniShadowFactor(shadowIndex, pl.position);
    vec4 c = CalcLightByDirection(pl.base, direction, shadow, n, viewDir);
    float attenuation = pl.exponent * dist * dist + pl.linear * dist + pl.constant;
    return c / attenuation;
}

vec4 CalcSpotLight(SpotLight sl, vec3 n, vec3 viewDir)
{
    vec3 toFrag = FragPos - sl.base.position;
    float dist = length(toFrag);
    vec3 rayDirection = toFrag / dist;
    float slFactor = dot(rayDirection, sl.direction);
    if (slFactor <= sl.edge) return vec4(0.0);

    vec4 c = CalcLightByDirection(sl.base.base, rayDirection, 0.0, n, viewDir);
    float attenuation = sl.base.exponent * dist * dist + sl.base.linear * dist + sl.base.constant;
    c /= attenuation;
    // fade towards the rim of the cone: 1 on the axis, 0 at the edge
    return c * (1.0 - (1.0 - slFactor) * (1.0 / (1.0 - sl.edge)));
}

void main()
{
    vec3 n = normalize(Normal);
    vec3 viewDir = normalize(eyePosition - FragPos);

    vec3 toSun = normalize(-directionalLight.direction);
    float sunShadow = CalcDirectionalShadowFactor(n, toSun);
    vec4 total = CalcLightByDirection(directionalLight.base, directionalLight.direction, sunShadow, n, viewDir);

    for (int i = 0; i < pointLightCount; ++i) total += CalcPointLight(pointLights[i], i, n, viewDir);
    for (int i = 0; i < spotLightCount;  ++i) total += CalcSpotLight(spotLights[i], n, viewDir);

    vec4 tex = texture(theTexture, TexCoord);
    vec3 lit = tex.rgb * total.rgb + material.emissive;
    float fogAmount = 1.0 - exp(-fogDensity * length(eyePosition - FragPos));   // distance haze, never fully hides the goal
    colour = vec4(mix(lit, fogColour, clamp(fogAmount, 0.0, 0.85)), 1.0);
}
