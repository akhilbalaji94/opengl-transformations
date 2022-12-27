// Defined before OpenGL and GLUT includes to avoid deprecation message in OSX
#define GL_SILENCE_DEPRECATION
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "cyCodeBase/cyTriMesh.h"
#include "cyCodeBase/cyMatrix.h"
#include "cyCodeBase/cyGL.h"

int width = 1280, height = 720;
GLFWwindow* window;
cy::TriMesh teapotMesh;
cy::GLSLProgram prog;
bool doRotate = false, doZoom = false;
double rotX = 5.23599, rotY = 0, distZ = 40;
double lastX, lastY;

GLclampf Red = 0.0f, Green = 0.0f, Blue = 0.0f, Alpha = 1.0f;

void renderScene()
{
    // Clear Color buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(Red, Green, Blue, Alpha);
    glDrawArrays(GL_POINTS, 0, teapotMesh.NV());
}

bool compileShaders() {
    bool shaderSuccess = prog.BuildFiles("shader.vert", "shader.frag");
    return shaderSuccess;
}

void processNormalKeyCB(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    else if (key == GLFW_KEY_F6 && action == GLFW_PRESS) {
        std::cout << "Recompiling shaders..." << std::endl;
        bool shaderSuccess = compileShaders();
        if (!shaderSuccess) {
            std::cout << "Error Recompiling shaders" << std::endl;
        }
    }
}

static void error_callback(int error, const char* description)
{
    std::cerr << "Error: " << description << std::endl;
}
double deg2rad (double degrees) {
    return degrees * 4.0 * atan (1.0) / 180.0;
}

void processMouseButtonCB(GLFWwindow* window, int button, int action, int mods)
{
    double xpos, ypos;
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        doZoom = true;
        glfwGetCursorPos(window, &xpos, &ypos);
        lastX = xpos;
        lastY = ypos;
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
        doZoom = false;
        glfwGetCursorPos(window, &xpos, &ypos);
        lastX = xpos;
        lastY = ypos;
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        doRotate = true;
        glfwGetCursorPos(window, &xpos, &ypos);
        lastX = xpos;
        lastY = ypos;
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        doRotate = false;
        glfwGetCursorPos(window, &xpos, &ypos);
        lastX = xpos;
        lastY = ypos;
    }
}

void processMousePosCB(GLFWwindow* window, double xpos, double ypos)
{
    double xDiff = lastX - xpos;
    double yDiff = lastY - ypos;
    // Calculate camera zoom based on mouse movement in y direction
    if (doZoom) {
        distZ += yDiff * 0.05;
    }

    // Calculate camera rotation based on mouse movement in x direction
    if (doRotate) {
        rotX -= yDiff * 0.005;
        rotY -= xDiff * 0.005;
    }

    lastX  = xpos;
    lastY = ypos;
}

int main(int argc, char** argv)
{
    glfwSetErrorCallback(error_callback);
    
    // Initialize GLFW
    if (!glfwInit())
        exit(EXIT_FAILURE);
        
    // Create a windowed mode window and its OpenGL context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(width, height, "Transformations", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);

    //Initialize GLEW
    glewExperimental = GL_TRUE; 
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        std::cout << "Error: " << glewGetErrorString(err) << std::endl;
        return 1;
    }

    // Print and test OpenGL context infos
    std::cout << glGetString(GL_VERSION) << std::endl;
    std::cout << glGetString(GL_RENDERER) << std::endl;

    // Setup GLFW callbacks
    glfwSetKeyCallback(window, processNormalKeyCB);
    glfwSetMouseButtonCallback(window, processMouseButtonCB);
    glfwSetCursorPosCallback(window, processMousePosCB);
    glfwSwapInterval(1);

    //OpenGL initializations
    glViewport(0, 0, width, height);
    // CY_GL_REGISTER_DEBUG_CALLBACK;

    //Load object mesh
    bool meshSuccess = teapotMesh.LoadFromFileObj(argv[1]);
    if (!meshSuccess)
    {
        std::cout << "Error loading mesh" << std::endl;
        return 1;
    }

    //Compute bounding box
    teapotMesh.ComputeBoundingBox();
    cy::Vec3f teapotCenter = (teapotMesh.GetBoundMax() - teapotMesh.GetBoundMin())/2;
    cy::Vec3f teapotOrigin = teapotMesh.GetBoundMin() + teapotCenter;


    //Create vertex array object
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    //Create vertex buffer object
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec3f)*teapotMesh.NV(), &teapotMesh.V(0), GL_STATIC_DRAW);
    
    //Setup Shader program
    std::cout << "Compiling shaders..." << std::endl;
    bool shaderSuccess = compileShaders();
    if (!shaderSuccess)
    {
        std::cout << "Error compiling shaders" << std::endl;
        return 1;
    }

    //Loop until the user closes the window
    while (!glfwWindowShouldClose(window))
    {
        // Calculate camera transformations
        cy::Matrix3f rotXMatrix = cy::Matrix3f::RotationX(rotX);
        cy::Matrix3f rotYMatrix = cy::Matrix3f::RotationY(rotY);
        cy::Matrix4f transMatrix = cy::Matrix4f::Translation(-teapotOrigin);
        cy::Matrix4f view = cy::Matrix4f::View(cy::Vec3f(0, 0, distZ), cy::Vec3f(0, 0, 0), cy::Vec3f(0, 1, 0));
        cy::Matrix4f projMatrix = cy::Matrix4f::Perspective(deg2rad(60), float(width)/float(height), 0.1f, 1000.0f);
        cy::Matrix4f mvp = projMatrix * view * rotXMatrix * rotYMatrix * transMatrix;
        
        //Set Program and Program Attributes
        prog.Bind();
        prog["mvp"] = mvp;
        GLuint pos_location = glGetAttribLocation(prog.GetID(), "pos");
        glEnableVertexAttribArray(pos_location);
        glVertexAttribPointer(pos_location, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

        renderScene();

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}