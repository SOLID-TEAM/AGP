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
	Light uLight[2];
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
	Light uLight[2];
};

void main()
{
	vec3 uNormal = normalize(vNormal);
	vec3 diffuse, ambient;

	float ambientFactor = 0.2;
	float diffuseFactor = 0.8;
	
	for(int i = 0; i < uLightCount; ++i)
	{
		// TODO: sum all lights contributions
		if(uLight[i].type == 1u)
		{
			float lightContribution = max(dot(normalize(uLight[i].direction), uNormal), 0.0);

			diffuse = vec3(1) * uLight[i].color;//lightContribution * diffuseFactor * uLight[i].color;
			ambient = lightContribution * ambientFactor * uLight[i].color;
			diffuse = vec3(1);
		}
	}
	//oColor = texture(uTexture, vTexCoord);
	//oColor = vec4(normalize(vNormal), 1.0);
	//oColor = vec4(vPosition, 1.0);
	vec4 baseColor = texture(uTexture, vTexCoord);
	vec4 objColor = baseColor * vec4(ambient, 1.0) + // ambient
					baseColor * vec4(diffuse, 1.0);  // diffuse (for all the lights)

	oColor = vec4(diffuse, 1.0);
}

#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.
