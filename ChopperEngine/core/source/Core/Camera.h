#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <imgui/imgui.h>
#include "GLFW/glfw3.h"

class Camera
{
    
public:
    Camera();

    bool enabled_ = true;
    float speed_ = 2.5f;          // movement speed units/second
    float sensitivity_ = 0.1f;    // mouse sensitivity
    
    void init(GLFWwindow* window, float width, float height);
    
    void update(const double dt);

    void imGui()
    {
        ImGui::Text("Camera Pos"); 
    }
    
    void setupPerspective(float fovy, float aspect, float znear, float zfar);

    void setupOrtho(
        float left, float right,
        float bottom, float top,
        float znear, float zfar);

    void setupFrustum(
        float left, float right,
        float bottom, float top,
        float znear, float zfar);

    void setResolution(float width, float height);
    void setPosition(const float pos[3]);
    void setViewDirection(const float dir[3]);

    void setSpeed(const float speed);
    void setSensitibity(const float sensitivity);
    void setEnabled(const bool enabled);

    glm::vec3 getPosition();
    glm::vec3 getDirection();
    float getFov() const;
    void setFov(float fov);
    // type true = perspective, false = ortho
    void setPerspective(bool type);

    glm::mat4 getView();
    glm::mat4 getProj();

private:
    
    GLFWwindow* window_ = nullptr;

    float width_ = 1920.0f;
    float height_ = 1080.0f;

    glm::vec3 pos_ = {0.0f, 1.0f, 3.0f};   // start slightly back
    glm::vec3 dir_ = {0.0f, 0.0f, -1.0f};  // looking forward
    glm::vec3 up_  = {0.0f, 1.0f, 0.0f};   // world up

    glm::mat4 view_{};
    glm::mat4 proj_{};

    float fovy_   = glm::radians(45.0f);
    float aspect_ = 1920.0f / 1080.0f;
    float znear_  = 0.1f;
    float zfar_   = 100.0f;

    glm::vec2 mouse_pos_       = {0.0f, 0.0f};
    glm::vec2 last_mouse_pos_  = {width_ / 2.0f, height_ / 2.0f};
    glm::vec2 accum_mouse_offset_ = {0.0f, 0.0f};

    // yaw/pitch angles for free-fly cam
    float yaw_   = -90.0f; // facing -Z
    float pitch_ = 0.0f;

    bool is_perspective = true;
    
};
