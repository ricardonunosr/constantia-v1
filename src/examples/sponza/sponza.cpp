#include "sponza.h"
#include <glad/gl.h>
#include <spdlog/spdlog.h>

#include "camera.h"
#include "imgui.h"
#include "model.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

std::unique_ptr<Camera> SponzaLayer::m_camera = nullptr;
std::unique_ptr<Camera> SponzaLayer::m_second_camera = nullptr;

SponzaLayer::SponzaLayer(const std::string& name) : Layer(name)
{
    std::string base_path_assets = "./data/";
    m_sponza = std::make_unique<Model>(base_path_assets + "sponza/sponza.obj");
    m_light = std::make_unique<Model>(base_path_assets + "cube/cube.obj");

    // Sponza
    std::string vertex_shader_path = base_path_assets + "shaders/basic.vert";
    std::string fragment_shader_path = base_path_assets + "shaders/basic.frag";
    char* vertex_shader_source = (char*)read_entire_file(vertex_shader_path.c_str());
    char* fragment_shader_source = (char*)read_entire_file(fragment_shader_path.c_str());

    m_shader = (SponzaShader*)malloc(sizeof(SponzaShader));
    opengl_create_shader(vertex_shader_source, fragment_shader_source, &m_shader->common);
    m_shader->light_position = glGetUniformLocation(m_shader->common.program_id, "light.position");
    m_shader->light_ambient = glGetUniformLocation(m_shader->common.program_id, "light.ambient");
    m_shader->light_diffuse = glGetUniformLocation(m_shader->common.program_id, "light.diffuse");
    m_shader->light_specular = glGetUniformLocation(m_shader->common.program_id, "light.specular");
    m_shader->light_constant = glGetUniformLocation(m_shader->common.program_id, "light.constant");
    m_shader->light_linear = glGetUniformLocation(m_shader->common.program_id, "light.linear");
    m_shader->light_quadratic = glGetUniformLocation(m_shader->common.program_id, "light.quadratic");

    // Light
    std::string vertex_shader_light_path = base_path_assets + "shaders/light.vert";
    std::string fragment_shader_light_path = base_path_assets + "shaders/light.frag";
    char* vertex_shader_light_source = (char*)read_entire_file(vertex_shader_light_path.c_str());
    char* fragment_shader_light_source = (char*)read_entire_file(fragment_shader_light_path.c_str());

    OpenGLProgramCommon* light_shader = (OpenGLProgramCommon*)malloc(sizeof(OpenGLProgramCommon));
    opengl_create_shader(vertex_shader_light_source, fragment_shader_light_source, light_shader);
    m_light_shader = light_shader;

    m_camera = std::make_unique<Camera>();
    m_second_camera = std::make_unique<Camera>();

    // TODO: find a better way to define this
    glfwSetCursorPosCallback(app.window, mouse_callback);
    glfwSetMouseButtonCallback(app.window, mouse_button_callback);

    // Main Texture Framebuffer
    //-------------------------
    glGenFramebuffers(1, &m_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

    glGenTextures(1, &m_texture_colorbuffer);
    glBindTexture(GL_TEXTURE_2D, m_texture_colorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // attach it to currently bound framebuffer object
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture_colorbuffer, 0);

    // Generate RenderBuffer for depth and/or stencil attachments
    glGenRenderbuffers(1, &m_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WIDTH, HEIGHT);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        spdlog::error("Framebuffer is not complete!");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glUseProgram(m_shader->common.program_id);
    glUniform1f(m_shader->common.material_shininess, 64.0f);
    glUniform3f(m_shader->light_ambient, 1.0f, 1.0f, 1.0f);
    glUniform3f(m_shader->light_diffuse, 1.0f, 1.0f, 1.0f);
    glUniform3f(m_shader->light_specular, 1.0f, 1.0f, 1.0f);
    glUniform1f(m_shader->light_constant, 1.0f);
    glUniform1f(m_shader->light_linear, 0.09f);
    glUniform1f(m_shader->light_quadratic, 0.032f);
    glUseProgram(0);
}

void SponzaLayer::on_ui_render(float delta_time) const
{
    //    ImGui::ShowDemoWindow();
    //    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

    //    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    if (ImGui::Begin("Second Camera"))
    {
        ImVec2 window_size = ImGui::GetWindowSize();
        ImGui::Image(reinterpret_cast<ImTextureID>(m_texture_colorbuffer), window_size, ImVec2(0, 1), ImVec2(1, 0));
        ImGui::End();
    }
    //    ImGui::PopStyleVar();

    if (ImGui::Begin("Metrics/Debugger"))
    {
        ImGui::Text("Application average %.3f ms/frame (%.3f FPS)", delta_time * 1000.0f,
                    1000.0f / (1000.0f * delta_time));
        ImGui::Text("%d vertices, %d indices (%d triangles)", metrics.vertex_count, metrics.indices_count,
                    metrics.indices_count / 3);
        ImGui::Separator();

        ImGui::End();
    }
}

void SponzaLayer::update(float delta_time) const
{
    std::cout << "=======================Sponza loop start=============================\n";
    m_camera->update(app.window, delta_time);
    // m_second_camera->update(Application::get().get_window(), delta_time);

    float light_x = 2.0f * sin(glfwGetTime());
    float light_y = 1.0f;
    float light_z = 1.5f * cos(glfwGetTime());
    glm::vec3 light_pos = glm::vec3(light_x, light_y, light_z);

    Frustum frustum = create_frustum_from_camera(*m_camera, 1280.0f / 720.0f, 45.0f, 0.1f, 1000.0f);
    unsigned int total = 0;
    unsigned int display = 0;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.02f));

    glm::mat4 light_transform = glm::mat4(1.0f);
    light_transform = glm::translate(light_transform, light_pos);
    light_transform = glm::scale(light_transform, glm::vec3(0.2f));

    {
        // Primary Framebuffer
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glClearColor(0.5f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glUseProgram(m_shader->common.program_id);
        glUniformMatrix4fv(m_shader->common.projection, 1, false, glm::value_ptr(m_camera->m_projection));
        glUniformMatrix4fv(m_shader->common.view, 1, false, glm::value_ptr(m_camera->view_matrix()));
        glUniform3fv(m_shader->common.view_pos, 1, &m_camera->m_position[0]);
        glUniform3fv(m_shader->light_position, 1, &light_pos[0]);

        glUniform1f(m_shader->common.material_shininess, 64.0f);
        glUniform3f(m_shader->light_ambient, 1.0f, 1.0f, 1.0f);
        glUniform3f(m_shader->light_diffuse, 1.0f, 1.0f, 1.0f);
        glUniform3f(m_shader->light_specular, 1.0f, 1.0f, 1.0f);
        glUniform1f(m_shader->light_constant, 1.0f);
        glUniform1f(m_shader->light_linear, 0.09f);
        glUniform1f(m_shader->light_quadratic, 0.032f);

        glUniformMatrix4fv(m_shader->common.model, 1, GL_FALSE, glm::value_ptr(model));
        m_sponza->draw(frustum, model, (OpenGLProgramCommon*)m_shader, display, total);

        glUseProgram(m_light_shader->program_id);
        glUniformMatrix4fv(m_light_shader->model, 1, GL_FALSE, glm::value_ptr(light_transform));
        m_light->draw(frustum, model, m_light_shader, display, total);
    }

    {
        // Second Framebuffer
        glViewport(0, 0, 1920, 1080);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glUseProgram(m_shader->common.program_id);
        glUniformMatrix4fv(m_shader->common.projection, 1, false, glm::value_ptr(m_second_camera->m_projection));
        glUniformMatrix4fv(m_shader->common.view, 1, false, glm::value_ptr(m_second_camera->view_matrix()));
        glUniform3fv(m_shader->common.view_pos, 1, &m_camera->m_position[0]);
        glUniform3fv(m_shader->light_position, 1, &light_pos[0]);

        glUniform1f(m_shader->common.material_shininess, 64.0f);
        glUniform3f(m_shader->light_ambient, 1.0f, 1.0f, 1.0f);
        glUniform3f(m_shader->light_diffuse, 1.0f, 1.0f, 1.0f);
        glUniform3f(m_shader->light_specular, 1.0f, 1.0f, 1.0f);
        glUniform1f(m_shader->light_constant, 1.0f);
        glUniform1f(m_shader->light_linear, 0.09f);
        glUniform1f(m_shader->light_quadratic, 0.032f);

        m_sponza->draw(frustum, model, (OpenGLProgramCommon*)m_shader, display, total);

        glUseProgram(m_light_shader->program_id);
        glUniformMatrix4fv(m_light_shader->model, 1, GL_FALSE, glm::value_ptr(light_transform));
        m_light->draw(frustum, model, m_light_shader, display, total);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUseProgram(0);
    }

    std::cout << "=======================Sponza loop end=============================\n";
}
