#include <iostream>
#include <fstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "model.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void ProcessInput(GLFWwindow* window);
int Init(GLFWwindow*& window);
void CreateGeometry(GLuint &VAO, GLuint &EBO, int &size, int &numIndices);
void CreateShaders();
void CreateProgram(GLuint& programID, const char* vertex, const char* fragment);

unsigned int GeneratePlane(const char* heightmap, unsigned char* &data, GLenum format, int comp, float hScale, float xzScale, unsigned int& indexCount, unsigned int& heightmapID);
void RenderBox(glm::mat4& view, glm::mat4& projection, int triangleIndexCount, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale);
void RenderSkyBox();
void RenderTerrain();
void RenderModel(Model* model, GLuint& programID, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, glm::vec3 color = glm::vec3(0, 0, 0), bool untextured = false);

void CreateFrameBuffer(int width, int height, unsigned int& frameBufferID, unsigned int& colorBufferID, unsigned int& depthBufferID);
void RenderToBuffer(unsigned int frameBufferTo, unsigned int colorBufferFrom, GLuint shader);
void RenderTerrainScanner(unsigned int frameBufferTo, unsigned int colorBufferFrom, unsigned int depthBufferFrom, GLuint shader);

void RenderQuad();

//Callbacks
void Mouse_Callback(GLFWwindow* window, double xpos, double ypos);
void Key_Callback(GLFWwindow* window, int key, int scancode, int action, int mods);

bool keys[2048];

//Utils
void LoadFile(const char* filename, char*& output);
GLuint loadTexture(const char* path, int comp = 0);

//Program ID's
GLuint simpleProgram, skyBoxProgram, terrainProgram, modelProgram, untexturedModelProgram, blitProgram, terrainScanProgram;

const int WIDTH = 1280, HEIGHT = 720;

float lastX, lastY;
bool firstMouse = true;
float camYaw, camPitch;

//world data
glm::vec3 lightDirection = glm::normalize(glm::vec3(-0.5f, -0.5f, -0.5f));
glm::vec3 cameraPosition = glm::vec3(100, 125.0f, 100.0f);
glm::quat camQuat = glm::quat(glm::vec3(glm::radians(camPitch), glm::radians(camYaw), 0));

glm::mat4 view;
glm::mat4 projection;

GLuint boxVAO, boxEBO, boxTex, boxNormal, boxGradientTex;
int boxSize, boxIndexCount;

//Terrain Data

GLuint terrainVAO, terrainIndexCount, heightMapID, heightMapNormalID;
unsigned char* heightmapData;

GLuint dirt, sand, grass, rock, snow;

Model* backpack;
Model* house;
Model* ironMan;

unsigned int frameBuff1, frameBuff2;
unsigned int colorBuff1, colorBuff2;
unsigned int depthBuff1, depthBuff2;

int main()
{
    GLFWwindow* window;
    int result = Init(window);
    if (result != 0) return result;
    
    stbi_set_flip_vertically_on_load(true);
    
    CreateShaders();
    CreateGeometry(boxVAO, boxEBO, boxSize, boxIndexCount);
    
    //Terrain
    terrainVAO = GeneratePlane("textures/heightmap3.png", heightmapData, GL_RGBA, 4, 250.0f, 5.0f, terrainIndexCount, heightMapID);
    heightMapNormalID = loadTexture("textures/heightmapNormal3.png");

    dirt = loadTexture("textures/dirt.jpg", 4);
    sand = loadTexture("textures/sand.jpg", 4);
    grass = loadTexture("textures/grass.png", 4);
    rock = loadTexture("textures/rock.jpg", 4);
    snow = loadTexture("textures/snow.jpg", 4);

    backpack = new Model("models/backpack/backpack.obj");
    house = new Model("models/cottage/cottage_obj.obj");
    ironMan = new Model("models/IronMan/IronMan.obj");

    //Box textures
    boxTex = loadTexture("textures/container2.png");
    boxNormal = loadTexture("textures/container2normal.png");
    //Gradient tex for cell shading
    boxGradientTex = loadTexture("textures/GradientTexture2.png");

    glViewport(0, 0, WIDTH, HEIGHT);

    //Matrices

    //Update view matrix everytime camera moves
    //Update world matrix everytime objects are moved/changed/scaled

    /*glm::mat4 world = glm::mat4(1.0f);
    world = glm::rotate(world, glm::radians(45.0f), glm::vec3(0, 1, 0));
    world = glm::scale(world, glm::vec3(1, 1, 1));
    world = glm::translate(world, glm::vec3(0, 0, 0));*/
    
    CreateFrameBuffer(WIDTH, HEIGHT, frameBuff1, colorBuff1, depthBuff1);
    CreateFrameBuffer(WIDTH, HEIGHT, frameBuff2, colorBuff2, depthBuff2);


    view = glm::lookAt(cameraPosition, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    projection = glm::perspective(glm::radians(45.0f), WIDTH / (float)HEIGHT, 0.05f, 10000.0f);

    while (!glfwWindowShouldClose(window))
    {
        //Input
        ProcessInput(window);

        glBindFramebuffer(GL_FRAMEBUFFER, frameBuff1); 
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_DEPTH);

        //Clear scene
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //Draw scene
        float t = glfwGetTime();

        RenderSkyBox();
        RenderTerrain();
        RenderBox(view, projection, boxIndexCount, glm::vec3(100, 350, 300), glm::vec3(t * 0.2, t * .4, t * -0.2), glm::vec3(200, 200, 200));
        RenderBox(view, projection, boxIndexCount, glm::vec3(1500, 150, 1300), glm::vec3(1, 1, 1), glm::vec3(10, 10, 10));
        RenderModel(backpack, modelProgram, glm::vec3(800, 250, 1100), glm::vec3(0, t * .2, 0), glm::vec3(50, 50, 50));
        //RenderModel(house, untexturedModelProgram, glm::vec3(1500, 20, 1300), glm::vec3(0, t * 5, 0), glm::vec3(5, 5, 5), glm::vec4(1, 1, 0, 1), true);
        RenderModel(ironMan, untexturedModelProgram, glm::vec3(800, -300, 1100), glm::vec3(0, t * .2, 0), glm::vec3(3, 3, 3), glm::vec3(.5, .1, .1), true);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //glDisable(GL_DEPTH_TEST);
        //glUseProgram(terrainScanProgram);

        RenderTerrainScanner(0, colorBuff1, depthBuff1, terrainScanProgram);

        //Swap & Poll
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void RenderSkyBox()
{
    // OpenGL Setup
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_DEPTH);

    glUseProgram(skyBoxProgram);

    //Matrices

    //Update view matrix everytime camera moves
    //Update world matrix everytime objects are moved/changed/scaled

    glm::mat4 world = glm::mat4(1.0f);
    world = glm::translate(world, cameraPosition);
    world = glm::scale(world, glm::vec3(100, 100, 100));

    glUniformMatrix4fv(glGetUniformLocation(skyBoxProgram, "world"), 1, GL_FALSE, glm::value_ptr(world));
    glUniformMatrix4fv(glGetUniformLocation(skyBoxProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(skyBoxProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glUniform3fv(glGetUniformLocation(skyBoxProgram, "lightDirection"), 1, glm::value_ptr(lightDirection));
    glUniform3fv(glGetUniformLocation(skyBoxProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));

    glBindVertexArray(boxVAO);
    glDrawElements(GL_TRIANGLES, boxIndexCount, GL_UNSIGNED_INT, 0);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DEPTH);
}

void RenderTerrain()
{
    glEnable(GL_DEPTH);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glUseProgram(terrainProgram);

    glm::mat4 world = glm::mat4(1.0f);

    //glUniform1i(glGetUniformLocation(terrainProgram, "mainTex"), 0);

    glUniformMatrix4fv(glGetUniformLocation(terrainProgram, "world"), 1, GL_FALSE, glm::value_ptr(world));
    glUniformMatrix4fv(glGetUniformLocation(terrainProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(terrainProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    //make the sun move
    //float t = glfwGetTime();
    //lightDirection = glm::normalize(glm::vec3(glm::sin(t), -0.5f, glm::cos(t)));

    glUniform3fv(glGetUniformLocation(terrainProgram, "lightDirection"), 1, glm::value_ptr(lightDirection));
    glUniform3fv(glGetUniformLocation(terrainProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, heightMapID);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, heightMapNormalID);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, dirt);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, sand);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, grass);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, rock);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, snow);

    //std::cout << heightmapID << std::endl;

    glBindVertexArray(terrainVAO);
    glDrawElements(GL_TRIANGLES, terrainIndexCount, GL_UNSIGNED_INT, 0);
}

void RenderModel(Model* model, GLuint& programID, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, glm::vec3 color, bool untextured)
{
    //glEnable(GL_BLEND);
    
    //blends
    
    //alpha
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //additive 
    //glBlendFunc(GL_ONE, GL_ONE);
    //soft additive 
    //glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE);
    //multiply
    //glBlendFunc(GL_DST_COLOR, GL_ZERO);
    //double multiply

    glEnable(GL_DEPTH);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glUseProgram(programID);

    glm::mat4 world = glm::mat4(1.0f);
    world = glm::translate(world, pos);
    world = world * glm::toMat4(glm::quat(rot));
    world = glm::scale(world, scale);

    glUniformMatrix4fv(glGetUniformLocation(programID, "world"), 1, GL_FALSE, glm::value_ptr(world));
    glUniformMatrix4fv(glGetUniformLocation(programID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(programID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    if(untextured)
    {
        glm::vec4 objectColor = glm::vec4(color, 1);
        glUniform4fv(glGetUniformLocation(programID, "defaultColor"), 1, glm::value_ptr(objectColor));
    }
    glUniform3fv(glGetUniformLocation(programID, "lightDirection"), 1, glm::value_ptr(lightDirection));
    glUniform3fv(glGetUniformLocation(programID, "cameraPosition"), 1, glm::value_ptr(cameraPosition));

    model->Draw(programID);

    glDisable(GL_BLEND);
}

unsigned int GeneratePlane(const char* heightmap, unsigned char* &data, GLenum format, int comp, float hScale, float xzScale, unsigned int& indexCount, unsigned int& heightmapID) {
    int width, height, channels;
    //unsigned char* data = nullptr;
    if (heightmap != nullptr) {
        data = stbi_load(heightmap, &width, &height, &channels, comp);
        if (data) {
            glGenTextures(1, &heightmapID);
            glBindTexture(GL_TEXTURE_2D, heightmapID);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        std::cout << "Heightmap Loaded! " << heightmapID << std::endl;
    }

    int stride = 8;
    float* vertices = new float[(width * height) * stride];
    unsigned int* indices = new unsigned int[(width - 1) * (height - 1) * 6];

    int vertexIndex = 0;
    for (int i = 0; i < (width * height); i++) {
        // TODO: calculate x/z values
        int x = i % width;
        int z = i / width;

        float texHeight = (float)data[i * comp];

        // TODO: set position
        vertices[vertexIndex++] = x * xzScale;
        vertices[vertexIndex++] = (texHeight / 255.0f) * hScale;
        vertices[vertexIndex++] = z * xzScale;

        // TODO: set normal
        vertices[vertexIndex++] = 0;
        vertices[vertexIndex++] = 1;
        vertices[vertexIndex++] = 0;

        // TODO: set uv
        vertices[vertexIndex++] = x / (float)width;
        vertices[vertexIndex++] = z / (float)height;

    }

    // OPTIONAL TODO: Calculate normal
    // TODO: Set normal

    vertexIndex = 0;
    for (int i = 0; i < (width - 1) * (height - 1); i++) {
        // TODO: calculate x/z values
        int x = i % (width - 1);
        int z = i / (width - 1);

        int vertex = z * width + x;

        indices[vertexIndex++] = vertex;
        indices[vertexIndex++] = vertex + width;
        indices[vertexIndex++] = vertex + width + 1;
        
        indices[vertexIndex++] = vertex;
        indices[vertexIndex++] = vertex + width + 1;
        indices[vertexIndex++] = vertex + 1;

    }

    unsigned int vertSize = (width * height) * stride * sizeof(float);
    indexCount = ((width - 1) * (height - 1) * 6);

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertSize, vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);

    // vertex information!
    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * stride, 0);
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * stride, (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);
    // uv
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * stride, (void*)(sizeof(float) * 6));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    delete[] vertices;
    delete[] indices;

    //stbi_image_free(data);

    std::cout << "Plane generated! " << VAO << std::endl;

    return VAO;
}

void ProcessInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    bool camChanged = false;

    if(keys[GLFW_KEY_W])
    {
        cameraPosition += camQuat * glm::vec3(0, 0, 1) * 0.2f;
        camChanged = true;
    }
    if (keys[GLFW_KEY_A])
    {
        cameraPosition += camQuat * glm::vec3(1, 0, 0) * 0.2f;
        camChanged = true;
    }
    if (keys[GLFW_KEY_S])
    {
        cameraPosition += camQuat * glm::vec3(0, 0, -1 * 0.2f);
        camChanged = true;
    }
    if (keys[GLFW_KEY_D])
    {
        cameraPosition += camQuat * glm::vec3(-1, 0, 0) * 0.2f;
        camChanged = true;
    }

    if(camChanged)
    {
        glm::vec3 camForward = camQuat * glm::vec3(0, 0, 1);
        glm::vec3 camUp = camQuat * glm::vec3(0, 1, 0);
        view = glm::lookAt(cameraPosition, cameraPosition + camForward, camUp);
    }
}

int Init(GLFWwindow*& window)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    window = glfwCreateWindow(WIDTH, HEIGHT, "OPENGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    //Register callbacks
    glfwSetCursorPosCallback(window, Mouse_Callback);
    glfwSetKeyCallback(window, Key_Callback);

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    return 0;
}

void CreateGeometry(GLuint &VAO, GLuint &EBO, int &size, int &numIndices)
{
    // need 24 vertices for normal/uv-mapped Cube
    float vertices[] = {
        // positions            //colors            // tex coords   // normals          //tangents      //bitangents
        0.5f, -0.5f, -0.5f,     1.0f, 1.0f, 1.0f,   1.f, 1.f,       0.f, -1.f, 0.f,     -1.f, 0.f, 0.f,  0.f, 0.f, 1.f,
        0.5f, -0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   1.f, 0.f,       0.f, -1.f, 0.f,     -1.f, 0.f, 0.f,  0.f, 0.f, 1.f,
        -0.5f, -0.5f, 0.5f,     1.0f, 1.0f, 1.0f,   0.f, 0.f,       0.f, -1.f, 0.f,     -1.f, 0.f, 0.f,  0.f, 0.f, 1.f,
        -0.5f, -0.5f, -.5f,     1.0f, 1.0f, 1.0f,   0.f, 1.f,       0.f, -1.f, 0.f,     -1.f, 0.f, 0.f,  0.f, 0.f, 1.f,

        0.5f, 0.5f, -0.5f,      1.0f, 1.0f, 1.0f,   1.f, 1.f,       1.f, 0.f, 0.f,     0.f, -1.f, 0.f,  0.f, 0.f, 1.f,
        0.5f, 0.5f, 0.5f,       1.0f, 1.0f, 1.0f,   1.f, 0.f,       1.f, 0.f, 0.f,     0.f, -1.f, 0.f,  0.f, 0.f, 1.f,

        0.5f, 0.5f, 0.5f,       1.0f, 1.0f, 1.0f,   1.f, 0.f,       0.f, 0.f, 1.f,     1.f, 0.f, 0.f,  0.f, -1.f, 0.f,
        -0.5f, 0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   0.f, 0.f,       0.f, 0.f, 1.f,     1.f, 0.f, 0.f,  0.f, -1.f, 0.f,

        -0.5f, 0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   0.f, 0.f,      -1.f, 0.f, 0.f,     0.f, 1.f, 0.f,  0.f, 0.f, 1.f,
        -0.5f, 0.5f, -.5f,      1.0f, 1.0f, 1.0f,   0.f, 1.f,      -1.f, 0.f, 0.f,     0.f, 1.f, 0.f,  0.f, 0.f, 1.f,

        -0.5f, 0.5f, -.5f,      1.0f, 1.0f, 1.0f,   0.f, 1.f,      0.f, 0.f, -1.f,     1.f, 0.f, 0.f,  0.f, 1.f, 0.f,
        0.5f, 0.5f, -0.5f,      1.0f, 1.0f, 1.0f,   1.f, 1.f,      0.f, 0.f, -1.f,     1.f, 0.f, 0.f,  0.f, 1.f, 0.f,

        -0.5f, 0.5f, -.5f,      1.0f, 1.0f, 1.0f,   1.f, 1.f,       0.f, 1.f, 0.f,     1.f, 0.f, 0.f,  0.f, 0.f, 1.f,
        -0.5f, 0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   1.f, 0.f,       0.f, 1.f, 0.f,     1.f, 0.f, 0.f,  0.f, 0.f, 1.f,

        0.5f, -0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   1.f, 1.f,       0.f, 0.f, 1.f,     1.f, 0.f, 0.f,  0.f, -1.f, 0.f,
        -0.5f, -0.5f, 0.5f,     1.0f, 1.0f, 1.0f,   0.f, 1.f,       0.f, 0.f, 1.f,     1.f, 0.f, 0.f,  0.f, -1.f, 0.f,

        -0.5f, -0.5f, 0.5f,     1.0f, 1.0f, 1.0f,   1.f, 0.f,       -1.f, 0.f, 0.f,     0.f, 1.f, 0.f,  0.f, 0.f, 1.f,
        -0.5f, -0.5f, -.5f,     1.0f, 1.0f, 1.0f,   1.f, 1.f,       -1.f, 0.f, 0.f,     0.f, 1.f, 0.f,  0.f, 0.f, 1.f,

        -0.5f, -0.5f, -.5f,     1.0f, 1.0f, 1.0f,   0.f, 0.f,       0.f, 0.f, -1.f,     1.f, 0.f, 0.f,  0.f, 1.f, 0.f,
        0.5f, -0.5f, -0.5f,     1.0f, 1.0f, 1.0f,   1.f, 0.f,       0.f, 0.f, -1.f,     1.f, 0.f, 0.f,  0.f, 1.f, 0.f,

        0.5f, -0.5f, -0.5f,     1.0f, 1.0f, 1.0f,   0.f, 1.f,       1.f, 0.f, 0.f,     0.f, -1.f, 0.f,  0.f, 0.f, 1.f,
        0.5f, -0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   0.f, 0.f,       1.f, 0.f, 0.f,     0.f, -1.f, 0.f,  0.f, 0.f, 1.f,

        0.5f, 0.5f, -0.5f,      1.0f, 1.0f, 1.0f,   0.f, 1.f,       0.f, 1.f, 0.f,     1.f, 0.f, 0.f,  0.f, 0.f, 1.f,
        0.5f, 0.5f, 0.5f,       1.0f, 1.0f, 1.0f,   0.f, 0.f,       0.f, 1.f, 0.f,     1.f, 0.f, 0.f,  0.f, 0.f, 1.f
    };

    unsigned int indices[] = {  // note that we start from 0!
        // DOWN
        0, 1, 2,   // first triangle
        0, 2, 3,    // second triangle
        // BACK
        14, 6, 7,   // first triangle
        14, 7, 15,    // second triangle
        // RIGHT
        20, 4, 5,   // first triangle
        20, 5, 21,    // second triangle
        // LEFT
        16, 8, 9,   // first triangle
        16, 9, 17,    // second triangle
        // FRONT
        18, 10, 11,   // first triangle
        18, 11, 19,    // second triangle
        // UP
        22, 12, 13,   // first triangle
        22, 13, 23,    // second triangle
    };
    
    int stride = (3 + 3 + 2 + 3 + 3 + 3) * sizeof(float);
    size = sizeof(vertices) / stride;
    numIndices = sizeof(indices) / sizeof(int);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    //Layout of the vertex data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_TRUE, stride, (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 3, GL_FLOAT, GL_TRUE, stride, (void*)(11 * sizeof(float)));
    glEnableVertexAttribArray(4);

    glVertexAttribPointer(5, 3, GL_FLOAT, GL_TRUE, stride, (void*)(14 * sizeof(float)));
    glEnableVertexAttribArray(5);
}

void CreateShaders()
{
    CreateProgram(simpleProgram, "shaders/Vertex.shader", "shaders/Fragment.shader");

    //Set texture channels
    glUseProgram(simpleProgram);
    glUniform1i(glGetUniformLocation(simpleProgram, "mainTex"), 0);
    glUniform1i(glGetUniformLocation(simpleProgram, "normalTex"), 1);
    glUniform1i(glGetUniformLocation(simpleProgram, "gradientTex"), 2);

    CreateProgram(skyBoxProgram, "shaders/skyboxVertex.shader", "shaders/skyboxFragment.shader");
    CreateProgram(terrainProgram, "shaders/terrainVertex.shader", "shaders/terrainFragment.shader");

    glUseProgram(terrainProgram);
    glUniform1i(glGetUniformLocation(terrainProgram, "mainTex"), 0);
    glUniform1i(glGetUniformLocation(terrainProgram, "normalTex"), 1);

    glUniform1i(glGetUniformLocation(terrainProgram, "dirt"), 2);
    glUniform1i(glGetUniformLocation(terrainProgram, "sand"), 3);
    glUniform1i(glGetUniformLocation(terrainProgram, "grass"), 4);
    glUniform1i(glGetUniformLocation(terrainProgram, "rock"), 5);
    glUniform1i(glGetUniformLocation(terrainProgram, "snow"), 6);

    CreateProgram(modelProgram, "shaders/modelVertex.shader", "shaders/modelFragment.shader");

    glUseProgram(modelProgram);

    glUniform1i(glGetUniformLocation(modelProgram, "texture_diffuse1"), 0);
    glUniform1i(glGetUniformLocation(modelProgram, "texture_specular1"), 1);
    glUniform1i(glGetUniformLocation(modelProgram, "texture_normal1"), 2);
    glUniform1i(glGetUniformLocation(modelProgram, "texture_roughness1"), 3);
    glUniform1i(glGetUniformLocation(modelProgram, "texture_ao1"), 4);

    CreateProgram(untexturedModelProgram, "shaders/modelVertex.shader", "shaders/modelUntexturedFragment.shader");
    glUseProgram(untexturedModelProgram);

    CreateProgram(blitProgram, "shaders/imgVertex.shader", "shaders/imgFragment.shader");
    glUseProgram(blitProgram);
    glUniform1i(glGetUniformLocation(blitProgram, "mainTex"), 0);

    CreateProgram(terrainScanProgram, "shaders/terrainScanVertex.shader", "shaders/terrainScanFragment.shader");
    glUseProgram(terrainScanProgram);
    glUniform1i(glGetUniformLocation(terrainScanProgram, "screenTexture"), 0);
    glUniform1i(glGetUniformLocation(terrainScanProgram, "depthTexture"), 1);

}

void CreateProgram(GLuint& programID, const char* vertex, const char* fragment)
{
    char* vertexSrc;
    char* fragmentSrc;
    
    LoadFile(vertex, vertexSrc);
    LoadFile(fragment, fragmentSrc);
    
    GLuint vertexShaderID, fragmentShaderID;
    //GLuint programID;

    int success;
    char infoLog[512];
    vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShaderID, 1, &vertexSrc, nullptr);
    glCompileShader(vertexShaderID);
    glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &success);
    
    if(!success)
    {
        glGetShaderInfoLog(vertexShaderID, 512, nullptr, infoLog);
        std::cout << "ERROR COMIPILING VERTEXT SHADER\n" << infoLog << std::endl;
    }
    
    fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShaderID, 1, &fragmentSrc, nullptr);
    glCompileShader(fragmentShaderID);
    glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &success);

    if(!success)
    {
        glGetShaderInfoLog(fragmentShaderID, 512, nullptr, infoLog);
        std::cout << "ERROR COMIPILING FRAGMENT SHADER\n" << infoLog << std::endl;
    }
    
    programID = glCreateProgram();
    glAttachShader(programID, vertexShaderID);
    glAttachShader(programID, fragmentShaderID);
    glLinkProgram(programID);

    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if(!success)
    {
        glGetProgramInfoLog(fragmentShaderID, 512, nullptr, infoLog);
        std::cout << "ERROR LINKING PROGRAM\n" << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);

    delete vertexSrc;
    delete fragmentSrc;
}

void LoadFile(const char* filename, char*& output)
{
    std::ifstream file(filename, std::ios::binary);

    if(file.is_open())
    {
        file.seekg(0, file.end);
        int length = file.tellg();
        file.seekg(0, file.beg);

        output = new char[length + 1];

        file.read(output, length);
        output[length] = '\0';

        file.close();
    }
    else
    {
        output = nullptr;    
    }
}

GLuint loadTexture(const char* path, int comp)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, numChannels;
    unsigned char* data = stbi_load(path, &width, &height, &numChannels, comp);

    if(data)
    {
        if (comp != 0) numChannels = comp;
        if(numChannels == 3)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        }
        else if(numChannels == 4)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
    }
    else
    {
        std::cout << "Error loading texture: " << path << std::endl;
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return textureID;
}

void RenderBox(glm::mat4 &view, glm::mat4 &projection, int triangleIndexCount, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale)
{
   /* glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);*/

    glEnable(GL_DEPTH);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glm::mat4 world = glm::mat4(1.0f);
    world = glm::translate(world, pos);
    world = world * glm::toMat4(glm::quat(rot));
    world = glm::scale(world, scale);

    glUseProgram(simpleProgram);

    glUniformMatrix4fv(glGetUniformLocation(simpleProgram, "world"), 1, GL_FALSE, glm::value_ptr(world));
    glUniformMatrix4fv(glGetUniformLocation(simpleProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(simpleProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glUniform3fv(glGetUniformLocation(simpleProgram, "lightDirection"), 1, glm::value_ptr(lightDirection));
    glUniform3fv(glGetUniformLocation(simpleProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, boxTex);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, boxNormal);

    //For cellshading
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, boxGradientTex);

    glBindVertexArray(boxVAO);
    //glBindVertexArray(triangleEBO);
    //glDrawArrays(GL_TRIANGLES, 0, triangleSize);
    glDrawElements(GL_TRIANGLES, triangleIndexCount, GL_UNSIGNED_INT, 0);
}

void CreateFrameBuffer(int width, int height, unsigned int &frameBufferID, unsigned int &colorBufferID, unsigned int &depthBufferID)
{
    //Generate framebuffer
    glGenFramebuffers(1, &frameBufferID);
    //Generate colorbuffer
    glGenTextures(1, &colorBufferID);
    glBindTexture(GL_TEXTURE_2D, colorBufferID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //Generate depth buffer
    /*glGenRenderbuffers(1, &depthBufferID);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBufferID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);*/

    glGenTextures(1, &depthBufferID);
    glBindTexture(GL_TEXTURE_2D, depthBufferID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //attach buffers
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferID);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBufferID, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthBufferID, 0);
    //glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufferID);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Framebuffer not complete" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderToBuffer(unsigned int frameBufferTo, unsigned int colorBufferFrom, GLuint shader)
{
    //std::cout << "Rendering to buffer! " << "Buffer: " << frameBufferTo << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferTo);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorBufferFrom);

    RenderQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//void RenderToBuffer(unsigned int frameBufferTo, unsigned int colorBufferFrom, unsigned int depthBufferFrom, GLuint shader)
//{
//    //std::cout << "Rendering to buffer! " << "Buffer: " << frameBufferTo << std::endl;
//    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferTo);
//
//    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//    glUseProgram(shader);
//    glActiveTexture(GL_TEXTURE0);
//    glBindTexture(GL_TEXTURE_2D, colorBufferFrom);
//
//    glActiveTexture(GL_TEXTURE1);
//    glBindTexture(GL_TEXTURE_2D, depthBufferFrom);
//
//    RenderQuad();
//
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
//}

void RenderTerrainScanner(unsigned int frameBufferTo, unsigned int colorBufferFrom, unsigned int depthBufferFrom, GLuint shader)
{
    
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferTo);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorBufferFrom);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthBufferFrom);

    glm::mat4 inverseView = glm::inverse(view);
    glm::mat4 inverseProjection = glm::inverse(projection);
    glm::vec2 screenResolution = glm::vec2(WIDTH, HEIGHT);

    glUniformMatrix4fv(glGetUniformLocation(terrainScanProgram, "inverseView"), 1, GL_FALSE, glm::value_ptr(inverseView));
    glUniformMatrix4fv(glGetUniformLocation(terrainScanProgram, "inverseProjection"), 1, GL_FALSE, glm::value_ptr(inverseProjection));
    glUniform2fv(glGetUniformLocation(terrainScanProgram, "screenResolution"), 1, glm::value_ptr(screenResolution));

    RenderQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

GLuint quadVAO = 0;
GLuint quadVBO = 0;

void RenderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] =
        {
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Mouse_Callback(GLFWwindow* window, double xpos, double ypos)
{
    float x = (float)xpos;
    float y = (float)ypos;

    if(firstMouse)
    {
        lastX = x;
        lastY = y;
        firstMouse = false;
    }

    float dx = x - lastX;
    float dy = y - lastY;
    lastX = x;
    lastY = y;

    camYaw -= dx * 0.2;
    camPitch = glm::clamp(camPitch + dy * 0.2f, -90.0f, 90.0f);

    if(camYaw > 180.0f)
    {
        camYaw -= 360.0f;
    }

    if(camYaw < -180.0f)
    {
        camYaw += 360.0f;
    }

    //std::cout << "CamYaw: " << camYaw << "CamPitch: " << camPitch << std::endl;
    camQuat = glm::quat(glm::vec3(glm::radians(camPitch), glm::radians(camYaw), 0));

    glm::vec3 camForward = camQuat * glm::vec3(0, 0, 1);
    glm::vec3 camUp = camQuat * glm::vec3(0, 1, 0);
    view = glm::lookAt(cameraPosition, cameraPosition + camForward, camUp);
}
void Key_Callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(action == GLFW_PRESS)
    {
        //store key is pressed
        keys[key] = true;
    }
    else if(action == GLFW_RELEASE)
    {
        //store key is released
        keys[key] = false;
    }
}