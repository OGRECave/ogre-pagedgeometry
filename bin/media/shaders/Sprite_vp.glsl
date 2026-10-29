#include <OgreUnifiedShader.h>

uniform mat4 worldViewProj;
uniform vec3 camPos;
uniform vec2 uvScroll;
#ifdef FADE
uniform float fadeGap;
uniform float invisibleDist;
#endif

MAIN_PARAMETERS

IN(vec4 vertex, POSITION)   // xyz: pivot, w: height of the billboard center above the pivot
IN(vec4 uv0, TEXCOORD0)     // xy: corner offset along right/up, zw: texcoords
IN(vec4 colour, COLOR)

OUT(vec4 oUV, TEXCOORD0)
OUT(vec4 oColour, COLOR)
OUT(float oFogCoord, FOG)

MAIN_DECLARATION
{
    //Face the camera, but stay upright
    vec3 pivot = vertex.xyz;
    vec3 dir = normalize(pivot - camPos);
    vec3 right = vec3(-dir.z, 0.0, dir.x) * inversesqrt(max(dir.x * dir.x + dir.z * dir.z, 1e-6));
    vec3 up = cross(right, dir);
    gl_Position = mul(worldViewProj, vec4(pivot + right * uv0.x + up * uv0.y, 1.0));

    //Color
    oColour = colour;

    //Fade out in the distance
#ifdef FADE
    float dist = distance(camPos.xz, pivot.xz);
    oColour.w = (invisibleDist - dist) / fadeGap;
#endif

    //UV scroll
    oUV = vec4(uv0.zw + uvScroll, 0.0, 0.0);

    //Fog
    oFogCoord = gl_Position.z;
}
