#pragma once
#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_glfw.h>
#include <ImGui/imgui_impl_opengl3.h>

namespace GUI
{
	int Setup(int (*OnGui)());
    int RenderFrame();
    void ShutDown();

    void SetWindowSize(int x, int y);

    inline GLFWwindow* window;

    void GetMousePos(double *x, double *y);
}
