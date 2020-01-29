uniform sampler2D texMap;

void main()
{
  float fogf = (gl_Fog.end - gl_FogFragCoord) * gl_Fog.scale;
  gl_FragColor = texture2D(texMap, gl_TexCoord[0].xy) * gl_Color;
	gl_FragColor.rgb = mix(gl_Fog.color.rgb, gl_FragColor.rgb, fogf);
}
