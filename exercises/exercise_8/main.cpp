#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <vector>
#include <chrono>
#include <math.h>

// NEW! as our scene gets more complex, we start using more helper classes
//  I recommend that you read through the camera.h and model.h files to see if you can map the the previous
//  lessons to this implementation
#include "shader.h"
#include "camera.h"
#include "model.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// glfw and input functions
// ------------------------
void processInput(GLFWwindow* window);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_input_callback(GLFWwindow* window, int button, int other, int action, int mods);
void cursor_input_callback(GLFWwindow* window, double posX, double posY);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

// screen settings
// ---------------
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
const float PI = 3.14159265359;
float currentTime;
const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
const unsigned int RAINSPLASH_WIDTH = 2048, RAINSPLASH_HEIGHT = 2048;

// global variables used for rendering
// -----------------------------------
Shader* shader;
Shader* pbr_shading;
Shader* shadowMap_shader;
Shader* rainSplash_shader;
Shader* particle_shader;
Shader* splash_shader;


Model* carBodyModel;
Model* carPaintModel;
Model* carInteriorModel;
Model* carLightModel;
Model* carWindowsModel;
Model* carWheelModel;
Model* floorModel;

Model* houseBodyModel;
Model* houseRoofModel;
Model* houseDetailsModel;
Model* stoneModel;


GLuint carBodyTexture;
GLuint carPaintTexture;
GLuint carLightTexture;
GLuint carWindowsTexture;
GLuint carWheelTexture;
GLuint floorTexture;

GLuint splashTexture;

Camera camera(glm::vec3(0.0f, 1.6f, 5.0f));

// -- particle taken from ex 4
const unsigned int particleSize = 3;            // particle attributes
const unsigned int sizeOfFloat = 4;             // bytes in a float
unsigned int particleId = 0;                    // keep track of last particle to be updated
unsigned int particleVAO, particleVBO;
const unsigned int numberOfparticles = 10000;
const unsigned int vertexBufferSize = particleSize*sizeOfFloat*numberOfparticles;


Shader* skyboxShader;
unsigned int skyboxVAO; // skybox handle
unsigned int cubemapTexture; // skybox texture handle

unsigned int shadowMap, shadowMapFBO;
unsigned int rainMap, rainMapFBO;
glm::mat4 lightSpaceMatrix;
glm::mat4 rainSpaceMatrix;

// global variables used for control
// ---------------------------------
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
float deltaTime;
bool isPaused = false; // stop camera movement when GUI is open

float lightRotationSpeed = 1.0f;


// structure to hold particle info
//--------------------------------


// structure to hold lighting info
// -------------------------------
struct Light
{
    Light(glm::vec3 position, glm::vec3 color, float intensity, float radius)
        : position(position), color(color), intensity(intensity), radius(radius)
    {
    }

    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    float radius;
};

// structure to hold config info
// -------------------------------
struct Config
{
    Config() : lights()
    {
        // Adding lights
        //lights.emplace_back(position, color, intensity, radius);

        // light 1
        lights.emplace_back(glm::vec3(-15.9f, 14.7f, 2.7f), glm::vec3(1.0f, 1.0f, 1.0f), 0.6f, 0.0f);

    }



    // ambient light
    glm::vec3 ambientLightColor = {1.0f, 1.0f, 1.0f};
    float ambientLightIntensity = 0.25f;

    // material
    glm::vec3 reflectionColor = {1.0f, 0.01f, 0.01f};
    float ambientReflectance = 0.75f;
    float diffuseReflectance = 0.75f;
    float specularReflectance = 0.5f;
    float specularExponent = 10.0f;
    float roughness = 0.5f;
    float metalness = 0.5f;

    std::vector<Light> lights;

    // Rain
    float rainCount = 10000;
    float rainBoxSize = 30.0f;
    float splashQuadSize = 0.05f;
    float splashSpeed = 0.255f;
    glm::vec3 forward = {2.0f,0.0f,-0.01f};
    glm::vec3 velocity = glm::vec3(-1.0f,-9.82f,0.0f);

} config;


// function declarations
// ---------------------
void setAmbientUniforms(glm::vec3 ambientLightColor);
void setLightUniforms(Light &light);

// Taken inspiration from ex 4
void emitParticle(glm::vec3 start);
void createRainBox();
float getRandomOffset();
glm::vec3 getPostionVec(int index);

void createShadowMap();

// Taken from ex 8
void createRainMap();

// Taken from ex 4
void createParticleVertexBufferObject();

// Taken from ex 4
void bindParticleAttributes();

void setRainUniforms();
void setShadowUniforms();
void setRainMapUniforms();
void setSplashUniforms();

void drawSkybox();
void drawShadowMap();

// Taken from ex 8
void drawRainMap();

void drawObjects();
void drawGui();

void setupForwardAdditionalPass();
void resetForwardAdditionalPass();
unsigned int initSkyboxBuffers();
unsigned int loadCubemap(vector<std::string> faces);




int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    // glfw window creation
    // --------------------

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Rainy day", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_input_callback);
    glfwSetKeyCallback(window, key_input_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // load the shaders and the 3D models
    // ----------------------------------

    pbr_shading = new Shader("shaders/common_shading.vert", "shaders/pbr_shading.frag");
    shader = pbr_shading;

    carBodyModel = new Model("car/Body_LOD0.obj");
    carPaintModel = new Model("car/Paint_LOD0.obj");
    carInteriorModel = new Model("car/Interior_LOD0.obj");
    carLightModel = new Model("car/Light_LOD0.obj");
    carWindowsModel = new Model("car/Windows_LOD0.obj");
    carWheelModel = new Model("car/Wheel_LOD0.obj");
    floorModel = new Model("floor/floor.obj");

    houseBodyModel = new Model("house/Housebody_LOD0.obj");
    houseRoofModel = new Model("house/Roof_LOD0.obj");
    houseDetailsModel = new Model("house/Detail_LOD0.obj");
    stoneModel = new Model("house/Stone_LOD0.obj");

    splashTexture = TextureFromFileMod("splashAlbedo.png","rain",1);
    // init skybox
    vector<std::string> faces
    {
            "skybox/right.tga",
            "skybox/left.tga",
            "skybox/top.tga",
            "skybox/bottom.tga",
            "skybox/front.tga",
            "skybox/back.tga"
    };
    cubemapTexture = loadCubemap(faces);
    skyboxVAO = initSkyboxBuffers();
    skyboxShader = new Shader("shaders/skybox.vert", "shaders/skybox.frag");

    // --- Shadow map
    createShadowMap();
    shadowMap_shader = new Shader("shaders/shadowmap.vert", "shaders/shadowmap.frag");
    glDepthRange(-1,1);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_FRAMEBUFFER_SRGB);


    // --- rain splash
    createRainMap();
    rainSplash_shader = new Shader("shaders/rainsplash.vert", "shaders/rainsplash.frag");


    particle_shader = new Shader("shaders/particle.vert", "shaders/particle.frag","shaders/particle.geo");
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    createParticleVertexBufferObject();
    glDisable(GL_BLEND);

    splash_shader = new Shader("shaders/splash.vert", "shaders/splash.frag","shaders/splash.geo");

    // Dear IMGUI init
    // ---------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // Setup Platform/Renderer bindings


    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    float loopInterval = 0.02f;
    auto begin = std::chrono::high_resolution_clock::now();


    createRainBox();

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        static float lastFrame = 0.0f;
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        auto frameStart = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> appTime = frameStart - begin;
        currentTime = appTime.count();


        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 viewProjection = projection * view;

        processInput(window);


        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        drawSkybox();
        drawShadowMap();
        drawRainMap();


        shader->use();

        // First light + ambient
        setAmbientUniforms(config.ambientLightColor * config.ambientLightIntensity);
        setLightUniforms(config.lights[0]);
        setShadowUniforms();



        drawObjects();

        // Additional additive lights
        setupForwardAdditionalPass();
        for (int i = 1; i < config.lights.size(); ++i)
        {
            setLightUniforms(config.lights[i]);
            drawObjects();
        }
        resetForwardAdditionalPass();

        shader = particle_shader;
        particle_shader->use();
        setRainMapUniforms();
        setRainUniforms();


        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(particleVAO);
        glDrawArrays(GL_POINTS,0,numberOfparticles);
        glBindVertexArray(0);
        glDisable(GL_BLEND);


        shader = splash_shader;
        splash_shader->use();
        setSplashUniforms();
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(particleVAO);
        glDrawArrays(GL_POINTS,0,numberOfparticles);
        glBindVertexArray(0);
        glDisable(GL_BLEND);

        shader = pbr_shading;

        if (isPaused) {
            drawGui();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

        std::chrono::duration<float> elapsed = std::chrono::high_resolution_clock::now()-frameStart;
        while (loopInterval > elapsed.count()) {
            // busy waiting
            elapsed = std::chrono::high_resolution_clock::now() - frameStart;
        }
    }

    // Cleanup
    // -------
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    delete carBodyModel;
    delete houseBodyModel;
    delete carPaintModel;
    delete carInteriorModel;
    delete carLightModel;
    delete carWindowsModel;
    delete carWheelModel;
    delete floorModel;
    delete pbr_shading;
    delete shadowMap_shader;
    glDeleteVertexArrays(1, &particleVAO);
    glDeleteBuffers(1, &particleVBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}



// Taken inspiration from ex 4
void emitParticle(glm::vec3 start){
    glBindVertexArray(particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);

    float data[particleSize];

    data[0] = start.x;
    data[1] = start.y;
    data[2] = start.z;


    glBufferSubData(GL_ARRAY_BUFFER, particleId * particleSize * sizeOfFloat, particleSize * sizeOfFloat, data);
    particleId = (particleId + 1) % vertexBufferSize;

    glBindVertexArray(0);
}


float getRandomOffset(){

    return rand()/(float) RAND_MAX;
}


glm::vec3 getPostionVec(int index){
    float fragment = cbrt(config.rainCount);

    float x = getRandomOffset()+ fmod(index, fragment);
    float y = fmod(getRandomOffset()+ index/fragment,fragment);
    float z = (index/(fragment*fragment) + getRandomOffset());

    return (glm::vec3( x,y,z )/fragment)*config.rainBoxSize;
}


void createRainBox(){

    for (int i =0; i<config.rainCount; i+=2) {
        emitParticle( getPostionVec(i));
    }
}

// Taken from ex 8
void createRainMap()
{
    glGenTextures(1, &rainMap);
    glBindTexture(GL_TEXTURE_2D, rainMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, RAINSPLASH_WIDTH, RAINSPLASH_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);


    glGenFramebuffers(1, &rainMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, rainMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rainMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void createShadowMap()
{
    // create depth texture
    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // if you replace GL_LINEAR with GL_NEAREST you will see pixelation in the borders of the shadow
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // if you replace GL_LINEAR with GL_NEAREST you will see pixelation in the borders of the shadow
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // attach depth texture as FBO's depth buffer
    glGenFramebuffers(1, &shadowMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Taken from ex 4
void createParticleVertexBufferObject(){
    glGenVertexArrays(1, &particleVAO);
    glGenBuffers(1, &particleVBO);

    glBindVertexArray(particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);

    std::vector<float> data(vertexBufferSize * particleSize);
    for(unsigned int i = 0; i < data.size(); i++)
        data[i] = 0.0f;

    glBufferData(GL_ARRAY_BUFFER, vertexBufferSize * particleSize * sizeOfFloat, &data[0], GL_DYNAMIC_DRAW);
    bindParticleAttributes();
}

// Taken from ex 4
void bindParticleAttributes(){
    glBindVertexArray(particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);

    int posSize = 3;
    GLuint vertexLocation = glGetAttribLocation(particle_shader->ID, "pos");
    glEnableVertexAttribArray(vertexLocation);
    glVertexAttribPointer(vertexLocation, posSize, GL_FLOAT, GL_FALSE,
                          particleSize * sizeOfFloat, (void*)0);

    glBindVertexArray(0);
}


void setShadowUniforms()
{
    shader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
    shader->setInt("shadowMap", 6);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, shadowMap);

    shader->setMat4("rainSpaceMatrix", rainSpaceMatrix);
    shader->setInt("rainMap", 7);
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, rainMap);

}
void setRainMapUniforms()
{
    shader->setMat4("rainSpaceMatrix", rainSpaceMatrix);
    shader->setInt("rainMap", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rainMap);

}

void setRainUniforms() {

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 viewProjection = projection * view;

    shader->setMat4("viewProjection", viewProjection);

    shader->setFloat("currentTime", currentTime);
    shader->setFloat("boxSize", config.rainBoxSize);
    shader->setFloat("deltaTime", deltaTime);

    shader->setVec3("cameraPosition", camera.Position);
    shader->setVec3("forward", config.forward);

    shader->setVec3("velocity", config.velocity);

}

void setSplashUniforms(){
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 viewProjection = projection * view;
    shader->setMat4("viewProjection", viewProjection);

    shader->setFloat("currentTime", currentTime);
    shader->setFloat("boxSize", config.rainBoxSize);
    shader->setFloat("deltaTime", deltaTime);

    shader->setVec3("cameraPosition", camera.Position);
    shader->setVec3("forward", config.forward);

    shader->setVec3("velocity", config.velocity);
    shader->setFloat("splashSpeed", config.splashSpeed);


    shader->setFloat("splashQuadSize",config.splashQuadSize);
    shader->setMat4("rainSpaceMatrix", rainSpaceMatrix);
    shader->setInt("rainMap", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rainMap);


    shader->setInt("splashTexture", 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, splashTexture);


}


void setAmbientUniforms(glm::vec3 ambientLightColor)
{
    // ambient uniforms
    shader->setVec4("ambientLightColor", glm::vec4(ambientLightColor, glm::length(ambientLightColor) > 0.0f ? 1.0f : 0.0f));
}
void setLightUniforms(Light& light)
{
    glm::vec3 lightEnergy = light.color * light.intensity;


    if (shader->ID==pbr_shading->ID){
        lightEnergy *= PI;
    }

    // light uniforms
    shader->setVec3("lightPosition", light.position);
    shader->setVec3("lightColor", lightEnergy);
    shader->setFloat("lightRadius", light.radius);

}

void drawShadowMap()
{
    Shader* currShader = shader;
    shader = shadowMap_shader;

    // setup depth shader
    shader->use();

    // We use an ortographic projection since it is a directional light.
    // left, right, bottom, top, near and far values define the 3D volume relative to
    // the light position and direction that will be rendered to produce the depth texture.
    // Geometry outside of this range will not be considered when computing shadows.
    float near_plane = 1.0f;
    float shadowMapSize = 6.0f;
    float shadowMapDepthRange = 10.0f;
    float half = shadowMapSize / 2.0f;
    glm::mat4 lightProjection = glm::ortho(-half, half, -half, half, near_plane, near_plane + shadowMapDepthRange);
    glm::mat4 lightView = glm::lookAt(glm::normalize(config.lights[0].position) * shadowMapDepthRange * 0.5f, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
    lightSpaceMatrix = lightProjection * lightView;
    shader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

    // setup framebuffer size
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

    // bind our depth texture to the frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);

    // clear the depth texture/depth buffer
    glClear(GL_DEPTH_BUFFER_BIT);

    // draw scene from the light's perspective into the depth texture
    drawObjects();

    // unbind the depth texture from the frame buffer, now we can render to the screen (frame buffer) again
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

    shader = currShader;
}

// Taken from ex 8
void drawRainMap()
{
    Shader* currShader = shader;
    shader = rainSplash_shader;

    shader->use();

    float near_plane = 0.5f;
    float rainSplashSize = 25.0f;
    float rainSplashDepthRange = 10.0f;
    float half = rainSplashSize / 2.0f;
    glm::vec3 center = glm::vec3(0.0f);

    glm::mat4 rainProjection = glm::ortho(-half, half, -half, half, near_plane, near_plane + rainSplashDepthRange);
    glm::mat4 raintView = glm::lookAt(glm::normalize(-config.velocity) * rainSplashDepthRange * 0.5f, center, glm::vec3(0.0, 1.0, 0.0));
    rainSpaceMatrix = rainProjection * raintView;
    shader->setMat4("rainSpaceMatrix", rainSpaceMatrix);


    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glViewport(0, 0, RAINSPLASH_WIDTH, RAINSPLASH_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, rainMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    drawObjects();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    shader = currShader;
}
void drawSkybox()
{
    // render skybox
    glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
    skyboxShader->use();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    skyboxShader->setMat4("projection", projection);
    skyboxShader->setMat4("view", view);
    skyboxShader->setInt("skybox", 0);

    // skybox cube
    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // set depth function back to default
}
void drawObjects()
{
    // the typical transformation uniforms are already set for you, these are:
    // projection (perspective projection matrix)
    // view (to map world space coordinates to the camera space, so the camera position becomes the origin)
    // model (for each model part we draw)

    // camera parameters
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 viewProjection = projection * view;

    // camera position
    shader->setVec3("camPosition", camera.Position);
    // set viewProjection matrix uniform
    shader->setMat4("viewProjection", viewProjection);

    // set up skybox texture
    shader->setInt("skybox", 5);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    // material uniforms for car paint
    shader->setVec3("reflectionColor", config.reflectionColor);
    shader->setFloat("ambientReflectance", config.ambientReflectance);
    shader->setFloat("diffuseReflectance", config.diffuseReflectance);
    shader->setFloat("specularReflectance", config.specularReflectance);
    shader->setFloat("specularExponent", config.specularExponent);
    shader->setFloat("roughness", config.roughness);
    shader->setFloat("metalness", config.metalness);

    glm::mat4 model = glm::mat4(1.0f);
    shader->setMat4("model", model);
    carPaintModel->Draw(*shader);

    // material uniforms for other car parts (hardcoded)
    shader->setVec3("reflectionColor", 1.0f, 1.0f, 1.0f);
    shader->setFloat("ambientReflectance", 0.75f);
    shader->setFloat("diffuseReflectance", 0.75f);
    shader->setFloat("specularReflectance", 0.5f);
    shader->setFloat("specularExponent", 10.0f);
    shader->setFloat("roughness", 0.5f);
    shader->setFloat("metalness", 0.5f);

    carBodyModel->Draw(*shader);

    // draw car
    shader->setMat4("model", model);
    carLightModel->Draw(*shader);
    carInteriorModel->Draw(*shader);

    // draw wheel
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-.7432f, .328f, 1.39f));
    shader->setMat4("model", model);
    carWheelModel->Draw(*shader);

    // draw wheel
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-.7432f, .328f, -1.39f));
    shader->setMat4("model", model);
    carWheelModel->Draw(*shader);

    // draw wheel
    model = glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(0.0, 1.0, 0.0));
    model = glm::translate(model, glm::vec3(-.7432f, .328f, 1.39f));
    shader->setMat4("model", model);
    carWheelModel->Draw(*shader);

    // draw wheel
    model = glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(0.0, 1.0, 0.0));
    model = glm::translate(model, glm::vec3(-.7432f, .328f, -1.39f));
    shader->setMat4("model", model);
    carWheelModel->Draw(*shader);

    // draw floor
    model = glm::scale(glm::mat4(1.0), glm::vec3(5.f, 5.f, 5.f));
    shader->setMat4("model", model);

    shader->setVec4("texCoordTransform", glm::vec4(4.0f, 4.0f, 0, 0));
    floorModel->Draw(*shader);
    shader->setVec4("texCoordTransform", glm::vec4(1, 1, 0, 0));


    shader->setFloat("roughness", 0.25f);
    model = glm::mat4(1.0f);
    shader->setMat4("model", model);

    carWindowsModel->Draw(*shader);


    // draw house
    model = glm::scale(glm::mat4(1.0f), glm::vec3(5.f, 5.f, 5.f));
    model = glm::rotate(model, glm::pi<float>(), glm::vec3(0.0, 1.0, 0.0));
    model = glm::translate(model, glm::vec3(1.55f, -0.05f, 0.0f));
    shader->setMat4("model", model);
    houseDetailsModel->Draw(*shader);

    shader->setVec4("texCoordTransform", glm::vec4(12.0f, 12.0f, 0, 0));
    shader->setFloat("roughness", 0.95f);
    shader->setFloat("specularReflectance", 0.002f);
    shader->setFloat("specularExponent", 0.02f);
    houseBodyModel->Draw(*shader);


    shader->setVec4("texCoordTransform", glm::vec4(4.0f, 4.0f, 0, 0));
    shader->setFloat("roughness", 0.95f);
    shader->setFloat("specularReflectance", 0.05f);
    houseRoofModel->Draw(*shader);

    shader->setVec4("texCoordTransform", glm::vec4(1, 1, 0, 0));
    shader->setFloat("roughness", 0.95f);
    shader->setFloat("specularReflectance", 0.005f);
    stoneModel->Draw(*shader);


}
void drawGui(){
    glDisable(GL_FRAMEBUFFER_SRGB);
    float minMaxValue = 15.0f;

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("Settings");
        ImGui::Text("Camera: %f, %f, %f",camera.Position.x,camera.Position.y,camera.Position.x);
        ImGui::Separator();

        ImGui::Text("Rain: ");
        ImGui::DragFloat3("Rain velocity", (float*)&config.velocity, .1f, -minMaxValue, minMaxValue);
        ImGui::DragFloat("Rain splash size", (float*)&config.splashQuadSize, .01f, 0.01f, 0.1f);
        ImGui::DragFloat("Rain splash speed", (float*)&config.splashSpeed, .01f, 0.1f, 1.0f);

        ImGui::Separator();


        ImGui::Separator();


        ImGui::Text("Light 1: ");
        ImGui::DragFloat3("light 1 direction", (float*)&config.lights[0].position, .1f, -20, 20);
        ImGui::ColorEdit3("light 1 color", (float*)&config.lights[0].color);
        ImGui::SliderFloat("light 1 intensity", &config.lights[0].intensity, 0.0f, 2.0f);
        ImGui::Separator();

        ImGui::Text("Car paint material: ");
        ImGui::ColorEdit3("color", (float*)&config.reflectionColor);
        ImGui::Separator();



        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glEnable(GL_FRAMEBUFFER_SRGB);
}









void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    //if (isPaused)
      //  return;

    // movement commands
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);



}


void setupForwardAdditionalPass()
{
    // Remove ambient from additional passes
    setAmbientUniforms(glm::vec3(0.0f));

    // Enable additive blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    // Set depth test to GL_EQUAL (only the fragments that match the depth buffer are rendered)
    glDepthFunc(GL_EQUAL);

    // Disable shadowmap
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, 0);
}
void cursor_input_callback(GLFWwindow* window, double posX, double posY){

    // camera rotation
    static bool firstMouse = true;
    if (firstMouse)
    {
        lastX = (float)posX;
        lastY = (float)posY;
        firstMouse = false;
    }

    float xoffset = (float)posX - lastX;
    float yoffset = lastY - (float)posY; // reversed since y-coordinates go from bottom to top

    lastX = (float)posX;
    lastY = (float)posY;

    if (isPaused)
        return;

    // we use the handy camera class from LearnOpenGL to handle our camera
    camera.ProcessMouseMovement(xoffset, yoffset);
}
void key_input_callback(GLFWwindow* window, int button, int other, int action, int mods){
    // controls pause mode
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){
        isPaused = !isPaused;
        glfwSetInputMode(window, GLFW_CURSOR, isPaused ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }

}
// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll((float)yoffset);
}
// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
void resetForwardAdditionalPass()
{
    // Restore ambient
    setAmbientUniforms(config.ambientLightColor * config.ambientLightIntensity);

    //Disable blend and restore default blend function
    glDisable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);

    // Restore default depth test
    glDepthFunc(GL_LESS);
}
// init the VAO of the skybox
// --------------------------
unsigned int initSkyboxBuffers() {
    // triangles forming the six faces of a cube
    // note that the camera is placed inside of the cube, so the winding order
    // is selected to make the triangles visible from the inside
    float skyboxVertices[108]{
            // positions
            -1.0f, 1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,

            -1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f,

            -1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, 1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, 1.0f,
            1.0f, -1.0f, 1.0f
    };

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);

    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    return skyboxVAO;
}
// loads a cubemap texture from 6 individual texture faces
// order:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front)
// -Z (back)
// -------------------------------------------------------
unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrComponents;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_SRGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return textureID;
}

