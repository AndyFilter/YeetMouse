#include "ImGuiExtensions.h"

bool ImGui::ModeSelectable(const char *label, bool is_selected, ImGuiSelectableFlags flags, const ImVec2 &size) {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 24));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5);
    if (!is_selected)
        ImGui::PushStyleColor(ImGuiCol_Button, {0, 0, 0, 0});
    else
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));

    bool ret = false;

    if (ImGui::Button(label, size)) {
        ret = true;
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    return ret;
}

bool ImGui::ParameterSlider(const char *label, float &value, float v_speed, float v_min, float v_max) {
    //ImGui::Text()
    return true;
}
