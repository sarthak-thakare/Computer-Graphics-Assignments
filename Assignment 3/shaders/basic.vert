#version 330 core
layout(location=0) in vec4 vPosition;
layout(location=1) in vec4 vColor;
layout(location=2) in vec3 vNormal;
layout(location=3) in vec2 vUV;

uniform mat4 MVP;
uniform mat4 Model;

const int MAX_LIGHTS = 3;
uniform int numLights;
uniform vec3 lightPos[MAX_LIGHTS];
uniform vec3 lightColor[MAX_LIGHTS];
uniform int  lightOn[MAX_LIGHTS];

uniform vec3 toyLightPos;
uniform vec3 toyLightColor;
uniform int  toyLightOn;

uniform int useTexture; // 0 or 1

out vec4 litColor;
out vec2 uv;

void main(){
    gl_Position = MVP * vPosition;

    // Transform to world
    vec3 P = (Model * vPosition).xyz;
    mat3 Nmat = transpose(inverse(mat3(Model)));
    vec3 N = normalize(Nmat * vNormal);

    vec3 base = vColor.rgb;
    // Lower ambient so large flat surfaces (ceiling) respond more to lights
    vec3 ambient = 0.1 * base;
    vec3 sum = ambient;

    for(int i=0;i<numLights && i<MAX_LIGHTS;i++){
        if(lightOn[i]==0) continue;
        vec3 L = normalize(lightPos[i] - P);
        float ndotl = max(dot(N,L), 0.0);
        sum += base * lightColor[i] * ndotl;
    }
    if(toyLightOn==1){
        vec3 L = normalize(toyLightPos - P);
        float ndotl = max(dot(N,L), 0.0);
        sum += base * toyLightColor * ndotl;
    }
    // Clamp to avoid washing out textures
    sum = clamp(sum, vec3(0.0), vec3(1.0));
    litColor = vec4(sum, vColor.a);
    uv = vUV;
}
