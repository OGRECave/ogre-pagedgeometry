/// Vertex shader include; requires OgreUnifiedShader.h to be included first.

#ifdef OGRE_HLSL
#ifndef atan
#define atan atan2
#endif
#endif

/// Selects the impostor atlas image rendered from the direction closest to the camera.
/// pivot: billboard pivot position
/// centerHeight: height of the billboard center above the pivot
/// camPos: camera position in object space
/// impostorParams: x yaw angles, y pitch angles, z min pitch, w pitch angles per radian
/// Returns the atlas texture coordinate offset. The texcoords passed to the
/// shader should point to the image of yaw 0 (already offset by the impostor
/// rotation); the atlas wraps around horizontally.
vec2 pgImpostorTexOffset(vec3 pivot, float centerHeight, vec3 camPos, vec4 impostorParams)
{
    vec3 toCam = camPos - vec3(pivot.x, pivot.y + centerHeight, pivot.z);
    float yaw = atan(toCam.x, toCam.z);
    float pitch = atan(toCam.y, length(toCam.xz));
    float yawIndex = floor(yaw * impostorParams.x / 6.2831853 + 0.5);
    float pitchIndex = clamp(floor((pitch - impostorParams.z) * impostorParams.w), 0.0, impostorParams.y - 1.0);
    return vec2(yawIndex / impostorParams.x, pitchIndex / impostorParams.y);
}
