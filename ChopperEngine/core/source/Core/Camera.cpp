#include "Camera.h"

Camera::Camera()
{
}

void Camera::setupFrustum(float left, float right, float bottom, float top, float znear, float zfar)
{
    proj_ = glm::frustum(left, right, bottom, top, znear, zfar);
    znear_ = znear;
    zfar_ = zfar;
}

void Camera::setupPerspective(float fovy, float aspect, float znear, float zfar)
{
    fovy_ = fovy;
    aspect_ = aspect;
    znear_ = znear;
    zfar_ = zfar;

    proj_ = glm::perspective(fovy_, aspect_, znear_, zfar_);
    proj_[1][1] *= -1;
}

void Camera::setupOrtho(float left, float right, float bottom, float top, float znear, float zfar)
{
    proj_ = glm::ortho(left, right, bottom, top, znear, zfar);
    proj_[1][1] *= -1; // Vulkan fix
    znear_ = znear;
    zfar_ = zfar;
}

void Camera::update(const double dt)
{
    if (nullptr == window_) return;
    if (!enabled_) return;

    // Check mouse state
    static bool wasMouseDown = false;
    bool isMouseDown = glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    if (!isMouseDown)
    {
        wasMouseDown = false;
        return;
    }

    double xpos, ypos;
    glfwGetCursorPos(window_, &xpos, &ypos);
    if (!wasMouseDown)
    {
        last_mouse_pos_ = {xpos, ypos};
        wasMouseDown = true;
        return;
    }

    if (xpos > width_)
    {
        glfwSetCursorPos(window_, 1, ypos);
        glfwGetCursorPos(window_, &xpos, &ypos);
        last_mouse_pos_ = {xpos, ypos};
    }
    if (xpos < 0)
    {
        glfwSetCursorPos(window_, width_, ypos);
        glfwGetCursorPos(window_, &xpos, &ypos);
        last_mouse_pos_ = {xpos, ypos};
    }

    if (ypos > height_)
    {
        glfwSetCursorPos(window_, xpos, 1);
        glfwGetCursorPos(window_, &xpos, &ypos);
        last_mouse_pos_ = {xpos, ypos};
    }
    if (ypos < 0)
    {
        glfwSetCursorPos(window_, xpos, height_);
        glfwGetCursorPos(window_, &xpos, &ypos);
        last_mouse_pos_ = {xpos, ypos};
    }

    // --- FlyCam Movement ---
    float velocity = speed_ * static_cast<float>(dt);
    glm::vec3 right = glm::normalize(glm::cross(dir_, up_));

    if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS)
        pos_ += dir_ * velocity;
    if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS)
        pos_ -= dir_ * velocity;
    if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS)
        pos_ -= right * velocity;
    if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS)
        pos_ += right * velocity;
    if (glfwGetKey(window_, GLFW_KEY_E) == GLFW_PRESS)
        pos_ += up_ * velocity;
    if (glfwGetKey(window_, GLFW_KEY_Q) == GLFW_PRESS)
        pos_ -= up_ * velocity;

    // --- Mouse Look ---

    glfwGetCursorPos(window_, &xpos, &ypos);

    float xoffset = static_cast<float>(xpos - last_mouse_pos_.x);
    float yoffset = static_cast<float>(last_mouse_pos_.y - ypos); // reversed Y
    last_mouse_pos_ = {xpos, ypos};

    xoffset *= sensitivity_;
    yoffset *= sensitivity_;

    yaw_ += xoffset;
    pitch_ += yoffset;

    // Clamp pitch
    if (pitch_ > 89.0f) pitch_ = 89.0f;
    if (pitch_ < -89.0f) pitch_ = -89.0f;

    // Update direction vector
    glm::vec3 front;
    front.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    front.y = sin(glm::radians(pitch_));
    front.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    dir_ = glm::normalize(front);

    // Update view matrix
    view_ = glm::lookAt(pos_, pos_ + dir_, up_);
}


void Camera::setResolution(float width, float height)
{
    width_ = width;
    height_ = height;
    aspect_ = width_ / height_;
}

void Camera::setPosition(const float pos[3])
{
    pos_ = {pos[0], pos[1], pos[2]};
}

void Camera::setViewDirection(const float dir[3])
{
    dir_ = {dir[0], dir[1], dir[2]};
}

void Camera::setSpeed(const float speed)
{
    speed_ = glm::max(speed, 0.0f);
}

void Camera::setSensitibity(const float sensitivity)
{
    sensitivity_ = glm::max(sensitivity, 0.0f);
}

void Camera::setEnabled(const bool enabled)
{
    enabled_ = enabled;
}

glm::vec3 Camera::getPosition()
{
    return pos_;
}

glm::vec3 Camera::getDirection()
{
    return dir_;
}

void Camera::init(GLFWwindow* window, float const width, float const height)
{
    window_ = window;
    setResolution(width, height);
    if (is_perspective)setupPerspective(fovy_, aspect_, znear_, zfar_);
    // Update direction vector
    glm::vec3 front;
    front.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    front.y = sin(glm::radians(pitch_));
    front.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    dir_ = glm::normalize(front);

    // Update view matrix
    view_ = glm::lookAt(pos_, pos_ + dir_, up_);
}

float Camera::getFov() const
{
    return glm::degrees(fovy_);
}

void Camera::setFov(float const fov)
{
    fovy_ = glm::radians(fov);
}

void Camera::setPerspective(bool type)
{
    is_perspective = type;
    if (is_perspective)
    {
        setupPerspective(fovy_, aspect_, znear_, zfar_);
    }
}

glm::mat4 Camera::getView()
{
    return view_;
}

glm::mat4 Camera::getProj()
{
    return proj_;
}
