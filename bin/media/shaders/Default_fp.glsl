#include <OgreUnifiedShader.h>

uniform SAMPLER2D(texMap, 0);

uniform vec4 fogColour;
uniform vec4 fogParams;

MAIN_PARAMETERS
IN(vec4 oUV, TEXCOORD0)
IN(vec4 oColour, COLOR)
IN(float oFogCoord, FOG)
MAIN_DECLARATION
{
    gl_FragColor = texture2D(texMap, oUV.xy) * oColour;
#ifdef ALPHA_TEST
    if(gl_FragColor.a < 0.5)
        discard;
#endif
#ifdef FOG
    float fogf = (fogParams.z - oFogCoord) * fogParams.w;
    gl_FragColor.rgb = mix(fogColour.rgb, gl_FragColor.rgb, fogf);
#endif
}
