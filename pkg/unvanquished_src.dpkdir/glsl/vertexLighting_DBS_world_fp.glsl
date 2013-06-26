/*
===========================================================================
Copyright (C) 2008-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/* vertexLighting_DBS_world_fp.glsl */

uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_NormalMap;
uniform sampler2D	u_SpecularMap;
uniform float		u_AlphaThreshold;
uniform vec3		u_ViewOrigin;
uniform float		u_DepthScale;
uniform	float		u_LightWrapAround;

varying vec3		var_Position;
varying vec4		var_TexDiffuseNormal;
//varying vec2		var_TexSpecular;
#if defined(USE_NORMAL_MAPPING)
varying vec3		var_AmbientLight;
varying vec3		var_DirectedLight;
varying vec3		var_LightDirection;
#endif
varying vec4		var_LightColor;
varying vec3		var_Tangent;
varying vec3		var_Binormal;
varying vec3		var_Normal;



void	main()
{
#if defined(USE_NORMAL_MAPPING)

	// construct object-space-to-tangent-space 3x3 matrix
	mat3 objectToTangentMatrix = mat3(	var_Tangent.x, var_Binormal.x, var_Normal.x,
							var_Tangent.y, var_Binormal.y, var_Normal.y,
							var_Tangent.z, var_Binormal.z, var_Normal.z	);

#if defined(TWOSIDED)
	if(gl_FrontFacing)
	{
		objectToTangentMatrix = -objectToTangentMatrix;
	}
#endif

	// compute view direction in tangent space
	vec3 V = normalize(objectToTangentMatrix * (u_ViewOrigin - var_Position));

	vec2 texDiffuse = var_TexDiffuseNormal.st;
	vec2 texNormal = var_TexDiffuseNormal.pq;
	vec2 texSpecular = texNormal; //var_TexSpecular.st;

#if defined(USE_PARALLAX_MAPPING)

	// ray intersect in view direction

	// size and start position of search in texture space
	vec2 S = V.xy * -u_DepthScale / V.z;

#if 0
	vec2 texOffset = vec2(0.0);
	for(int i = 0; i < 4; i++) {
		vec4 Normal = texture2D(u_NormalMap, texNormal.st + texOffset);
		float height = Normal.a * 0.2 - 0.0125;
		texOffset += height * Normal.z * S;
	}
#else
	float depth = RayIntersectDisplaceMap(texNormal, S, u_NormalMap);

	// compute texcoords offset
	vec2 texOffset = S * depth;
#endif

	texDiffuse.st += texOffset;
	texNormal.st += texOffset;
	texSpecular.st += texOffset;
#endif // USE_PARALLAX_MAPPING

	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse);

	if( abs(diffuse.a + u_AlphaThreshold) <= 1.0 )
	{
		discard;
		return;
	}

	// compute normal in tangent space from normalmap
	vec3 N = 2.0 * (texture2D(u_NormalMap, texNormal).xyz - 0.5);
	#if defined(r_NormalScale)
	N.z *= r_NormalScale;
	normalize(N);
	#endif

	// compute light direction in tangent space
	vec3 L = normalize(objectToTangentMatrix * -var_LightDirection);

 	// compute half angle in tangent space
	vec3 H = normalize(L + V);

	// compute the light term
#if defined(r_WrapAroundLighting)
	float NL = clamp(dot(N, L) + u_LightWrapAround, 0.0, 1.0) / clamp(1.0 + u_LightWrapAround, 0.0, 1.0);
#else
	float NL = clamp(dot(N, L), 0.0, 1.0);
#endif
	vec3 light = var_AmbientLight + var_DirectedLight.rgb * NL;

	// compute the specular term
	vec3 specular = texture2D(u_SpecularMap, texSpecular).rgb * var_DirectedLight.rgb * pow(clamp(dot(N, H), 0.0, 1.0), r_SpecularExponent) * r_SpecularScale;

	// compute final color
	vec4 color = vec4(diffuse.rgb, 1.0);
	color.rgb *= light;
	color.rgb += specular;

#if defined(r_DeferredShading)
	gl_FragData[0] = color; 								// var_Color;
	gl_FragData[1] = vec4(diffuse.rgb, var_LightColor.a);	// vec4(var_Color.rgb, 1.0 - var_Color.a);
	gl_FragData[2] = vec4(N, var_LightColor.a);
	gl_FragData[3] = vec4(specular, var_LightColor.a);
#else
	gl_FragColor = color;
#endif


#elif defined(COMPAT_Q3A)

	vec3 N = normalize(var_Normal);

#if defined(TWOSIDED)
	if(gl_FrontFacing)
	{
		N = -N;
	}
#endif

	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, var_TexDiffuseNormal.st);

	if( abs(diffuse.a + u_AlphaThreshold) <= 1.0 )
	{
		discard;
		return;
	}

	vec4 color = vec4(diffuse.rgb * var_LightColor.rgb, var_LightColor.a);

	// gl_FragColor = vec4(diffuse.rgb * var_LightColor.rgb, diffuse.a);
	// color = vec4(vec3(1.0, 0.0, 0.0), diffuse.a);
	// gl_FragColor = vec4(vec3(diffuse.a, diffuse.a, diffuse.a), 1.0);
	// gl_FragColor = vec4(vec3(var_LightColor.a, var_LightColor.a, var_LightColor.a), 1.0);
	// gl_FragColor = var_LightColor;

#if 0 //defined(r_ShowTerrainBlends)
	color = vec4(vec3(var_LightColor.a), 1.0);
#endif

#if defined(r_DeferredShading)
	gl_FragData[0] = color;
	gl_FragData[1] = vec4(diffuse.rgb, var_LightColor.a);
	gl_FragData[2] = vec4(N, var_LightColor.a);
	gl_FragData[3] = vec4(0.0, 0.0, 0.0, var_LightColor.a);
#else
	gl_FragColor = color;
#endif

#else // USE_NORMAL_MAPPING

	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, var_TexDiffuseNormal.st);

	if( abs(diffuse.a + u_AlphaThreshold) <= 1.0 )
	{
		discard;
		return;
	}

	vec3 N;

#if defined(TWOSIDED)
	if(gl_FrontFacing)
	{
		N = -normalize(var_Normal);
	}
	else
#endif
	{
		N = normalize(var_Normal);
	}

	vec3 L = normalize(var_LightDirection);

	// compute the light term
#if defined(r_WrapAroundLighting)
	float NL = clamp(dot(N, L) + u_LightWrapAround, 0.0, 1.0) / clamp(1.0 + u_LightWrapAround, 0.0, 1.0);
#else
	float NL = clamp(dot(N, L), 0.0, 1.0);
#endif

	vec3 light = var_LightColor.rgb * NL;

	vec4 color = vec4(diffuse.rgb * light, var_LightColor.a);
	// vec4 color = vec4(vec3(NL, NL, NL), diffuse.a);

#if defined(r_DeferredShading)
	gl_FragData[0] = color; 								// var_Color;
	gl_FragData[1] = vec4(diffuse.rgb, var_LightColor.a);	// vec4(var_Color.rgb, 1.0 - var_Color.a);
	gl_FragData[2] = vec4(N, var_LightColor.a);
	gl_FragData[3] = vec4(0.0, 0.0, 0.0, var_LightColor.a);
#else
	gl_FragColor = color;
#endif


#endif // USE_NORMAL_MAPPING

#if 0
#if defined(USE_PARALLAX_MAPPING)
	gl_FragColor = vec4(vec3(1.0, 0.0, 0.0), 1.0);
#elif defined(USE_NORMAL_MAPPING)
	gl_FragColor = vec4(vec3(0.0, 0.0, 1.0), 1.0);
#else
	gl_FragColor = vec4(vec3(0.0, 1.0, 0.0), 1.0);
#endif
#endif
}
