#define USE_OGRE_FROM_FUTURE
#include <OgreUnifiedShader.h>

SAMPLER2D(texMap, 0);

uniform vec4 fogColour;
uniform vec4 fogParams;

#ifdef ALPHA_TEST
// 1 for geometry fading in with distance, so its dither pattern complements the one fading out
uniform float ditherInvert;

// ordered dither threshold in [0, 15/16] for a 4x4 pixel block
float bayer2(vec2 a) { a = floor(a); return fract(dot(a, vec2(0.5, a.y * 0.75))); }
float bayer4(vec2 a) { return bayer2(0.5 * a) * 0.25 + bayer2(a); }
#endif

MAIN_PARAMETERS
IN(vec4 oUV, TEXCOORD0)
IN(vec4 oColour, COLOR)
IN(float oFogCoord, FOG)
MAIN_DECLARATION
{
    gl_FragColor = texture2D(texMap, oUV.xy);
#ifdef ALPHA_TEST
    // distance fades use screen-door transparency, so fading geometry needs no blending or sorting
    float threshold = abs(ditherInvert * (15.0 / 16.0) - bayer4(gl_FragCoord.xy));
    if(gl_FragColor.a < 0.5 || oColour.a <= threshold)
        discard;
#endif
    gl_FragColor *= saturate(oColour);
#ifdef USE_FOG
    float fogf = saturate((fogParams.z - abs(oFogCoord)) * fogParams.w);
    gl_FragColor.rgb = mix(fogColour.rgb, gl_FragColor.rgb, fogf);
#endif
}
