#include <OgreUnifiedShader.h>

uniform mat4 worldViewProj;

#ifdef LIGHTING
uniform vec4 objSpaceLight;
uniform vec4 lightDiffuse;
uniform vec4 lightAmbient;
#endif

#ifdef FADE
uniform vec3 camPos;
uniform float fadeGap;
uniform float invisibleDist;
#endif

#ifdef ANIMATE
uniform float time;
#endif


MAIN_PARAMETERS

IN(vec4 vertex, POSITION)
IN(vec3 normal, NORMAL)
#ifdef VERTEXCOLOUR
IN(vec4 colour, COLOR)
#endif

IN(vec4 uv0, TEXCOORD0)
#ifdef ANIMATE
IN(vec4 uv1, TEXCOORD1)
IN(vec4 uv2, TEXCOORD2)
#endif

#ifdef OGRE_HLSL
OUT(vec4 gl_TexCoord[1], TEXCOORD0)
OUT(vec4 gl_FrontColor, COLOR)
OUT(float gl_FogFragCoord, FOG)
#endif

MAIN_DECLARATION
{
#ifdef LIGHTING
    //Perform lighting calculations (no specular)
    vec3 light = normalize(objSpaceLight.xyz - (vertex.xyz * objSpaceLight.w));
    float diffuseFactor = max(dot(normal, light), 0.0);
#   ifdef VERTEXCOLOUR
    gl_FrontColor = (lightAmbient + diffuseFactor * lightDiffuse) * colour;
#   else
    gl_FrontColor = (lightAmbient + diffuseFactor * lightDiffuse);
#   endif
#else
#   ifdef VERTEXCOLOUR
    gl_FrontColor = colour;
#   else
    gl_FrontColor = vec4(1.0, 1.0, 1.0, 1.0);
#   endif
#endif

#ifdef FADE
    //Fade out in the distance
    float dist = distance(camPos.xz, vertex.xz);
    gl_FrontColor.a *= (invisibleDist - dist) / fadeGap;
#endif

    gl_TexCoord[0] = uv0;

#ifdef ANIMATE
    vec4 params = uv1;
    vec4 originPos = uv2;

	float radiusCoeff = params.x;
	float heightCoeff = params.y;
	float factorX = params.z;
	float factorY = params.w;
	vec4 tmpPos = vertex;
    /* 
    2 different methods are used to for the sin calculation :
    - the first one gives a better effect but at the cost of a few fps because of the 2 sines
    - the second one uses less ressources but is a bit less realistic

    a sin approximation could be use to optimize performances
    */
#   if 1
	tmpPos.y += sin(time + originPos.z + tmpPos.y + tmpPos.x) * radiusCoeff * radiusCoeff * factorY;
	tmpPos.x += sin(time + originPos.z ) * heightCoeff * heightCoeff * factorX;
#   else
	float sinval = sin(time + originPos.z );
	tmpPos.y += sinval * radiusCoeff * radiusCoeff * factorY;
	tmpPos.x += sinval * heightCoeff * heightCoeff * factorX;
#   endif
    vertex = tmpPos;
#endif

    gl_Position = mul(worldViewProj, vertex);
    gl_FogFragCoord = gl_Position.z;
}