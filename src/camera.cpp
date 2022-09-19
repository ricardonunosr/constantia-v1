#include "camera.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera()
{
    m_camera_pos = glm::vec3(-25.0f, 3.0f, 0.0f);
    m_camera_front = glm::vec3(1.0f, 0.0f, 0.0f);
    m_camera_up = glm::vec3(0.0f, 1.0f, 0.0f);
    m_projection_matrix = glm::perspective(glm::radians(45.0f), (float)1280 / (float)720, 0.1f, 100.0f);
    m_view_matrix = glm::lookAt(m_camera_pos, m_camera_pos + m_camera_front, m_camera_up);
}

void Camera::process_input(GLFWwindow* window, float delta_time)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        m_editor = false;
        m_enabled = true;
    }

    if (glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        m_editor = true;
        m_enabled = false;
    }

    float camera_speed = 3.5f * delta_time; // adjust accordingly
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        m_camera_pos += camera_speed * m_camera_front;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        m_camera_pos -= camera_speed * m_camera_front;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        m_camera_pos -= glm::normalize(glm::cross(m_camera_front, m_camera_up)) * camera_speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        m_camera_pos += glm::normalize(glm::cross(m_camera_front, m_camera_up)) * camera_speed;
    if (glfwGetKey(window, GLFW_KEY_Q) != 0)
        m_camera_pos += camera_speed * m_camera_up;
    if (glfwGetKey(window, GLFW_KEY_E) != 0)
        m_camera_pos -= camera_speed * m_camera_up;

    m_view_matrix = glm::lookAt(m_camera_pos, m_camera_pos + m_camera_front, m_camera_up);
}

void Camera::move_camera(double xpos, double ypos)
{
    if (!m_enabled)
        return;
    if (m_first_mouse)
    {
        m_last_x = xpos;
        m_last_y = ypos;
        m_first_mouse = false;
    }

    float xoffset = xpos - m_last_x;
    float yoffset = m_last_y - ypos;
    m_last_x = xpos;
    m_last_y = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    m_yaw += xoffset;
    m_pitch += yoffset;

    if (m_pitch > 89.0f)
        m_pitch = 89.0f;
    if (m_pitch < -89.0f)
        m_pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    direction.y = sin(glm::radians(m_pitch));
    direction.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_camera_front = glm::normalize(direction);
}

void Camera::zoom(double /*xoffset*/, double yoffset)
{
    if (!m_enabled)
        return;
    m_fov -= (float)yoffset;
    if (m_fov < 1.0f)
        m_fov = 1.0f;
    if (m_fov > 45.0f)
        m_fov = 45.0f;
}
