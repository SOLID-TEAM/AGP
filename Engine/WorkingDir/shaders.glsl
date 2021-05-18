///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef TEXTURED_GEOMETRY

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;
	gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;

void main()
{
	oColor = texture(uTexture, vTexCoord);
}

#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#ifdef GEOMETRY_PASS

#if defined(VERTEX)

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec3 vPosition;
out vec3 vNormal;
out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal = vec3(uWorldMatrix * vec4(aNormal, 0.0));
	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT)

layout(location = 0) out vec4 gPosition; // TODO: no texture output if vec3
layout(location = 1) out vec4 gNormal;   // no texture output if vec3
layout(location = 2) out vec4 gAlbedoSpec;
layout(location = 3) out vec4 gDepthGray;

in vec3 vPosition;
in vec3 vNormal;
in vec2 vTexCoord;

uniform sampler2D uTexture;

void main()
{
	gPosition = vec4(vPosition,1.0);
	gNormal = vec4(normalize(vNormal), 1.0);
	gAlbedoSpec = texture(uTexture, vTexCoord);
	gDepthGray = vec4(gl_FragCoord.zzz, 1.0);
}

#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#ifdef LIGHT_PASS_VOLUMES

#if defined(VERTEX)

layout(location = 0) in vec3 aPosition;
//layout(location = 1) in vec2 aTexCoord;


//out vec2 vTexCoord;
uniform mat4 WVP;


void main()
{
	//vTexCoord = aTexCoord;
	gl_Position = WVP * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT)

struct Light
{
	unsigned int type;
	vec3		 color;
	vec3		 direction;
	vec3		 position;
};

//in vec2 vTexCoord;

layout(location = 0) out vec4 lightingPassTex;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

uniform int lightIdx;

layout(binding = 0) uniform sampler2D gPosition;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gAlbedoSpec;

void main()
{
	vec2 vTexCoord = gl_FragCoord.xy / vec2(800, 600);
    vec3 vPosition = texture(gPosition, vTexCoord).rgb;
	vec3 uNormal = texture(gNormal, vTexCoord).rgb;
	vec3 vViewDir = normalize(uCameraPosition - vPosition);
	vec3 diffuse, ambient, specular;

	float ambientFactor = 0.2;
	float diffuseFactor = 0.8;
	float shininess = 20.0;
	float specFactor = 0.7;

	vec3 lightColor = uLight[lightIdx].color;

	if(uLight[lightIdx].type == 0u) // directional
	{
		vec3 lightDir = normalize(-uLight[lightIdx].direction);
		float lightContribution = max(dot(uNormal, lightDir), 0.0);

		diffuse = lightContribution * diffuseFactor * lightColor;
		ambient = lightContribution * ambientFactor * lightColor;

		// specular
		vec3 r = reflect(-lightDir, uNormal);
		float specComponent = pow(max(dot(normalize(vViewDir), r), 0.0), shininess);
		specular += specComponent * specFactor * lightColor;
	}
	else // point light
	{
		vec3 lightDir = normalize(uLight[lightIdx].position - vPosition);
		float lightContribution = max(dot(uNormal, lightDir), 0.0);

		vec3 d = lightContribution * diffuseFactor * lightColor;
		vec3 a = lightContribution * ambientFactor * lightColor;

		// specular
		vec3 r = reflect(-lightDir, uNormal);
		float specComponent = pow(max(dot(normalize(vViewDir), r), 0.0), shininess);

		// attenuation
		float constant = 1.0;
		float linear = 0.09;
		float quadratic = 0.032;
		float dist = length(uLight[lightIdx].position - vPosition);
		float attenuation = 1.0 / (constant + linear * dist +
								       quadratic * (dist * dist));
		vec3 s = specComponent * specFactor * lightColor;

		a *= attenuation;
		d *= attenuation;
		s *= attenuation;

		diffuse = d;
		ambient = a;
		specular = s;
	}
	
	vec4 albedo = texture(gAlbedoSpec, vTexCoord);
	vec4 objColor = albedo * vec4(ambient, 1.0) + // ambient
					albedo * vec4(diffuse, 1.0) + // diffuse
					albedo * vec4(specular, 1.0); // specular

	lightingPassTex = objColor;//vec4(diffuse, 1.0);
}


#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#ifdef POINT_LIGHT_PASS_VOLUMES

#if defined(VERTEX)

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

//uniform mat4 worldMatrix;
uniform mat4 worldViewProjection;

void main()
{
	vTexCoord = aTexCoord;
	gl_Position = worldViewProjection * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT)

struct Light
{
	unsigned int type;
	vec3		 color;
	vec3		 direction;
	vec3		 position;
};

in vec2 vTexCoord;

layout(location = 0) out vec4 lightingPassTex;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

uniform int lightIdx;

layout(binding = 0) uniform sampler2D gPosition;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gAlbedoSpec;

void main()
{
	vec2 texCoord = gl_FragCoord.xy / vec2(800,600);
    vec3 vPosition = texture(gPosition, texCoord).rgb;
	vec3 uNormal = texture(gNormal, texCoord).rgb;
	vec3 vViewDir = normalize(uCameraPosition - vPosition);
	vec3 diffuse, ambient, specular;

	float ambientFactor = 0.2;
	float diffuseFactor = 0.8;
	float shininess = 20.0;
	float specFactor = 0.7;

	vec3 lightColor = uLight[lightIdx].color;

//	if(uLight[lightIdx].type == 0u) // directional
//	{
//		vec3 lightDir = normalize(-uLight[lightIdx].direction);
//		float lightContribution = max(dot(uNormal, lightDir), 0.0);
//
//		diffuse = lightContribution * diffuseFactor * lightColor;
//		ambient = lightContribution * ambientFactor * lightColor;
//
//		// specular
//		vec3 r = reflect(lightDir, uNormal);
//		float specComponent = pow(max(dot(normalize(vViewDir), r), 0.0), shininess);
//		specular += specComponent * specFactor * lightColor;
//	}
//	else // point light
//	{
		vec3 lightDir = normalize(uLight[lightIdx].position - vPosition);
		float lightContribution = max(dot(uNormal, lightDir), 0.0);

		vec3 d = lightContribution * diffuseFactor * lightColor;
		vec3 a = lightContribution * ambientFactor * lightColor;

		// specular
		vec3 r = reflect(-lightDir, uNormal);
		float specComponent = pow(max(dot(normalize(vViewDir), r), 0.0), shininess);

		// attenuation
		float constant = 1.0;
		float linear = 0.09;
		float quadratic = 0.032;
		float dist = length(uLight[lightIdx].position - vPosition);
		float attenuation = 1.0 / (constant + linear * dist +
								       quadratic * (dist * dist));
		vec3 s = specComponent * specFactor * lightColor;

		a *= attenuation;
		d *= attenuation;
		s *= attenuation;

		diffuse = d;
		ambient = a;
		specular = s;
	//}
	
	vec4 albedo = texture(gAlbedoSpec, texCoord);
	vec4 objColor = albedo * vec4(ambient, 1.0) + // ambient
					albedo * vec4(diffuse, 1.0) + // diffuse
					albedo * vec4(specular, 1.0); // specular

	lightingPassTex = objColor;//vec4(diffuse, 1.0);
}


#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#ifdef LIGHTING_PASS

#if defined(VERTEX)

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;
	gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT)

struct Light
{
	unsigned int type;
	vec3		 color;
	vec3		 direction;
	vec3		 position;
};

in vec2 vTexCoord;

layout(location = 0) out vec4 lightingPassTex;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

layout(binding = 0) uniform sampler2D gPosition;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gAlbedoSpec;

void main()
{
    vec3 vPosition = texture(gPosition, vTexCoord).rgb;
	vec3 uNormal = texture(gNormal, vTexCoord).rgb;
	vec3 vViewDir = normalize(uCameraPosition - vPosition);
	vec3 diffuse, ambient, specular;

	float ambientFactor = 0.2;
	float diffuseFactor = 0.8;
	float shininess = 20.0;
	float specFactor = 0.7;
	
	for(int i = 0; i < uLightCount; ++i)
	{
		vec3 lightColor = uLight[i].color;
		
		if(uLight[i].type == 0u) // directional
		{
			vec3 lightDir = normalize(-uLight[i].direction);
			float lightContribution = max(dot(uNormal, lightDir), 0.0);

			diffuse += lightContribution * diffuseFactor * lightColor;
			ambient += lightContribution * ambientFactor * lightColor;

			// specular
			vec3 r = reflect(-lightDir, uNormal);
			float specComponent = pow(max(dot(normalize(vViewDir), r), 0.0), shininess);
			specular += specComponent * specFactor * lightColor; 
		}
		else // point light
		{
			vec3 lightDir = normalize(uLight[i].position - vPosition);
			float lightContribution = max(dot(uNormal, lightDir), 0.0);

			vec3 d = lightContribution * diffuseFactor * lightColor;
			vec3 a = lightContribution * ambientFactor * lightColor;

			// specular
			vec3 r = reflect(-lightDir, uNormal);
			float specComponent = pow(max(dot(normalize(vViewDir), r), 0.0), shininess);

			// attenuation
			float constant = 1.0;
			float linear = 0.09;
			float quadratic = 0.032;
			float dist = length(uLight[i].position - vPosition);
			float attenuation = 1.0 / (constant + linear * dist +
								       quadratic * (dist * dist));
			vec3 s = specComponent * specFactor * lightColor;

			a *= attenuation;
			d *= attenuation;
			s *= attenuation;

			diffuse += d;
			ambient += a;
			specular += s;

		}
	}
	
	vec4 albedo = texture(gAlbedoSpec, vTexCoord);
	vec4 objColor = albedo * vec4(ambient, 1.0) + // ambient
					albedo * vec4(diffuse, 1.0) + // diffuse
					albedo * vec4(specular, 1.0); // specular

	lightingPassTex = objColor;//vec4(diffuse, 1.0);
}

#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#ifdef SIMPLE_PATRICK

struct Light
{
	unsigned int type;
	vec3		 color;
	vec3		 direction;
	vec3		 position;
};

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
//layout(location = 3) in vec3 aTangent;
//layout(location = 4) in vec3 aBitangent;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vPosition; // in worldspace
out vec3 vNormal;   // in worldspace
out vec3 vViewDir;  // in worldspace

void main()
{
	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal = vec3(uWorldMatrix * vec4(aNormal, 0.0));
	vViewDir = uCameraPosition - vPosition;
	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;
in vec3 vViewDir;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

void main()
{
	vec3 uNormal = normalize(vNormal);
	vec3 diffuse, ambient, specular;

	float ambientFactor = 0.2;
	float diffuseFactor = 0.8;
	float shininess = 20.0;
	float specFactor = 0.7;
	
	for(int i = 0; i < uLightCount; ++i)
	{
		vec3 lightColor = uLight[i].color;
		
		if(uLight[i].type == 0u) // directional
		{
			vec3 lightDir = normalize(-uLight[i].direction);
			float lightContribution = max(dot(uNormal, lightDir), 0.0);

			diffuse += lightContribution * diffuseFactor * lightColor;
			ambient += lightContribution * ambientFactor * lightColor;

			// specular
			vec3 r = reflect(-lightDir, uNormal);
			float specComponent = pow(max(dot(normalize(vViewDir), r), 0.0), shininess);
			specular += specComponent * specFactor * lightColor; 
		}
		else // point light
		{
			vec3 lightDir = normalize(uLight[i].position - vPosition);
			float lightContribution = max(dot(uNormal, lightDir), 0.0);

			vec3 d = lightContribution * diffuseFactor * lightColor;
			vec3 a = lightContribution * ambientFactor * lightColor;

			// specular
			vec3 r = reflect(-lightDir, uNormal);
			float specComponent = pow(max(dot(normalize(vViewDir), r), 0.0), shininess);

			// attenuation
			float constant = 1.0;
			float linear = 0.09;
			float quadratic = 0.032;
			float dist = length(uLight[i].position - vPosition);
			float attenuation = 1.0 / (constant + linear * dist +
								       quadratic * (dist * dist));
			vec3 s = specComponent * specFactor * lightColor;

			a *= attenuation;
			d *= attenuation;
			s *= attenuation;

			diffuse += d;
			ambient += a;
			specular += s;

		}
	}
	
	vec4 baseColor = texture(uTexture, vTexCoord);
	vec4 objColor = baseColor * vec4(ambient, 1.0) + // ambient
					baseColor * vec4(diffuse, 1.0) + // diffuse
					baseColor * vec4(specular, 1.0); // specular

	oColor = objColor;//vec4(diffuse, 1.0);
}

#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.
