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

void main()
{
	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal = vec3(uWorldMatrix * vec4(aNormal, 0.0));
	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

vec4 CalculateLightInternal(Light light, vec3 lightDirection, vec3 normal);
vec4 CalculateDirectionalLight(Light light, vec3 normal);

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

const float PI = 3.14159265359;
  
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 FresnelSchlick(float cosTheta, vec3 baseReflection);


void main()
{
	vec3 N = normalize(vNormal); 
    vec3 V = normalize(uCameraPosition - vPosition);

    vec3  albedo = pow(texture(uTexture, vTexCoord).rgb , vec3(2.2));
	float metallic = 0.1;
	float roughness = 0.5;
	float ao = 1.0;
	
	vec3 baseReflection = mix( vec3(0.04), albedo, metallic);

	vec3 Lo = vec3(0.0);

	for(int i = 0; i < uLightCount; ++i)
	{
		Light light = uLight[i];
		
		// Directional Light
		if(light.type == 0u) 
		{
			vec3 L = normalize(-light.direction);
			vec3 H = normalize(V + L);
			vec3 radiance = light.color ;        
			
			// cook-torrance brdf

			float NDF = DistributionGGX(N, H, roughness);        
			float G   = GeometrySmith(N, V, L, roughness);      
			vec3 F    = FresnelSchlick(max(dot(H, V), 0.0), baseReflection);       
			
			vec3 kS = F;
			vec3 kD = vec3(1.0) - kS;
			kD *= 1.0 - metallic;	  
			
			vec3 numerator    = NDF * G * F;
			float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
			vec3 specular     = numerator / max(denominator, 0.001);  
			    
			// add to outgoing radiance Lo
			float NdotL = max(dot(N, L), 0.0);                
			Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
		}
		// Point Light
		else 
		{
			// calculate per-light radiance
			vec3 L = normalize(light.position - vPosition);
			vec3 H = normalize(V + L);
			float distance    = length(light.position - vPosition);
			float attenuation = 1.0 / (distance * distance);
			vec3 radiance     = light.color * attenuation;        
			
			// cook-torrance brdf

			float NDF = DistributionGGX(N, H, roughness);        
			float G   = GeometrySmith(N, V, L, roughness);      
			vec3 F    = FresnelSchlick(max(dot(H, V), 0.0), baseReflection);       
			
			vec3 kS = F;
			vec3 kD = vec3(1.0) - kS;
			kD *= 1.0 - metallic;	  
			
			vec3 numerator    = NDF * G * F;
			float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
			vec3 specular     = numerator / max(denominator, 0.001);  
			    
			// add to outgoing radiance Lo
			float NdotL = max(dot(N, L), 0.0);                
			Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
		}
	}

	vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;
	
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  

	oColor =  vec4(color, 1.0);
}

vec3 FresnelSchlick(float cosTheta, vec3 baseReflection)
{
    return baseReflection + (1.0 - baseReflection) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}  

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0000001);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V),  0.0000001);
    float NdotL = max(dot(N, L), 0.0000001);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.

//
//vec4 CalculateLightInternal(Light light, vec3 lightDirection, vec3 normal)
//{
//	float ambientIntensity = 0.1;
//	float diffuseIntensity = 0.4;
//	float specularIntensity = 1.0;
//	float specularPower = 32.0; // 1 to 32
//
//	vec4 ambientColor = vec4(light.color * ambientIntensity, 1.0); //
//	float diffuseFactor = dot( normal, -lightDirection);
//	vec4 diffuseColor = vec4(0.0);
//	vec4 specularColor = vec4(0.0);
//	
//	if (diffuseFactor > 0.0) 
//	{
//		diffuseColor = vec4(light.color*  diffuseIntensity * diffuseFactor, 1.0) ; //
//
//		vec3 vertexToEye = normalize(uCameraPosition - vPosition);
//		vec3 lightReflect = normalize(reflect(lightDirection, normal));
//		float specularFactor = dot(vertexToEye, lightReflect);
//
//		if (specularFactor > 0.0) 
//		{
//			specularFactor = pow(specularFactor, specularPower);
//			specularColor = vec4(light.color * specularIntensity * specularFactor, 1.0);
//		}
//	}
//
//	return (ambientColor + diffuseColor + specularColor) ;
//}
//
//
//vec4 CalculateDirectionalLight(Light light, vec3 normal)
//{
//    return CalculateLightInternal(light,  normalize(light.direction) , normal);
//}
//
//void main()
//{
//	vec3 normal = normalize(vNormal);
//
//	
//	vec4 totalLight = vec4(0.0);
//
//	for(int i = 0; i < uLightCount; ++i)
//	{
//		Light light = uLight[i];
//		
//		// Directional Light
//		if(light.type == 0u) 
//		{
//			totalLight += CalculateDirectionalLight(light, normal);
//		
//		}
//		// Point Light
//		else 
//		{
////		    float constant = 1.0;
////			float linear = 0.09;
////			float quadratic = 0.032;
////			float exponential = 0.032;
////
////			vec3 lightDirection = vPosition - light.position;
////			float dist = length(lightDirection);
////			lightDirection = normalize(lightDirection);
////
////			vec4 ambientColor = vec4(light.color * ambientIntensity, 1.0);
////			float diffuseFactor = dot( normal, -lightDirection);
////			vec4 diffuseColor = vec4(0.0);
////			vec4 specularColor = vec4(0.0);
////
////			if (diffuseFactor > 0.0) 
////			{
////				diffuseColor = vec4(light.color*  diffuseIntensity * diffuseFactor, 1.0) ;
////				vec3 lightReflect = normalize(reflect(lightDirection, normal));
////				float specularFactor = dot(vViewDir, lightReflect);
////
////				if (specularFactor > 0.0) 
////				{
////					specularFactor = pow(specularFactor, specularPower);
////					specularColor = vec4(light.color * specularIntensity * specularFactor, 1.0);
////				}
////			}
////
////			vec4 color = ambientColor + diffuseColor + specularColor ;
////			float attenuation = constant + linear * dist + exponential * dist * dist;
////			
////			totalLight += color / attenuation;
//		}
//
//
//		oColor =  texture(uTexture, vTexCoord) * totalLight;
//	}
//}
//