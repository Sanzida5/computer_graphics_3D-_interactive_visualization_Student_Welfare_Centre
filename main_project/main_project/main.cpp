
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>

static int SCR_WIDTH = 1280;
static int SCR_HEIGHT = 720;
static const float PI = 3.14159265f;
int activeViewport = -1;

bool doorsVisible = true;
bool fanOn = true;
float doorOpenAngle = 0.f;
bool wireframeMode = false;
bool doorAnimating = false;
bool doorOpening = true;
int  currentW = 1280, currentH = 720;


// ─── Camera ──────────────────────────────────────────────────────────────────
struct Camera {
    glm::vec3 position{ 0.f, 10.f, 200.f };
    glm::vec3 front{ 0.f, 0.f, -1.f };
    glm::vec3 up{ 0.f, 1.f, 0.f };
    glm::vec3 right{ 1.f, 0.f, 0.f };
    glm::vec3 worldUp{ 0.f, 1.f, 0.f };
    float yaw = -90.f, pitch = 0.f, roll = 0.f;
    float speed = 12.f;
    bool birdsEyeMode = false, orbitMode = false;
};

struct LightingState {
    bool directionalLightOn = true, pointLightsOn = true, spotLightOn = true;
    bool ambientOn = true, diffuseOn = true, specularOn = true;
};

// ─── Globals ─────────────────────────────────────────────────────────────────
Camera        camera;
LightingState lighting;
bool  dayMode = true;
int   textureMode = 0;
float deltaTime = 0.f, lastFrame = 0.f;

unsigned int texBrick, texGrass, texConcrete, texMarble, texWood;

unsigned int cubeVAO, cubeVBO;
unsigned int cylVAO, cylVBO;  int cylCount;
unsigned int sphVAO, sphVBO;  int sphCount;
unsigned int conVAO, conVBO;  int conCount;

unsigned int bezierSweepVAO, bezierSweepVBO;  int bezierSweepCount = 0;
unsigned int splineSweepVAO, splineSweepVBO;  int splineSweepCount = 0;
unsigned int ruledSurfVAO, ruledSurfVBO;    int ruledSurfCount = 0;

// ─── Colour palette ──────────────────────────────────────────────────────────
const glm::vec3 COL_CONCRETE(0.82f, 0.82f, 0.80f);
const glm::vec3 COL_ROOF(0.75f, 0.75f, 0.73f);
const glm::vec3 COL_COLUMN(0.90f, 0.90f, 0.88f);
const glm::vec3 COL_REDBRICK(0.72f, 0.28f, 0.18f);
const glm::vec3 COL_GLASS(0.20f, 0.55f, 1.00f);
const glm::vec3 COL_DOOR(0.20f, 0.30f, 0.40f);
const glm::vec3 COL_PLAZA(0.78f, 0.42f, 0.30f);
const glm::vec3 COL_GRASS(0.25f, 0.52f, 0.22f);
const glm::vec3 COL_TRUNK(0.40f, 0.28f, 0.15f);
const glm::vec3 COL_PALM(0.15f, 0.55f, 0.20f);
const glm::vec3 COL_SIGN(0.95f, 0.95f, 0.95f);
const glm::vec3 COL_BEAM(0.68f, 0.68f, 0.66f);
const glm::vec3 COL_MARBLE(0.90f, 0.88f, 0.85f);
const glm::vec3 COL_STONE(0.55f, 0.50f, 0.48f);
const glm::vec3 COL_LETTER(0.15f, 0.08f, 0.05f);
const glm::vec3 COL_WOOD_DARK(0.35f, 0.22f, 0.10f);
const glm::vec3 COL_WOOD_MED(0.55f, 0.38f, 0.18f);
const glm::vec3 COL_FABRIC(0.25f, 0.40f, 0.65f);
const glm::vec3 COL_FABRIC2(0.65f, 0.22f, 0.18f);
const glm::vec3 COL_WHITE(0.95f, 0.95f, 0.95f);
const glm::vec3 COL_METAL(0.60f, 0.62f, 0.65f);
const glm::vec3 COL_CARPET(0.30f, 0.45f, 0.35f);
const glm::vec3 COL_LAMP(1.00f, 0.95f, 0.70f);
const glm::vec3 COL_BOOK_R(0.75f, 0.15f, 0.10f);
const glm::vec3 COL_BOOK_B(0.10f, 0.20f, 0.65f);
const glm::vec3 COL_BOOK_G(0.10f, 0.50f, 0.20f);
const glm::vec3 COL_WATER(0.20f, 0.60f, 0.85f);
const glm::vec3 COL_SCREEN(0.05f, 0.05f, 0.15f);


const char* vertexShaderSource = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 GouraudColor;

uniform mat4 model, view, projection;
uniform vec3 objectColor, viewPos;
uniform int  texMode;
uniform float uvTile;

struct DirLight {
    vec3 direction;
    vec3 ambient, diffuse, specular;
};
uniform DirLight dirLight;
uniform bool ambientOn, diffuseOn, specularOn;
uniform bool directionalLightOn;

void main(){
    FragPos  = vec3(model * vec4(aPos, 1.0));
    Normal   = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord * uvTile;
    gl_Position = projection * view * vec4(FragPos, 1.0);

    if(texMode == 2 && directionalLightOn){
        vec3 norm     = normalize(Normal);
        vec3 lightDir = normalize(-dirLight.direction);
        vec3 viewDir  = normalize(viewPos - FragPos);
        vec3 reflDir  = reflect(-lightDir, norm);
        float diff    = max(dot(norm, lightDir), 0.0);
        float spec = pow(max(dot(viewDir, reflDir), 0.0), 64.0);  // ছিল 32.0
        vec3 a = ambientOn  ? dirLight.ambient  * objectColor        : vec3(0.0);
        vec3 d = diffuseOn  ? dirLight.diffuse  * diff * objectColor : vec3(0.0);
        vec3 s = specularOn ? dirLight.specular * spec * vec3(0.5)   : vec3(0.0);
        GouraudColor = a + d + s;
    } else {
        GouraudColor = objectColor * (ambientOn ? 0.30 : 0.05);
    }
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 GouraudColor;

uniform vec3  objectColor, viewPos;
uniform sampler2D texSampler;
uniform bool  hasTexture;
uniform int   texMode;
uniform vec3  emissive;
uniform vec3  globalAmbient;

uniform bool directionalLightOn, pointLightsOn, spotLightOn;
uniform bool ambientOn, diffuseOn, specularOn;

// ── Directional light ──────────────────────────────────────────────────────
struct DirLight {
    vec3 direction;
    vec3 ambient, diffuse, specular;
};
uniform DirLight dirLight;

// ── Point lights ───────────────────────────────────────────────────────────
#define NR_POINT_LIGHTS 9
struct PointLight {
    vec3  position;
    vec3  ambient, diffuse, specular;
    float constant, linear, quadratic;
};
uniform PointLight pointLights[NR_POINT_LIGHTS];

// ── Spot light (camera flashlight — no attenuation for large scene) ────────
struct SpotLight {
    vec3  position, direction;
    float cutOff;
    vec3  ambient, diffuse, specular;
};
uniform SpotLight spotLight;

// ── Light calculation functions ────────────────────────────────────────────
vec3 CalcDirLight(vec3 norm, vec3 viewDir){
    vec3 ld    = normalize(-dirLight.direction);
    float diff = max(dot(norm, ld), 0.0);
    vec3 ref   = reflect(-ld, norm);
   float spec = pow(max(dot(viewDir, ref), 0.0), 64.0);  // ছিল 32.0

    vec3 a = ambientOn  ? dirLight.ambient          : vec3(0.0);
    vec3 d = diffuseOn  ? dirLight.diffuse  * diff  : vec3(0.0);
    vec3 s = specularOn ? dirLight.specular * spec * vec3(0.5) : vec3(0.0);
    return a + d + s;
}

vec3 CalcPointLight(int idx, vec3 norm, vec3 fp, vec3 viewDir){
    vec3 ld    = normalize(pointLights[idx].position - fp);
    float diff = max(dot(norm, ld), 0.0);
    vec3 ref   = reflect(-ld, norm);
   float spec = pow(max(dot(viewDir, ref), 0.0), 64.0);  // ছিল 32.0

    float dist = length(pointLights[idx].position - fp);
    float att  = 1.0 / (pointLights[idx].constant
                      + pointLights[idx].linear    * dist
                      + pointLights[idx].quadratic * dist * dist);
    vec3 a = ambientOn  ? pointLights[idx].ambient          : vec3(0.0);
    vec3 d = diffuseOn  ? pointLights[idx].diffuse  * diff  : vec3(0.0);
    vec3 s = specularOn ? pointLights[idx].specular * spec  : vec3(0.0);
    return (a + d + s) * att;
}

vec3 CalcSpotLight(vec3 norm, vec3 fp, vec3 viewDir){
    vec3 ld     = normalize(spotLight.position - fp);
    float theta = dot(ld, normalize(-spotLight.direction));
    if(theta > spotLight.cutOff){
        float diff = max(dot(norm, ld), 0.0);
        vec3 ref   = reflect(-ld, norm);
       float spec = pow(max(dot(viewDir, ref), 0.0), 64.0);  // ছিল 32.0

        vec3 a = ambientOn  ? spotLight.ambient          : vec3(0.0);
        vec3 d = diffuseOn  ? spotLight.diffuse  * diff  : vec3(0.0);
        vec3 s = specularOn ? spotLight.specular * spec  : vec3(0.0);
        return a + d + s;
    }
    return ambientOn ? spotLight.ambient : vec3(0.0);
}

// ── Full Phong — returns light factor (NOT yet multiplied by objectColor) ──
// FIX: globalAmbient is returned as-is here; main() does * objectColor once.
// Previously globalAmbient * objectColor was here → double multiplication bug.
vec3 CalcFullPhong(vec3 norm, vec3 viewDir){
    vec3 light = emissive;
    if(ambientOn) light += globalAmbient;               // <-- NO * objectColor here
    if(directionalLightOn)
        light += CalcDirLight(norm, viewDir);
    if(pointLightsOn)
        for(int i = 0; i < NR_POINT_LIGHTS; i++)
            light += CalcPointLight(i, norm, FragPos, viewDir);
    if(spotLightOn)
        light += CalcSpotLight(norm, FragPos, viewDir);
    return light;
}

void main(){
    vec3 norm     = normalize(Normal);
    vec3 viewDir  = normalize(viewPos - FragPos);
    vec3 texColor = hasTexture ? texture(texSampler, TexCoord).rgb : objectColor;
    vec3 result;
    if     (texMode == 0) result = CalcFullPhong(norm, viewDir) * objectColor;
    else if(texMode == 1) result = CalcFullPhong(norm, viewDir) * texColor;
    else if(texMode == 2) result = mix(GouraudColor, texColor, 0.5);
    else                  result = mix(CalcFullPhong(norm, viewDir) * objectColor, texColor, 0.5);
    FragColor = vec4(result, 1.0);
}
)";
// ══════════════════════════════════════════════════════════════════════════════
//  PROCEDURAL TEXTURES
// ══════════════════════════════════════════════════════════════════════════════
static unsigned int uploadTexture(unsigned char* data, int w, int h) {
    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return tex;
}
static inline unsigned char clamp8(int v) {
    return (unsigned char)std::max(0, std::min(255, v));
}
static unsigned int createBrickTexture() {
    const int W = 128, H = 64;
    static unsigned char data[W * H * 3];
    const int bH = 14, bW = 32, mort = 2;
    for (int y = 0; y < H; y++) {
        int row = y / bH, off = (row % 2) * (bW / 2);
        bool yM = (y % bH) < mort;
        for (int x = 0; x < W; x++) {
            bool xM = ((x + off) % bW) < mort;
            int i = (y * W + x) * 3;
            if (yM || xM) { data[i] = 148; data[i + 1] = 138; data[i + 2] = 128; }
            else {
                int var = ((x * 7 + y * 13 + row * 31) % 40) - 20;
                data[i] = clamp8(185 + var); data[i + 1] = clamp8(72 + var / 2); data[i + 2] = clamp8(48 + var / 3);
            }
        }
    }
    return uploadTexture(data, W, H);
}
static unsigned int createGrassTexture() {
    const int W = 64, H = 64;
    static unsigned char data[W * H * 3];
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            int i = (y * W + x) * 3;
            float n = sinf(x * 0.7f) * cosf(y * 0.5f) + sinf(x * 1.3f + y * 0.9f) * 0.5f;
            int v = (int)(n * 18);
            data[i] = clamp8(50 + v); data[i + 1] = clamp8(132 + v * 2); data[i + 2] = clamp8(40 + v);
        }
    return uploadTexture(data, W, H);
}
static unsigned int createConcreteTexture() {
    const int W = 64, H = 64;
    static unsigned char data[W * H * 3];
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            int i = (y * W + x) * 3;
            float n = sinf(x * 2.1f + y * 3.7f) * cosf(x * 1.3f - y * 2.9f);
            int v = (int)(n * 10);
            data[i] = clamp8(198 + v); data[i + 1] = clamp8(196 + v); data[i + 2] = clamp8(192 + v);
        }
    return uploadTexture(data, W, H);
}
static unsigned int createMarbleTexture() {
    const int W = 128, H = 128;
    static unsigned char data[W * H * 3];
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            int i = (y * W + x) * 3;
            float t = sinf((x + y * 0.5f) * 0.15f + sinf(x * 0.08f) * 2.0f + sinf(y * 0.12f) * 1.5f);
            t = (t + 1.0f) * 0.5f;
            int v = (int)(195 + t * 55);
            data[i] = clamp8(v + 6); data[i + 1] = clamp8(v + 2); data[i + 2] = clamp8(v - 4);
        }
    return uploadTexture(data, W, H);
}
static unsigned int createWoodTexture() {
    const int W = 64, H = 64;
    static unsigned char data[W * H * 3];
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            int i = (y * W + x) * 3;
            float r = sqrtf((float)((x - 32) * (x - 32) + (y - 32) * (y - 32)));
            float ring = (sinf(r * 0.8f + sinf(x * 0.3f) * 1.5f) + 1.0f) * 0.5f;
            data[i] = clamp8(100 + (int)(ring * 80));
            data[i + 1] = clamp8(60 + (int)(ring * 50));
            data[i + 2] = clamp8(30 + (int)(ring * 20));
        }
    return uploadTexture(data, W, H);
}
static void createAllTextures() {
    texBrick = createBrickTexture();
    texGrass = createGrassTexture();
    texConcrete = createConcreteTexture();
    texMarble = createMarbleTexture();
    texWood = createWoodTexture();
}

// ══════════════════════════════════════════════════════════════════════════════
//  GEOMETRY SETUP  (primitives)
// ══════════════════════════════════════════════════════════════════════════════
static void setupCube() {
    float v[] = {
        -0.5f,-0.5f,-0.5f, 0,0,-1, 1,0,  0.5f,-0.5f,-0.5f, 0,0,-1, 0,0,  0.5f, 0.5f,-0.5f, 0,0,-1, 0,1,
         0.5f, 0.5f,-0.5f, 0,0,-1, 0,1, -0.5f, 0.5f,-0.5f, 0,0,-1, 1,1, -0.5f,-0.5f,-0.5f, 0,0,-1, 1,0,
        -0.5f,-0.5f, 0.5f, 0,0,1,  0,0,  0.5f,-0.5f, 0.5f, 0,0,1,  1,0,  0.5f, 0.5f, 0.5f, 0,0,1,  1,1,
         0.5f, 0.5f, 0.5f, 0,0,1,  1,1, -0.5f, 0.5f, 0.5f, 0,0,1,  0,1, -0.5f,-0.5f, 0.5f, 0,0,1,  0,0,
        -0.5f, 0.5f, 0.5f,-1,0,0,  1,1, -0.5f, 0.5f,-0.5f,-1,0,0,  0,1, -0.5f,-0.5f,-0.5f,-1,0,0,  0,0,
        -0.5f,-0.5f,-0.5f,-1,0,0,  0,0, -0.5f,-0.5f, 0.5f,-1,0,0,  1,0, -0.5f, 0.5f, 0.5f,-1,0,0,  1,1,
         0.5f, 0.5f, 0.5f, 1,0,0,  0,1,  0.5f, 0.5f,-0.5f, 1,0,0,  1,1,  0.5f,-0.5f,-0.5f, 1,0,0,  1,0,
         0.5f,-0.5f,-0.5f, 1,0,0,  1,0,  0.5f,-0.5f, 0.5f, 1,0,0,  0,0,  0.5f, 0.5f, 0.5f, 1,0,0,  0,1,
        -0.5f,-0.5f,-0.5f, 0,-1,0, 0,0,  0.5f,-0.5f,-0.5f, 0,-1,0, 1,0,  0.5f,-0.5f, 0.5f, 0,-1,0, 1,1,
         0.5f,-0.5f, 0.5f, 0,-1,0, 1,1, -0.5f,-0.5f, 0.5f, 0,-1,0, 0,1, -0.5f,-0.5f,-0.5f, 0,-1,0, 0,0,
        -0.5f, 0.5f,-0.5f, 0,1,0,  0,1,  0.5f, 0.5f,-0.5f, 0,1,0,  1,1,  0.5f, 0.5f, 0.5f, 0,1,0,  1,0,
         0.5f, 0.5f, 0.5f, 0,1,0,  1,0, -0.5f, 0.5f, 0.5f, 0,1,0,  0,0, -0.5f, 0.5f,-0.5f, 0,1,0,  0,1
    };
    glGenVertexArrays(1, &cubeVAO); glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);                   glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);
}
static void setupCylinder(int segs) {
    std::vector<float> v;
    const float r = 0.5f, h = 1.0f;
    for (int i = 0; i < segs; ++i) {
        float t0 = 2.f * PI * (float)i / segs, t1 = 2.f * PI * (float)(i + 1) / segs;
        float u0 = (float)i / segs, u1 = (float)(i + 1) / segs;
        float x0 = r * cosf(t0), z0 = r * sinf(t0), x1 = r * cosf(t1), z1 = r * sinf(t1);
        v.insert(v.end(), { x0,-h / 2,z0,x0,0,z0,u0,0 }); v.insert(v.end(), { x1,-h / 2,z1,x1,0,z1,u1,0 }); v.insert(v.end(), { x0,h / 2,z0,x0,0,z0,u0,1 });
        v.insert(v.end(), { x1,-h / 2,z1,x1,0,z1,u1,0 }); v.insert(v.end(), { x1,h / 2,z1,x1,0,z1,u1,1 }); v.insert(v.end(), { x0,h / 2,z0,x0,0,z0,u0,1 });
        v.insert(v.end(), { 0,h / 2,0,0,1,0,0.5f,0.5f }); v.insert(v.end(), { x0,h / 2,z0,0,1,0,0.5f + 0.5f * cosf(t0),0.5f + 0.5f * sinf(t0) }); v.insert(v.end(), { x1,h / 2,z1,0,1,0,0.5f + 0.5f * cosf(t1),0.5f + 0.5f * sinf(t1) });
        v.insert(v.end(), { 0,-h / 2,0,0,-1,0,0.5f,0.5f }); v.insert(v.end(), { x1,-h / 2,z1,0,-1,0,0.5f + 0.5f * cosf(t1),0.5f + 0.5f * sinf(t1) }); v.insert(v.end(), { x0,-h / 2,z0,0,-1,0,0.5f + 0.5f * cosf(t0),0.5f + 0.5f * sinf(t0) });
    }
    glGenVertexArrays(1, &cylVAO); glGenBuffers(1, &cylVBO);
    glBindVertexArray(cylVAO); glBindBuffer(GL_ARRAY_BUFFER, cylVBO);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);                  glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);
    cylCount = (int)(v.size() / 8);
}
static void setupSphere(int stacks, int sectors) {
    std::vector<float> v;
    for (int i = 0; i < stacks; i++) {
        float phi0 = PI * ((float)i / stacks - 0.5f), phi1 = PI * ((float)(i + 1) / stacks - 0.5f);
        for (int j = 0; j < sectors; j++) {
            float th0 = 2.f * PI * (float)j / sectors, th1 = 2.f * PI * (float)(j + 1) / sectors;
            auto vert = [&](float phi, float th) {
                float cp = cosf(phi), sp = sinf(phi), ct = cosf(th), st = sinf(th);
                float nx = cp * ct, ny = sp, nz = cp * st;
                v.insert(v.end(), { nx * 0.5f,ny * 0.5f,nz * 0.5f,nx,ny,nz,th / (2.f * PI),(phi + PI * 0.5f) / PI });
                };
            vert(phi0, th0); vert(phi0, th1); vert(phi1, th0);
            vert(phi0, th1); vert(phi1, th1); vert(phi1, th0);
        }
    }
    glGenVertexArrays(1, &sphVAO); glGenBuffers(1, &sphVBO);
    glBindVertexArray(sphVAO); glBindBuffer(GL_ARRAY_BUFFER, sphVBO);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);                  glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);
    sphCount = (int)(v.size() / 8);
}
static void setupCone(int segs) {
    std::vector<float> v;
    const float r = 0.5f, h = 1.0f, slope = r / h;
    for (int i = 0; i < segs; i++) {
        float t0 = 2.f * PI * (float)i / segs, t1 = 2.f * PI * (float)(i + 1) / segs;
        float u0 = (float)i / segs, u1 = (float)(i + 1) / segs;
        float x0 = r * cosf(t0), z0 = r * sinf(t0), x1 = r * cosf(t1), z1 = r * sinf(t1);
        glm::vec3 n0 = glm::normalize(glm::vec3(cosf(t0), slope, sinf(t0)));
        glm::vec3 n1 = glm::normalize(glm::vec3(cosf(t1), slope, sinf(t1)));
        glm::vec3 na = glm::normalize(glm::vec3(cosf((t0 + t1) * 0.5f), slope, sinf((t0 + t1) * 0.5f)));
        v.insert(v.end(), { x0,-h / 2,z0,n0.x,n0.y,n0.z,u0,0 });
        v.insert(v.end(), { x1,-h / 2,z1,n1.x,n1.y,n1.z,u1,0 });
        v.insert(v.end(), { 0,h / 2,0,na.x,na.y,na.z,(u0 + u1) * 0.5f,1 });
        v.insert(v.end(), { 0,-h / 2,0,0,-1,0,0.5f,0.5f });
        v.insert(v.end(), { x1,-h / 2,z1,0,-1,0,0.5f + 0.5f * cosf(t1),0.5f + 0.5f * sinf(t1) });
        v.insert(v.end(), { x0,-h / 2,z0,0,-1,0,0.5f + 0.5f * cosf(t0),0.5f + 0.5f * sinf(t0) });
    }
    glGenVertexArrays(1, &conVAO); glGenBuffers(1, &conVBO);
    glBindVertexArray(conVAO); glBindBuffer(GL_ARRAY_BUFFER, conVBO);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);                  glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);
    conCount = (int)(v.size() / 8);
}

// ══════════════════════════════════════════════════════════════════════════════
//  CURVE MATHEMATICS
// ══════════════════════════════════════════════════════════════════════════════
static float bernstein(int n, int i, float t) {
    float c = 1.f;
    for (int k = 0; k < i; k++) c *= (float)(n - k) / (float)(k + 1);
    return c * powf(t, (float)i) * powf(1.f - t, (float)(n - i));
}
static glm::vec2 bezierPoint(const std::vector<glm::vec2>& ctrl, float t) {
    int n = (int)ctrl.size() - 1;
    glm::vec2 pt(0.f);
    for (int i = 0; i <= n; i++) pt += ctrl[i] * bernstein(n, i, t);
    return pt;
}
static glm::vec2 catmullRomPoint(const std::vector<glm::vec2>& pts, int s, float t) {
    int n = (int)pts.size();
    int p0 = std::max(0, s - 1), p1 = s;
    int p2 = std::min(n - 1, s + 1), p3 = std::min(n - 1, s + 2);
    float t2 = t * t, t3 = t2 * t;
    return pts[p0] * (-0.5f * t3 + t2 - 0.5f * t) + pts[p1] * (1.5f * t3 - 2.5f * t2 + 1.0f)
        + pts[p2] * (-1.5f * t3 + 2.0f * t2 + 0.5f * t) + pts[p3] * (0.5f * t3 - 0.5f * t2);
}
static int uploadMesh(unsigned int& VAO, unsigned int& VBO, const std::vector<float>& verts) {
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO); glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);                   glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));   glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));   glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    return (int)(verts.size() / 8);
}
static void pushQuad(std::vector<float>& verts,
    glm::vec3 p00, glm::vec3 p10, glm::vec3 p01, glm::vec3 p11,
    glm::vec2 uv00, glm::vec2 uv10, glm::vec2 uv01, glm::vec2 uv11)
{
    glm::vec3 e1 = p10 - p00, e2 = p01 - p00;
    glm::vec3 n = glm::normalize(glm::cross(e1, e2));
    auto push = [&](glm::vec3 p, glm::vec3 nn, glm::vec2 uv) {
        verts.insert(verts.end(), { p.x,p.y,p.z,nn.x,nn.y,nn.z,uv.x,uv.y });
        };
    push(p00, n, uv00); push(p10, n, uv10); push(p01, n, uv01);
    push(p10, n, uv10); push(p11, n, uv11); push(p01, n, uv01);
}

// ── A) BEZIER SWEPT SURFACE ──────────────────────────────────────────────────
static void generateBezierSweepVAO() {
    std::vector<glm::vec2> ctrl = {
        {0.0f,0.0f},{1.8f,0.3f},{0.8f,2.0f},{0.6f,3.5f},{1.4f,5.0f},
        {1.6f,6.5f},{1.0f,7.5f},{1.2f,8.2f},{0.5f,8.5f},
    };
    const int tSteps = 40, sSteps = 36;
    std::vector<glm::vec2> profile;
    for (int i = 0; i <= tSteps; i++) profile.push_back(bezierPoint(ctrl, (float)i / tSteps));
    std::vector<float> verts;
    for (int si = 0; si < sSteps; si++) {
        float theta0 = 2.f * PI * (float)si / sSteps, theta1 = 2.f * PI * (float)(si + 1) / sSteps;
        float u0 = (float)si / sSteps, u1 = (float)(si + 1) / sSteps;
        for (int ti = 0; ti < tSteps; ti++) {
            float v0 = (float)ti / tSteps, v1 = (float)(ti + 1) / tSteps;
            glm::vec2 pr0 = profile[ti], pr1 = profile[ti + 1];
            auto toWorld = [](glm::vec2 pr, float th)->glm::vec3 {return{ pr.x * cosf(th),pr.y,-pr.x * sinf(th) }; };
            pushQuad(verts, toWorld(pr0, theta0), toWorld(pr0, theta1), toWorld(pr1, theta0), toWorld(pr1, theta1),
                { u0,v0 }, { u1,v0 }, { u0,v1 }, { u1,v1 });
        }
    }
    bezierSweepCount = uploadMesh(bezierSweepVAO, bezierSweepVBO, verts);
    std::cout << "[Bezier Sweep] vertices=" << bezierSweepCount << std::endl;
}

// ── B) SPLINE (Catmull-Rom) SWEPT SURFACE ────────────────────────────────────
static void generateSplineSweepVAO() {
    std::vector<glm::vec2> ctrl = {
        {0.30f,0.0f},{0.30f,0.5f},{0.20f,1.0f},{0.15f,3.0f},{0.15f,6.0f},
        {0.20f,7.0f},{0.40f,7.5f},{0.45f,8.0f},{0.20f,8.5f},
    };
    const int segsPerSpan = 10, sSteps = 36;
    int numSpans = (int)ctrl.size() - 1;
    int tSteps = numSpans * segsPerSpan;
    std::vector<glm::vec2> profile;
    for (int i = 0; i <= tSteps; i++) {
        float globalT = (float)i / segsPerSpan;
        int s = (int)floorf(globalT); float localT = globalT - s;
        s = std::min(s, numSpans - 1);
        profile.push_back(catmullRomPoint(ctrl, s, localT));
    }
    std::vector<float> verts;
    for (int si = 0; si < sSteps; si++) {
        float theta0 = 2.f * PI * (float)si / sSteps, theta1 = 2.f * PI * (float)(si + 1) / sSteps;
        float u0 = (float)si / sSteps, u1 = (float)(si + 1) / sSteps;
        for (int ti = 0; ti < tSteps; ti++) {
            float v0 = (float)ti / tSteps, v1 = (float)(ti + 1) / tSteps;
            glm::vec2 pr0 = profile[ti], pr1 = profile[ti + 1];
            auto toWorld = [](glm::vec2 pr, float th)->glm::vec3 {return{ pr.x * cosf(th),pr.y,-pr.x * sinf(th) }; };
            pushQuad(verts, toWorld(pr0, theta0), toWorld(pr0, theta1), toWorld(pr1, theta0), toWorld(pr1, theta1),
                { u0,v0 }, { u1,v0 }, { u0,v1 }, { u1,v1 });
        }
    }
    splineSweepCount = uploadMesh(splineSweepVAO, splineSweepVBO, verts);
    std::cout << "[Spline Sweep] vertices=" << splineSweepCount << std::endl;
}

// ── C) RULED SURFACE ─────────────────────────────────────────────────────────
static void generateRuledSurfaceVAO() {
    std::vector<float> verts;
    const int slices = 72, stacks = 40;
    const float totalHeight = 6.0f, baseRadius = 5.5f, topRadius = 4.8f;
    auto radiusFunc = [&](float t)->float {
        float baseLerp = glm::mix(baseRadius, topRadius, t);
        return baseLerp + sinf(t * PI * 3.0f) * 0.3f;
        };
    auto twistFunc = [&](float t)->float {return t * PI * 2.0f * 1.5f; };
    std::vector<std::vector<glm::vec3>> grid(stacks + 1, std::vector<glm::vec3>(slices + 1));
    for (int i = 0; i <= stacks; i++) {
        float t = (float)i / stacks, y = t * totalHeight, r = radiusFunc(t), twist = twistFunc(t);
        for (int j = 0; j <= slices; j++) {
            float theta = (float)j / slices * 2.0f * PI + twist;
            grid[i][j] = { r * cosf(theta),y,r * sinf(theta) };
        }
    }
    const int stripWidth = 4, gapWidth = 2;
    for (int si = 0; si < stacks; si++) {
        for (int sj = 0; sj < slices; sj++) {
            int posInStrip = sj % (stripWidth + gapWidth);
            if (posInStrip >= stripWidth) continue;
            glm::vec3 p00 = grid[si][sj], p10 = grid[si][(sj + 1) % slices];
            glm::vec3 p01 = grid[si + 1][sj], p11 = grid[si + 1][(sj + 1) % slices];
            float u0 = (float)sj / slices, u1 = (float)(sj + 1) / slices;
            float v0 = (float)si / stacks, v1 = (float)(si + 1) / stacks;
            pushQuad(verts, p00, p10, p01, p11, { u0,v0 }, { u1,v0 }, { u0,v1 }, { u1,v1 });
        }
    }
    for (int si = 0; si < stacks; si++) {
        int posInBand = si % (stripWidth + gapWidth);
        if (posInBand >= stripWidth) continue;
        for (int sj = 0; sj < slices; sj++) {
            glm::vec3 p00 = grid[si][sj], p10 = grid[si][(sj + 1) % slices];
            glm::vec3 p01 = grid[si + 1][sj], p11 = grid[si + 1][(sj + 1) % slices];
            float u0 = (float)sj / slices, u1 = (float)(sj + 1) / slices;
            float v0 = (float)si / stacks, v1 = (float)(si + 1) / stacks;
            pushQuad(verts, p00, p10, p01, p11, { u0,v0 }, { u1,v0 }, { u0,v1 }, { u1,v1 });
        }
    }
    ruledSurfCount = uploadMesh(ruledSurfVAO, ruledSurfVBO, verts);
    std::cout << "[Ruled Surface] vertices=" << ruledSurfCount << std::endl;
}
static void setupAllCurveSurfaces() {
    generateBezierSweepVAO();
    generateSplineSweepVAO();
    generateRuledSurfaceVAO();
}

// ══════════════════════════════════════════════════════════════════════════════
//  DRAW HELPERS
// ══════════════════════════════════════════════════════════════════════════════
static void applyTexture(unsigned int shader, unsigned int texID, float uvTile) {
    glUniform1f(glGetUniformLocation(shader, "uvTile"), uvTile);
    int effMode = (texID != 0) ? textureMode : 0;
    glUniform1i(glGetUniformLocation(shader, "texMode"), effMode);
    if (texID != 0 && textureMode != 0) {
        glUniform1i(glGetUniformLocation(shader, "hasTexture"), 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);
        glUniform1i(glGetUniformLocation(shader, "texSampler"), 0);
    }
    else {
        glUniform1i(glGetUniformLocation(shader, "hasTexture"), 0);
    }
}
static void drawCube(unsigned int sh, glm::mat4 m, glm::vec3 col, unsigned int tex = 0, float tile = 1.f) {
    glUniformMatrix4fv(glGetUniformLocation(sh, "model"), 1, GL_FALSE, glm::value_ptr(m));
    glUniform3fv(glGetUniformLocation(sh, "objectColor"), 1, glm::value_ptr(col));
    applyTexture(sh, tex, tile); glBindVertexArray(cubeVAO); glDrawArrays(GL_TRIANGLES, 0, 36);
}
static void drawGlass(unsigned int sh, glm::mat4 m) {
    static const glm::vec3 gc(0.15f, 0.50f, 1.00f);
    static const glm::vec3 em(0.08f, 0.25f, 0.55f);
    static const glm::vec3 zero(0.f, 0.f, 0.f);
    glUniformMatrix4fv(glGetUniformLocation(sh, "model"), 1, GL_FALSE, glm::value_ptr(m));
    glUniform3fv(glGetUniformLocation(sh, "objectColor"), 1, glm::value_ptr(gc));
    glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(em));
    glBindVertexArray(cubeVAO); glDrawArrays(GL_TRIANGLES, 0, 36);
    glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(zero));
}
static void drawSignCube(unsigned int sh, glm::mat4 m) {
    static const glm::vec3 wc(1.f, 1.f, 1.f);
    static const glm::vec3 em(0.9f, 0.9f, 0.9f);
    static const glm::vec3 zero(0.f, 0.f, 0.f);
    glUniformMatrix4fv(glGetUniformLocation(sh, "model"), 1, GL_FALSE, glm::value_ptr(m));
    glUniform3fv(glGetUniformLocation(sh, "objectColor"), 1, glm::value_ptr(wc));
    glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(em));
    glBindVertexArray(cubeVAO); glDrawArrays(GL_TRIANGLES, 0, 36);
    glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(zero));
}
static void drawCylinder(unsigned int sh, glm::mat4 m, glm::vec3 col, unsigned int tex = 0, float tile = 1.f) {
    glUniformMatrix4fv(glGetUniformLocation(sh, "model"), 1, GL_FALSE, glm::value_ptr(m));
    glUniform3fv(glGetUniformLocation(sh, "objectColor"), 1, glm::value_ptr(col));
    applyTexture(sh, tex, tile); glBindVertexArray(cylVAO); glDrawArrays(GL_TRIANGLES, 0, cylCount);
}
static void drawSphere(unsigned int sh, glm::mat4 m, glm::vec3 col, unsigned int tex = 0, float tile = 1.f) {
    glUniformMatrix4fv(glGetUniformLocation(sh, "model"), 1, GL_FALSE, glm::value_ptr(m));
    glUniform3fv(glGetUniformLocation(sh, "objectColor"), 1, glm::value_ptr(col));
    applyTexture(sh, tex, tile); glBindVertexArray(sphVAO); glDrawArrays(GL_TRIANGLES, 0, sphCount);
}
static void drawCone(unsigned int sh, glm::mat4 m, glm::vec3 col, unsigned int tex = 0, float tile = 1.f) {
    glUniformMatrix4fv(glGetUniformLocation(sh, "model"), 1, GL_FALSE, glm::value_ptr(m));
    glUniform3fv(glGetUniformLocation(sh, "objectColor"), 1, glm::value_ptr(col));
    applyTexture(sh, tex, tile); glBindVertexArray(conVAO); glDrawArrays(GL_TRIANGLES, 0, conCount);
}
static void drawCrystalGlass(unsigned int sh, glm::mat4 m) {
    static const glm::vec3 gc(0.85f, 0.95f, 1.00f);
    static const glm::vec3 em(0.40f, 0.70f, 0.90f);
    static const glm::vec3 zero(0.f, 0.f, 0.f);
    glUniformMatrix4fv(glGetUniformLocation(sh, "model"), 1, GL_FALSE, glm::value_ptr(m));
    glUniform3fv(glGetUniformLocation(sh, "objectColor"), 1, glm::value_ptr(gc));
    glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(em));
    glBindVertexArray(cubeVAO); glDrawArrays(GL_TRIANGLES, 0, 36);
    glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(zero));
}
static void drawLamp(unsigned int sh, glm::mat4 m) {
    static const glm::vec3 lc(1.00f, 0.95f, 0.70f);
    static const glm::vec3 em(0.80f, 0.75f, 0.50f);
    static const glm::vec3 zero(0.f, 0.f, 0.f);
    glUniformMatrix4fv(glGetUniformLocation(sh, "model"), 1, GL_FALSE, glm::value_ptr(m));
    glUniform3fv(glGetUniformLocation(sh, "objectColor"), 1, glm::value_ptr(lc));
    glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(em));
    glBindVertexArray(cubeVAO); glDrawArrays(GL_TRIANGLES, 0, 36);
    glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(zero));
}
static void drawMesh(unsigned int sh, unsigned int VAO, int count,
    glm::mat4 model, glm::vec3 col, unsigned int tex = 0, float tile = 1.f)
{
    glUniformMatrix4fv(glGetUniformLocation(sh, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(sh, "objectColor"), 1, glm::value_ptr(col));
    applyTexture(sh, tex, tile);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, count);
}

// ══════════════════════════════════════════════════════════════════════════════
//  UTILITIES
// ══════════════════════════════════════════════════════════════════════════════
static void updateCameraVectors() {
    glm::vec3 front;
    front.x = cosf(glm::radians(camera.yaw)) * cosf(glm::radians(camera.pitch));
    front.y = sinf(glm::radians(camera.pitch));
    front.z = sinf(glm::radians(camera.yaw)) * cosf(glm::radians(camera.pitch));
    camera.front = glm::normalize(front);
    glm::vec3 baseRight = glm::normalize(glm::cross(camera.front, camera.worldUp));
    glm::mat4 rollMat = glm::rotate(glm::mat4(1.f), glm::radians(camera.roll), camera.front);
    camera.right = glm::normalize(glm::vec3(rollMat * glm::vec4(baseRight, 0.f)));
    camera.up = glm::normalize(glm::cross(camera.right, camera.front));
}
static glm::mat4 customPerspective(float fovY, float aspect, float zN, float zF) {
    float t = tanf(fovY / 2.f); glm::mat4 r(0.f);
    r[0][0] = 1.f / (aspect * t); r[1][1] = 1.f / t;
    r[2][2] = -(zF + zN) / (zF - zN); r[2][3] = -1.f; r[3][2] = -(2.f * zF * zN) / (zF - zN);
    return r;
}
static void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    SCR_WIDTH = w; SCR_HEIGHT = h; glViewport(0, 0, w, h);
}
static std::string itos(int v) { std::ostringstream ss; ss << v; return ss.str(); }
static unsigned int compileShader(unsigned int type, const char* src) {
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL); glCompileShader(s);
    int ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[512]; glGetShaderInfoLog(s, 512, NULL, log); std::cerr << "Shader error:\n" << log << std::endl; }
    return s;
}
static unsigned int createShaderProgram() {
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs); glLinkProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs); return prog;
}

// ══════════════════════════════════════════════════════════════════════════════
//  LIGHTING SETUP
// ══════════════════════════════════════════════════════════════════════════════
// ══════════════════════════════════════════════════════════════════════════════
//  LIGHTING SETUP  (struct-style uniforms, 9 point lights, global ambient)
// ══════════════════════════════════════════════════════════════════════════════
// ══════════════════════════════════════════════════════════════════════════════
//  LIGHTING SETUP
// ══════════════════════════════════════════════════════════════════════════════
static void setupLighting(unsigned int sh) {
    glUniform1i(glGetUniformLocation(sh, "directionalLightOn"), lighting.directionalLightOn ? 1 : 0);
    glUniform1i(glGetUniformLocation(sh, "pointLightsOn"), lighting.pointLightsOn ? 1 : 0);
    glUniform1i(glGetUniformLocation(sh, "spotLightOn"), lighting.spotLightOn ? 1 : 0);
    glUniform1i(glGetUniformLocation(sh, "ambientOn"), lighting.ambientOn ? 1 : 0);
    glUniform1i(glGetUniformLocation(sh, "diffuseOn"), lighting.diffuseOn ? 1 : 0);
    glUniform1i(glGetUniformLocation(sh, "specularOn"), lighting.specularOn ? 1 : 0);
   

    glUniform3f(glGetUniformLocation(sh, "viewPos"),
        camera.position.x, camera.position.y, camera.position.z);
    float ambStr = dayMode ? 0.15f : 0.04f;  
    glUniform3f(glGetUniformLocation(sh, "globalAmbient"), ambStr, ambStr, ambStr);
    // ── Directional light (sun / moon) ─────────────────────────────────────
    if (dayMode) {
        glUniform3f(glGetUniformLocation(sh, "dirLight.direction"), -0.3f, -1.0f, -0.5f);
        glUniform3f(glGetUniformLocation(sh, "dirLight.ambient"), 0.20f, 0.20f, 0.18f);  
        glUniform3f(glGetUniformLocation(sh, "dirLight.diffuse"), 0.75f, 0.72f, 0.65f);  
        glUniform3f(glGetUniformLocation(sh, "dirLight.specular"), 0.60f, 0.60f, 0.55f); 
    }
    else {
        glUniform3f(glGetUniformLocation(sh, "dirLight.direction"), -0.2f, -1.0f, -0.3f);
        glUniform3f(glGetUniformLocation(sh, "dirLight.ambient"), 0.05f, 0.05f, 0.10f);
        glUniform3f(glGetUniformLocation(sh, "dirLight.diffuse"), 0.10f, 0.10f, 0.20f);
        glUniform3f(glGetUniformLocation(sh, "dirLight.specular"), 0.30f, 0.30f, 0.50f);
    }

    // ── 9 Point lights — campus-wide coverage ──────────────────────────────
    // Elevated positions (y=8-12) so they illuminate from above, visible from anywhere.
    // 0-1 : front building (indoor ceiling)
    // 2-3 : back building  (indoor ceiling)
    // 4   : connector corridor
    // 5   : front plaza / entrance (outdoor)
    // 6   : annex / student welfare sign (outdoor)
    // 7   : west side / pond area (outdoor)
    // 8   : east side / road area (outdoor)
    glm::vec3 ptPos[9] = {
        {-15.f, 8.f,   3.f},   // 0  front building, west half
        {  8.f, 8.f,   3.f},   // 1  front building, east half
        { -5.f, 8.f, -22.f},   // 2  back building interior
        {-20.f, 8.f, -22.f},   // 3  back building, west end
        { 15.f, 8.f, -10.f},   // 4  connector corridor
        { -5.f,12.f,  15.f},   // 5  front plaza (outdoor, high)
        { 22.f,10.f,   0.f},   // 6  annex area (outdoor)
        {-45.f, 8.f,  -8.f},   // 7  pond / west side (outdoor)
        { 35.f, 8.f,   5.f},   // 8  road / east side (outdoor)
    };
    glm::vec3 ptCol[9] = {
        {1.00f, 0.92f, 0.75f},  // 0  warm white (indoor)
        {0.92f, 0.92f, 1.00f},  // 1  cool white (indoor)
        {1.00f, 0.92f, 0.80f},  // 2  warm white (indoor)
        {0.85f, 1.00f, 0.92f},  // 3  green-white (indoor)
        {1.00f, 0.95f, 0.82f},  // 4  corridor warm
        {0.95f, 0.95f, 0.90f},  // 5  plaza neutral white
        {1.00f, 0.88f, 0.65f},  // 6  amber (annex sign)
        {0.70f, 0.85f, 1.00f},  // 7  blue-white (pond)
        {0.90f, 0.92f, 0.88f},  // 8  road neutral
    };

    // Attenuation: indoor (0-4) = shorter range, outdoor (5-8) = longer range
    float linearVal[9] = { 0.027f,0.027f,0.027f,0.027f,0.027f,
                              0.014f,0.014f,0.022f,0.014f };
    float quadraticVal[9] = { 0.0028f,0.0028f,0.0028f,0.0028f,0.0028f,
                              0.0007f,0.0007f,0.0015f,0.0007f };

    for (int i = 0; i < 9; i++) {
        std::string n = "pointLights[" + itos(i) + "]";
        glm::vec3 col = ptCol[i];
        glUniform3fv(glGetUniformLocation(sh, (n + ".position").c_str()), 1, glm::value_ptr(ptPos[i]));
        glUniform3fv(glGetUniformLocation(sh, (n + ".ambient").c_str()), 1, glm::value_ptr(col * 0.08f));
        glUniform3fv(glGetUniformLocation(sh, (n + ".diffuse").c_str()), 1, glm::value_ptr(col * 0.45f));
        glUniform3fv(glGetUniformLocation(sh, (n + ".specular").c_str()), 1, glm::value_ptr(col));
        glUniform1f(glGetUniformLocation(sh, (n + ".constant").c_str()), 1.0f);
        glUniform1f(glGetUniformLocation(sh, (n + ".linear").c_str()), linearVal[i]);
        glUniform1f(glGetUniformLocation(sh, (n + ".quadratic").c_str()), quadraticVal[i]);
    }

    // ── Spot light (camera flashlight) ─────────────────────────────────────
    // No attenuation — scene scale is too large (50-100 units), attenuation
    // makes it invisible at typical viewing distances.
    glUniform3f(glGetUniformLocation(sh, "spotLight.position"),
        camera.position.x, camera.position.y, camera.position.z);
    glUniform3f(glGetUniformLocation(sh, "spotLight.direction"),
        camera.front.x, camera.front.y, camera.front.z);
    glUniform1f(glGetUniformLocation(sh, "spotLight.cutOff"), cosf(glm::radians(12.5f)));
    glUniform3f(glGetUniformLocation(sh, "spotLight.ambient"), 0.0f, 0.0f, 0.0f);
    glUniform3f(glGetUniformLocation(sh, "spotLight.diffuse"), 1.0f, 1.0f, 1.0f);
    glUniform3f(glGetUniformLocation(sh, "spotLight.specular"), 1.0f, 1.0f, 1.0f);

    glUniform3f(glGetUniformLocation(sh, "emissive"), 0, 0, 0);
}
// ══════════════════════════════════════════════════════════════════════════════
//  ANIMATED DOOR
// ══════════════════════════════════════════════════════════════════════════════
static void drawAnimatedDoor(unsigned int sh,
    float hx, float hy, float hz, float dw, float dh, float dd,
    float angle, bool hingeLeft = true, float faceDir = 1.f)
{
    glm::mat4 base = glm::translate(glm::mat4(1.f), { hx,hy,hz });
    float swingAngle = hingeLeft ? -angle : angle;
    base = glm::rotate(base, glm::radians(swingAngle * faceDir), { 0,1,0 });
    float offsetX = hingeLeft ? (dw * 0.5f) : -(dw * 0.5f);
    glm::mat4 m = glm::scale(glm::translate(base, { offsetX,dh * 0.5f,0.f }), { dw,dh,dd });
    drawCube(sh, m, COL_DOOR);
    m = glm::scale(glm::translate(base, { offsetX,dh - 0.08f,0.f }), { dw,0.12f,dd + 0.02f });
    drawCube(sh, m, { 0.15f,0.22f,0.30f });
    m = glm::scale(glm::translate(base, { offsetX,0.08f,0.f }), { dw,0.12f,dd + 0.02f });
    drawCube(sh, m, { 0.15f,0.22f,0.30f });
    float lx = hingeLeft ? (0.08f) : -(dw - 0.08f);
    m = glm::scale(glm::translate(base, { lx,dh * 0.5f,0.f }), { 0.12f,dh,dd + 0.02f });
    drawCube(sh, m, { 0.15f,0.22f,0.30f });
    float rx = hingeLeft ? (dw - 0.08f) : (-0.08f);
    m = glm::scale(glm::translate(base, { rx,dh * 0.5f,0.f }), { 0.12f,dh,dd + 0.02f });
    drawCube(sh, m, { 0.15f,0.22f,0.30f });
    float hndX = hingeLeft ? (dw - 0.15f) : -(dw - 0.15f);
    m = glm::scale(glm::translate(base, { hndX,dh * 0.45f,dd * 0.6f }), { 0.05f,0.20f,0.05f });
    drawCylinder(sh, m, { 0.75f,0.62f,0.15f });
    m = glm::scale(glm::translate(base, { hndX,dh * 0.45f,dd * 0.6f }), { 0.08f,0.08f,0.08f });
    drawSphere(sh, m, { 0.80f,0.68f,0.20f });
}

// ══════════════════════════════════════════════════════════════════════════════
//  CEILING FAN
// ══════════════════════════════════════════════════════════════════════════════
static float fanAngle = 0.f;
static void renderCeilingFan(unsigned int sh, float x, float y, float z, float speedMult = 1.0f) {
    glm::mat4 m = glm::scale(glm::translate(glm::mat4(1.f), { x,y,z }), { 0.30f,0.18f,0.30f });
    drawCylinder(sh, m, { 0.55f,0.55f,0.58f });
    m = glm::scale(glm::translate(glm::mat4(1.f), { x,y - 0.25f,z }), { 0.05f,0.50f,0.05f });
    drawCylinder(sh, m, { 0.50f,0.50f,0.52f });
    m = glm::scale(glm::translate(glm::mat4(1.f), { x,y - 0.52f,z }), { 0.18f,0.07f,0.18f });
    drawCylinder(sh, m, { 0.48f,0.48f,0.50f });
    float angle = fanOn ? (fanAngle * speedMult) : 0.f;
    for (int i = 0; i < 4; i++) {
        float bladeAngle = angle + i * 90.f;
        glm::mat4 blade = glm::translate(glm::mat4(1.f), { x,y - 0.55f,z });
        blade = glm::rotate(blade, glm::radians(bladeAngle), { 0,1,0 });
        blade = glm::translate(blade, { 0.55f,0.f,0.f });
        blade = glm::scale(blade, { 1.0f,0.04f,0.30f });
        glm::vec3 bladeCol = fanOn ? glm::vec3(0.45f, 0.30f, 0.15f) : glm::vec3(0.30f, 0.20f, 0.10f);
        drawCube(sh, blade, bladeCol, texWood, 1.f);
    }
    m = glm::scale(glm::translate(glm::mat4(1.f), { x,y - 0.65f,z }), { 0.14f,0.14f,0.14f });
    if (fanOn) drawLamp(sh, m);
    else drawCylinder(sh, m, { 0.60f,0.58f,0.50f });
}

// ══════════════════════════════════════════════════════════════════════════════
//  FURNITURE
// ══════════════════════════════════════════════════════════════════════════════
static void renderChair(unsigned int sh, float x, float y, float z, float rotY = 0.f) {
    const glm::vec3 WD(0.35f, 0.22f, 0.10f), FB(0.25f, 0.40f, 0.65f);
    glm::mat4 base = glm::rotate(glm::translate(glm::mat4(1.f), { x,y,z }), glm::radians(rotY), { 0,1,0 });
    auto T = [&](glm::vec3 t, glm::vec3 s, glm::vec3 c) {drawCube(sh, glm::scale(glm::translate(base, t), s), c, texWood, 1.f); };
    T({ -0.17f,0.20f,-0.17f }, { 0.06f,0.40f,0.06f }, WD); T({ 0.17f,0.20f,-0.17f }, { 0.06f,0.40f,0.06f }, WD);
    T({ -0.17f,0.20f, 0.17f }, { 0.06f,0.40f,0.06f }, WD); T({ 0.17f,0.20f, 0.17f }, { 0.06f,0.40f,0.06f }, WD);
    T({ 0.00f,0.42f,0.00f }, { 0.40f,0.06f,0.40f }, WD); T({ 0.00f,0.46f,0.00f }, { 0.36f,0.06f,0.36f }, FB);
    T({ 0.00f,0.72f,-0.17f }, { 0.40f,0.06f,0.06f }, WD); T({ 0.00f,0.58f,-0.17f }, { 0.40f,0.06f,0.06f }, WD);
    T({ 0.00f,0.65f,-0.17f }, { 0.36f,0.22f,0.05f }, FB);
    T({ -0.17f,0.60f,-0.17f }, { 0.06f,0.34f,0.06f }, WD); T({ 0.17f,0.60f,-0.17f }, { 0.06f,0.34f,0.06f }, WD);
}
static void renderTable(unsigned int sh, float x, float y, float z,
    float w = 1.2f, float d = 0.7f, float h = 0.75f, float rotY = 0.f) {
    const glm::vec3 WM(0.55f, 0.38f, 0.18f), WD(0.35f, 0.22f, 0.10f);
    glm::mat4 base = glm::rotate(glm::translate(glm::mat4(1.f), { x,y,z }), glm::radians(rotY), { 0,1,0 });
    auto T = [&](glm::vec3 t, glm::vec3 s, glm::vec3 c) {drawCube(sh, glm::scale(glm::translate(base, t), s), c, texWood, 1.f); };
    T({ 0.f,h,0.f }, { w,0.06f,d }, WM);
    float lx = w * 0.5f - 0.05f, lz = d * 0.5f - 0.05f;
    T({ -lx,h * 0.5f,-lz }, { 0.07f,h,0.07f }, WD); T({ lx,h * 0.5f,-lz }, { 0.07f,h,0.07f }, WD);
    T({ -lx,h * 0.5f, lz }, { 0.07f,h,0.07f }, WD); T({ lx,h * 0.5f, lz }, { 0.07f,h,0.07f }, WD);
    T({ 0.f,h * 0.3f,0.f }, { w * 0.85f,0.04f,0.05f }, WD);
}
static void renderBookshelf(unsigned int sh, float x, float y, float z, float rotY = 0.f) {
    const glm::vec3 WD(0.35f, 0.22f, 0.10f);
    glm::mat4 base = glm::rotate(glm::translate(glm::mat4(1.f), { x,y,z }), glm::radians(rotY), { 0,1,0 });
    auto T = [&](glm::vec3 t, glm::vec3 s, glm::vec3 c) {drawCube(sh, glm::scale(glm::translate(base, t), s), c, texWood, 1.f); };
    T({ 0.f,1.0f,0.f }, { 1.2f,2.0f,0.35f }, WD);
    glm::vec3 SH(0.55f, 0.38f, 0.18f);
    for (int i = 0; i < 4; i++) { float sy = 0.25f + i * 0.48f; T({ 0.f,sy,0.f }, { 1.10f,0.04f,0.28f }, SH); }
    glm::vec3 bColors[] = { {0.75f,0.15f,0.10f},{0.10f,0.20f,0.65f},{0.10f,0.50f,0.20f},
                         {0.70f,0.55f,0.10f},{0.55f,0.10f,0.60f},{0.80f,0.40f,0.10f} };
    for (int shelf = 0; shelf < 4; shelf++) {
        float sy = 0.28f + shelf * 0.48f;
        for (int b = 0; b < 6; b++) { float bx = -0.48f + b * 0.18f; T({ bx,sy + 0.18f,0.f }, { 0.12f,0.30f,0.20f }, bColors[b % 6]); }
    }
}
static void renderSofa(unsigned int sh, float x, float y, float z, float rotY = 0.f) {
    const glm::vec3 FR(0.65f, 0.22f, 0.18f), WD(0.35f, 0.22f, 0.10f);
    glm::mat4 base = glm::rotate(glm::translate(glm::mat4(1.f), { x,y,z }), glm::radians(rotY), { 0,1,0 });
    auto T = [&](glm::vec3 t, glm::vec3 s, glm::vec3 c, unsigned int tex) {
        drawCube(sh, glm::scale(glm::translate(base, t), s), c, tex, 1.f);
        };
    // base frame
    T({ 0.f,0.18f,0.f }, { 1.80f,0.35f,0.70f }, WD, texWood);
    // seat cushions
    for (int i = 0; i < 3; i++) {
        float sx = -0.60f + i * 0.60f;
        T({ sx,0.40f,0.f }, { 0.55f,0.18f,0.62f }, FR, texConcrete);
    }
    // back rest
    T({ 0.f,0.70f,-0.28f }, { 1.80f,0.40f,0.16f }, FR, texConcrete);
    // arm rests
    T({ -0.90f,0.55f,0.f }, { 0.14f,0.35f,0.70f }, WD, texWood);
    T({ 0.90f,0.55f,0.f }, { 0.14f,0.35f,0.70f }, WD, texWood);
    // legs
    T({ -0.80f,0.07f,-0.28f }, { 0.10f,0.14f,0.10f }, WD, texWood);
    T({ 0.80f,0.07f,-0.28f }, { 0.10f,0.14f,0.10f }, WD, texWood);
    T({ -0.80f,0.07f, 0.28f }, { 0.10f,0.14f,0.10f }, WD, texWood);
    T({ 0.80f,0.07f, 0.28f }, { 0.10f,0.14f,0.10f }, WD, texWood);
}



static void renderWaterCooler(unsigned int sh, float x, float y, float z) {
    const glm::vec3 WH(0.92f, 0.92f, 0.94f), WB(0.20f, 0.60f, 0.85f);
    glm::mat4 base = glm::translate(glm::mat4(1.f), { x,y,z });
    auto T = [&](glm::vec3 t, glm::vec3 s, glm::vec3 c, unsigned int tex) {
        drawCube(sh, glm::scale(glm::translate(base, t), s), c, tex, 1.f);
        };
    auto S = [&](glm::vec3 t, glm::vec3 s, glm::vec3 c, unsigned int tex) {
        drawSphere(sh, glm::scale(glm::translate(base, t), s), c, tex, 1.f);
        };
    // body
    T({ 0.f,0.55f,0.f }, { 0.30f,1.10f,0.30f }, WH, texConcrete);
    // water tank (sphere on top)
    S({ 0.f,1.28f,0.f }, { 0.22f,0.28f,0.22f }, WB, texMarble);
    // buttons
    T({ -0.07f,0.60f,0.16f }, { 0.06f,0.06f,0.04f }, { 0.85f,0.20f,0.20f }, 0);
    T({ 0.07f,0.60f,0.16f }, { 0.06f,0.06f,0.04f }, { 0.20f,0.50f,0.85f }, 0);
    // base
    T({ 0.f,0.06f,0.f }, { 0.35f,0.12f,0.35f }, { 0.50f,0.50f,0.52f }, texConcrete);
}


static void renderACUnit(unsigned int sh, float x, float y, float z, float rotY = 0.f) {
    glm::mat4 base = glm::rotate(glm::translate(glm::mat4(1.f), { x,y,z }), glm::radians(rotY), { 0,1,0 });
    auto T = [&](glm::vec3 t, glm::vec3 s, glm::vec3 c, unsigned int tex) {
        drawCube(sh, glm::scale(glm::translate(base, t), s), c, tex, 1.f);
        };
    // main body
    T({ 0.f,0.f,0.f }, { 1.0f,0.22f,0.28f }, { 0.88f,0.88f,0.90f }, texConcrete);
    // vents
    for (int i = 0; i < 5; i++) {
        float vx = -0.36f + i * 0.18f;
        T({ vx,-0.06f,0.12f }, { 0.12f,0.06f,0.04f }, { 0.70f,0.70f,0.72f }, texConcrete);
    }
    // indicator light
    T({ 0.35f,0.04f,0.12f }, { 0.20f,0.06f,0.02f }, { 0.10f,0.80f,0.60f }, 0);
}

static void renderReceptionCounter(unsigned int sh, float x, float y, float z) {
    const glm::vec3 WM(0.55f, 0.38f, 0.18f), WH(0.90f, 0.88f, 0.85f);
    glm::mat4 base = glm::translate(glm::mat4(1.f), { x,y,z });
    auto T = [&](glm::vec3 t, glm::vec3 s, glm::vec3 c, unsigned int tex) {
        drawCube(sh, glm::scale(glm::translate(base, t), s), c, tex, 1.f);
        };
    // body
    T({ 0.f,0.55f,0.f }, { 2.5f,1.1f,0.55f }, WM, texWood);
    // marble top
    T({ 0.f,1.12f,0.f }, { 2.6f,0.06f,0.65f }, WH, texMarble);
    // monitor
    T({ 0.5f,1.35f,-0.10f }, { 0.45f,0.30f,0.04f }, { 0.05f,0.05f,0.15f }, 0);
    // monitor stand
    T({ 0.5f,1.17f,-0.05f }, { 0.06f,0.20f,0.08f }, { 0.45f,0.45f,0.48f }, texConcrete);
    // keyboard
    T({ 0.5f,1.14f, 0.10f }, { 0.38f,0.02f,0.14f }, { 0.40f,0.40f,0.42f }, texConcrete);
}

static void renderIndoorPlant(unsigned int sh, float x, float y, float z) {
    glm::mat4 base = glm::translate(glm::mat4(1.f), { x,y,z });
    glm::mat4 m;
    // pot body
    m = glm::scale(glm::translate(base, { 0.f,0.18f,0.f }), { 0.22f,0.36f,0.22f });
    drawCylinder(sh, m, { 0.60f,0.28f,0.15f }, texBrick, 1.f);
    // pot rim
    m = glm::scale(glm::translate(base, { 0.f,0.38f,0.f }), { 0.20f,0.04f,0.20f });
    drawCylinder(sh, m, { 0.28f,0.18f,0.08f }, texConcrete, 1.f);
    // lower foliage
    m = glm::scale(glm::translate(base, { 0.f,0.75f,0.f }), { 0.40f,0.50f,0.40f });
    drawCone(sh, m, { 0.15f,0.55f,0.20f }, texGrass, 1.f);
    // upper foliage
    m = glm::scale(glm::translate(base, { 0.f,1.00f,0.f }), { 0.25f,0.38f,0.25f });
    drawCone(sh, m, { 0.12f,0.50f,0.18f }, texGrass, 1.f);
}

// ── 9) renderCoffeeTable — top texMarble, stem+base texWood ──────────────────
static void renderCoffeeTable(unsigned int sh, float x, float y, float z) {
    glm::mat4 base = glm::translate(glm::mat4(1.f), { x,y,z });
    glm::mat4 m;
    // marble top
    m = glm::scale(glm::translate(base, { 0.f,0.38f,0.f }), { 0.70f,0.06f,0.70f });
    drawCylinder(sh, m, { 0.90f,0.88f,0.85f }, texMarble, 1.f);
    // stem
    m = glm::scale(glm::translate(base, { 0.f,0.20f,0.f }), { 0.10f,0.40f,0.10f });
    drawCylinder(sh, m, { 0.35f,0.22f,0.10f }, texWood, 1.f);
    // base
    m = glm::scale(glm::translate(base, { 0.f,0.04f,0.f }), { 0.50f,0.08f,0.50f });
    drawCylinder(sh, m, { 0.35f,0.22f,0.10f }, texWood, 1.f);
}

// ══════════════════════════════════════════════════════════════════════════════
//  EAST SIDE ROAD + WALKING PEDESTRIANS
// ══════════════════════════════════════════════════════════════════════════════
struct Pedestrian {
    float pathT;     // 0..1 position along current segment
    int   segment;   // which segment of the path (0-5)
    float speed;     // walking speed multiplier
    glm::vec3 shirtColor;
    glm::vec3 pantsColor;
    float walkPhase; // for leg animation
};
// ══════════════════════════════════════════════════════════════════════════════
//  MISSING DEFINITIONS FOR PEDESTRIANS
// ══════════════════════════════════════════════════════════════════════════════
const glm::vec3 COL_PANTS(0.30f, 0.30f, 0.35f);
static std::vector<Pedestrian> pedestrians;


// Simplified person rendering for pedestrians



// Road path: loops from front plaza, through east side, back to plaza
// Road path: loops in front of front building

//static const int ROAD_PATH_COUNT = 6;
//tatic const int ROAD_PATH_COUNT = 6;
// Road path: loops in front of front building
static const int ROAD_PATH_COUNT = 8;
static const glm::vec3 ROAD_PATH[] = {
    { -22.f, 1.0f,  20.f },   // seg 0: west, building front
    {  12.f, 1.0f,  20.f },   // seg 1: east, building front
    {  12.f, 1.0f,  28.f },   // seg 2: toward road
    {  35.f, 1.0f,  28.f },   // seg 3: road north end
    {  35.f, 1.0f, -35.f },   // seg 4: walk full road south
    {  28.f, 1.0f, -35.f },   // seg 5: cross road
    {  28.f, 1.0f,  28.f },   // seg 6: back north along road
    { -22.f, 1.0f,  20.f },   // seg 7: back to building front
};

static void renderPerson(unsigned int sh, float x, float y, float z,
    float rotY, float walkPhase,
    glm::vec3 shirtColor, glm::vec3 pantsColor)
{
    glm::mat4 base = glm::rotate(
        glm::translate(glm::mat4(1.f), { x, y, z }),
        glm::radians(rotY), { 0.f, 1.f, 0.f });

    float legSwing = sinf(walkPhase) * 22.f;   // degrees
    float armSwing = sinf(walkPhase + PI) * 18.f;
    float bodyLean = sinf(walkPhase * 2.f) * 1.2f;  // subtle torso sway
    float headBob = fabsf(sinf(walkPhase)) * 0.04f; // head bounces up

    // ── SHOES ──────────────────────────────────────────────────────────────
    glm::vec3 shoeCol(0.12f, 0.10f, 0.08f);
    // left shoe
    {
        glm::mat4 hip = glm::translate(base, { -0.07f, 0.f, 0.f });
        hip = glm::rotate(hip, glm::radians(-legSwing), { 1.f,0.f,0.f });
        glm::mat4 m = glm::scale(glm::translate(hip, { 0.f,-0.04f, 0.06f }),
            { 0.11f, 0.07f, 0.22f });
        drawCube(sh, m, shoeCol);
    }
    // right shoe
    {
        glm::mat4 hip = glm::translate(base, { 0.07f, 0.f, 0.f });
        hip = glm::rotate(hip, glm::radians(legSwing), { 1.f,0.f,0.f });
        glm::mat4 m = glm::scale(glm::translate(hip, { 0.f,-0.04f, 0.06f }),
            { 0.11f, 0.07f, 0.22f });
        drawCube(sh, m, shoeCol);
    }

    // ── LOWER LEGS (shin) ──────────────────────────────────────────────────
    glm::vec3 sockCol(0.85f, 0.85f, 0.85f);
    // left shin
    {
        glm::mat4 hip = glm::translate(base, { -0.07f, 0.f, 0.f });
        hip = glm::rotate(hip, glm::radians(-legSwing), { 1.f,0.f,0.f });
        // knee bends opposite to hip
        hip = glm::translate(hip, { 0.f, 0.22f, 0.f });
        hip = glm::rotate(hip, glm::radians(fabsf(legSwing) * 0.6f), { 1.f,0.f,0.f });
        glm::mat4 m = glm::scale(glm::translate(hip, { 0.f, 0.11f, 0.f }),
            { 0.09f, 0.22f, 0.09f });
        drawCube(sh, m, pantsColor);
    }
    // right shin
    {
        glm::mat4 hip = glm::translate(base, { 0.07f, 0.f, 0.f });
        hip = glm::rotate(hip, glm::radians(legSwing), { 1.f,0.f,0.f });
        hip = glm::translate(hip, { 0.f, 0.22f, 0.f });
        hip = glm::rotate(hip, glm::radians(fabsf(legSwing) * 0.6f), { 1.f,0.f,0.f });
        glm::mat4 m = glm::scale(glm::translate(hip, { 0.f, 0.11f, 0.f }),
            { 0.09f, 0.22f, 0.09f });
        drawCube(sh, m, pantsColor);
    }

    // ── UPPER LEGS (thigh) ─────────────────────────────────────────────────
    // left thigh
    {
        glm::mat4 hip = glm::translate(base, { -0.07f, 0.22f, 0.f });
        hip = glm::rotate(hip, glm::radians(-legSwing), { 1.f,0.f,0.f });
        glm::mat4 m = glm::scale(glm::translate(hip, { 0.f, 0.13f, 0.f }),
            { 0.12f, 0.26f, 0.12f });
        drawCube(sh, m, pantsColor);
    }
    // right thigh
    {
        glm::mat4 hip = glm::translate(base, { 0.07f, 0.22f, 0.f });
        hip = glm::rotate(hip, glm::radians(legSwing), { 1.f,0.f,0.f });
        glm::mat4 m = glm::scale(glm::translate(hip, { 0.f, 0.13f, 0.f }),
            { 0.12f, 0.26f, 0.12f });
        drawCube(sh, m, pantsColor);
    }

    // ── BELT ───────────────────────────────────────────────────────────────
    {
        glm::mat4 m = glm::scale(glm::translate(base, { 0.f, 0.50f, 0.f }),
            { 0.27f, 0.05f, 0.20f });
        drawCube(sh, m, glm::vec3(0.20f, 0.15f, 0.08f));
    }

    // ── TORSO ──────────────────────────────────────────────────────────────
    {
        glm::mat4 torsoBase = glm::translate(base, { 0.f, 0.53f, 0.f });
        torsoBase = glm::rotate(torsoBase, glm::radians(bodyLean), { 0.f,0.f,1.f });

        // main torso block
        glm::mat4 m = glm::scale(glm::translate(torsoBase, { 0.f, 0.20f, 0.f }),
            { 0.30f, 0.38f, 0.20f });
        drawCube(sh, m, shirtColor);

        // shirt collar shadow
        m = glm::scale(glm::translate(torsoBase, { 0.f, 0.39f, 0.f }),
            { 0.18f, 0.05f, 0.18f });
        drawCube(sh, m, shirtColor * 0.8f);

        // shirt front detail (buttons strip)
        m = glm::scale(glm::translate(torsoBase, { 0.f, 0.20f, 0.102f }),
            { 0.05f, 0.28f, 0.01f });
        drawCube(sh, m, shirtColor * 0.75f);

        // ── UPPER ARMS ─────────────────────────────────────────────────────
        glm::vec3 skinCol(0.92f, 0.78f, 0.66f);
        // left upper arm
        {
            glm::mat4 sh2 = glm::translate(torsoBase, { -0.18f, 0.30f, 0.f });
            sh2 = glm::rotate(sh2, glm::radians(armSwing), { 1.f,0.f,0.f });
            glm::mat4 am = glm::scale(glm::translate(sh2, { 0.f,-0.10f, 0.f }),
                { 0.09f, 0.20f, 0.09f });
            drawCube(sh, am, shirtColor);

            // left forearm
            sh2 = glm::translate(sh2, { 0.f,-0.20f, 0.f });
            sh2 = glm::rotate(sh2, glm::radians(20.f), { 1.f,0.f,0.f });
            am = glm::scale(glm::translate(sh2, { 0.f,-0.09f, 0.f }),
                { 0.08f, 0.18f, 0.08f });
            drawCube(sh, am, skinCol);
        }
        // right upper arm
        {
            glm::mat4 sh2 = glm::translate(torsoBase, { 0.18f, 0.30f, 0.f });
            sh2 = glm::rotate(sh2, glm::radians(-armSwing), { 1.f,0.f,0.f });
            glm::mat4 am = glm::scale(glm::translate(sh2, { 0.f,-0.10f, 0.f }),
                { 0.09f, 0.20f, 0.09f });
            drawCube(sh, am, shirtColor);

            // right forearm
            sh2 = glm::translate(sh2, { 0.f,-0.20f, 0.f });
            sh2 = glm::rotate(sh2, glm::radians(20.f), { 1.f,0.f,0.f });
            am = glm::scale(glm::translate(sh2, { 0.f,-0.09f, 0.f }),
                { 0.08f, 0.18f, 0.08f });
            drawCube(sh, am, skinCol);
        }

        // ── NECK ───────────────────────────────────────────────────────────
        glm::mat4 neckBase = glm::translate(torsoBase, { 0.f, 0.43f, 0.f });
        glm::mat4 nm = glm::scale(neckBase, { 0.09f, 0.10f, 0.09f });
        drawCylinder(sh, nm, skinCol);

        // ── HEAD ───────────────────────────────────────────────────────────
        glm::mat4 headBase = glm::translate(torsoBase,
            { 0.f, 0.53f + headBob, 0.f });

        // skull (slightly flattened sphere)
        nm = glm::scale(headBase, { 0.19f, 0.22f, 0.19f });
        drawSphere(sh, nm, skinCol);

        // hair cap
        nm = glm::scale(glm::translate(headBase, { 0.f, 0.08f, -0.02f }),
            { 0.20f, 0.14f, 0.20f });
        drawSphere(sh, nm, glm::vec3(0.15f, 0.10f, 0.05f));

        // eyes (left)
        nm = glm::scale(glm::translate(headBase, { -0.06f, 0.02f, 0.17f }),
            { 0.03f, 0.03f, 0.03f });
        drawSphere(sh, nm, glm::vec3(0.08f, 0.05f, 0.02f));
        // eyes (right)
        nm = glm::scale(glm::translate(headBase, { 0.06f, 0.02f, 0.17f }),
            { 0.03f, 0.03f, 0.03f });
        drawSphere(sh, nm, glm::vec3(0.08f, 0.05f, 0.02f));

        // nose
        nm = glm::scale(glm::translate(headBase, { 0.f, -0.02f, 0.19f }),
            { 0.025f, 0.025f, 0.04f });
        drawSphere(sh, nm, skinCol * 0.88f);
    }
}

static void initPedestrians() {
    // varied skin tones for realism
    glm::vec3 shirts[] = {
        {0.78f,0.15f,0.12f}, {0.15f,0.28f,0.75f}, {0.82f,0.62f,0.12f},
        {0.12f,0.62f,0.28f}, {0.62f,0.12f,0.62f}, {0.92f,0.52f,0.18f},
        {0.18f,0.68f,0.78f}, {0.78f,0.78f,0.18f}, {0.52f,0.20f,0.42f},
        {0.22f,0.42f,0.32f}
    };
    glm::vec3 pants[] = {
        {0.15f,0.20f,0.38f}, {0.28f,0.28f,0.32f}, {0.35f,0.22f,0.10f},
        {0.10f,0.18f,0.10f}, {0.32f,0.32f,0.35f}, {0.20f,0.20f,0.22f},
        {0.38f,0.28f,0.18f}, {0.18f,0.22f,0.38f}, {0.30f,0.30f,0.30f},
        {0.25f,0.18f,0.12f}
    };
    pedestrians.clear();
    for (int i = 0; i < 10; i++) {
        Pedestrian p;
        p.segment = (i * 2) % ROAD_PATH_COUNT;
        p.pathT = (float)i / 10.f;
        p.speed = 0.10f + (i % 5) * 0.025f;
        p.shirtColor = shirts[i % 10];
        p.pantsColor = pants[i % 10];
        p.walkPhase = (float)i * 0.71f;
        pedestrians.push_back(p);
    }
}
static void updatePedestrians(float dt) {
    for (auto& ped : pedestrians) {
        ped.pathT += ped.speed * dt;
        if (ped.pathT > 1.f) {
            ped.pathT -= 1.f;
            ped.segment = (ped.segment + 1) % ROAD_PATH_COUNT;
        }
        ped.walkPhase += dt * 8.f; // animation speed
        if (ped.walkPhase > 2.f * PI) ped.walkPhase -= 2.f * PI;
    }
}

static void renderPedestrians(unsigned int sh) {
    for (auto& ped : pedestrians) {
        int seg = ped.segment;
        int nextSeg = (seg + 1) % ROAD_PATH_COUNT;
        glm::vec3 from = ROAD_PATH[seg];
        glm::vec3 to = ROAD_PATH[nextSeg];
        glm::vec3 pos = glm::mix(from, to, ped.pathT);

        glm::vec3 dir = glm::normalize(to - from);
        float rotY = glm::degrees(atan2f(dir.x, dir.z));

        renderPerson(sh, pos.x, pos.y, pos.z, rotY, ped.walkPhase, ped.shirtColor, ped.pantsColor);
    }
}
static void renderRoad(unsigned int sh) {
    // Main road surface - longer
    glm::mat4 m = glm::scale(glm::translate(glm::mat4(1.f), { 32.f, 0.02f, 0.f }), { 48.f, 0.05f, 60.f });
    drawCube(sh, m, { 0.35f, 0.35f, 0.35f }, texConcrete, 2.f);

    // Road markings
    for (int i = 0; i < 12; i++) {
        float z = -35.f + i * 6.5f;
        m = glm::scale(glm::translate(glm::mat4(1.f), { 32.f, 0.04f, z }), { 1.2f, 0.02f, 2.5f });
        drawCube(sh, m, { 0.90f, 0.90f, 0.20f }, 0, 1.f);
    }

    // Road shoulder edges
    m = glm::scale(glm::translate(glm::mat4(1.f), { 56.f, 0.02f, 0.f }), { 0.6f, 0.04f, 60.f });
    drawCube(sh, m, { 0.55f, 0.50f, 0.48f }, 0, 1.f);
    m = glm::scale(glm::translate(glm::mat4(1.f), { 14.f, 0.02f, 0.f }), { 0.6f, 0.04f, 60.f });
    drawCube(sh, m, { 0.55f, 0.50f, 0.48f }, 0, 1.f);
}
static float tvTime = 0.f;

static void renderTV(unsigned int sh, float x, float y, float z, float rotY = 0.f) {
    glm::mat4 base = glm::rotate(
        glm::translate(glm::mat4(1.f), { x, y, z }),
        glm::radians(rotY), { 0.f, 1.f, 0.f });

    auto T = [&](glm::vec3 t, glm::vec3 s, glm::vec3 c) {
        drawCube(sh, glm::scale(glm::translate(base, t), s), c);
        };
    auto C = [&](glm::vec3 t, glm::vec3 s, glm::vec3 c) {
        drawCylinder(sh, glm::scale(glm::translate(base, t), s), c);
        };
    auto S = [&](glm::vec3 t, glm::vec3 s, glm::vec3 c) {
        drawSphere(sh, glm::scale(glm::translate(base, t), s), c);
        };

    auto setEmissive = [&](glm::mat4 m, glm::vec3 col, glm::vec3 em) {
        static const glm::vec3 zero(0.f);
        glUniformMatrix4fv(glGetUniformLocation(sh, "model"), 1, GL_FALSE, glm::value_ptr(m));
        glUniform3fv(glGetUniformLocation(sh, "objectColor"), 1, glm::value_ptr(col));
        glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(em));
        glUniform1i(glGetUniformLocation(sh, "hasTexture"), 0);
        glUniform1i(glGetUniformLocation(sh, "texMode"), 0);
        glBindVertexArray(cubeVAO); glDrawArrays(GL_TRIANGLES, 0, 36);
        glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(zero));
        };
    auto setEmissiveCyl = [&](glm::mat4 m, glm::vec3 col, glm::vec3 em) {
        static const glm::vec3 zero(0.f);
        glUniformMatrix4fv(glGetUniformLocation(sh, "model"), 1, GL_FALSE, glm::value_ptr(m));
        glUniform3fv(glGetUniformLocation(sh, "objectColor"), 1, glm::value_ptr(col));
        glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(em));
        glUniform1i(glGetUniformLocation(sh, "hasTexture"), 0);
        glUniform1i(glGetUniformLocation(sh, "texMode"), 0);
        glBindVertexArray(cylVAO); glDrawArrays(GL_TRIANGLES, 0, cylCount);
        glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(zero));
        };
    auto setEmissiveSph = [&](glm::mat4 m, glm::vec3 col, glm::vec3 em) {
        static const glm::vec3 zero(0.f);
        glUniformMatrix4fv(glGetUniformLocation(sh, "model"), 1, GL_FALSE, glm::value_ptr(m));
        glUniform3fv(glGetUniformLocation(sh, "objectColor"), 1, glm::value_ptr(col));
        glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(em));
        glUniform1i(glGetUniformLocation(sh, "hasTexture"), 0);
        glUniform1i(glGetUniformLocation(sh, "texMode"), 0);
        glBindVertexArray(sphVAO); glDrawArrays(GL_TRIANGLES, 0, sphCount);
        glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(zero));
        };

    // ── TV BODY ────────────────────────────────────────────────────────────
    T({ 0.f, 0.f, 0.f }, { 1.80f, 1.05f, 0.10f }, { 0.06f,0.06f,0.08f });
    T({ 0.f, 0.f, 0.052f }, { 1.68f, 0.93f, 0.02f }, { 0.04f,0.04f,0.06f });

    // ── SCREEN BASE (green football pitch) ────────────────────────────────
    {
        glm::vec3 pitchCol(0.08f, 0.52f, 0.12f);
        glm::vec3 pitchEm = pitchCol * 1.4f;
        glm::mat4 sm = glm::scale(glm::translate(base, { 0.f,0.f,0.065f }),
            { 1.60f,0.85f,0.01f });
        setEmissive(sm, pitchCol, pitchEm);
    }

    // ── PITCH STRIPES (alternating dark/light green) ───────────────────────
    for (int i = 0; i < 8; i++) {
        if (i % 2 == 0) continue;
        float sx = -0.76f + i * 0.20f;
        glm::vec3 sc(0.06f, 0.42f, 0.09f);
        glm::mat4 sm = glm::scale(glm::translate(base, { sx, 0.f, 0.067f }),
            { 0.18f, 0.83f, 0.008f });
        setEmissive(sm, sc, sc * 1.3f);
    }

    // ── PITCH LINES (white) ────────────────────────────────────────────────
    glm::vec3 lineCol(0.95f, 0.95f, 0.95f);
    glm::vec3 lineEm = lineCol * 2.0f;

    // center line
    {
        glm::mat4 lm = glm::scale(glm::translate(base, { 0.f, 0.f, 0.070f }),
            { 0.012f, 0.83f, 0.005f });
        setEmissive(lm, lineCol, lineEm);
    }
    // center circle (approximated with thin ring segments)
    for (int i = 0; i < 16; i++) {
        float ang = 2.f * PI * i / 16.f;
        float nx = cosf(ang) * 0.18f;
        float ny = sinf(ang) * 0.32f;
        glm::mat4 lm = glm::scale(
            glm::translate(base, { nx, ny, 0.070f }),
            { 0.018f, 0.018f, 0.005f });
        setEmissive(lm, lineCol, lineEm);
    }
    // top boundary line
    {
        glm::mat4 lm = glm::scale(glm::translate(base, { 0.f, 0.415f, 0.070f }),
            { 1.58f, 0.012f, 0.005f });
        setEmissive(lm, lineCol, lineEm);
    }
    // bottom boundary line
    {
        glm::mat4 lm = glm::scale(glm::translate(base, { 0.f,-0.415f, 0.070f }),
            { 1.58f, 0.012f, 0.005f });
        setEmissive(lm, lineCol, lineEm);
    }
    // left boundary
    {
        glm::mat4 lm = glm::scale(glm::translate(base, { -0.78f, 0.f, 0.070f }),
            { 0.012f, 0.83f, 0.005f });
        setEmissive(lm, lineCol, lineEm);
    }
    // right boundary
    {
        glm::mat4 lm = glm::scale(glm::translate(base, { 0.78f, 0.f, 0.070f }),
            { 0.012f, 0.83f, 0.005f });
        setEmissive(lm, lineCol, lineEm);
    }
    // left penalty box
    {
        glm::mat4 lm = glm::scale(glm::translate(base, { -0.58f, 0.f, 0.070f }),
            { 0.012f, 0.44f, 0.005f });
        setEmissive(lm, lineCol, lineEm);
        lm = glm::scale(glm::translate(base, { -0.68f, 0.22f, 0.070f }),
            { 0.20f, 0.012f, 0.005f });
        setEmissive(lm, lineCol, lineEm);
        lm = glm::scale(glm::translate(base, { -0.68f,-0.22f, 0.070f }),
            { 0.20f, 0.012f, 0.005f });
        setEmissive(lm, lineCol, lineEm);
    }
    // right penalty box
    {
        glm::mat4 lm = glm::scale(glm::translate(base, { 0.58f, 0.f, 0.070f }),
            { 0.012f, 0.44f, 0.005f });
        setEmissive(lm, lineCol, lineEm);
        lm = glm::scale(glm::translate(base, { 0.68f, 0.22f, 0.070f }),
            { 0.20f, 0.012f, 0.005f });
        setEmissive(lm, lineCol, lineEm);
        lm = glm::scale(glm::translate(base, { 0.68f,-0.22f, 0.070f }),
            { 0.20f, 0.012f, 0.005f });
        setEmissive(lm, lineCol, lineEm);
    }

    // ── ANIMATED BALL ──────────────────────────────────────────────────────
    // Ball moves in a figure-8 / dribble path across the pitch
    float bt = tvTime * 0.8f;
    float ballX = sinf(bt) * 0.55f;
    float ballY = sinf(bt * 2.f) * 0.28f;
    // bounce: ball bobs up on each "step"
    float bounce = fabsf(sinf(bt * 4.f)) * 0.04f;
    ballY += bounce;

    // ball shadow on pitch
    {
        glm::vec3 shadowC(0.04f, 0.35f, 0.08f);
        glm::mat4 sm = glm::scale(
            glm::translate(base, { ballX, ballY - bounce - 0.01f, 0.071f }),
            { 0.055f, 0.018f, 0.005f });
        setEmissive(sm, shadowC, shadowC);
    }
    // ball body (white)
    {
        glm::vec3 bc(0.95f, 0.95f, 0.95f);
        glm::mat4 bm = glm::scale(
            glm::translate(base, { ballX, ballY, 0.075f }),
            { 0.055f, 0.055f, 0.055f });
        setEmissiveSph(bm, bc, bc * 1.5f);
    }
    // ball black patches (3 small dots rotating with ball)
    for (int p = 0; p < 3; p++) {
        float pang = tvTime * 3.f + p * 2.094f;
        float px = ballX + cosf(pang) * 0.018f;
        float py = ballY + sinf(pang) * 0.018f;
        glm::vec3 pc(0.05f, 0.05f, 0.05f);
        glm::mat4 pm = glm::scale(
            glm::translate(base, { px, py, 0.080f }),
            { 0.016f, 0.016f, 0.010f });
        setEmissive(pm, pc, pc * 0.5f);
    }

    // ── ANIMATED PLAYERS ──────────────────────────────────────────────────
    // Team A (red) — 3 players chasing ball
    glm::vec3 teamA(0.85f, 0.12f, 0.12f);
    glm::vec3 teamB(0.12f, 0.25f, 0.85f);
    glm::vec3 playerSkin(0.88f, 0.72f, 0.58f);

    auto drawTVPlayer = [&](float px, float py, glm::vec3 teamCol, float phase) {
        // body
        glm::mat4 pm = glm::scale(
            glm::translate(base, { px, py + 0.04f, 0.076f }),
            { 0.045f, 0.07f, 0.02f });
        setEmissive(pm, teamCol, teamCol * 1.2f);
        // head
        pm = glm::scale(
            glm::translate(base, { px, py + 0.095f, 0.076f }),
            { 0.030f, 0.030f, 0.020f });
        setEmissiveSph(pm, playerSkin, playerSkin * 1.0f);
        // left leg
        float legL = sinf(phase) * 0.025f;
        pm = glm::scale(
            glm::translate(base, { px - 0.012f, py - 0.01f + legL, 0.076f }),
            { 0.016f, 0.05f, 0.015f });
        setEmissive(pm, { 0.20f,0.20f,0.25f }, { 0.10f,0.10f,0.12f });
        // right leg
        pm = glm::scale(
            glm::translate(base, { px + 0.012f, py - 0.01f - legL, 0.076f }),
            { 0.016f, 0.05f, 0.015f });
        setEmissive(pm, { 0.20f,0.20f,0.25f }, { 0.10f,0.10f,0.12f });
        };

    // Team A players — orbit loosely around ball
    for (int i = 0; i < 3; i++) {
        float angle = tvTime * 0.5f + i * 2.094f;
        float dist = 0.18f + i * 0.06f;
        float px = ballX + cosf(angle) * dist;
        float py = ballY + sinf(angle) * dist * 0.5f;
        px = glm::clamp(px, -0.72f, 0.72f);
        py = glm::clamp(py, -0.38f, 0.38f);
        drawTVPlayer(px, py, teamA, tvTime * 6.f + i * 1.2f);
    }
    // Team B players — defend opposite side
    for (int i = 0; i < 3; i++) {
        float angle = tvTime * 0.4f + i * 2.094f + PI;
        float dist = 0.22f + i * 0.07f;
        float px = -ballX + cosf(angle) * dist;
        float py = ballY + sinf(angle) * dist * 0.5f;
        px = glm::clamp(px, -0.72f, 0.72f);
        py = glm::clamp(py, -0.38f, 0.38f);
        drawTVPlayer(px, py, teamB, tvTime * 6.f + i * 1.5f + PI);
    }

    // ── GOALPOSTS ─────────────────────────────────────────────────────────
    glm::vec3 postCol(0.92f, 0.92f, 0.92f);
    // left goal
    {
        glm::mat4 gm = glm::scale(glm::translate(base, { -0.79f, 0.f, 0.072f }),
            { 0.008f, 0.22f, 0.008f });
        setEmissive(gm, postCol, postCol * 1.5f);
        gm = glm::scale(glm::translate(base, { -0.76f, 0.11f, 0.072f }),
            { 0.06f, 0.008f, 0.008f });
        setEmissive(gm, postCol, postCol * 1.5f);
    }
    // right goal
    {
        glm::mat4 gm = glm::scale(glm::translate(base, { 0.79f, 0.f, 0.072f }),
            { 0.008f, 0.22f, 0.008f });
        setEmissive(gm, postCol, postCol * 1.5f);
        gm = glm::scale(glm::translate(base, { 0.76f, 0.11f, 0.072f }),
            { 0.06f, 0.008f, 0.008f });
        setEmissive(gm, postCol, postCol * 1.5f);
    }

    // ── SCOREBOARD BANNER (top of screen) ─────────────────────────────────
    {
        // banner background
        glm::vec3 bannerC(0.05f, 0.05f, 0.15f);
        glm::mat4 bm = glm::scale(glm::translate(base, { 0.f, 0.385f, 0.071f }),
            { 1.58f, 0.08f, 0.005f });
        setEmissive(bm, bannerC, bannerC * 2.f);

        // team A score block (red)
        bm = glm::scale(glm::translate(base, { -0.38f, 0.385f, 0.072f }),
            { 0.28f, 0.065f, 0.004f });
        setEmissive(bm, teamA, teamA * 1.3f);

        // team B score block (blue)
        bm = glm::scale(glm::translate(base, { 0.38f, 0.385f, 0.072f }),
            { 0.28f, 0.065f, 0.004f });
        setEmissive(bm, teamB, teamB * 1.3f);

        // score separator (white)
        bm = glm::scale(glm::translate(base, { 0.f, 0.385f, 0.073f }),
            { 0.06f, 0.065f, 0.003f });
        setEmissive(bm, lineCol, lineEm);

        // pulsing "LIVE" dot
        float livePulse = (sinf(tvTime * 3.f) > 0.f) ? 1.f : 0.2f;
        glm::vec3 liveC(livePulse, 0.05f, 0.05f);
        bm = glm::scale(glm::translate(base, { 0.68f, 0.385f, 0.073f }),
            { 0.055f, 0.040f, 0.003f });
        setEmissive(bm, liveC, liveC * 2.f);
    }

    // ── CROWD FLICKER (bottom strip) ──────────────────────────────────────
    for (int i = 0; i < 14; i++) {
        float cx = -0.74f + i * 0.112f;
        float flk = 0.3f + 0.7f * fabsf(sinf(tvTime * 3.f + i * 0.8f));
        glm::vec3 crowdC(flk * 0.6f, flk * 0.3f, flk * 0.1f);
        glm::mat4 cm = glm::scale(
            glm::translate(base, { cx, -0.39f, 0.071f }),
            { 0.09f, 0.04f, 0.004f });
        setEmissive(cm, crowdC, crowdC * 1.5f);
    }

    // ── TV STAND ──────────────────────────────────────────────────────────
    T({ 0.f,-0.60f, 0.05f }, { 0.08f,0.22f,0.08f }, { 0.10f,0.10f,0.12f });
    T({ 0.f,-0.72f, 0.10f }, { 0.55f,0.05f,0.30f }, { 0.10f,0.10f,0.12f });

    // ── SPEAKER GRILLES ───────────────────────────────────────────────────
    for (int s = -1; s <= 1; s += 2) {
        float sx = s * 0.92f;
        T({ sx, 0.f, 0.04f }, { 0.06f,0.80f,0.06f }, { 0.08f,0.08f,0.10f });
        for (int g = 0; g < 5; g++) {
            float gy = -0.28f + g * 0.14f;
            T({ sx, gy, 0.055f }, { 0.04f,0.02f,0.02f }, { 0.05f,0.05f,0.06f });
        }
    }

    // ── POWER LED ─────────────────────────────────────────────────────────
    float ledBlink = (sinf(tvTime * 1.5f) > 0.7f) ? 1.f : 0.3f;
    {
        glm::vec3 lc(0.f, ledBlink, 0.f);
        glm::mat4 lm = glm::scale(
            glm::translate(base, { -0.82f,-0.45f, 0.06f }),
            { 0.03f,0.03f,0.03f });
        setEmissive(lm, lc, lc * 2.f);
    }
}

static void renderComputerDesk(unsigned int sh, float x, float y, float z, float rotY = 0.f) {
    glm::mat4 base = glm::rotate(glm::translate(glm::mat4(1.f), { x,y,z }), glm::radians(rotY), { 0,1,0 });
    auto T = [&](glm::vec3 t, glm::vec3 s, glm::vec3 c, unsigned int tex = 0) {drawCube(sh, glm::scale(glm::translate(base, t), s), c, tex ? tex : 0, 1.f); };
    T({ 0.f,0.75f,0.f }, { 1.4f,0.05f,0.65f }, { 0.55f,0.38f,0.18f }, texWood);
    T({ -0.65f,0.37f,-0.28f }, { 0.06f,0.75f,0.06f }, { 0.35f,0.22f,0.10f });
    T({ 0.65f,0.37f,-0.28f }, { 0.06f,0.75f,0.06f }, { 0.35f,0.22f,0.10f });
    T({ -0.65f,0.37f, 0.28f }, { 0.06f,0.75f,0.06f }, { 0.35f,0.22f,0.10f });
    T({ 0.65f,0.37f, 0.28f }, { 0.06f,0.75f,0.06f }, { 0.35f,0.22f,0.10f });
    T({ 0.f,1.18f,-0.22f }, { 0.55f,0.35f,0.04f }, { 0.05f,0.05f,0.15f });
    T({ 0.f,0.84f,-0.18f }, { 0.07f,0.18f,0.07f }, { 0.45f,0.45f,0.48f });
    T({ 0.f,0.80f,-0.10f }, { 0.28f,0.03f,0.14f }, { 0.45f,0.45f,0.48f });
    T({ 0.f,0.78f, 0.05f }, { 0.44f,0.02f,0.16f }, { 0.40f,0.40f,0.42f });
    T({ 0.32f,0.78f,0.05f }, { 0.10f,0.03f,0.14f }, { 0.40f,0.40f,0.42f });
    T({ -0.55f,1.05f,-0.22f }, { 0.18f,0.40f,0.24f }, { 0.50f,0.50f,0.52f });
}
static void renderCarpet(unsigned int sh, float x, float y, float z, float w, float d) {
    glm::mat4 m = glm::scale(glm::translate(glm::mat4(1.f), { x,y + 0.01f,z }), { w,0.03f,d });
    drawCube(sh, m, { 0.30f,0.45f,0.35f }, texGrass, 2.f);
}
// ══════════════════════════════════════════════════════════════════════════════
//  SCENE: GROUND
// ══════════════════════════════════════════════════════════════════════════════
static void renderGround(unsigned int sh) {
    glm::mat4 m = glm::scale(glm::translate(glm::mat4(1.f), { 0.f,-0.05f,0.f }), { 300.f,0.1f,300.f });
    drawCube(sh, m, COL_GRASS, texGrass, 30.f);
    m = glm::scale(glm::translate(glm::mat4(1.f), { 0.f,0.01f,22.f }), { 50.f,0.08f,30.f });
    drawCube(sh, m, COL_PLAZA, texBrick, 6.f);
}

// ══════════════════════════════════════════════════════════════════════════════
//  SCENE: PALM TREE
// ══════════════════════════════════════════════════════════════════════════════
static void renderPalmTree(unsigned int sh, glm::vec3 base) {
    for (int i = 0; i < 5; i++) {
        float y = base.y + 0.5f + i, sc = 0.20f - i * 0.015f;
        glm::mat4 m = glm::translate(glm::mat4(1.f), { base.x,y,base.z });
        m = glm::rotate(m, glm::radians((float)(i * 4)), { 0,1,0 });
        m = glm::scale(m, { sc,1.f,sc });
        m = glm::rotate(m, glm::radians(90.f), { 1,0,0 });
        drawCylinder(sh, m, COL_TRUNK, texWood, 1.f);
    }
    for (int i = 0; i < 6; i++) {
        glm::mat4 m = glm::translate(glm::mat4(1.f), { base.x,base.y + 5.5f,base.z });
        m = glm::rotate(m, glm::radians(i * 60.f), { 0,1,0 });
        m = glm::translate(m, { 1.2f,0.3f,0.f });
        m = glm::rotate(m, glm::radians(-30.f), { 0,0,1 });
        m = glm::scale(m, { 2.5f,0.12f,0.5f });
        drawCube(sh, m, COL_PALM, texGrass, 1.f);
    }
}

// ══════════════════════════════════════════════════════════════════════════════
//  SCENE: FRONT BUILDING
// ══════════════════════════════════════════════════════════════════════════════
static void renderMainBuilding(unsigned int sh) {
    const float FZ = 3.f, FX = -5.f;
    const float FZN = 8.5f, FZS = -2.f;
    const float FXLEN = 38.f;
    glm::mat4 m;

    m = glm::scale(glm::translate(glm::mat4(1.f), { FX - 19.2f,4.f,FZ }), { 0.4f,8.f,10.f });
    drawCube(sh, m, COL_CONCRETE, texConcrete, 2.f);
    m = glm::scale(glm::translate(glm::mat4(1.f), { FX + 19.2f,4.f,FZ }), { 0.4f,8.f,10.f });
    drawCube(sh, m, COL_CONCRETE, texConcrete, 2.f);
    m = glm::scale(glm::translate(glm::mat4(1.f), { FX,8.2f,FZ }), { FXLEN + 1.f,0.4f,11.f });
    drawCube(sh, m, COL_ROOF, texConcrete, 5.f);
    m = glm::scale(glm::translate(glm::mat4(1.f), { FX,0.3f,FZ }), { FXLEN + 1.f,0.6f,11.5f });
    drawCube(sh, m, COL_CONCRETE, texConcrete, 5.f);

    drawAnimatedDoor(sh, FX - 3.8f, 0.6f, FZN + 0.3f, 4.f, 8.0f, 0.12f, doorOpenAngle, true, 1.f);
    drawAnimatedDoor(sh, FX + 3.8f, 0.6f, FZN + 0.3f, 4.f, 8.0f, 0.12f, doorOpenAngle, false, 1.f);
    drawAnimatedDoor(sh, FX - 3.8f, 0.6f, FZS - 0.3f, 4.f, 8.0f, 0.12f, doorOpenAngle, true, -1.f);
    drawAnimatedDoor(sh, FX + 3.8f, 0.6f, FZS - 0.3f, 4.f, 8.0f, 0.12f, doorOpenAngle, false, -1.f);

    for (int i = 0; i < 11; i++) {
        float x = FX - 19.f + i * 3.8f;
        glm::mat4 cm = glm::translate(glm::mat4(1.f), { x,4.f,FZS });
        cm = glm::rotate(cm, glm::radians(90.f), { 1,0,0 }); cm = glm::scale(cm, { 0.45f,0.45f,8.f });
        drawCylinder(sh, cm, COL_COLUMN, texConcrete, 2.f);
        cm = glm::scale(glm::translate(glm::mat4(1.f), { x,8.f,FZS }), { 0.6f,0.4f,0.6f });
        drawCube(sh, cm, COL_COLUMN, texConcrete, 1.f);
    }
    for (int i = 0; i < 10; i++) {
        float x = FX - 17.1f + i * 3.8f;
        if (x > FX - 3.5f && x < FX + 3.5f) continue;
        m = glm::scale(glm::translate(glm::mat4(1.f), { x,4.f,FZS }), { 3.3f,7.5f,0.3f });
        drawGlass(sh, m);
    }
    for (int i = 0; i < 11; i++) {
        float x = FX - 19.f + i * 3.8f;
        glm::mat4 cm = glm::translate(glm::mat4(1.f), { x,4.f,FZN });
        cm = glm::rotate(cm, glm::radians(90.f), { 1,0,0 }); cm = glm::scale(cm, { 0.45f,0.45f,8.f });
        drawCylinder(sh, cm, COL_COLUMN, texConcrete, 2.f);
        cm = glm::scale(glm::translate(glm::mat4(1.f), { x,8.f,FZN }), { 0.6f,0.4f,0.6f });
        drawCube(sh, cm, COL_COLUMN, texConcrete, 1.f);
    }
    for (int i = 0; i < 10; i++) {
        float x = FX - 17.1f + i * 3.8f;
        if (x > FX - 3.5f && x < FX + 3.5f) continue;
        m = glm::scale(glm::translate(glm::mat4(1.f), { x,4.f,FZN }), { 3.3f,7.5f,0.3f });
        drawCrystalGlass(sh, m);
    }
}

// ══════════════════════════════════════════════════════════════════════════════
//  SCENE: BACK BUILDING
// ══════════════════════════════════════════════════════════════════════════════
static void renderBackBuilding(unsigned int sh) {
    const float BZ = -22.f, BX = -5.f;
    const float BZN = BZ + 5.f, BZS = BZ - 5.f;
    const float BLEN = 38.f;
    glm::mat4 m;

    m = glm::scale(glm::translate(glm::mat4(1.f), { BX - 19.2f,4.f,BZ }), { 0.4f,8.f,10.f });
    drawCube(sh, m, COL_CONCRETE, texConcrete, 2.f);
    m = glm::scale(glm::translate(glm::mat4(1.f), { BX + 19.2f,4.f,BZ }), { 0.4f,8.f,10.f });
    drawCube(sh, m, COL_CONCRETE, texConcrete, 2.f);
    m = glm::scale(glm::translate(glm::mat4(1.f), { BX,8.2f,BZ }), { BLEN + 1.f,0.4f,11.f });
    drawCube(sh, m, COL_ROOF, texConcrete, 5.f);
    m = glm::scale(glm::translate(glm::mat4(1.f), { BX,0.3f,BZ }), { BLEN + 1.f,0.6f,11.5f });
    drawCube(sh, m, COL_CONCRETE, texConcrete, 5.f);

    for (int i = 0; i < 11; i++) {
        float x = BX - 19.f + i * 3.8f;
        glm::mat4 cm = glm::translate(glm::mat4(1.f), { x,4.f,BZN });
        cm = glm::rotate(cm, glm::radians(90.f), { 1,0,0 }); cm = glm::scale(cm, { 0.45f,0.45f,8.f });
        drawCylinder(sh, cm, COL_COLUMN, texConcrete, 2.f);
        cm = glm::scale(glm::translate(glm::mat4(1.f), { x,8.f,BZN }), { 0.6f,0.4f,0.6f });
        drawCube(sh, cm, COL_COLUMN, texConcrete, 1.f);
    }
    for (int i = 0; i < 10; i++) {
        float x = BX - 17.1f + i * 3.8f;
        if (x > BX - 3.5f && x < BX + 3.5f) continue;
        m = glm::scale(glm::translate(glm::mat4(1.f), { x,4.f,BZN }), { 3.3f,7.5f,0.3f });
        drawGlass(sh, m);
    }
    m = glm::scale(glm::translate(glm::mat4(1.f), { BX,4.f,BZS }), { BLEN,8.f,0.4f });
    drawCube(sh, m, COL_CONCRETE, texConcrete, 5.f);
    drawAnimatedDoor(sh, BX - 3.8f, 0.6f, BZN + 0.3f, 4.0f, 8.0f, 0.12f, doorOpenAngle, true, 1.f);
    drawAnimatedDoor(sh, BX + 3.8f, 0.6f, BZN + 0.3f, 4.0f, 8.0f, 0.12f, doorOpenAngle, false, 1.f);
}

// ══════════════════════════════════════════════════════════════════════════════
//  SCENE: CONNECTOR

//  Only concrete walls remain. Two doors: east (cafeteria) + west.
// ══════════════════════════════════════════════════════════════════════════════
static void renderConnector(unsigned int sh) {
    const float MX = 15.f, MZ = -10.5f;
    const float XWID = 10.f, ZDEP = 38.f;
    const float ZN = MZ + ZDEP * 0.5f;   //  8.5
    const float ZS = MZ - ZDEP * 0.5f;   // -29.5
    const float STEP = ZDEP / 10.f;
    glm::mat4 m;

    // ── door geometry constants ─────────────────────────────────────────────
    const float DOOR_Z_HALF = 1.45f;
    const float DOOR_Z_MIN = MZ - DOOR_Z_HALF;   // east door
    const float DOOR_Z_MAX = MZ + DOOR_Z_HALF;
    const float DOOR_WIDTH = DOOR_Z_MAX - DOOR_Z_MIN;
    const float DOOR_HEIGHT = 8.0f;
    const float DOOR_THICK = 0.10f;
    const float EAST_X = MX + XWID * 0.5f;  // 20.0
    const float WEST_X = MX - XWID * 0.5f;  // 10.0

    // ── A)  CAFETERIA SIGN (east wall, unchanged) ───────────────────────────
    {
        const float LW = 0.90f, LH = 1.36f, LD = 0.20f, SP = 1.04f;
        const float SX = EAST_X + 0.22f;
        const float ly = 7.20f;
        float lz = MZ - SP * 4.0f;
        const glm::vec3 LB(0.52f, 0.33f, 0.16f);
        auto be = [&](float oz, float oy, float sw, float sh2) {
            glm::mat4 mm = glm::translate(glm::mat4(1.f), { SX,ly + oy + 2,lz + oz + 8 });
            mm = glm::scale(mm, { LD,sh2,sw });
            drawCube(sh, mm, LB, texWood, 1.f);
            };
        // C
        be(LW - .07f, 0.f, .13f, LH); be(LW * .5f, LH * .42f, LW, .13f); be(LW * .5f, -LH * .42f, LW, .13f); lz -= SP;
        // A
        be(.07f, 0.f, .13f, LH); be(LW - .07f, 0.f, .13f, LH); be(LW * .5f, LH * .42f, LW, .13f); be(LW * .5f, 0.f, LW, .13f); lz -= SP;
        // F
        be(LW - .07f, 0.f, .13f, LH); be(LW * .5f, LH * .42f, LW, .13f); be(LW * .55f, 0.f, LW * .85f, .13f); lz -= SP;
        // E
        be(LW - .07f, 0.f, .13f, LH); be(LW * .5f, LH * .42f, LW, .13f); be(LW * .55f, 0.f, LW * .85f, .13f); be(LW * .5f, -LH * .42f, LW, .13f); lz -= SP;
        // T
        be(LW * .5f, LH * .42f, LW, .13f); be(LW * .5f, 0.f, .13f, LH); lz -= SP;
        // E
        be(LW - .07f, 0.f, .13f, LH); be(LW * .5f, LH * .42f, LW, .13f); be(LW * .55f, 0.f, LW * .85f, .13f); be(LW * .5f, -LH * .42f, LW, .13f); lz -= SP;
        // R
        be(LW - .07f, 0.f, .13f, LH); be(LW * .5f, LH * .42f, LW * .7f, .13f); be(.07f, LH * .2f, .13f, LH * .5f); be(LW * .5f, 0.f, LW * .85f, .13f); be(.07f, -LH * .22f, .13f, LH * .38f); lz -= SP;
        // I
        be(LW * .5f, 0.f, .13f, LH); lz -= SP;
        // A
        be(.07f, 0.f, .13f, LH); be(LW - .07f, 0.f, .13f, LH); be(LW * .5f, LH * .42f, LW, .13f); be(LW * .5f, 0.f, LW, .13f);
    }

    // ── B)  EAST DOOR (cafeteria door) ─────────────────────────────────────
    {
        glm::mat4 base = glm::translate(glm::mat4(1.f), { EAST_X,0.2f,DOOR_Z_MAX });
        base = glm::rotate(base, glm::radians(-doorOpenAngle), { 0.f,-1.f,0.f });
        glm::mat4 leaf = glm::scale(
            glm::translate(base, { DOOR_THICK * 0.5f,DOOR_HEIGHT * 0.5f,-(DOOR_WIDTH * 0.5f) }),
            { DOOR_THICK,DOOR_HEIGHT,DOOR_WIDTH });
        drawCube(sh, leaf, { 0.55f,0.35f,0.15f }, texWood, 1.f);
        glm::mat4 topRail = glm::scale(
            glm::translate(base, { DOOR_THICK * 0.5f,DOOR_HEIGHT - 0.08f,-(DOOR_WIDTH * 0.5f) }),
            { DOOR_THICK + 0.02f,0.14f,DOOR_WIDTH });
        drawCube(sh, topRail, { 0.15f,0.22f,0.30f }, 0, 1.f);
        glm::mat4 botRail = glm::scale(
            glm::translate(base, { DOOR_THICK * 0.5f,0.08f,-(DOOR_WIDTH * 0.5f) }),
            { DOOR_THICK + 0.02f,0.14f,DOOR_WIDTH });
        drawCube(sh, botRail, { 0.15f,0.22f,0.30f }, 0, 1.f);
        glm::mat4 rod = glm::scale(
            glm::translate(base, { DOOR_THICK,DOOR_HEIGHT * 0.45f,-(DOOR_WIDTH * 0.80f) }),
            { 0.05f,0.20f,0.05f });
        drawCylinder(sh, rod, { 0.75f,0.62f,0.15f });
        glm::mat4 knob = glm::scale(
            glm::translate(base, { DOOR_THICK * 1.2f,DOOR_HEIGHT * 0.45f,-(DOOR_WIDTH * 0.80f) }),
            { 0.10f,0.10f,0.10f });
        drawSphere(sh, knob, { 0.80f,0.68f,0.20f });
    }

    // ── C)  WEST DOOR + solid wall segments flanking it ────────────────────
    {
        float northLen = ZN - DOOR_Z_MAX;
        float northCen = (ZN + DOOR_Z_MAX) * 0.5f;
        if (northLen > 0.05f) {
            m = glm::scale(glm::translate(glm::mat4(1.f), { WEST_X,4.f,northCen }), { 0.3f,8.f,northLen });
            drawCube(sh, m, COL_CONCRETE, texConcrete, 2.f);
        }
        float southLen = DOOR_Z_MIN - ZS;
        float southCen = (DOOR_Z_MIN + ZS) * 0.5f;
        if (southLen > 0.05f) {
            m = glm::scale(glm::translate(glm::mat4(1.f), { WEST_X,4.f,southCen }), { 0.3f,8.f,southLen });
            drawCube(sh, m, COL_CONCRETE, texConcrete, 2.f);
        }
        // West door leaf
        glm::mat4 base = glm::translate(glm::mat4(1.f), { WEST_X,0.2f,DOOR_Z_MAX });
        base = glm::rotate(base, glm::radians(doorOpenAngle), { 0.f,1.f,0.f });
        glm::mat4 leaf = glm::scale(
            glm::translate(base, { -DOOR_THICK * 0.5f,DOOR_HEIGHT * 0.5f,-(DOOR_WIDTH * 0.5f) }),
            { DOOR_THICK,DOOR_HEIGHT,DOOR_WIDTH });
        drawCube(sh, leaf, { 0.55f,0.35f,0.15f }, texWood, 1.f);
        glm::mat4 topRail = glm::scale(
            glm::translate(base, { -DOOR_THICK * 0.5f,DOOR_HEIGHT - 0.08f,-(DOOR_WIDTH * 0.5f) }),
            { DOOR_THICK + 0.02f,0.14f,DOOR_WIDTH });
        drawCube(sh, topRail, { 0.15f,0.22f,0.30f }, 0, 1.f);
        glm::mat4 botRail = glm::scale(
            glm::translate(base, { -DOOR_THICK * 0.5f,0.08f,-(DOOR_WIDTH * 0.5f) }),
            { DOOR_THICK + 0.02f,0.14f,DOOR_WIDTH });
        drawCube(sh, botRail, { 0.15f,0.22f,0.30f }, 0, 1.f);
        glm::mat4 rod = glm::scale(
            glm::translate(base, { -DOOR_THICK,DOOR_HEIGHT * 0.45f,-(DOOR_WIDTH * 0.75f) }),
            { 0.05f,0.20f,0.05f });
        drawCylinder(sh, rod, { 0.75f,0.62f,0.15f });
        glm::mat4 knob = glm::scale(
            glm::translate(base, { -DOOR_THICK * 1.5f,DOOR_HEIGHT * 0.45f,-(DOOR_WIDTH * 0.75f) }),
            { 0.10f,0.10f,0.10f });
        drawSphere(sh, knob, { 0.80f,0.68f,0.20f });
    }

    // ── D)  SOUTH WALL – fully solid concrete (no glass) ───────────────────
    m = glm::scale(glm::translate(glm::mat4(1.f), { MX,4.f,ZS }), { XWID,8.f,0.4f });
    drawCube(sh, m, COL_CONCRETE, texConcrete, 2.f);


    m = glm::scale(glm::translate(glm::mat4(1.f), { 20.0f,5.f,2.25f }), { 0.4f,10.f,12.5f });
    drawCube(sh, m, COL_REDBRICK, texBrick, 2.f);

    // ── F)  ROOF + FLOOR ────────────────────────────────────────────────────
    m = glm::scale(glm::translate(glm::mat4(1.f), { MX,8.2f,MZ }), { XWID + 1.f,0.4f,ZDEP + 1.f });
    drawCube(sh, m, COL_ROOF, texConcrete, 6.f);
    m = glm::scale(glm::translate(glm::mat4(1.f), { MX,0.05f,MZ }), { XWID,0.1f,ZDEP });
    drawCube(sh, m, COL_CONCRETE, texConcrete, 5.f);
    m = glm::scale(glm::translate(glm::mat4(1.f), { MX,0.3f,MZ }), { XWID + 1.f,0.6f,ZDEP + 1.f });
    drawCube(sh, m, COL_CONCRETE, texConcrete, 5.f);

    // ── G)  EAST WALL – two solid concrete segments flanking door gap ───────
    {
        float northLen = ZN - DOOR_Z_MAX;
        float northCen = (ZN + DOOR_Z_MAX) * 0.5f;
        if (northLen > 0.05f) {
            m = glm::scale(glm::translate(glm::mat4(1.f), { EAST_X,4.f,northCen }), { 0.4f,8.f,northLen });
            drawCube(sh, m, COL_CONCRETE, texConcrete, 3.f);
        }
        float southLen = DOOR_Z_MIN - ZS;
        float southCen = (DOOR_Z_MIN + ZS) * 0.5f;
        if (southLen > 0.05f) {
            m = glm::scale(glm::translate(glm::mat4(1.f), { EAST_X,4.f,southCen }), { 0.4f,8.f,southLen });
            drawCube(sh, m, COL_CONCRETE, texConcrete, 3.f);
        }
    }

    // ── H)  COLUMNS on north + south faces (decorative, no glass) ──────────
    for (int i = 0; i < 5; i++) {
        float x = WEST_X + i * (XWID / 4.f);
        // north face columns
        glm::mat4 cm = glm::translate(glm::mat4(1.f), { x,4.f,ZN });
        cm = glm::rotate(cm, glm::radians(90.f), { 1,0,0 });
        cm = glm::scale(cm, { 0.35f,0.35f,8.f });
        drawCylinder(sh, cm, COL_COLUMN, texConcrete, 2.f);
        cm = glm::scale(glm::translate(glm::mat4(1.f), { x,8.f,ZN }), { 0.5f,0.4f,0.5f });
        drawCube(sh, cm, COL_COLUMN, texConcrete, 1.f);
        // south face columns
        cm = glm::translate(glm::mat4(1.f), { x,4.f,ZS });
        cm = glm::rotate(cm, glm::radians(90.f), { 1,0,0 });
        cm = glm::scale(cm, { 0.35f,0.35f,8.f });
        drawCylinder(sh, cm, COL_COLUMN, texConcrete, 2.f);
        cm = glm::scale(glm::translate(glm::mat4(1.f), { x,8.f,ZS }), { 0.5f,0.4f,0.5f });
        drawCube(sh, cm, COL_COLUMN, texConcrete, 1.f);
    }
    // West face vertical columns (skip door gap)
    for (int i = 0; i < 11; i++) {
        float z = ZN - i * STEP;
        if (z > DOOR_Z_MIN - 0.3f && z < DOOR_Z_MAX + 0.3f) continue;
        glm::mat4 cyl = glm::scale(glm::translate(glm::mat4(1.f), { WEST_X,4.f,z }), { 0.45f,8.f,0.45f });
        drawCylinder(sh, cyl, COL_COLUMN, texConcrete, 2.f);
        m = glm::scale(glm::translate(glm::mat4(1.f), { WEST_X,8.f,z }), { 0.65f,0.4f,0.65f });
        drawCube(sh, m, COL_COLUMN, texConcrete, 1.f);
    }

}

// ══════════════════════════════════════════════════════════════════════════════
//  INTERIOR
// ══════════════════════════════════════════════════════════════════════════════
static void renderFrontBuildingInterior(unsigned int sh) {
    const float IY = 0.6f;
    renderCarpet(sh, -5.f, IY, 3.f, 36.f, 9.f);
    // TV on west wall
    renderTV(sh, -23.5f, IY + 1.8f, 2.f, 90.f);
    renderReceptionCounter(sh, -5.f, IY, 5.5f);
    renderSofa(sh, -18.f, IY, 5.0f, 270.f);
    renderSofa(sh, -18.f, IY, 1.5f, 270.f);
  //  renderSofa(sh, -18.f, IY, -2.0f, 270.f);


    // Coffee table in front of sofas (between sofas and TV)
    renderCoffeeTable(sh, -19.5f, IY, 1.5f);
  //  renderSofa(sh, -18.f, IY, 4.f, 0.f); renderSofa(sh, -18.f, IY, 0.f, 0.f);
    renderCoffeeTable(sh, -18.f, IY, 2.f);
    renderIndoorPlant(sh, -21.f, IY, 5.f); renderIndoorPlant(sh, -21.f, IY, -0.5f);
    for (int i = 0; i < 4; i++) {
        renderChair(sh, 8.f + i * 1.0f, IY, 5.f, 180.f);
        renderChair(sh, 8.f + i * 1.0f, IY, 2.f, 0.f);
    }
    renderTable(sh, 9.5f, IY, 3.5f, 1.5f, 0.7f, 0.75f, 0.f);
    for (int i = 0; i < 3; i++) {
        renderComputerDesk(sh, 5.f, IY, 4.5f - i * 2.5f, 0.f);
        renderChair(sh, 5.f, IY, 3.5f - i * 2.5f, 180.f);
    }
    //renderNoticeBoard(sh, -14.f, IY + 1.8f, -1.5f, 0.f);
    //renderNoticeBoard(sh, -2.f, IY + 1.8f, -1.5f, 0.f);
    for (int i = 0; i < 5; i++) renderCeilingFan(sh, -20.f + i * 8.f, 7.8f, 3.f, 1.0f + i * 0.1f);
    renderWaterCooler(sh, -23.f, IY, 7.f); renderWaterCooler(sh, 12.f, IY, 7.f);
    renderIndoorPlant(sh, 12.f, IY, 7.5f); renderIndoorPlant(sh, -23.f, IY, 7.5f);
    renderACUnit(sh, -15.f, 7.0f, -1.6f, 0.f); renderACUnit(sh, 3.f, 7.0f, -1.6f, 0.f);
}
static void renderBackBuildingInterior(unsigned int sh) {
    const float IY = 0.6f, BZ = -22.f, BX = -5.f;
    renderCarpet(sh, BX, IY, BZ, 35.f, 9.f);
    for (int row = 0; row < 3; row++) {
        float tz = BZ + 3.f - row * 3.0f;
        for (int col = 0; col < 4; col++) {
            float tx = BX - 15.f + col * 8.f;
            renderTable(sh, tx, IY, tz, 1.6f, 0.7f, 0.75f, 0.f);
            renderChair(sh, tx - 0.35f, IY, tz + 0.85f, 180.f); renderChair(sh, tx + 0.35f, IY, tz + 0.85f, 180.f);
            renderChair(sh, tx - 0.35f, IY, tz - 0.85f, 0.f);   renderChair(sh, tx + 0.35f, IY, tz - 0.85f, 0.f);
        }
    }
    for (int i = 0; i < 5; i++) renderBookshelf(sh, BX - 16.f + i * 7.f, IY, BZ - 4.2f, 0.f);
    glm::mat4 m = glm::scale(glm::translate(glm::mat4(1.f), { BX,IY + 0.15f,BZ + 4.2f }), { 18.f,0.30f,2.5f });
    drawCube(sh, m, { 0.70f,0.55f,0.40f }, texWood, 3.f);
    //renderTable(sh, BX, IY + 0.30f, BZ + 3.5f, 0.8f, 0.5f, 0.85f, 0.f);
   // renderNoticeBoard(sh, BX - 2.f, IY + 0.30f + 1.5f, BZ + 4.8f, 0.f);
    //renderNoticeBoard(sh, BX + 2.5f, IY + 0.30f + 1.5f, BZ + 4.8f, 0.f);
    for (int i = 0; i < 5; i++) renderCeilingFan(sh, BX - 16.f + i * 8.f, 7.8f, BZ, 1.0f + i * 0.15f);
    renderWaterCooler(sh, BX - 22.f, IY, BZ + 3.f);
    renderIndoorPlant(sh, BX - 22.f, IY, BZ - 3.f); renderIndoorPlant(sh, BX + 17.f, IY, BZ + 3.f);
    renderIndoorPlant(sh, BX + 17.f, IY, BZ - 3.f);
    renderACUnit(sh, BX - 12.f, 7.0f, BZ - 4.7f, 0.f); renderACUnit(sh, BX + 5.f, 7.0f, BZ - 4.7f, 0.f);
}
static void renderConnectorInterior(unsigned int sh) {
    const float MX = 15.f, MZ = -10.5f, IY = 0.6f;
    renderCarpet(sh, MX, IY, MZ, 8.5f, 35.f);
    for (int row = 0; row < 8; row++) {
        float tz = 7.f - row * 4.5f;
        for (int col = 0; col < 2; col++) {
            float tx = MX - 2.5f + col * 5.f;
            glm::mat4 mt = glm::scale(glm::translate(glm::mat4(1.f), { tx,IY + 0.72f,tz }), { 0.85f,0.06f,0.85f });
            drawCylinder(sh, mt, { 0.90f,0.88f,0.85f }, texMarble, 1.f);
            mt = glm::scale(glm::translate(glm::mat4(1.f), { tx,IY + 0.38f,tz }), { 0.10f,0.76f,0.10f });
            drawCylinder(sh, mt, { 0.35f,0.22f,0.10f }, texWood, 1.f);
            mt = glm::scale(glm::translate(glm::mat4(1.f), { tx,IY + 0.04f,tz }), { 0.45f,0.08f,0.45f });
            drawCylinder(sh, mt, { 0.35f,0.22f,0.10f }, texWood, 1.f);
            renderChair(sh, tx + 0.60f, IY, tz, 270.f); renderChair(sh, tx - 0.60f, IY, tz, 90.f);
            renderChair(sh, tx, IY, tz + 0.60f, 180.f); renderChair(sh, tx, IY, tz - 0.60f, 0.f);
        }
    }
   
    // NOTE: the interior counter glass panels (small glass panes above counter)
    // are kept since they are interior decorative elements, not exterior walls
    
    const float fanZ[3] = { 6.0f,-10.5f,-26.0f };
    for (int i = 0; i < 3; ++i) renderCeilingFan(sh, MX, 7.8f, fanZ[i], 1.0f + 0.05f * i);
    renderWaterCooler(sh, MX - 4.2f, IY, 8.0f); renderWaterCooler(sh, MX - 4.2f, IY, -26.0f);
   // renderNoticeBoard(sh, MX, IY + 1.8f, 9.2f, 0.f);
    renderACUnit(sh, MX, 7.0f, 9.0f, 0.f); renderACUnit(sh, MX, 7.0f, -28.0f, 0.f);
}
static void renderInterior(unsigned int sh) {
    renderFrontBuildingInterior(sh);
    renderBackBuildingInterior(sh);
    renderConnectorInterior(sh);
}

// ══════════════════════════════════════════════════════════════════════════════
//  SCENE: COURTYARD + ANNEX
// ══════════════════════════════════════════════════════════════════════════════
static void renderCourtyard(unsigned int sh) {
    glm::mat4 m;
    m = glm::scale(glm::translate(glm::mat4(1.f), { -19.f,0.01f,-11.5f }), { 18.f,0.08f,14.f });
    drawCube(sh, m, COL_PLAZA, texBrick, 4.f);
    m = glm::scale(glm::translate(glm::mat4(1.f), { 9.f,0.01f,-11.5f }), { 18.f,0.08f,14.f });
    drawCube(sh, m, COL_PLAZA, texBrick, 4.f);
}
static void renderCurvedAnnex(unsigned int sh) {
    float cx = 20.f, cz = 3.f, radius = 7.f; int segs = 14;
    for (int i = 0; i < segs; i++) {
        float t0 = glm::radians(-10.f + (float)i * (190.f / segs));
        float t1 = glm::radians(-10.f + (float)(i + 1) * (190.f / segs));
        float tm = (t0 + t1) * 0.5f;
        float wx = cx + radius * cosf(tm), wz = cz + radius * sinf(tm);
        float segLen = 2.f * radius * sinf((t1 - t0) * 0.5f) + 0.05f;
        glm::mat4 m = glm::translate(glm::mat4(1.f), { wx,5.f,wz });
        m = glm::rotate(m, -tm + glm::radians(90.f), { 0,1,0 });
        m = glm::scale(m, { segLen,10.f,0.55f });
        drawCube(sh, m, COL_REDBRICK, texBrick, 2.f);
    }
    glm::mat4 m;
    m = glm::scale(glm::translate(glm::mat4(1.f), { 19.f,5.f,-4.f }), { 16.f,10.f,0.4f });
    drawCube(sh, m, COL_REDBRICK, texBrick, 3.f);

    // ── West wall: single unified slab covering z=-4.0 to z=+8.6
    //    (replaces the two old partial walls at z=-0.5/w=7.5 and z=-2/w=4.5)
    //    Center = (-4.0 + 8.6)/2 = 2.3,  total depth = 12.6
    //    thickness 0.60 to avoid z-fighting with connector east wall at x=20
    m = glm::scale(glm::translate(glm::mat4(1.f), { 20.0f, 5.f, 2.3f }), { 0.60f, 10.f, 12.6f });
    drawCube(sh, m, COL_REDBRICK, texBrick, 2.f);

    // Roof
    m = glm::scale(glm::translate(glm::mat4(1.f), { 20.f,10.2f,1.f }), { 13.5f,0.4f,11.5f });
    drawCube(sh, m, COL_REDBRICK, texBrick, 3.f);
    // Sign letters (unchanged)
    {
        const float SZ = 10.4f, LW = 0.38f, LH = 0.62f, LD = 0.15f, SP = 0.42f;
        float lx = 18.5f, ly1 = 7.09f;
        auto b1 = [&](float ox, float oy, float sw, float sh2) {
            glm::mat4 mm = glm::translate(glm::mat4(1.f), { lx + ox,ly1 + oy,SZ });
            mm = glm::scale(mm, { sw,sh2,LD }); drawSignCube(sh, mm); };
        // S
        b1(LW * .5f, LH * .42f, LW, .13f); b1(.07f, LH * .14f, .13f, LH * .3f);
        b1(LW * .5f, 0.f, LW, .13f); b1(LW - .07f, -LH * .14f, .13f, LH * .3f);
        b1(LW * .5f, -LH * .42f, LW, .13f); lx += SP;
        // T
        b1(LW * .5f, LH * .42f, LW, .13f); b1(LW * .5f, 0.f, .13f, LH); lx += SP;
        // U
        b1(.07f, 0.f, .13f, LH); b1(LW - .07f, 0.f, .13f, LH);
        b1(LW * .5f, -LH * .42f, LW, .13f); lx += SP;
        // D
        b1(.07f, 0.f, .13f, LH); b1(LW * .5f, LH * .42f, LW * .7f, .13f);
        b1(LW * .5f, -LH * .42f, LW * .7f, .13f); b1(LW - .07f, 0.f, .13f, LH * .6f); lx += SP;
        // E
        b1(.07f, 0.f, .13f, LH); b1(LW * .5f, LH * .42f, LW, .13f);
        b1(LW * .45f, 0.f, LW * .85f, .13f); b1(LW * .5f, -LH * .42f, LW, .13f); lx += SP;
        // N
        b1(.07f, 0.f, .13f, LH); b1(LW - .07f, 0.f, .13f, LH);
        b1(.08f, LH * .40f, .12f, LH * .22f); b1(.12f, LH * .22f, .12f, LH * .22f);
        b1(.17f, LH * .05f, .12f, LH * .22f); b1(.22f, -LH * .05f, .12f, LH * .22f);
        b1(.27f, -LH * .22f, .12f, LH * .22f); b1(.30f, -LH * .40f, .12f, LH * .22f); lx += SP;
        // T
        b1(LW * .5f, LH * .42f, LW, .13f); b1(LW * .5f, 0.f, .13f, LH); lx += SP + .2f;
        // W
        b1(.07f, 0.f, .13f, LH); b1(LW - .07f, 0.f, .13f, LH);
        b1(LW * .5f, -LH * .42f, LW, .13f); b1(LW * .3f, -.1f, .13f, LH * .5f);
        b1(LW * .7f, -.1f, .13f, LH * .5f); lx += SP;
        // E
        b1(.07f, 0.f, .13f, LH); b1(LW * .5f, LH * .42f, LW, .13f);
        b1(LW * .45f, 0.f, LW * .85f, .13f); b1(LW * .5f, -LH * .42f, LW, .13f); lx += SP;
        // L
        b1(.07f, 0.f, .13f, LH); b1(LW * .5f, -LH * .42f, LW, .13f); lx += SP;
        // F
        b1(.07f, 0.f, .13f, LH); b1(LW * .5f, LH * .42f, LW, .13f);
        b1(LW * .45f, 0.f, LW * .85f, .13f); lx += SP;
        // A
        b1(.07f, 0.f, .13f, LH); b1(LW - .07f, 0.f, .13f, LH);
        b1(LW * .5f, LH * .42f, LW, .13f); b1(LW * .5f, 0.f, LW, .13f); lx += SP;
        // R
        b1(.07f, 0.f, .13f, LH); b1(LW * .5f, LH * .42f, LW * .7f, .13f);
        b1(LW - .07f, LH * .2f, .13f, LH * .5f); b1(LW * .5f, 0.f, LW * .85f, .13f);
        b1(LW - .07f, -LH * .22f, .13f, LH * .38f); lx += SP;
        // E
        b1(.07f, 0.f, .13f, LH); b1(LW * .5f, LH * .42f, LW, .13f);
        b1(LW * .45f, 0.f, LW * .85f, .13f); b1(LW * .5f, -LH * .42f, LW, .13f);

        // CENTRE row 2
        lx = 20.0f; float ly2 = 6.0f;
        auto b2 = [&](float ox, float oy, float sw, float sh2) {
            glm::mat4 mm = glm::translate(glm::mat4(1.f), { lx + ox,ly2 + oy,SZ });
            mm = glm::scale(mm, { sw,sh2,LD }); drawSignCube(sh, mm); };
        // C
        b2(.07f, 0.f, .13f, LH); b2(LW * .5f, LH * .42f, LW, .13f);
        b2(LW * .5f, -LH * .42f, LW, .13f); lx += SP;
        // E
        b2(.07f, 0.f, .13f, LH); b2(LW * .5f, LH * .42f, LW, .13f);
        b2(LW * .45f, 0.f, LW * .85f, .13f); b2(LW * .5f, -LH * .42f, LW, .13f); lx += SP;
        // N
        b2(.07f, 0.f, .13f, LH); b2(LW - .07f, 0.f, .13f, LH);
        b2(.08f, LH * .40f, .12f, LH * .22f); b2(.12f, LH * .22f, .12f, LH * .22f);
        b2(.17f, LH * .05f, .12f, LH * .22f); b2(.22f, -LH * .05f, .12f, LH * .22f);
        b2(.27f, -LH * .22f, .12f, LH * .22f); b2(.30f, -LH * .40f, .12f, LH * .22f); lx += SP;
        // T
        b2(LW * .5f, LH * .42f, LW, .13f); b2(LW * .5f, 0.f, .13f, LH); lx += SP;
        // R
        b2(.07f, 0.f, .13f, LH); b2(LW * .5f, LH * .42f, LW * .7f, .13f);
        b2(LW - .07f, LH * .2f, .13f, LH * .5f); b2(LW * .5f, 0.f, LW * .85f, .13f);
        b2(LW - .07f, -LH * .22f, .13f, LH * .38f); lx += SP;
        // E
        b2(.07f, 0.f, .13f, LH); b2(LW * .5f, LH * .42f, LW, .13f);
        b2(LW * .45f, 0.f, LW * .85f, .13f); b2(LW * .5f, -LH * .42f, LW, .13f);
    }
    // COL_SIGN board
    m = glm::scale(glm::translate(glm::mat4(1.f), { 27.f,6.5f,-3.6f }), { 5.5f,1.2f,0.2f });
    drawCube(sh, m, COL_SIGN);
    
}
// ══════════════════════════════════════════════════════════════════════════════
//  SCENE: DECORATIVE SPHERE + CONE TREES
// ══════════════════════════════════════════════════════════════════════════════
static void renderDecorativeSphere(unsigned int sh) {
    glm::mat4 m = glm::scale(glm::translate(glm::mat4(1.f), { -18.f,0.5f,14.f }), { 1.6f,1.0f,1.6f });
    drawCube(sh, m, COL_STONE, texConcrete, 1.f);
    m = glm::translate(glm::mat4(1.f), { -18.f,1.6f,14.f });
    m = glm::rotate(m, glm::radians(90.f), { 1,0,0 }); m = glm::scale(m, { 0.5f,0.5f,1.2f });
    drawCylinder(sh, m, COL_STONE, texConcrete, 1.f);
    m = glm::scale(glm::translate(glm::mat4(1.f), { -18.f,4.0f,14.f }), { 4.5f,4.5f,4.5f });
    drawSphere(sh, m, COL_MARBLE, texMarble, 1.f);
}
static void renderConeTrees(unsigned int sh) {
    auto coneTree = [&](float x, float z) {
        glm::mat4 m = glm::translate(glm::mat4(1.f), { x,1.5f,z });
        m = glm::rotate(m, glm::radians(90.f), { 1,0,0 }); m = glm::scale(m, { 0.4f,0.4f,3.f });
        drawCylinder(sh, m, COL_TRUNK, texWood, 1.f);
        m = glm::scale(glm::translate(glm::mat4(1.f), { x,4.5f,z }), { 3.5f,4.5f,3.5f });
        drawCone(sh, m, COL_PALM, texGrass, 2.f);
        m = glm::scale(glm::translate(glm::mat4(1.f), { x,7.f,z }), { 2.3f,3.5f,2.3f });
        drawCone(sh, m, COL_PALM, texGrass, 1.5f);
        m = glm::scale(glm::translate(glm::mat4(1.f), { x,9.f,z }), { 1.2f,2.5f,1.2f });
        drawCone(sh, m, COL_PALM, texGrass, 1.f);
        };
    coneTree(-38.f, -5.f); coneTree(-35.f, 3.f); coneTree(10.f, 14.f);
}

// ══════════════════════════════════════════════════════════════════════════════
//  CURVE SURFACE OBJECTS
// ══════════════════════════════════════════════════════════════════════════════
static void renderBezierVase(unsigned int sh) {
    glm::mat4 ped = glm::scale(glm::translate(glm::mat4(1.f), { 20.f,0.f,18.f }), { 1.8f,0.5f,1.8f });
    drawCube(sh, ped, COL_STONE, texConcrete, 1.f);
    glm::mat4 model = glm::translate(glm::mat4(1.f), { 20.f,0.5f,18.f });
    model = glm::scale(model, { 0.50f,0.50f,0.50f });
    drawMesh(sh, bezierSweepVAO, bezierSweepCount, model, glm::vec3(0.90f, 0.60f, 0.30f), texMarble, 1.f);
}

static bool lampPostLightOn = true;
static void renderSplineLampPost(unsigned int sh, float x, float z) {
    glm::mat4 model = glm::translate(glm::mat4(1.f), { x,0.f,z });
    model = glm::scale(model, { 1.2f,2.5f,1.2f });
    drawMesh(sh, splineSweepVAO, splineSweepCount, model, COL_CONCRETE, texConcrete, 2.f);
    glm::mat4 globe = glm::scale(glm::translate(glm::mat4(1.f), { x,2.5f * 8.5f,z }), { 3.2f,3.2f,3.2f });
    if (lampPostLightOn) {
        static const glm::vec3 lc(1.00f, 0.98f, 0.80f);
        static const glm::vec3 em(1.00f, 0.95f, 0.60f);
        static const glm::vec3 zero(0.f, 0.f, 0.f);
        glUniformMatrix4fv(glGetUniformLocation(sh, "model"), 1, GL_FALSE, glm::value_ptr(globe));
        glUniform3fv(glGetUniformLocation(sh, "objectColor"), 1, glm::value_ptr(lc));
        glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(em));
        glBindVertexArray(sphVAO); glDrawArrays(GL_TRIANGLES, 0, sphCount);
        glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(zero));
    }
    else {
        drawSphere(sh, globe, glm::vec3(0.30f, 0.30f, 0.30f), 0, 1.f);
    }
}
static void renderRuledCanopy(unsigned int sh) {
    glm::mat4 model = glm::translate(glm::mat4(1.f), { -12.f,0.f,14.f });
    model = glm::scale(model, { 0.5f,0.5f,0.5f });
    drawMesh(sh, ruledSurfVAO, ruledSurfCount, model, glm::vec3(0.85f, 0.82f, 0.75f), texConcrete, 2.5f);
}

// ══════════════════════════════════════════════════════════════════════════════
//  FULL SCENE
// ══════════════════════════════════════════════════════════════════════════════
// In renderScene(), add before the closing brace:

// ══════════════════════════════════════════════════════════════════════════════
//  FRACTAL TREE
// ══════════════════════════════════════════════════════════════════════════════
static void renderFractalBranch(unsigned int sh, glm::mat4 base,
    float length, float radius, int depth)
{
    if (depth <= 0 || length < 0.15f) return;

    // Draw this branch segment as a cylinder
    glm::mat4 m = glm::translate(base, { 0.f, length * 0.5f, 0.f });
    m = glm::scale(m, { radius * 2.f, length, radius * 2.f });
    glm::vec3 col = (depth > 3)
        ? glm::mix(COL_TRUNK, glm::vec3(0.25f, 0.18f, 0.08f), 1.f - (float)depth / 8.f)
        : glm::mix(COL_PALM, COL_TRUNK, (float)depth / 4.f);
    drawCylinder(sh, m, col, (depth <= 2) ? texGrass : texWood, 1.f);

    // Move to tip of this branch
    glm::mat4 tip = glm::translate(base, { 0.f, length, 0.f });

    if (depth == 1) {
        // Leaf cluster at tips
        glm::mat4 leaf = glm::scale(tip, { 0.9f, 0.9f, 0.9f });
        drawSphere(sh, leaf, COL_PALM, texGrass, 1.f);
        return;
    }

    float childLen = length * 0.68f;
    float childRadius = radius * 0.65f;
    float spreadAngle = 32.f;

    // 3 child branches, spread around Y, each tilted outward
    for (int i = 0; i < 3; ++i) {
        float yRot = 120.f * i + depth * 17.f; // stagger by depth for variety
        float tiltX = spreadAngle;

        glm::mat4 branch = tip;
        branch = glm::rotate(branch, glm::radians(yRot), { 0.f, 1.f, 0.f });
        branch = glm::rotate(branch, glm::radians(tiltX), { 1.f, 0.f, 0.f });

        renderFractalBranch(sh, branch, childLen, childRadius, depth - 1);
    }
}

static void renderFractalTree(unsigned int sh, float x, float y, float z) {
    // Base trunk pedestal
    glm::mat4 base = glm::translate(glm::mat4(1.f), { x, y, z });

    // Start recursion: trunk points straight up
    renderFractalBranch(sh, base, 3.5f, 0.22f, 7);
}

// ══════════════════════════════════════════════════════════════════════════════
//  POND – full west side of campus
// ══════════════════════════════════════════════════════════════════════════════
static void renderPond(unsigned int sh) {
    const float POND_X = -48.f;   // pond center X (west side)
    const float POND_Z = -8.f;   // pond center Z
    const float POND_W = 18.f;   // width  (X axis)
    const float POND_L = 45.f;   // length (Z axis)
    const float POND_D = 0.6f;  // depth below ground
    const float WATER_Y = 0.02f; // water surface Y 
    glm::vec3 mudCol(0.38f, 0.28f, 0.18f);
    glm::vec3 stoneCol(0.55f, 0.52f, 0.48f);
    glm::vec3 grassEdge(0.22f, 0.48f, 0.18f);
    glm::mat4 m = glm::scale(
        glm::translate(glm::mat4(1.f), { POND_X, -POND_D * 0.5f + 0.01f, POND_Z }),
        { POND_W - 0.5f, POND_D, POND_L - 0.5f });
    drawCube(sh, m, mudCol, texConcrete, 3.f);

    // ── STONE BORDER ──────────────────────────────────────────────────────
    // north
    m = glm::scale(
        glm::translate(glm::mat4(1.f),
            { POND_X, 0.08f, POND_Z + POND_L * 0.5f }),
        { POND_W + 0.8f, 0.18f, 0.9f });
    drawCube(sh, m, stoneCol, texConcrete, 2.f);
    // south
    m = glm::scale(
        glm::translate(glm::mat4(1.f),
            { POND_X, 0.08f, POND_Z - POND_L * 0.5f }),
        { POND_W + 0.8f, 0.18f, 0.9f });
    drawCube(sh, m, stoneCol, texConcrete, 2.f);
    // east
    m = glm::scale(
        glm::translate(glm::mat4(1.f),
            { POND_X + POND_W * 0.5f, 0.08f, POND_Z }),
        { 0.9f, 0.18f, POND_L + 0.8f });
    drawCube(sh, m, stoneCol, texConcrete, 2.f);
    // west
    m = glm::scale(
        glm::translate(glm::mat4(1.f),
            { POND_X - POND_W * 0.5f, 0.08f, POND_Z }),
        { 0.9f, 0.18f, POND_L + 0.8f });
    drawCube(sh, m, stoneCol, texConcrete, 2.f);

    // ── CORNER STONES ─────────────────────────────────────────────────────
    float cxs[] = { POND_X - POND_W * 0.5f, POND_X + POND_W * 0.5f,
                    POND_X - POND_W * 0.5f, POND_X + POND_W * 0.5f };
    float czs[] = { POND_Z + POND_L * 0.5f, POND_Z + POND_L * 0.5f,
                    POND_Z - POND_L * 0.5f, POND_Z - POND_L * 0.5f };
    for (int i = 0; i < 4; i++) {
        m = glm::scale(
            glm::translate(glm::mat4(1.f), { cxs[i], 0.12f, czs[i] }),
            { 1.0f, 0.22f, 1.0f });
        drawCube(sh, m, stoneCol * 0.85f, texConcrete, 1.f);
    }

    // ── ANIMATED WATER ────────────────────────────────────────────────────
    //float wt = tvTime;
    // ── ANIMATED WATER ────────────────────────────────────────────────────
    float wt = tvTime;
    float wPulse = 0.5f + 0.5f * sinf(wt * 0.8f);
    float waveA = sinf(wt * 1.2f) * 0.018f;
    float waveB = sinf(wt * 0.9f + 1.f) * 0.012f;

    // base water color (shifts between cyan shades like pool)
    glm::vec3 waterCol(
        0.05f + 0.08f * wPulse,
        0.45f + 0.20f * wPulse,
        0.75f + 0.15f * wPulse);
    glm::vec3 waterEm = waterCol * 1.6f;

    // main water slab
    {
        glm::mat4 wm = glm::scale(
            glm::translate(glm::mat4(1.f), { POND_X + waveA, WATER_Y, POND_Z + waveB }),
            { POND_W - 0.8f, 0.10f, POND_L - 0.8f });
        static const glm::vec3 zero(0.f);
        glUniformMatrix4fv(glGetUniformLocation(sh, "model"), 1, GL_FALSE, glm::value_ptr(wm));
        glUniform3fv(glGetUniformLocation(sh, "objectColor"), 1, glm::value_ptr(waterCol));
        glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(waterEm));
        glUniform1i(glGetUniformLocation(sh, "hasTexture"), 0);
        glUniform1i(glGetUniformLocation(sh, "texMode"), 0);
        glBindVertexArray(cubeVAO); glDrawArrays(GL_TRIANGLES, 0, 36);
        glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(zero));
    }

    // wave ripple strips — horizontal bands moving across water (like pool)
    for (int i = 0; i < 8; i++) {
        float rippleOffset = fmodf(wt * 0.6f + i * 0.38f, 1.f);
        float rz = (POND_Z - POND_L * 0.5f + 0.15f) + rippleOffset * (POND_L - 0.30f);
        float rAlpha = sinf(rippleOffset * PI);
        glm::vec3 rippleC(
            0.55f + 0.30f * rAlpha,
            0.80f + 0.15f * rAlpha,
            0.98f);
        glm::vec3 rippleEm = rippleC * 1.8f * rAlpha;
        glm::mat4 rm = glm::scale(
            glm::translate(glm::mat4(1.f), { POND_X + waveA, WATER_Y + 0.04f, rz }),
            { POND_W - 0.70f, 0.04f, 0.18f });
        static const glm::vec3 zero(0.f);
        glUniformMatrix4fv(glGetUniformLocation(sh, "model"), 1, GL_FALSE, glm::value_ptr(rm));
        glUniform3fv(glGetUniformLocation(sh, "objectColor"), 1, glm::value_ptr(rippleC));
        glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(rippleEm));
        glUniform1i(glGetUniformLocation(sh, "hasTexture"), 0);
        glUniform1i(glGetUniformLocation(sh, "texMode"), 0);
        glBindVertexArray(cubeVAO); glDrawArrays(GL_TRIANGLES, 0, 36);
        glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(zero));
    }

    // sparkle glints on water surface (like pool)
    for (int i = 0; i < 10; i++) {
        float sPhase = sinf(wt * 2.5f + i * 0.79f);
        if (sPhase < 0.4f) continue;
        float sx = POND_X - POND_W * 0.4f + i * (POND_W * 0.8f / 9.f);
        float sz = POND_Z + sinf(wt * 1.1f + i * 1.3f) * POND_L * 0.35f;
        float sStr = (sPhase - 0.4f) / 0.6f;
        glm::vec3 glintC(0.9f, 0.97f, 1.0f);
        glm::vec3 glintEm = glintC * 3.0f * sStr;
        glm::mat4 gm = glm::scale(
            glm::translate(glm::mat4(1.f), { sx, WATER_Y + 0.05f, sz }),
            { 0.10f, 0.03f, 0.10f });
        static const glm::vec3 zero(0.f);
        glUniformMatrix4fv(glGetUniformLocation(sh, "model"), 1, GL_FALSE, glm::value_ptr(gm));
        glUniform3fv(glGetUniformLocation(sh, "objectColor"), 1, glm::value_ptr(glintC));
        glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(glintEm));
        glUniform1i(glGetUniformLocation(sh, "hasTexture"), 0);
        glUniform1i(glGetUniformLocation(sh, "texMode"), 0);
        glBindVertexArray(cubeVAO); glDrawArrays(GL_TRIANGLES, 0, 36);
        glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(zero));
    }

    // ── LILY PADS (floating, gently bobbing) ──────────────────────────────
    struct LilyPos { float x, z; };
    LilyPos lilies[] = {
        {POND_X - 4.f, POND_Z + 8.f},
        {POND_X + 3.f, POND_Z - 5.f},

        {POND_X - 2.f, POND_Z + 15.f},
        {POND_X + 5.f, POND_Z - 14.f},
        {POND_X,       POND_Z + 1.f },
        {POND_X - 5.f, POND_Z - 10.f},
    };
    for (auto& lp : lilies) {
        float bob = sinf(wt * 0.8f + lp.x * 0.3f) * 0.03f;
        glm::mat4 lm = glm::scale(
            glm::translate(glm::mat4(1.f), { lp.x, WATER_Y + 0.05f + bob, lp.z }),
            { 0.55f, 0.04f, 0.55f });
        drawCylinder(sh, lm, { 0.15f, 0.52f, 0.18f }, texGrass, 1.f);
        lm = glm::scale(
            glm::translate(glm::mat4(1.f), { lp.x, WATER_Y + 0.10f + bob, lp.z }),
            { 0.18f, 0.18f, 0.18f });
        drawSphere(sh, lm, { 0.95f, 0.75f, 0.80f });
    }

    // ── DUCKS (2 ducks swimming) ──────────────────────────────────────────
    for (int d = 0; d < 2; d++) {
        float dPhase = wt * 0.3f + d * PI;
        float dx = POND_X + cosf(dPhase) * (POND_W * 0.28f);
        float dz = POND_Z + sinf(dPhase) * (POND_L * 0.28f);
        float dRot = atan2f(-sinf(dPhase), cosf(dPhase));
        float bob = sinf(wt * 2.f + d) * 0.015f;

        glm::mat4 dbase = glm::rotate(
            glm::translate(glm::mat4(1.f), { dx, WATER_Y + 0.12f + bob, dz }),
            dRot, { 0.f, 1.f, 0.f });
        // body
        glm::mat4 dm = glm::scale(glm::translate(dbase, { 0.f,0.f,0.f }),
            { 0.28f, 0.18f, 0.42f });
        drawSphere(sh, dm, (d == 0) ? glm::vec3(0.92f, 0.88f, 0.80f)
            : glm::vec3(0.25f, 0.45f, 0.22f));
        // head
        dm = glm::scale(glm::translate(dbase, { 0.f,0.14f,0.18f }),
            { 0.16f, 0.16f, 0.16f });
        drawSphere(sh, dm, (d == 0) ? glm::vec3(0.88f, 0.84f, 0.75f)
            : glm::vec3(0.18f, 0.38f, 0.18f));
        // beak
        dm = glm::scale(glm::translate(dbase, { 0.f,0.10f,0.28f }),
            { 0.07f, 0.05f, 0.10f });
        drawCube(sh, dm, { 0.90f,0.62f,0.10f });
        // wake ripple
        dm = glm::scale(glm::translate(dbase, { 0.f,-0.06f,0.f }),
            { 0.35f, 0.04f, 0.50f });
        drawCylinder(sh, dm, { 0.65f,0.82f,0.90f });
    }

    // ── POND-SIDE PALM TREES ──────────────────────────────────────────────
    renderPalmTree(sh, { POND_X - POND_W * 0.5f - 1.5f,  0.f, POND_Z + 10.f });
    renderPalmTree(sh, { POND_X - POND_W * 0.5f - 1.5f,  0.f, POND_Z - 10.f });
    renderPalmTree(sh, { POND_X + POND_W * 0.5f + 1.5f,  0.f, POND_Z + 18.f });

}
// ══════════════════════════════════════════════════════════════════════════════
//  ROOFTOP SWIMMING POOL  (back building roof)
// ══════════════════════════════════════════════════════════════════════════════
static void renderSwimmingPool(unsigned int sh) {
    const float PX = -5.f;
    const float PZ = -22.f;
    const float ROOF = 8.4f;
    const float PWD = 38.f;   // back building full width (BLEN)
    const float PLD = 10.f;   // back building full depth
    const float PDEP = 1.2f;
    glm::vec3 tileCol(0.75f, 0.88f, 0.95f);  // light blue tile
    glm::vec3 wallCol(0.90f, 0.90f, 0.88f);  // concrete surround
    glm::vec3 deckCol(0.82f, 0.78f, 0.72f);  // pool deck

    // ── POOL DECK (surrounding area) ──────────────────────────────────────
    glm::mat4 m = glm::scale(
        glm::translate(glm::mat4(1.f), { PX, ROOF + 0.05f, PZ }),
        { PWD + 5.f, 0.10f, PLD + 5.f });
    drawCube(sh, m, deckCol, texConcrete, 3.f);

    // ── POOL SHELL walls ──────────────────────────────────────────────────
    // north wall
    m = glm::scale(
        glm::translate(glm::mat4(1.f), { PX, ROOF + PDEP * 0.5f, PZ + PLD * 0.5f }),
        { PWD, PDEP, 0.25f });
    drawCube(sh, m, tileCol, texMarble, 2.f);
    // south wall
    m = glm::scale(
        glm::translate(glm::mat4(1.f), { PX, ROOF + PDEP * 0.5f, PZ - PLD * 0.5f }),
        { PWD, PDEP, 0.25f });
    drawCube(sh, m, tileCol, texMarble, 2.f);
    // east wall
    m = glm::scale(
        glm::translate(glm::mat4(1.f), { PX + PWD * 0.5f, ROOF + PDEP * 0.5f, PZ }),
        { 0.25f, PDEP, PLD });
    drawCube(sh, m, tileCol, texMarble, 2.f);
    // west wall
    m = glm::scale(
        glm::translate(glm::mat4(1.f), { PX - PWD * 0.5f, ROOF + PDEP * 0.5f, PZ }),
        { 0.25f, PDEP, PLD });
    drawCube(sh, m, tileCol, texMarble, 2.f);
    // pool floor
    m = glm::scale(
        glm::translate(glm::mat4(1.f), { PX, ROOF + 0.08f, PZ }),
        { PWD - 0.25f, 0.12f, PLD - 0.25f });
    drawCube(sh, m, tileCol, texMarble, 3.f);

    // ── ANIMATED WATER SURFACE ─────────────────────────────────────────────
    // Multiple thin layers of water with shifting emissive color and wave offset
    float wt = tvTime;   // reuse tvTime for animation
    float waveA = sinf(wt * 1.2f) * 0.018f;
    float waveB = sinf(wt * 0.9f + 1.f) * 0.012f;

    // base water color shifts between cyan shades
    float wPulse = 0.5f + 0.5f * sinf(wt * 0.8f);
    glm::vec3 waterCol(
        0.05f + 0.08f * wPulse,
        0.45f + 0.20f * wPulse,
        0.75f + 0.15f * wPulse);
    glm::vec3 waterEm = waterCol * 1.6f;

    // main water slab
    {
        glm::mat4 wm = glm::scale(
            glm::translate(glm::mat4(1.f),
                { PX + waveA, ROOF + PDEP - 0.05f, PZ + waveB }),
            { PWD - 0.30f, 0.06f, PLD - 0.30f });
        static const glm::vec3 zero(0.f);
        glUniformMatrix4fv(glGetUniformLocation(sh, "model"), 1, GL_FALSE, glm::value_ptr(wm));
        glUniform3fv(glGetUniformLocation(sh, "objectColor"), 1, glm::value_ptr(waterCol));
        glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(waterEm));
        glUniform1i(glGetUniformLocation(sh, "hasTexture"), 0);
        glUniform1i(glGetUniformLocation(sh, "texMode"), 0);
        glBindVertexArray(cubeVAO); glDrawArrays(GL_TRIANGLES, 0, 36);
        glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(zero));
    }

    // wave ripple strips — horizontal bands moving across water
    for (int i = 0; i < 6; i++) {
        float rippleOffset = fmodf(wt * 0.6f + i * 0.38f, 1.f);
        float rz = (PZ - PLD * 0.5f + 0.15f) + rippleOffset * (PLD - 0.30f);
        float rAlpha = sinf(rippleOffset * PI);  // fade in/out
        glm::vec3 rippleC(
            0.55f + 0.30f * rAlpha,
            0.80f + 0.15f * rAlpha,
            0.98f);
        glm::vec3 rippleEm = rippleC * 1.8f * rAlpha;
        glm::mat4 rm = glm::scale(
            glm::translate(glm::mat4(1.f),
                { PX + waveA, ROOF + PDEP - 0.02f, rz }),
            { PWD - 0.32f, 0.04f, 0.12f });
        static const glm::vec3 zero(0.f);
        glUniformMatrix4fv(glGetUniformLocation(sh, "model"), 1, GL_FALSE, glm::value_ptr(rm));
        glUniform3fv(glGetUniformLocation(sh, "objectColor"), 1, glm::value_ptr(rippleC));
        glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(rippleEm));
        glUniform1i(glGetUniformLocation(sh, "hasTexture"), 0);
        glUniform1i(glGetUniformLocation(sh, "texMode"), 0);
        glBindVertexArray(cubeVAO); glDrawArrays(GL_TRIANGLES, 0, 36);
        glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(zero));
    }

    // sparkle glints on water surface
    for (int i = 0; i < 8; i++) {
        float sPhase = sinf(wt * 2.5f + i * 0.79f);
        if (sPhase < 0.4f) continue;   // only show bright glints
        float sx = PX - PWD * 0.4f + i * (PWD * 0.8f / 7.f);
        float sz = PZ + sinf(wt * 1.1f + i * 1.3f) * PLD * 0.35f;
        float sStr = (sPhase - 0.4f) / 0.6f;
        glm::vec3 glintC(0.9f, 0.97f, 1.0f);
        glm::vec3 glintEm = glintC * 3.0f * sStr;
        glm::mat4 gm = glm::scale(
            glm::translate(glm::mat4(1.f), { sx, ROOF + PDEP, sz }),
            { 0.08f, 0.03f, 0.08f });
        static const glm::vec3 zero(0.f);
        glUniformMatrix4fv(glGetUniformLocation(sh, "model"), 1, GL_FALSE, glm::value_ptr(gm));
        glUniform3fv(glGetUniformLocation(sh, "objectColor"), 1, glm::value_ptr(glintC));
        glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(glintEm));
        glUniform1i(glGetUniformLocation(sh, "hasTexture"), 0);
        glUniform1i(glGetUniformLocation(sh, "texMode"), 0);
        glBindVertexArray(cubeVAO); glDrawArrays(GL_TRIANGLES, 0, 36);
        glUniform3fv(glGetUniformLocation(sh, "emissive"), 1, glm::value_ptr(zero));
    }

    // ── POOL LADDER (east side) ────────────────────────────────────────────
    glm::vec3 ladderCol(0.75f, 0.75f, 0.78f);
    // two vertical rails
    m = glm::scale(
        glm::translate(glm::mat4(1.f),
            { PX + PWD * 0.5f - 0.15f, ROOF + PDEP * 0.5f + 0.2f, PZ + 0.3f }),
        { 0.05f, PDEP + 0.4f, 0.05f });
    drawCylinder(sh, m, ladderCol);
    m = glm::scale(
        glm::translate(glm::mat4(1.f),
            { PX + PWD * 0.5f - 0.15f, ROOF + PDEP * 0.5f + 0.2f, PZ - 0.3f }),
        { 0.05f, PDEP + 0.4f, 0.05f });
    drawCylinder(sh, m, ladderCol);
    for (int r = 0; r < 4; r++) {
        float ry = ROOF + 0.25f + r * 0.32f;
        m = glm::scale(
            glm::translate(glm::mat4(1.f),
                { PX + PWD * 0.5f - 0.15f, ry, PZ }),
            { 0.05f, 0.05f, 0.62f });
        drawCylinder(sh, m, ladderCol);
    }
    // rungs
    for (int r = 0; r < 4; r++) {
        float ry = ROOF + 0.25f + r * 0.32f;
        m = glm::scale(
            glm::translate(glm::mat4(1.f),
                { PX + PWD * 0.5f - 0.4f, ry, PZ + 1.2f }),
            { 0.05f, 0.05f, 0.42f });
        drawCylinder(sh, m, ladderCol);
    }

    // ── POOLSIDE SUNBEDS (2 either side) ──────────────────────────────────
    glm::vec3 bedCol(0.85f, 0.72f, 0.45f);
    glm::vec3 bedFrm(0.55f, 0.55f, 0.58f);
    auto sunbed = [&](float bx, float bz, float rotY2) {
        glm::mat4 bb = glm::rotate(
            glm::translate(glm::mat4(1.f), { bx, ROOF + 0.22f, bz }),
            glm::radians(rotY2), { 0.f,1.f,0.f });
        // frame
        glm::mat4 mm = glm::scale(glm::translate(bb, { 0.f,0.f,0.f }),
            { 0.55f, 0.08f, 1.60f });
        drawCube(sh, mm, bedFrm);
        // cushion
        mm = glm::scale(glm::translate(bb, { 0.f, 0.06f, 0.f }),
            { 0.48f, 0.07f, 1.52f });
        drawCube(sh, mm, bedCol);
        // legs
        for (int lg = -1; lg <= 1; lg += 2) {
            for (int lz2 = -1; lz2 <= 1; lz2 += 2) {
                mm = glm::scale(
                    glm::translate(bb, { lg * 0.22f, -0.10f, lz2 * 0.70f }),
                    { 0.05f, 0.20f, 0.05f });
                drawCylinder(sh, mm, bedFrm);
            }
        }
        };
    sunbed(PX, PZ + PLD * 0.5f + 1.0f, 0.f);
    sunbed(PX + 3.f, PZ + PLD * 0.5f + 1.0f, 0.f);
    sunbed(PX, PZ - PLD * 0.5f - 1.0f, 0.f);
    sunbed(PX + 3.f, PZ - PLD * 0.5f - 1.0f, 0.f);

    

    // ── UMBRELLA (poolside) ────────────────────────────────────────────────
    glm::vec3 umbCol(0.85f, 0.22f, 0.18f);
    auto umbrella = [&](float ux, float uz) {
        glm::mat4 um = glm::scale(
            glm::translate(glm::mat4(1.f), { ux, ROOF + 0.22f, uz }),
            { 0.06f, 1.20f, 0.06f });
        drawCylinder(sh, um, { 0.75f,0.75f,0.78f });
        um = glm::scale(
            glm::translate(glm::mat4(1.f), { ux, ROOF + 1.42f, uz }),
            { 1.80f, 0.18f, 1.80f });
        drawCone(sh, um, umbCol);
        };
    umbrella(PX - 2.f, PZ + PLD * 0.5f + 1.0f);
    umbrella(PX - 2.f, PZ - PLD * 0.5f - 1.0f);
}
static void renderScene(unsigned int sh) {
    renderGround(sh);
    renderRoad(sh);                           // NEW: East-side road
    renderMainBuilding(sh);
    renderConnector(sh);
    renderBackBuilding(sh);
    renderCourtyard(sh);
    renderInterior(sh);
    renderCurvedAnnex(sh);
    renderDecorativeSphere(sh);
    renderSwimmingPool(sh);
    renderPond(sh);
    // Fractal tree in front of back building
   // renderFractalTree(sh, -5.f, 0.6f, -14.5f);
    // Fractal tree at west end of back building
    renderFractalTree(sh, -30.f, 0.6f, -22.f);
    renderConeTrees(sh);
    renderPalmTree(sh, { -32.f,0.f,5.f }); renderPalmTree(sh, { -28.f,0.f,12.f });
    renderPalmTree(sh, { 36.f,0.f,4.f });  renderPalmTree(sh, { 32.f,0.f,13.f });
    renderBezierVase(sh);
    renderSplineLampPost(sh, -14.f, 10.f);
    renderSplineLampPost(sh, 4.f, 10.f);
    renderRuledCanopy(sh);
    renderPedestrians(sh);                   
}

static void applyCollision(glm::vec3& pos)
{
    const float CAM_R = 0.75f;

    // ── BUILDING BOUNDS ────────────────────────────────────────────────────
    const float FB_XMIN = -24.0f, FB_XMAX = 14.0f;
    const float FB_ZMIN = -2.0f, FB_ZMAX = 8.5f;

    const float BB_XMIN = -24.0f, BB_XMAX = 14.0f;
    const float BB_ZMIN = -27.0f, BB_ZMAX = -17.0f;

    const float CN_XMIN = 10.0f, CN_XMAX = 20.0f;
    const float CN_ZMIN = -29.5f, CN_ZMAX = 8.5f;

    // ── DOOR GAP RANGES ────────────────────────────────────────────────────
    const float DOOR_XLO = -9.1f;
    const float DOOR_XHI = 2.1f;
    const float DOOR_ZLO = -12.25f;
    const float DOOR_ZHI = -8.75f;

    // ── Y LEVELS ───────────────────────────────────────────────────────────
    const float FLOOR_Y = 0.8f;
    const float CEILING_Y = 7.8f;
    const float ROOF_SLAB_TOP = 8.4f;
    const float ROOF_WALK_Y = 8.8f;
    const float OUTDOOR_Y = 0.5f;

    // ── CLASSIFY ───────────────────────────────────────────────────────────
    bool inFB_XZ = (pos.x > FB_XMIN && pos.x < FB_XMAX && pos.z > FB_ZMIN && pos.z < FB_ZMAX);
    bool inBB_XZ = (pos.x > BB_XMIN && pos.x < BB_XMAX && pos.z > BB_ZMIN && pos.z < BB_ZMAX);
    bool inCN_XZ = (pos.x > CN_XMIN && pos.x < CN_XMAX && pos.z > CN_ZMIN && pos.z < CN_ZMAX);

    bool onAnyRoof = (inFB_XZ || inBB_XZ || inCN_XZ) && (pos.y >= ROOF_SLAB_TOP);
    bool indoors = (inFB_XZ || inBB_XZ || inCN_XZ) && (pos.y < ROOF_SLAB_TOP);

    // ── Y CLAMP ────────────────────────────────────────────────────────────
    if (onAnyRoof) { if (pos.y < ROOF_WALK_Y) pos.y = ROOF_WALK_Y; }
    else if (indoors) { pos.y = glm::clamp(pos.y, FLOOR_Y, CEILING_Y); }
    else { if (pos.y < OUTDOOR_Y) pos.y = OUTDOOR_Y; }

    // ── FRONT BUILDING WEST WALL ONLY ──────────────────────────────────────
    // Only check below roof
    if (pos.y < ROOF_SLAB_TOP)
    {
        // West wall collision at x = FB_XMIN (-24.0f)
        if (pos.x > FB_XMIN - CAM_R && pos.x < FB_XMIN + CAM_R &&
            pos.z >= FB_ZMIN && pos.z <= FB_ZMAX)
        {
            if (pos.x < FB_XMIN)
                pos.x = FB_XMIN - CAM_R;  // was outside, stay outside
            else
                pos.x = FB_XMIN + CAM_R;  // was inside, stay inside
        }
    }

    // ── WORLD BOUNDARY ─────────────────────────────────────────────────────
    pos.x = glm::clamp(pos.x, -65.f, 55.f);
    pos.z = glm::clamp(pos.z, -50.f, 65.f);
    pos.y = glm::clamp(pos.y, 0.3f, 250.f);
}
// ══════════════════════════════════════════════════════════════════════════════
//  INPUT
// ══════════════════════════════════════════════════════════════════════════════
static void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // ── Camera movement ────────────────────────────────────────────────────
    float vel = camera.speed * deltaTime;
    glm::vec3 desiredPos = camera.position;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) desiredPos += camera.front * vel;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) desiredPos -= camera.front * vel;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) desiredPos -= camera.right * vel;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) desiredPos += camera.right * vel;
    if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) desiredPos += camera.up * vel;
    if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) desiredPos -= camera.up * vel;

    // Per-axis sliding collision
    {
        glm::vec3 tryX = camera.position; tryX.x = desiredPos.x; applyCollision(tryX); camera.position.x = tryX.x;
        glm::vec3 tryZ = camera.position; tryZ.z = desiredPos.z; applyCollision(tryZ); camera.position.z = tryZ.z;
        glm::vec3 tryY = camera.position; tryY.y = desiredPos.y; applyCollision(tryY); camera.position.y = tryY.y;
    }

    // ── Camera rotation ────────────────────────────────────────────────────
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) { camera.pitch += 1.f; updateCameraVectors(); }
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) { camera.yaw += 1.f; updateCameraVectors(); }
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) { camera.roll += 1.f; updateCameraVectors(); }

    // ── Single-press toggles ───────────────────────────────────────────────
    static bool bP = false, oP = false, tP = false, dP = false, fP = false, uP = false, doorP = false;
    static bool wfP = false;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && !wfP) {
        wireframeMode = !wireframeMode;
        wfP = true;
        std::cout << "[W] Wireframe: " << (wireframeMode ? "ON" : "OFF") << "\n";
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE) wfP = false;

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !bP) {
        camera.birdsEyeMode = !camera.birdsEyeMode; bP = true;
        std::cout << "[B] Bird's-eye: " << (camera.birdsEyeMode ? "ON" : "OFF") << "\n";
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE) bP = false;

    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS && !oP) {
        camera.orbitMode = !camera.orbitMode; oP = true;
        std::cout << "[O] Orbit: " << (camera.orbitMode ? "ON" : "OFF") << "\n";
    }
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_RELEASE) oP = false;

    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !tP) {
        textureMode = (textureMode + 1) % 4; tP = true;
        const char* names[] = { "0-None","1-Simple","2-Gouraud","3-PhongBlend" };
        std::cout << "[T] Texture: " << names[textureMode] << "\n";
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE) tP = false;

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS && !dP) {
        dayMode = !dayMode; dP = true;
        std::cout << "[D] " << (dayMode ? "DAY" : "NIGHT") << "\n";
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE) dP = false;

    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fP) {
        fanOn = !fanOn; fP = true;
        std::cout << "[F] Fan: " << (fanOn ? "ON" : "OFF") << "\n";
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) fP = false;

    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS && !uP) {
        lampPostLightOn = !lampPostLightOn; uP = true;
        std::cout << "[U] Lamp: " << (lampPostLightOn ? "ON" : "OFF") << "\n";
    }
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_RELEASE) uP = false;

    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS && !doorP) {
        doorAnimating = true; doorOpening = (doorOpenAngle < 45.f); doorP = true;
    }
    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_RELEASE) doorP = false;

    // ══════════════════════════════════════════════════════════════════════
    //  V held → viewport switching
    //  V NOT held → lighting toggles
    //  These are MUTUALLY EXCLUSIVE — no key conflict possible
    // ══════════════════════════════════════════════════════════════════════
    bool vHeld = (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS);

    if (vHeld)
    {
        // Viewport keys — only fire when V is held
        static bool vp0 = false, vp1 = false, vp2 = false, vp3 = false, vp4 = false;

        if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS && !vp0) { activeViewport = -1; vp0 = true; std::cout << "[V+0] 4-split\n"; }
        if (glfwGetKey(window, GLFW_KEY_0) == GLFW_RELEASE) vp0 = false;
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && !vp1) { activeViewport = 0; vp1 = true; std::cout << "[V+1] Isometric\n"; }
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_RELEASE) vp1 = false;
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && !vp2) { activeViewport = 1; vp2 = true; std::cout << "[V+2] Front\n"; }
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_RELEASE) vp2 = false;
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && !vp3) { activeViewport = 2; vp3 = true; std::cout << "[V+3] Top-down\n"; }
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_RELEASE) vp3 = false;
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS && !vp4) { activeViewport = 3; vp4 = true; std::cout << "[V+4] Free cam\n"; }
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_RELEASE) vp4 = false;
    }
    else
    {
        // Lighting keys — only fire when V is NOT held
        static bool p1 = false, p2 = false, p3 = false, p4 = false, p5 = false, p6 = false;

        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && !p1) {
            lighting.directionalLightOn = !lighting.directionalLightOn; p1 = true;
            std::cout << "[1] Directional: " << (lighting.directionalLightOn ? "ON" : "OFF") << "\n";
        }
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_RELEASE) p1 = false;

        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && !p2) {
            lighting.pointLightsOn = !lighting.pointLightsOn; p2 = true;
            std::cout << "[2] Point lights: " << (lighting.pointLightsOn ? "ON" : "OFF") << "\n";
        }
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_RELEASE) p2 = false;

        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && !p3) {
            lighting.spotLightOn = !lighting.spotLightOn; p3 = true;
            std::cout << "[3] Spot light: " << (lighting.spotLightOn ? "ON" : "OFF") << "\n";
        }
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_RELEASE) p3 = false;

        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS && !p4) {
            lighting.ambientOn = !lighting.ambientOn; p4 = true;
            std::cout << "[4] Ambient: " << (lighting.ambientOn ? "ON" : "OFF") << "\n";
        }
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_RELEASE) p4 = false;

        if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS && !p5) {
            lighting.diffuseOn = !lighting.diffuseOn; p5 = true;
            std::cout << "[5] Diffuse: " << (lighting.diffuseOn ? "ON" : "OFF") << "\n";
        }
        if (glfwGetKey(window, GLFW_KEY_5) == GLFW_RELEASE) p5 = false;

        if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS && !p6) {
            lighting.specularOn = !lighting.specularOn; p6 = true;
            std::cout << "[6] Specular: " << (lighting.specularOn ? "ON" : "OFF") << "\n";
        }
        if (glfwGetKey(window, GLFW_KEY_6) == GLFW_RELEASE) p6 = false;
    }
}
// ══════════════════════════════════════════════════════════════════════════════
//  INSTRUCTIONS
// ══════════════════════════════════════════════════════════════════════════════
static void printInstructions() {
    const char* lines[] = {
        "",
        "╔══════════════════════════════════════════════════════════════════╗",
        "║   STUDENT WELFARE CENTRE – KUET     ║",
        "╠══════════════════════════════════════════════════════════════════╣",
        "║                      CAMERA MOVEMENT                            ║",
        "╠══════════════════════════════════════════════════════════════════╣",
        "║   Arrow UP          →  Move forward                             ║",
        "║   Arrow DOWN        →  Move backward                            ║",
        "║   Arrow LEFT        →  Strafe left                              ║",
        "║   Arrow RIGHT       →  Strafe right                             ║",
        "║   Page UP           →  Move up                                  ║",
        "║   Page DOWN         →  Move down                                ║",
        "╠══════════════════════════════════════════════════════════════════╣",
        "║                    CAMERA ROTATION                              ║",
        "╠══════════════════════════════════════════════════════════════════╣",
        "║   X                 →  Pitch (tilt up/down)                     ║",
        "║   Y                 →  Yaw   (turn left/right)                  ║",
        "║   Z                 →  Roll  (tilt sideways)                    ║",
        "╠══════════════════════════════════════════════════════════════════╣",
        "║                    CAMERA MODES                                 ║",
        "╠══════════════════════════════════════════════════════════════════╣",
        "║   B                 →  Toggle Bird's-eye view                   ║",
        "║   O                 →  Toggle Orbit mode                        ║",
        "╠══════════════════════════════════════════════════════════════════╣",
        "║                    SCENE CONTROLS                               ║",
        "╠══════════════════════════════════════════════════════════════════╣",
        "║   ENTER             →  Open / Close doors                       ║",
        "║   D                 →  Toggle Day / Night mode                  ║",
        "║   F                 →  Toggle ceiling Fan on/off                ║",
        "║   U                 →  Toggle Lamp post light on/off            ║",
        "╠══════════════════════════════════════════════════════════════════╣",
        "║                    TEXTURE MODES                                ║",
        "╠══════════════════════════════════════════════════════════════════╣",
        "║   T                 →  Cycle texture mode:                      ║",
        "║                         0 = No texture (color only)             ║",
        "║                         1 = Simple texture                      ║",
        "║                         2 = Gouraud shading + texture           ║",
        "║                         3 = Phong blend + texture               ║",
        "╠══════════════════════════════════════════════════════════════════╣",
        "║                    LIGHTING CONTROLS                            ║",
        "╠══════════════════════════════════════════════════════════════════╣",
        "║   1                 →  Toggle Directional light                 ║",
        "║   2                 →  Toggle Point lights                      ║",
        "║   3                 →  Toggle Spot light                        ║",
        "║   4                 →  Toggle Ambient component                 ║",
        "║   5                 →  Toggle Diffuse component                 ║",
        "║   6                 →  Toggle Specular component                ║",
        "╠══════════════════════════════════════════════════════════════════╣",
        "║                    VIEWPORT CONTROLS                            ║",
        "╠══════════════════════════════════════════════════════════════════╣",
        "║   V + 0             →  4-viewport split screen (default)        ║",
        "║   V + 1             →  Fullscreen: Isometric view               ║",
        "║   V + 2             →  Fullscreen: Front view                   ║",
        "║   V + 3             →  Fullscreen: Top-down view                ║",
        "║   V + 4             →  Fullscreen: Free camera view             ║",
        "╠══════════════════════════════════════════════════════════════════╣",
        "║                    CURVE SURFACES (Lab Req.)                    ║",
        "╠══════════════════════════════════════════════════════════════════╣",
        "║   Bezier Swept Surface   →  Vase in courtyard                   ║",
        "║   Catmull-Rom Spline     →  Lamp post columns                   ║",
        "║   Ruled Surface          →  Woven canopy near sphere            ║",
        "╠══════════════════════════════════════════════════════════════════╣",
        "║   ESC               →  Quit                                     ║",
        "╚══════════════════════════════════════════════════════════════════╝",
        ""
    };

    int count = sizeof(lines) / sizeof(lines[0]);
    for (int i = 0; i < count; i++) {
        std::cout << lines[i] << std::endl;
    }
}
// ══════════════════════════════════════════════════════════════════════════════
//  MAIN
// ══════════════════════════════════════════════════════════════════════════════
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* mon = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(mon);
    SCR_WIDTH = mode->width; SCR_HEIGHT = mode->height;

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "SWC KUET", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    int fbW, fbH; glfwGetFramebufferSize(window, &fbW, &fbH);
    SCR_WIDTH = fbW; SCR_HEIGHT = fbH;

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    glEnable(GL_DEPTH_TEST);

    unsigned int shader = createShaderProgram();
    setupCube(); setupCylinder(32); setupSphere(32, 32); setupCone(32);
    createAllTextures();
    setupAllCurveSurfaces();
    initPedestrians();
    updateCameraVectors();
    printInstructions();

    glUseProgram(shader);
    glUniform1i(glGetUniformLocation(shader, "texSampler"), 0);

    const glm::vec3 SC(0.f, 6.f, -10.f);

    // ── Render loop ───────────────────────────────────────────────────────
    while (!glfwWindowShouldClose(window))
    {
        float cf = (float)glfwGetTime();
        deltaTime = cf - lastFrame; lastFrame = cf;

        // Update
        updatePedestrians(deltaTime);
        processInput(window);   // <-- ALL key handling is inside here

        // Animations
        
            fanAngle += fanOn ? (deltaTime * 180.f) : 0.f;
            tvTime += deltaTime;  
            if (fanAngle > 360.f) fanAngle -= 360.f;
        
        if (doorAnimating) {
            if (doorOpening) {
                doorOpenAngle += deltaTime * 120.f;
                if (doorOpenAngle >= 90.f) { doorOpenAngle = 90.f; doorAnimating = false; }
            }
            else {
                doorOpenAngle -= deltaTime * 120.f;
                if (doorOpenAngle <= 0.f) { doorOpenAngle = 0.f; doorAnimating = false; }
            }
        }

        // Clear
        glClearColor(dayMode ? 0.53f : 0.05f,
            dayMode ? 0.81f : 0.05f,
            dayMode ? 0.92f : 0.12f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shader);
        setupLighting(shader);  // uploads ALL lighting uniforms every frame

        // ── Render viewports ──────────────────────────────────────────────
        if (activeViewport == -1)
        {
            // 4-split
            int hw = SCR_WIDTH / 2, hh = SCR_HEIGHT / 2;
            glm::mat4 proj = customPerspective(
                glm::radians(45.f), (float)hw / (float)hh, 0.1f, 500.f);
            glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(proj));

            glViewport(0, hh, hw, hh);
            {
                auto v = glm::lookAt(SC + glm::vec3(60, 45, 60), SC, { 0,1,0 });
                glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(v));
                glPolygonMode(GL_FRONT_AND_BACK, wireframeMode ? GL_LINE : GL_FILL);
                renderScene(shader);
            }

            glViewport(hw, hh, hw, hh);
            {
                auto v = glm::lookAt(SC + glm::vec3(0, 10, 80), SC, { 0,1,0 });
                glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(v));
                renderScene(shader);
            }

            glViewport(0, 0, hw, hh);
            {
                auto v = glm::lookAt(SC + glm::vec3(0, 110, 0), SC, { 0,0,-1 });
                glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(v));
                renderScene(shader);
            }

            glViewport(hw, 0, hw, hh);
            {
                glm::mat4 v;
                if (camera.birdsEyeMode) v = glm::lookAt(SC + glm::vec3(0, 90, 0), SC, { 0,0,-1 });
                else if (camera.orbitMode) {
                    float ox = SC.x + sinf(cf) * 70.f, oz = SC.z + cosf(cf) * 70.f;
                    v = glm::lookAt({ ox,SC.y + 20.f,oz }, SC, { 0,1,0 });
                }
                else                          v = glm::lookAt(camera.position, camera.position + camera.front, camera.up);
                glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(v));
             
                renderScene(shader);
            }
        }
        else
        {
            // Fullscreen
            glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
            glm::mat4 proj = customPerspective(
                glm::radians(45.f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 500.f);
            glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(proj));

            glm::mat4 v;
            switch (activeViewport) {
            case 0: v = glm::lookAt(SC + glm::vec3(60, 45, 60), SC, { 0,1,0 }); break;
            case 1: v = glm::lookAt(SC + glm::vec3(0, 10, 80), SC, { 0,1,0 }); break;
            case 2: v = glm::lookAt(SC + glm::vec3(0, 110, 0), SC, { 0,0,-1 }); break;
            default:
                if (camera.birdsEyeMode) v = glm::lookAt(SC + glm::vec3(0, 90, 0), SC, { 0,0,-1 });
                else if (camera.orbitMode) {
                    float ox = SC.x + sinf(cf) * 70.f, oz = SC.z + cosf(cf) * 70.f;
                    v = glm::lookAt({ ox,SC.y + 20.f,oz }, SC, { 0,1,0 });
                }
                else                          v = glm::lookAt(camera.position, camera.position + camera.front, camera.up);
                break;
            }
            glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(v));
            renderScene(shader);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

    } // end while

    glfwTerminate();
    return 0;
}