#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SOIL.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "shader_s.h"
#include "camera.h"

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
Camera camera(glm::vec3(0.0f, 1.0f, 5.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

float floorVertices[] = {
    -50.0f, 0.0f, -50.0f, 0.0f, 50.0f,
    50.0f, 0.0f, -50.0f, 50.0f, 50.0f,
    50.0f, 0.0f, 50.0f, 50.0f, 0.0f,
    -50.0f, 0.0f, 50.0f, 0.0f, 0.0f
};

unsigned int floorIndices[] = { 0, 1, 2, 2, 3, 0 };

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

GLuint loadTexture(const char* path, bool gammaCorrection) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    int width, height, channels;
    unsigned char* data = SOIL_load_image(path, &width, &height, &channels, 0);
    if (data) {
        GLenum internalFormat = GL_RGB;
        GLenum dataFormat = GL_RGB;
        if (channels == 4) {
            internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
            dataFormat = GL_RGBA;
        }
        else if (channels == 3) {
            internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
            dataFormat = GL_RGB;
        }
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        if (glewIsSupported("GL_EXT_texture_filter_anisotropic")) {
            float maxAniso;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);
        }
    }
    else {
        std::cerr << "Failed to load texture: " << path << std::endl;
    }
    SOIL_free_image_data(data);
    return textureID;
}

GLuint loadModel(const std::string& path, int& vertexCount) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);
    if (!scene || !scene->mNumMeshes) {
        std::cerr << "Model error: " << importer.GetErrorString() << std::endl;
        vertexCount = 0;
        return 0;
    }
    aiMesh* mesh = scene->mMeshes[0];
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        vertices.push_back(mesh->mVertices[i].x);
        vertices.push_back(mesh->mVertices[i].y);
        vertices.push_back(mesh->mVertices[i].z);
        if (mesh->mTextureCoords[0]) {
            vertices.push_back(mesh->mTextureCoords[0][i].x);
            vertices.push_back(mesh->mTextureCoords[0][i].y);
        }
        else {
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }
    }
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    vertexCount = indices.size();
    return VAO;
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3D Goat", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glewExperimental = GL_TRUE;
    glewInit();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glEnable(GL_FRAMEBUFFER_SRGB);

    Shader snailShader("floor.vs", "floor.fs");
    //Shader snailShader("snail.vs", "snail.fs");
    Shader floorShader("floor.vs", "floor.fs");

    int snailVertexCount;
    //GLuint snailVAO = loadModel("Goat-bl.obj", snailVertexCount);
    GLuint snailVAO = loadModel("coffee.obj", snailVertexCount);
    GLuint snailTexture = loadTexture("textures/coffee3.png", true);
    //GLuint snailTexture = loadTexture("textures/UVGoat.png", true);

    unsigned int floorVAO, floorVBO, floorEBO;
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glGenBuffers(1, &floorEBO);
    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(floorIndices), floorIndices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    //GLuint floorTexture = loadTexture("textures/grass.png", true);
    GLuint floorTexture = loadTexture("textures/tile-2.png", true);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput(window);
        glClearColor(0.1f, 0.4f, 0.6f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
            (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();


        floorShader.use();
        floorShader.setMat4("projection", projection);
        floorShader.setMat4("view", view);
        floorShader.setMat4("model", glm::mat4(1.0f));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        glBindVertexArray(floorVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        snailShader.use();
        snailShader.setMat4("projection", projection);
        snailShader.setMat4("view", view);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f));
        snailShader.setMat4("model", model);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, snailTexture);
        snailShader.setInt("texture1", 0);
        glBindVertexArray(snailVAO);
        glDrawElements(GL_TRIANGLES, snailVertexCount, GL_UNSIGNED_INT, 0);
  
        //snailShader.setVec3("lightPos", glm::vec3(8.0f, 8.0f, 2.0f)); - для прожектора
        snailShader.setVec3("lightPos", glm::vec3(0.0f, 4.0f, 1.0f));
        snailShader.setVec3("viewPos", camera.Position);
        snailShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));

        glm::vec3 targetPoint(0.0f, 0.5f, 0.5f); // Точка, куда светим
        glm::vec3 lightDir = glm::normalize(targetPoint - glm::vec3(2.0f, 4.0f, 2.0f));
        snailShader.setVec3("spotlightDirection", lightDir);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &snailVAO);
    glDeleteVertexArrays(1, &floorVAO);
    glfwTerminate();
    return 0;
}