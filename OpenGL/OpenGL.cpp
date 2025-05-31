#include <iostream>
#include <fstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void ProcessInput(GLFWwindow* window);
int Init(GLFWwindow*& window);
void CreateGeometry(GLuint &VAO, GLuint &EBO, int &size, int &numIndices);
void CreateShaders();
void CreateProgram(GLuint& programID, const char* vertex, const char* fragment);
void RenderBox(glm::mat4& world, glm::mat4& view, glm::mat4& projection, GLuint& boxTex, GLuint& boxNormal, GLuint& gradientTex, GLuint& triangleVAO, int triangleIndexCount);
void RenderSkyBox();

//Calbacks
void Mouse_Callback(GLFWwindow* window, double xpos, double ypos);

//Utils
void LoadFile(const char* filename, char*& output);
GLuint loadTexture(const char* path);

//Program ID's
GLuint simpleProgram, skyBoxProgram;

const int WIDTH = 1280, HEIGHT = 720;

//world data
glm::vec3 lightDirection = glm::normalize(glm::vec3(-.5f, -0.5f, -0.5f));
glm::vec3 cameraPosition = glm::vec3(0, 2.5f, -5.0f);

glm::mat4 view;
glm::mat4 projection;

GLuint boxVAO, boxEBO;
int boxSize, boxIndexCount;

float lastX, lastY;
bool firstMouse = true;
float camYaw, camPitch;

int main()
{
    GLFWwindow* window;
    int result = Init(window);
    if (result != 0) return result;

    CreateGeometry(boxVAO, boxEBO, boxSize, boxIndexCount);
    CreateShaders();
    
    //Box textures
    GLuint boxTex = loadTexture("textures/container2.png");
    GLuint boxNormal = loadTexture("textures/container2normal.png");
    //Gradient tex for cell shading
    GLuint gradientTex = loadTexture("textures/GradientTexture2.png");

    //Set texture channels
    glUseProgram(simpleProgram);
    glUniform1i(glGetUniformLocation(simpleProgram, "mainTex"), 0);
    glUniform1i(glGetUniformLocation(simpleProgram, "normalTex"), 1);
    glUniform1i(glGetUniformLocation(simpleProgram, "gradientTex"), 2);

    glViewport(0, 0, WIDTH, HEIGHT);

    //Matrices

    //Update view matrix everytime camera moves
    //Update world matrix everytime objects are moved/changed/scaled

    /*glm::mat4 world = glm::mat4(1.0f);
    world = glm::rotate(world, glm::radians(45.0f), glm::vec3(0, 1, 0));
    world = glm::scale(world, glm::vec3(1, 1, 1));
    world = glm::translate(world, glm::vec3(0, 0, 0));*/

    view = glm::lookAt(glm::vec3(0, 2.5f, -5.0f), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    projection = glm::perspective(glm::radians(25.0f), WIDTH / (float)HEIGHT, 0.1f, 100.0f);

    while (!glfwWindowShouldClose(window))
    {
        //Input
        ProcessInput(window);

        //RenderBox(world, view, projection, boxTex, boxNormal, gradientTex, boxVAO, boxIndexCount);

        RenderSkyBox();

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

    glUseProgram(skyBoxProgram);

    //Matrices

    //Update view matrix everytime camera moves
    //Update world matrix everytime objects are moved/changed/scaled

    glm::mat4 world = glm::mat4(1.0f);
    world = glm::translate(world, glm::vec3(0, 0, 0));
    world = glm::scale(world, glm::vec3(10, 10, 10));

    glUniformMatrix4fv(glGetUniformLocation(skyBoxProgram, "world"), 1, GL_FALSE, glm::value_ptr(world));
    glUniformMatrix4fv(glGetUniformLocation(skyBoxProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(skyBoxProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(boxVAO);
    glDrawElements(GL_TRIANGLES, boxIndexCount, GL_UNSIGNED_INT, 0);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH);
}

void ProcessInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
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
    CreateProgram(skyBoxProgram, "shaders/skyboxVertex.shader", "shaders/skyboxFragment.shader");

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

GLuint loadTexture(const char* path)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, numChannels;
    unsigned char* data = stbi_load(path, &width, &height, &numChannels, 0);

    if(data)
    {
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

void RenderBox(glm::mat4 &world, glm::mat4 &view, glm::mat4 &projection, GLuint &boxTex, GLuint &boxNormal, GLuint &gradientTex, GLuint &triangleVAO, int triangleIndexCount)
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUniformMatrix4fv(glGetUniformLocation(simpleProgram, "world"), 1, GL_FALSE, glm::value_ptr(world));
    glUniformMatrix4fv(glGetUniformLocation(simpleProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(simpleProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glUniform3fv(glGetUniformLocation(simpleProgram, "lightPosition"), 1, glm::value_ptr(lightDirection));
    glUniform3fv(glGetUniformLocation(simpleProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, boxTex);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, boxNormal);

    //For cellshading
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gradientTex);

    glBindVertexArray(triangleVAO);
    //glBindVertexArray(triangleEBO);
    //glDrawArrays(GL_TRIANGLES, 0, triangleSize);
    glDrawElements(GL_TRIANGLES, triangleIndexCount, GL_UNSIGNED_INT, 0);
}

void Mouse_Callback(GLFWwindow* window, double xpos, double ypos)
{
    float x = (float)xpos;
    float y = (float)ypos;

    if(firstMouse)
    {
        lastX = x;
        lastY = y;
    }

    float dx = x - lastX;
    float dy = y - lastY;
    lastX = x;
    lastY = y;

    camYaw += dx;
    camPitch = glm::clamp(camPitch + dy, -89.0f, 89.0f);

    if(camYaw > 180.0f)
    {
        camYaw -= 360.0f;
    }

    if(camYaw < -180.0f)
    {
        camYaw += 360.0f;
    }

    glm::quat camQuat = glm::quat(glm::vec3(glm::radians(camPitch), glm::radians(camYaw), 0));

    glm::vec3 camForward = glm::vec3(0, 0, 1);
    glm::vec3 camUp = glm::vec3(0, 1, 0);
    view = glm::lookAt(cameraPosition, cameraPosition + camForward, camUp);
}