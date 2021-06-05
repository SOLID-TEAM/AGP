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
#ifdef Z_PRE_PASS

#if defined(VERTEX) //////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
//layout(location = 1) in vec3 aNormal;

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

void main()
{
	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef SSAO_PASS

#if defined(VERTEX)

layout(location = 0) in vec3 aPosition;

void main()
{
	gl_Position = vec4(aPosition, 1.0);
}

///////////////////////////////////////////////////////////////////////

#elif defined(FRAGMENT)
layout(location = 0) out vec4 FragColor;

//in vec2 TexCoords;

layout(binding = 0) uniform sampler2D gPosition;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D texNoise;

uniform vec3 samples[64];
uniform mat4 projection;
uniform mat4 view;

int kernelSize = 64;
float radius = 0.5;
float bias = 0.025;

// tile noise tex on screen, screen dimensions divided by noise size
const vec2 noiseScale = vec2(800.0 / 4.0, 600.0 / 4.0);

// TODO: for readapt with forward/deferred reconstructing position from depth instead using this
//vec3 reconstructPixelPos(float depth, mat4 projectionMatrixInv, vec2 v)
//{
//	float xndc = gl_FragCoord.x/v.x * 2.0 - 1.0;
//	float yndc = gl_FragCoord.y/v.y * 2.0 - 1.0;
//	float zndc = depth * 2.0 - 1.0;
//	vec4 posNDC = vec4(xndc, yndc, zndc, 1.0);
//	vec4 posView = projectionMatrixInv * posNDC;
//	return posView.xyz / posView.w;
//}

void main()
{
	vec2 texCoords = gl_FragCoord.xy / vec2(800.0, 600.0);
	// TODO: rework this part to make ssao shader compatible with both renderer forward/deferred ---
	vec3 fragPos   = texture(gPosition, texCoords).xyz;//vec3(view * vec4(texture(gPosition, texCoords).xyz, 1.0));//
	vec3 normal    = texture(gNormal, texCoords).rgb;
	if(normal == vec3(0)) discard;
	// ---------------------------------------------------------------------------------------------
	vec3 randomVec = texture(texNoise, texCoords * noiseScale).xyz;

	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);

	float occlusion = 0.0;
	float position_depth = (view * vec4(fragPos, 1.0)).z;
	for(int i = 0; i < kernelSize; ++i)
	{
		// get sample pos
		//vec3 samplePos = TBN * samples[i]; // from tangent space to view space
		//samplePos = fragPos + samplePos * radius; // move the sample pos to our current frag origin
		vec4 samplePos = view * vec4(fragPos + TBN * samples[i] * radius, 1.0);

		// project sample pos to get position
		vec4 offset = vec4(samplePos.xyz, 1.0);
		offset = projection * offset; // from world space to clip space
		offset.xyz /= offset.w; // perspective divide
		offset.xyz = offset.xyz * 0.5 + 0.5; // transform to 0.0 - 1.0 range

		// TODO: rework this part to make ssao shader compatible with both renderer forward/deferred
		// get sample depth from position texture
		float sampleDepth = vec3(view * texture(gPosition, offset.xy)).z;//texture(gPosition, offset.xy).z; // get depth

		// ------------------------------------------------------------------------------------------
		// range check and accumulate
		float rangeCheck = smoothstep(0.0, 1.0, radius / abs(position_depth - sampleDepth));
		occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;

	}

	occlusion = 1.0 - (occlusion / kernelSize);
	FragColor = vec4(occlusion);
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
	uNormal = normalize(uNormal);
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





// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.
