#pragma once

struct GLFWwindow;

void im_gui_init(GLFWwindow* window);
void im_gui_cleanup();
void editor_im_gui_render(bool editor);