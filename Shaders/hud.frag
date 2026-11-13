#version 460
in vec2 UV;
in vec4 Col;
out vec4 colour;
uniform sampler2D hudTex;                // unit 1: the glyph strip (alpha channel is the shape)
void main()
{
    colour = vec4(Col.rgb, Col.a * texture(hudTex, UV).a);
}
