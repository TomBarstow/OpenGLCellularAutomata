#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <random>

// Global config
bool limitFramerate = true;         //Controls initial state of FPS limiter. User Control with 'F' key.
const double TARGET_FPS = 30.0;
const double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;

const int WINDOW_SIZE = 1024;       //Window size (square)
const int GRID_SIZE = 1024;          //Simulation size (square)

class CellularAutomata {
private:
    GLuint computeProgram = 0, renderProgram = 0;
    GLuint textureA = 0, textureB = 0;
    GLuint VAO = 0, VBO = 0;
    int width, height;
    bool useTextureA = true;

    float quadVertices[24] = {
        -1.0f,  1.0f, 0.0f, 1.0f,  -1.0f, -1.0f, 0.0f, 0.0f,  1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,   1.0f, -1.0f, 1.0f, 0.0f,  1.0f,  1.0f, 1.0f, 1.0f
    };

public:
    CellularAutomata(int w, int h) : width(w), height(h) {}

    bool initialize() {
        return createComputeShader() && createRenderShader() && (createTextures(), initializeGrid(), setupQuad(), true);
    }

    void step() {
        
        static int frameCount = 0;  // Add frame counter
        
        glUseProgram(computeProgram);

        GLuint inputTex = useTextureA ? textureA : textureB;
        GLuint outputTex = useTextureA ? textureB : textureA;

        //GL_RGBA8UI implementation for storing 4 channel, 8bit value per cell
        glBindImageTexture(0, inputTex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8UI);
        glBindImageTexture(1, outputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8UI);
        //GL_R32UI implementation for storing one 32bit value per cell
        //glBindImageTexture(0, inputTex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32UI);
        //glBindImageTexture(1, outputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32UI);

        glDispatchCompute((width + 15) / 16, (height + 15) / 16, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        useTextureA = !useTextureA;

        frameCount++;  // Increment frame counter
    }

    void render() {
        //glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(renderProgram);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, useTextureA ? textureA : textureB);
        glUniform1i(glGetUniformLocation(renderProgram, "cellTexture"), 0);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    void cleanup() {
        GLuint objects[] = { computeProgram, renderProgram, textureA, textureB, VAO, VBO };
        if (computeProgram) glDeleteProgram(computeProgram);
        if (renderProgram) glDeleteProgram(renderProgram);
        if (textureA) glDeleteTextures(1, &textureA);
        if (textureB) glDeleteTextures(1, &textureB);
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
    }

private:
    std::string loadShader(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open: " << filepath << std::endl;
            return "";
        }
        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

    bool compileShader(GLuint shader) {
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char log[512];
            glGetShaderInfoLog(shader, 512, NULL, log);
            std::cerr << "Shader compilation failed: " << log << std::endl;
        }
        return success;
    }

    bool linkProgram(GLuint program) {
        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            char log[512];
            glGetProgramInfoLog(program, 512, NULL, log);
            std::cerr << "Program linking failed: " << log << std::endl;
        }
        return success;
    }

    bool createComputeShader() {
        std::string source = loadShader("shaders/cellular.comp");
        if (source.empty()) return false;

        const char* src = source.c_str();
        GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(shader, 1, &src, NULL);
        glCompileShader(shader);

        if (!compileShader(shader)) return false;

        computeProgram = glCreateProgram();
        glAttachShader(computeProgram, shader);
        glLinkProgram(computeProgram);
        glDeleteShader(shader);

        return linkProgram(computeProgram);
    }

    bool createRenderShader() {
        std::string vertSource = loadShader("shaders/screen.vert");
        std::string fragSource = loadShader("shaders/screen.frag");
        if (vertSource.empty() || fragSource.empty()) return false;

        const char* vertSrc = vertSource.c_str();
        const char* fragSrc = fragSource.c_str();

        GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
        GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);

        glShaderSource(vertShader, 1, &vertSrc, NULL);
        glShaderSource(fragShader, 1, &fragSrc, NULL);
        glCompileShader(vertShader);
        glCompileShader(fragShader);

        if (!compileShader(vertShader) || !compileShader(fragShader)) return false;

        renderProgram = glCreateProgram();
        glAttachShader(renderProgram, vertShader);
        glAttachShader(renderProgram, fragShader);
        glLinkProgram(renderProgram);

        glDeleteShader(vertShader);
        glDeleteShader(fragShader);

        return linkProgram(renderProgram);
    }

    void createTextures() {
        glGenTextures(1, &textureA);
        glGenTextures(1, &textureB);

        for (GLuint tex : {textureA, textureB}) {
            glBindTexture(GL_TEXTURE_2D, tex);
            //GL_RGBA8UI implementation for storing 4 channel, 8bit value per cell
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8UI, width, height, 0, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, NULL);
            //GL_R32UI implementation for storing one 32bit value per cell
            //glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, width, height, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
    }

    void initializeGrid() {
        std::vector<uint8_t> data(width * height * 4);  // uint8_t for 8-bit channels
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);

        for (int i = 0; i < width * height; i++) {
            bool isAlive = dis(gen) < 0.3;
            data[i * 4 + 0] = isAlive ? 255 : 0;      // R channel
            data[i * 4 + 1] = isAlive ? 255 : 0;      // G channel
            data[i * 4 + 2] = isAlive ? 255 : 0;      // B channel 
            data[i * 4 + 3] = isAlive ? 255 : 0;      // A channel
        }

        glBindTexture(GL_TEXTURE_2D, textureA);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, data.data());
    }

    void setupQuad() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }
};

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //const int WINDOW_SIZE = 1024;
    //const int GRID_SIZE = 2048;

    GLFWwindow* window = glfwCreateWindow(WINDOW_SIZE, WINDOW_SIZE, "Cellular Automata", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << " | GPU: " << glGetString(GL_RENDERER) << std::endl;

    CellularAutomata ca(GRID_SIZE, GRID_SIZE);
    if (!ca.initialize()) return -1;

    glViewport(0, 0, WINDOW_SIZE, WINDOW_SIZE);

    double lastTime = glfwGetTime(), lastFrameTime = glfwGetTime();
    int frameCount = 0;
    bool fKeyPressed = false;

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;

        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fKeyPressed) {
            limitFramerate = !limitFramerate;
            std::cout << "Framerate: " << (limitFramerate ? "Limited" : "Unlimited") << std::endl;
            fKeyPressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) fKeyPressed = false;

        if (!limitFramerate || currentTime - lastFrameTime >= TARGET_FRAME_TIME) {
            frameCount++;
            if (currentTime - lastTime >= 1.0) {
                std::cout << "FPS: " << frameCount << (limitFramerate ? " (Limited)" : " (Unlimited)") << std::endl;
                frameCount = 0;
                lastTime = currentTime;
            }

            ca.step();
            ca.render();
            glfwSwapBuffers(window);
            lastFrameTime = currentTime;
        }
    }

    ca.cleanup();
    glfwTerminate();
    return 0;
}