#include <OgreUnifiedShader.h>

#ifdef OGRE_HLSL
#define atan atan2
#endif

uniform mat4 worldViewProj;
uniform vec3 camPos;
uniform vec2 uvScroll;
#ifdef FADE
uniform float fadeGap;
uniform float invisibleDist;
#endif
#ifdef IMPOSTOR
uniform vec4 impostorParams;   // x: yaw angles, y: pitch angles, z: min pitch, w: pitch angles per radian
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

#ifdef IMPOSTOR
    //Select the atlas image rendered from the direction closest to the camera.
    //The texcoords point to the image of yaw 0 (already offset by the impostor rotation),
    //the texture wraps around horizontally.
    vec3 toCam = camPos - vec3(pivot.x, pivot.y + vertex.w, pivot.z);
    float yaw = atan(toCam.x, toCam.z);
    float pitch = atan(toCam.y, length(toCam.xz));
    float yawIndex = floor(yaw * impostorParams.x / 6.2831853 + 0.5);
    float pitchIndex = clamp(floor((pitch - impostorParams.z) * impostorParams.w), 0.0, impostorParams.y - 1.0);
    oUV.xy += vec2(yawIndex / impostorParams.x, pitchIndex / impostorParams.y);
#endif

    //Fog
    oFogCoord = gl_Position.z;
}
