///////////////////////////////////////////////////////////////////////////////
// viewmanager.cpp
// ============
// manage the viewing of 3D objects within the viewport - camera, projection
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//  Modified for CS-330 Final Project
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>    

// declaration of the global variables and defines
namespace
{
    // Variables for window width and height
    const int WINDOW_WIDTH = 1000;
    const int WINDOW_HEIGHT = 800;
    const char* g_ViewName = "view";
    const char* g_ProjectionName = "projection";

    // camera object used for viewing and interacting with the 3D scene
    Camera* g_pCamera = nullptr;

    // these variables are used for mouse movement processing
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // time between current frame and last frame
    float gDeltaTime = 0.0f;
    float gLastFrame = 0.0f;
}

///////////////////////////////////////////////////////////////////////////////
 //  ViewManager()
 // 
 //  The constructor for the class
 ///////////////////////////////////////////////////////////////////////////////
ViewManager::ViewManager(ShaderManager* pShaderManager)
{
    // initialize the member variables
    m_pShaderManager = pShaderManager;
    m_pWindow = NULL;
    g_pCamera = new Camera();
    // default camera view parameters (perspective mode)
    g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
    g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
    g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
    g_pCamera->Zoom = 80;
    g_pCamera->MovementSpeed = 20;

    // orthographic mode is off by default
    m_bIsOrthographic = false;
}

///////////////////////////////////////////////////////////////////////////////
//  ~ViewManager()
//
//  The destructor for the class
///////////////////////////////////////////////////////////////////////////////
ViewManager::~ViewManager()
{
    // free up allocated memory
    m_pShaderManager = NULL;
    m_pWindow = NULL;
    if (NULL != g_pCamera)
    {
        delete g_pCamera;
        g_pCamera = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
 //  CreateDisplayWindow()
 //
 //  This method is used to create the main display window.
///////////////////////////////////////////////////////////////////////////////
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
    GLFWwindow* window = nullptr;

    // try to create the displayed OpenGL window
    window = glfwCreateWindow(
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        windowTitle,
        NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return NULL;
    }
    glfwMakeContextCurrent(window);

    // capture all mouse events
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // receive mouse moving events
    glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);
    // receive mouse scroll events
    glfwSetScrollCallback(window, &ViewManager::Scroll_Callback);

    // enable blending for supporting transparent rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_pWindow = window;

    return(window);
}

///////////////////////////////////////////////////////////////////////////////
//  Mouse_Position_Callback()
//
//  This method is called from GLFW whenever
//  the mouse is moved within the active GLFW display window.
///////////////////////////////////////////////////////////////////////////////
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
    // when the first mouse move event is received, this needs to be recorded so that
    // all subsequent mouse moves can correctly calculate the X position offset and Y
    // position offset for proper operation
    if (gFirstMouse)
    {
        gLastX = xMousePos;
        gLastY = yMousePos;
        gFirstMouse = false;
    }

    // calculate the X offset and Y offset values for moving the 3D camera accordingly
    float xOffset = xMousePos - gLastX;
    float yOffset = gLastY - yMousePos; // reversed since y-coordinates go from bottom to top

    // set the current positions into the last position variables
    gLastX = xMousePos;
    gLastY = yMousePos;

    // move the 3D camera according to the calculated offsets
    g_pCamera->ProcessMouseMovement(xOffset, yOffset);
}

///////////////////////////////////////////////////////////////////////////////
//  Scroll_Callback()
//
//   mouse wheel is scrolling
//  It adjusts the camera movement speed (clamped between 2 and 50 for now).
///////////////////////////////////////////////////////////////////////////////
void ViewManager::Scroll_Callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // yoffset: positive = scroll up, negative = scroll down
    float speedChange = static_cast<float>(yoffset) * 2.0f;
    float newSpeed = g_pCamera->MovementSpeed + speedChange;

    // clamp movement speed to reasonable range
    if (newSpeed < 2.0f)
        newSpeed = 2.0f;
    if (newSpeed > 50.0f)
        newSpeed = 50.0f;

    g_pCamera->MovementSpeed = newSpeed;
}

///////////////////////////////////////////////////////////////////////////////
//  ProcessKeyboardEvents()
//
//  process any keyboard events
//  that may be waiting in the event queue.
///////////////////////////////////////////////////////////////////////////////
void ViewManager::ProcessKeyboardEvents()
{
    // close the window if the escape key has been pressed
    if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(m_pWindow, true);
    }

    // WASD for forward/back/left/right
    if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
    {
        g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);
    }
    if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
    {
        g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);
    }
    if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
    {
        g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);
    }
    if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
    {
        g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);
    }

    // Q and E keys for vertical movement
    if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
    {
        // move camera up
        g_pCamera->Position.y += g_pCamera->MovementSpeed * gDeltaTime;
    }
    if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
    {
        // move camera down
        g_pCamera->Position.y -= g_pCamera->MovementSpeed * gDeltaTime;
    }

    // toggle between Orthographic and Perspective
    // O key: switch to Orthographic mode (front view)
    if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS && !m_bIsOrthographic)
    {
        m_bIsOrthographic = true;

        // store current camera state for later
        m_prevCameraPos = g_pCamera->Position;
        m_prevCameraFront = g_pCamera->Front;
        m_prevCameraUp = g_pCamera->Up;
        m_prevCameraZoom = g_pCamera->Zoom;

        // set camera to a fixed front view so that the bottom plane is not visible.
        // the plane (y=0) will be seen edge-on or from above, hiding its underside.
        g_pCamera->Position = glm::vec3(0.0f, 5.0f, 15.0f);
        g_pCamera->Front = glm::vec3(0.0f, 0.0f, -1.0f);
        g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
        // reset zoom
        g_pCamera->Zoom = 45.0f;

        // print status
        std::cout << "Switched to ORTHOGRAPHIC projection (front view)" << std::endl;
    }

    // P key: switch back to perspective mode
    if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS && m_bIsOrthographic)
    {
        m_bIsOrthographic = false;

        // Restore the camera state from before
        g_pCamera->Position = m_prevCameraPos;
        g_pCamera->Front = m_prevCameraFront;
        g_pCamera->Up = m_prevCameraUp;
        g_pCamera->Zoom = m_prevCameraZoom;

        std::cout << "Switched to PERSPECTIVE projection" << std::endl;
    }
}

///////////////////////////////////////////////////////////////////////////////
//  PrepareSceneView()
//
// This method is used for preparing the 3D scene by loading
//  the shapes, textures in memory to support the 3D scene
//  rendering
///////////////////////////////////////////////////////////////////////////////
void ViewManager::PrepareSceneView()
{
    glm::mat4 view;
    glm::mat4 projection;

    // per-frame timing
    float currentFrame = glfwGetTime();
    gDeltaTime = currentFrame - gLastFrame;
    gLastFrame = currentFrame;

    // process any keyboard events that may be waiting in the 
    // event queue
    ProcessKeyboardEvents();

    // get the current view matrix from the camera
    view = g_pCamera->GetViewMatrix();

    // (perspective or orthographic)
    if (m_bIsOrthographic)
    {
        // Orthographic projection: front view, plane's bottom not visible
        float aspect = (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT;
        float orthoSize = 8.0f;   // Controls how much of the scene is visible
        float left = -orthoSize * aspect;
        float right = orthoSize * aspect;
        float bottom = -orthoSize;
        float top = orthoSize;
        projection = glm::ortho(left, right, bottom, top, 0.1f, 100.0f);
    }
    else
    {
        // Perspective projection (default)
        projection = glm::perspective(glm::radians(g_pCamera->Zoom),
            (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }

    // if the shader manager object is valid
    if (NULL != m_pShaderManager)
    {
        // set the view matrix into the shader for proper rendering
        m_pShaderManager->setMat4Value(g_ViewName, view);
        // set the projection matrix into the shader for proper rendering
        m_pShaderManager->setMat4Value(g_ProjectionName, projection);
        // set the view position of the camera into the shader for proper lighting
        m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
    }
}