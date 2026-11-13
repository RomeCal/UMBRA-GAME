#version 460
in vec3 TexCoords;
out vec4 colour;
uniform samplerCube skybox;   // never assigned: stays on the default unit 0
uniform vec3 sunDir;          // unit vector pointing TOWARDS the sun
void main()
{
    vec3 d = normalize(TexCoords);
    vec3 sky = texture(skybox, d).rgb;
    float c = max(dot(d, normalize(sunDir)), 0.0);
    float disc = smoothstep(0.9993, 0.9997, c);
    float glow = pow(c, 48.0) * 0.45;
    sky += vec3(1.0, 0.93, 0.75) * (disc * 1.5 + glow);
    colour = vec4(sky, 1.0);
}
