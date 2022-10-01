#include "framebuffers.h"
#include <glad/gl.h>
#include <spdlog/spdlog.h>

#include "application.h"
#include "camera.h"
#include "model.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define WIDTH 1280
#define HEIGHT 720

std::unique_ptr<Camera> FrameBuffersLayer::m_camera = nullptr;

FrameBuffersLayer::FrameBuffersLayer(const std::string& name) : Layer(name)
{
    std::string base_path_assets = "./data/";
    m_cube = std::make_unique<Model>(base_path_assets + "cube/cube.obj");
    m_light = std::make_unique<Model>(base_path_assets + "cube/cube.obj");
    m_outline_shader =
        std::make_unique<Shader>(base_path_assets + "shaders/light.vert", base_path_assets + "shaders/outline.frag");
    m_post_processing_shader = std::make_unique<Shader>(base_path_assets + "shaders/postprocessing.vert",
                                                        base_path_assets + "shaders/postprocessing.frag");
    m_shader =
        std::make_unique<Shader>(base_path_assets + "shaders/basic.vert", base_path_assets + "shaders/basic.frag");
    m_light_shader =
        std::make_unique<Shader>(base_path_assets + "shaders/light.vert", base_path_assets + "shaders/light.frag");

    m_camera = std::make_unique<Camera>();

    // TODO: find a better way to define this
    glfwSetCursorPosCallback(Application::get().get_window(), mouse_callback);
    glfwSetMouseButtonCallback(Application::get().get_window(), mouse_button_callback);

    glGenFramebuffers(1, &m_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

    // generate texture
    glGenTextures(1, &m_texture_colorbuffer);
    glBindTexture(GL_TEXTURE_2D, m_texture_colorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1280, 720, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // attach it to currently bound framebuffer object
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture_colorbuffer, 0);

    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1280, 720);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        spdlog::error("ERROR::FRAMEBUFFER:: Framebuffer is not complete!");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    float quad_vertices[] = {// positions   // texCoords
                             -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,
                             -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f};

    m_vao = std::make_unique<VertexArray>();
    VertexBufferLayout layout = {{"aPos", DataType::Float2}, {"aTexCoords", DataType::Float2}};
    std::unique_ptr<VertexBuffer> vbo =
        std::make_unique<VertexBuffer>(layout, &quad_vertices[0], sizeof(quad_vertices));

    m_vao->add_buffer(*vbo);
    m_vao->unbind();
}

void FrameBuffersLayer::update(float delta_time) const
{
    m_camera->update(Application::get().get_window(), delta_time);

    float light_x = 2.0f * sin(glfwGetTime());
    float light_y = 1.0f;
    float light_z = 1.5f * cos(glfwGetTime());
    glm::vec3 light_pos = glm::vec3(light_x, light_y, light_z);

    Frustum frustum = create_frustum_from_camera(*m_camera, 1280.0f / 720.0f, 45.0f, 0.1f, 1000.0f);
    unsigned int total = 0;
    unsigned int display = 0;

    // First pass
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    m_shader->bind();
    m_shader->set_uniform_mat4("projection", m_camera->m_projection);
    m_shader->set_uniform_mat4("view", m_camera->view_matrix());
    m_shader->set_uniform3f("viewPos", m_camera->m_position);
    m_shader->set_uniform1f("material.shiny", 64.0f);
    m_shader->set_uniform3f("light.position", light_pos);
    m_shader->set_uniform3f("light.ambient", 1.0f, 1.0f, 1.0f);
    m_shader->set_uniform3f("light.diffuse", 1.0f, 1.0f, 1.0f);
    m_shader->set_uniform3f("light.specular", 1.0f, 1.0f, 1.0f);
    m_shader->set_uniform1f("light.constant", 1.0f);
    m_shader->set_uniform1f("light.linear", 0.09f);
    m_shader->set_uniform1f("light.quadratic", 0.032f);

    glStencilMask(0x00);
    m_shader->bind();
    glm::mat4 model = glm::mat4(1.0f);
    m_shader->set_uniform_mat4("model", model);
    m_cube->draw(frustum, model, *m_shader, display, total);

    m_light_shader->bind();
    m_light_shader->set_uniform_mat4("projection", m_camera->m_projection);
    m_light_shader->set_uniform_mat4("view", m_camera->view_matrix());

    glm::mat4 light_transform = glm::mat4(1.0f);
    light_transform = glm::translate(light_transform, light_pos);
    light_transform = glm::scale(light_transform, glm::vec3(0.2f));
    m_light_shader->set_uniform_mat4("model", light_transform);

    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);
    m_light->draw(frustum, model, *m_light_shader, display, total);

    // Scaled(outline)
    m_outline_shader->bind();
    m_outline_shader->set_uniform_mat4("projection", m_camera->m_projection);
    m_outline_shader->set_uniform_mat4("view", m_camera->view_matrix());

    glm::mat4 light_outline_transform = glm::mat4(1.0f);
    light_outline_transform = glm::translate(light_outline_transform, light_pos);
    light_outline_transform = glm::scale(light_outline_transform, glm::vec3(0.24f));
    m_outline_shader->set_uniform_mat4("model", light_outline_transform);

    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilMask(0x00);
    glDisable(GL_DEPTH_TEST);

    m_light->draw(frustum, model, *m_outline_shader, display, total);
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glEnable(GL_DEPTH_TEST);

    // Second Pass
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    m_post_processing_shader->bind();
    m_vao->bind();
    glDisable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_2D, m_texture_colorbuffer);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glEnable(GL_DEPTH_TEST);
}
