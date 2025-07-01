#include <array>
#include <csignal>
#include "gui.h"
#include "External/ImGui/implot.h"
#include "DriverHelper.h"
#include "FunctionHelper.h"
#include "ImGuiExtensions.h"
#include "ConfigHelper.h"
#include <chrono>
#include <vector>
#include <unistd.h>

#include "External/ImGui/imgui_internal.h"
#include "External/ImGui/implot_internal.h"

//#define USE_INPUT_DRAG

/* TODO
 * + Config export/import (from and to clipboard) in a human readable format or config.h
 * + Anisotropy
 * + Angle snapping
 * + Fully customizable curves (from polynomials)
 * - Clean up the parameter formatting to allow for different precisions
 */

AccelMode selected_mode = AccelMode_Linear;

const char *AccelModes[AccelMode_Count] = {"Current", "Linear", "Power", "Classic", "Motivity", "Natural", "Jump", "Look Up Table", "Custom Curve"};
#define NUM_MODES AccelMode_Count //(sizeof(AccelModes) / sizeof(char *))

Parameters params[NUM_MODES]; // Driver parameters for each mode
CachedFunction functions[NUM_MODES]; // Driver parameters for each mode
AccelMode used_mode = AccelMode_Linear;
bool was_initialized = false;
bool has_privilege = false;

static char LUT_user_data[4096];

void ResetParameters();

#define RefreshDevices() {devices = DriverHelper::DiscoverDevices(); \
                            if(selected_device >= devices.size())    \
                            selected_device = devices.size() - 1;}

int OnGui() {
    using namespace std::chrono;

    ImGuiContext& g = *GImGui;

    static float mouse_smooth = 0.75;
    static steady_clock::time_point last_apply_clicked;
    static bool show_custom_curve_control_points = true, move_control_points_along = false, show_custom_curve_LUT_points = false;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::BeginDisabled(!functions[selected_mode].isValid);
            if (ImGui::BeginMenu("Export")) {
                if (ImGui::MenuItem("Plain text")) {
                    //printf("Plain text:\n%s\n", ConfigHelper::ExportPlainText(params[selected_mode], true).c_str());
                    ConfigHelper::ExportPlainText(params[selected_mode], true);
                }
                ImGui::SetItemTooltip("Right click to copy to clipboard");
                if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                    auto exp_text = ConfigHelper::ExportPlainText(params[selected_mode], false);
                    ImGui::SetClipboardText(exp_text.c_str());
                    //ImGui::CloseCurrentPopup();
                }

                if (ImGui::MenuItem("Config.h format")) {
                    //printf("Plain text:\n%s\n", ConfigHelper::ExportConfig(params[selected_mode], true).c_str());
                    ConfigHelper::ExportConfig(params[selected_mode], true);
                }
                ImGui::SetItemTooltip("Right click to copy to clipboard");
                if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                    auto exp_text = ConfigHelper::ExportConfig(params[selected_mode], false);
                    ImGui::SetClipboardText(exp_text.c_str());
                    //ImGui::CloseCurrentPopup();
                }
                ImGui::EndMenu();
            }
            ImGui::EndDisabled();

            if (!functions[selected_mode].isValid)
                ImGui::SetItemTooltip("Can't export invalid parameters");

            Parameters imported_params;
            bool changed = false;
            if (ImGui::MenuItem("Import")) {
                if (ConfigHelper::ImportFile(LUT_user_data, imported_params)) {
                    changed = true;
                }
            }

            ImGui::SetItemTooltip("Right click to import from clipboard");
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                if (ConfigHelper::ImportClipboard(LUT_user_data, ImGui::GetClipboardText(), imported_params)) {
                    changed = true;
                    ImGui::CloseCurrentPopup();
                }
            }

            if (changed) {
                for (int i = 1; i < NUM_MODES; i++) {
                    if (i == AccelMode_CustomCurve) { // Preserve the custom curve points when copying
                        CustomCurve curve = params[AccelMode_CustomCurve].customCurve;
                        params[i] = imported_params;
                        params[i].customCurve = curve;
                        params[i].accelMode = AccelMode_Lut;

                        params[i].LUT_size = params[i].customCurve.ExportCurveToLUT(params[i].LUT_data_x, params[i].LUT_data_y);
                        params[i].customCurve.UpdateLUT();
                    }
                    else {
                        params[i] = imported_params;
                        params[i].accelMode = static_cast<AccelMode>(i == 0 ? used_mode : i);
                    }

                    functions[i] = CachedFunction(((float) PLOT_X_RANGE) / PLOT_POINTS, &params[i]);
                    functions[i].PreCacheFunc();
                }

                selected_mode = imported_params.accelMode;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    int hovered_mode = -1;

    /* ---------------------------- LEFT MODES WINDOW ---------------------------- */
    ImGui::SetNextWindowSizeConstraints({220, 0}, {FLT_MAX, FLT_MAX});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {12, 12});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {12, 12});
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if (ImGui::BeginChild("Modes", ImVec2(220, 0), ImGuiChildFlags_FrameStyle)) {
        ImGui::PopStyleColor();
        ImGui::SeparatorText("Mode Selection");
        for (int i = 1; i < NUM_MODES; i++) {
            const char *accel = AccelModes[i];
            if (ImGui::ModeSelectable(accel, i == selected_mode, 0, {-1, 0}))
                selected_mode = static_cast<AccelMode>(i);
            if (ImGui::IsItemHovered())
                hovered_mode = i;
        }
    } else
        ImGui::PopStyleColor();
    ImGui::EndChild();
    ImGui::PopStyleVar(2);

    ImGui::SameLine();

    /* ---------------------------- MIDDLE PARAMETERS WINDOW ---------------------------- */
    ImGui::SetNextWindowSizeConstraints({220, -1}, {420, FLT_MAX});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {10, 10});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {10, 10});
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if (ImGui::BeginChild("Parameters", ImVec2(220, -1), ImGuiChildFlags_FrameStyle | ImGuiChildFlags_ResizeX)) {
        auto avail = ImGui::GetContentRegionAvail();
        ImGui::PopStyleColor();
        ImGui::SeparatorText("Parameters");
        ImGui::PushItemWidth(avail.x);

        bool change = false;

        change |= ImGui::Checkbox("Use anisotropy", &params[selected_mode].use_anisotropy);
        ImGui::SetItemTooltip("Separate X/Y sensitivity values");

        // Display Global Parameters First
#ifdef USE_INPUT_DRAG
                change |= ImGui::DragFloat("##Sens_Param", &params[selected_mode].sens, 0.01, 0.01, 10, "Sensitivity %0.2f");
                change |= ImGui::DragFloat("##OutCap_Param", &params[selected_mode].outCap, 0.05, 0, 100, "Output Cap. %0.2f");
                change |= ImGui::DragFloat("##InCap_Param", &params[selected_mode].inCap, 0.1, 0, 200, "Input Cap. %0.2f");
                change |= ImGui::DragFloat("##Offset_Param", &params[selected_mode].offset, 0.05, -50, 50, "Offset %0.2f");
                change |= ImGui::DragFloat("##PreScale_Param", &params[selected_mode].preScale, 0.01, 0.01, 10, "Pre-Scale %0.2f");
                ImGui::SetItemTooltip("Used to adjust for DPI (Should be 800/DPI)");
                change |= ImGui::DragFloat("##Adv_Rotation", &params[selected_mode].rotation, 0.1, 0, 180,
                                                u8"Rotation Angle %0.2f°");
#else
        if (params[selected_mode].use_anisotropy) {
            change |= ImGui::SliderFloat("##Sens_Param", &params[selected_mode].sens, 0.005, 5, "Sensitivity X %.3f");
            change |= ImGui::SliderFloat("##SensY_Param", &params[selected_mode].sensY, 0.005, 5, "Sensitivity Y %.3f");
        } else
            change |= ImGui::SliderFloat("##Sens_Param", &params[selected_mode].sens, 0.005, 5, "Sensitivity %.3f");
        change |= ImGui::SliderFloat("##OutCap_Param", &params[selected_mode].outCap, 0, 5, "Output Cap. %0.2f");
        change |= ImGui::SliderFloat("##InCap_Param", &params[selected_mode].inCap, 0, 120, "Input Cap. %0.2f");
        change |= ImGui::SliderFloat("##Offset_Param", &params[selected_mode].offset, -50, 50, "Offset %0.2f");
        change |= ImGui::SliderFloat("##PreScale_Param", &params[selected_mode].preScale, 0.01, 10, "Pre-Scale %0.2f");
        ImGui::SetItemTooltip("Used to adjust for different DPI values (Set to 800/DPI)");
#endif

        ImGui::SeparatorText("Advanced");

        // Mode Specific Parameters
        ImGui::PushID(selected_mode);
        switch (selected_mode) {
            case AccelMode_Current: {
                break;
            }
            case AccelMode_Linear: // Linear
            {
#ifdef USE_INPUT_DRAG
                        change |= ImGui::DragFloat("##Accel_Param", &params[selected_mode].accel, 0.0001, 0.0005, 0.1, "Acceleration %0.4f", ImGuiSliderFlags_Logarithmic);
#else
                change |= ImGui::SliderFloat("##Accel_Param", &params[selected_mode].accel, 0.0005, 0.1,
                                             "Acceleration %0.4f", ImGuiSliderFlags_Logarithmic);
                ImGui::SetItemTooltip("Ctrl+LMB to input any value you want");
#endif
                break;
            }
            case AccelMode_Power: // Power
            {
#ifdef USE_INPUT_DRAG
                        change |= ImGui::DragFloat("##Accel_Param", &params[selected_mode].accel, 0.01, 0.01, 10, "Acceleration %0.2f");
                        change |= ImGui::DragFloat("##Exp_Param", &params[selected_mode].exponent, 0.01, 0.01, 1, "Exponent %0.2f");
#else
                change |= ImGui::SliderFloat("##Accel_Param", &params[selected_mode].accel, 0.001, 5,
                                             "Acceleration %0.3f");
                change |= ImGui::SliderFloat("##Exp_Param", &params[selected_mode].exponent, 0.01, 1, "Exponent %0.2f");
                change |= ImGui::SliderFloat("##OutOffset_Param", &params[selected_mode].midpoint, 0, 5, "Output Offset %0.2f");
#endif
                break;
            }
            case AccelMode_Classic: // Classic
            {
#ifdef USE_INPUT_DRAG
                        change |= ImGui::DragFloat("##Accel_Param", &params[selected_mode].accel, 0.001, 0.001, 2, "Acceleration %0.3f");
                        change |= ImGui::DragFloat("##Exp_Param", &params[selected_mode].exponent, 0.01, 2.01, 5, "Exponent %0.2f");
#else
                change |= ImGui::SliderFloat("##Accel_Param", &params[selected_mode].accel, 0.001, 2,
                                             "Acceleration %0.3f");
                change |= ImGui::SliderFloat("##Exp_Param", &params[selected_mode].exponent, 2.01, 5, "Exponent %0.2f");
#endif
                break;
            }
            case AccelMode_Motivity: // Motivity
            {
#ifdef USE_INPUT_DRAG
                        change |= ImGui::DragFloat("##Accel_Param", &params[selected_mode].accel, 0.01, 0.01, 10, "Acceleration %0.2f");
                        change |= ImGui::DragFloat("##MidPoint_Param", &params[selected_mode].midpoint, 0.05, 0.1, 50, "Start %0.2f");
#else
                change |= ImGui::SliderFloat("##Accel_Param", &params[selected_mode].accel, 0.01, 10,
                                             "Acceleration %0.2f");
                change |= ImGui::SliderFloat("##MidPoint_Param", &params[selected_mode].midpoint, 0.1, 50,
                                             "Midpoint %0.2f");
#endif
                break;
            }
            case AccelMode_Natural: // Natural
            {
#ifdef USE_INPUT_DRAG
                        change |= ImGui::DragFloat("##Accel_Param", &params[selected_mode].accel, 0.001, 0.001, 5, "Acceleration %0.3f");
                        change |= ImGui::DragFloat("##Exp_Param", &params[selected_mode].exponent, 0.01, 0.01, 8, "Limit %0.2f");
                        change |= ImGui::DragFloat("##MidPoint_Param", &params[selected_mode].midpoint, 0, 0.1, 50, "Start %0.2f");
#else
                change |= ImGui::SliderFloat("##Accel_Param", &params[selected_mode].accel, 0.001, 5,
                                             "Acceleration %0.3f");
                change |= ImGui::SliderFloat("##Exp_Param", &params[selected_mode].exponent, 0.01, 8,
                                             "Limit %0.2f");
		change |= ImGui::SliderFloat("##MidPoint_Param", &params[selected_mode].midpoint, 0, 50,
                                             "Midpoint %0.2f");

#endif
                change |= ImGui::Checkbox("##Smoothing_Param", &params[selected_mode].useSmoothing);
                ImGui::SameLine();
                ImGui::Text("Use Smoothing");
                break;
	    }
            case AccelMode_Jump: // Jump
            {
#ifdef USE_INPUT_DRAG
                        change |= ImGui::DragFloat("##Accel_Param", &params[selected_mode].accel, 0.01, 0, 10, "Acceleration %0.2f");
                        change |= ImGui::DragFloat("##MidPoint_Param", &params[selected_mode].midpoint, 0.05, 0.1, 50, "Start %0.2f");
                        change |= ImGui::DragFloat("##Exp_Param", &params[selected_mode].exponent, 0.01, 0.01, 1, "Smoothness %0.2f");
#else
                change |= ImGui::SliderFloat("##Accel_Param", &params[selected_mode].accel, 0, 10,
                                             "Acceleration %0.2f");
                change |= ImGui::SliderFloat("##MidPoint_Param", &params[selected_mode].midpoint, 0.1, 50,
                                             "Midpoint %0.2f");
                change |= ImGui::SliderFloat("##Exp_Param", &params[selected_mode].exponent, 0.01, 1,
                                             "Smoothness %0.2f");
#endif
                change |= ImGui::Checkbox("##Smoothing_Param", &params[selected_mode].useSmoothing);
                ImGui::SameLine();
                ImGui::Text("Use Smoothing");
                break;
            }
            case AccelMode_Lut: {
                ImGui::Text("LUT data:");
                change |= ImGui::InputTextWithHint("##LUT data", "x1,y1;x2,y2;x3,y3...", LUT_user_data,
                                                   sizeof(LUT_user_data), ImGuiInputTextFlags_AutoSelectAll);
                ImGui::SetItemTooltip("Format: x1,y1;x2,y2;x3,y3... (commas and semicolons are treated equally)");
                //change |= ImGui::DragFloat("##LUT_Stride_Param", &params[selected_mode].LUT_stride, 0.05, 0.05, 10, "Stride %0.2f");
                //ImGui::SetItemTooltip("Gap between each 'y' value");
                if (ImGui::Button("Save", {-1, 0})) {
                    change = true;

                    // Needs to be converted to int, because the kernel parameters don't deal too well with unsigned long longs
                    params[selected_mode].LUT_size = DriverHelper::ParseUserLutData(LUT_user_data,
                        params[selected_mode].LUT_data_x,
                        params[selected_mode].LUT_data_y,
                        std::size(params[selected_mode].LUT_data_x));
                }
                break;
            case AccelMode_CustomCurve: {
                if (ImGui::Button("Smooth Curve", {-1, 0})) {
                    params[selected_mode].customCurve.SmoothBezier();
                    change |= true;
                }
                ImGui::SetItemTooltip("Applies smoothing to the curve without moving the base (yellow) points");

                ImGui::Checkbox("Show Control Points", &show_custom_curve_control_points);

                ImGui::Checkbox("Link Control Points", &move_control_points_along);
                ImGui::SetItemTooltip("Moves control points along with it's parent curve point");

                auto& points = params[selected_mode].customCurve.points;
                auto& control_points = params[selected_mode].customCurve.control_points;

                ImGui::Unindent();
                for (int i = 0; i < points.size(); ++i) {
                    float p_min = i > 0 ? points[i-1].x + CURVE_POINTS_MARGIN : 0;
                    float p_max = i < points.size() - 1 ? points[i+1].x - CURVE_POINTS_MARGIN : 1000;
                    auto& p = points[i];
                    ImGui::PushID(i);
                    if (ImGui::TreeNode("CrvPnt", "Point %d", i)) {
                        auto p_before = p;
                        bool drag_changed = false;
                        ImGui::BeginGroup();
                        ImGui::PushMultiItemsWidths(2, ImGui::GetContentRegionAvail().x);
// Sliders don't work too well here, so I decided to stick with drag sliders
//#ifdef USE_INPUT_DRAG
                        drag_changed |= ImGui::DragFloat("##pos1x", &p.x, 0.5, p_min, p_max, "%.3f x");
                        ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);
                        drag_changed |= ImGui::DragFloat("##pos1y", &p.y, 0.01, 0, 10, "%.3f y");
// #else
//                         drag_changed |= ImGui::SliderFloat("##pos1x", &p.x, p_min, p_max);
//                         ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);
//                         drag_changed |= ImGui::SliderFloat("##pos1y", &p.y, 0, 10);
// #endif
                        ImGui::PopItemWidth(); ImGui::PopItemWidth();
                        ImGui::EndGroup();

                        change |= drag_changed;

                        if (move_control_points_along && drag_changed) {
                            ImVec2 drag = p - p_before;

                            //printf("held drag = (%.2f, %.2f)\n", drag.x, drag.y);
                            // Apply the Bezier point drag to it's control points
                            if (i == 0) {
                                control_points[0][0] += drag;
                            }
                            else if (i == points.size() - 1) {
                                control_points[i-1][1] += drag;
                            }
                            else {
                                control_points[i][0] += drag;
                                control_points[i-1][1] += drag;
                            }

                        }
                        if (points.size() > 1) {
                            ImGui::Separator();
                            for (int j = (i == points.size() - 1 || i == 0) ? 0 : 1; j >= 0; j--) {
                                ImGui::PushID(j);
                                ImGui::BeginGroup();
                                ImGui::PushMultiItemsWidths(2, ImGui::GetContentRegionAvail().x);
                                auto& p1 = control_points[i == points.size() - 1 ? (i-1) : (i-j)][i == 0 ? 0 : i == points.size() - 1 ? 1 : j];
                                // p.x = std::clamp(p.x, i > 0 ? points[i-1].x + 0.5f : 0, i < points.size() - 1 ? points[i+1].x - 0.5f : 1000);
                                change |= ImGui::DragFloat("##pos2x", &p1.x, 0.5, p_min, p_max, "%.3f x");
                                ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);
                                change |= ImGui::DragFloat("##pos2y", &p1.y, 0.01, 0, 10, "%.3f y");
                                ImGui::PopItemWidth(); ImGui::PopItemWidth();
                                ImGui::EndGroup();
                                ImGui::PopID();
                            }
                        }
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
                ImGui::Indent();

                ImGui::SeparatorText("LUT Export");
                ImGui::Checkbox("Show LUT Points", &show_custom_curve_LUT_points);

                if (change) {
                    params[selected_mode].customCurve.ApplyCurveConstraints();
                    params[selected_mode].LUT_size = params[selected_mode].customCurve.ExportCurveToLUT(params[selected_mode].LUT_data_x, params[selected_mode].LUT_data_y);
                    params[selected_mode].customCurve.UpdateLUT();
                }

                break;
            }
            }
            default: {
                break;
            }
        }
        ImGui::PopID();

        ImGui::SeparatorText("Rotation");
        change |= ImGui::SliderFloat("##Adv_AS_Threshold", &params[selected_mode].as_threshold, 0, 179.99,
                                     u8"Snapping Threshold %0.2f°");
        change |= ImGui::SliderFloat("##Adv_AS_Angle", &params[selected_mode].as_angle, 0, 179.99,
                                     u8"Snapping Angle %0.2f°");
        change |= ImGui::SliderFloat("##Adv_Rotation", &params[selected_mode].rotation, -180, 180,
                                     u8"Rotation Angle %0.2f°");
        if (params[selected_mode].as_threshold > 0)
            ImGui::SetItemTooltip("Rotation is applied after Angle Snapping");

        if (change)
            functions[selected_mode].PreCacheFunc();

        ImGui::PopItemWidth();
    } else
        ImGui::PopStyleColor();
    ImGui::EndChild();
    ImGui::PopStyleVar(2);

    ImGui::SameLine();


    ImGui::BeginGroup();

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {10, 10});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {10, 10});
    if (ImGui::BeginChild("PlotParameters",
                          ImVec2(-1, (ImGui::GetFrameHeightWithSpacing()) * 1 + ImGui::GetStyle().FramePadding.y),
                          ImGuiChildFlags_FrameStyle)) {
        auto avail = ImGui::GetContentRegionAvail();
        ImGui::PopStyleColor();
        ImGui::PushItemWidth(avail.x);
#ifdef USE_INPUT_DRAG
                ImGui::DragFloat("##MouseSmoothness", &mouse_smooth, 0.001, 0.0, 0.99, "Mouse Smoothness %0.2f");
#else
        ImGui::SliderFloat("##MouseSmoothness", &mouse_smooth, 0.0, 0.99, "Mouse Speed Smoothness %0.2f");
#endif
        ImGui::SetItemTooltip("Smooths out the mouse movement indicator (The small orange dot on the graph)");
        ImGui::PopItemWidth();
    } else
        ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    ImGui::EndChild();

    auto avail = ImGui::GetContentRegionAvail();

    // Calculate everything about the mouse speed
    static float recent_mouse_top_speed = 0;
    static steady_clock::time_point last_time_speed_record_broken = steady_clock::now();
    static steady_clock::time_point last_probe_time = steady_clock::now();
    static float last_frame_speed = 0;
    static ImVec2 last_mouse_pos = {0, 0};
    double mouse_pos[2];
    GUI::GetMousePos(mouse_pos, mouse_pos + 1);
    ImVec2 mouse_delta = {
        static_cast<float>(mouse_pos[0] - last_mouse_pos.x), static_cast<float>(mouse_pos[1] - last_mouse_pos.y)
    };
    float mouse_speed = std::sqrt(ImLengthSqr(mouse_delta));
    mouse_speed = mouse_speed / std::chrono::duration_cast<milliseconds>(steady_clock::now() - last_probe_time).count();
    mouse_speed /= functions[0].EvalFuncAt(mouse_speed); // Normalize in regard to acceleration (to get raw speed)
    // It's a rough approximation because in the time of 1 frame, tens of mouse packets can be received (thus "accelerated").
    last_probe_time = steady_clock::now();
    if (mouse_speed > recent_mouse_top_speed) {
        recent_mouse_top_speed = mouse_speed;
        last_time_speed_record_broken = steady_clock::now();
    }
    float avg_speed = fmaxf(mouse_speed * (1 - mouse_smooth) + last_frame_speed * mouse_smooth, 0.01);
    ImPlotPoint mousePoint_main = ImPlotPoint(avg_speed, avg_speed < params[selected_mode].offset
                                                             ? params[selected_mode].sens
                                                             : functions[selected_mode].EvalFuncAt(
                                                                 avg_speed - params[selected_mode].offset));

    last_mouse_pos = {(float) mouse_pos[0], (float) mouse_pos[1]};


    last_frame_speed = avg_speed;

    // Check if a second passed since the last highest mouse speed, if so reset the record speed dot
    if (duration_cast<milliseconds>(steady_clock::now() - last_time_speed_record_broken).count() > 1000)
        recent_mouse_top_speed = 0;

    ImPlotPoint mousePoint_topSpeed = ImPlotPoint(recent_mouse_top_speed,
                                                  recent_mouse_top_speed < params[selected_mode].offset
                                                      ? params[selected_mode].sens
                                                      : functions[selected_mode].EvalFuncAt(
                                                          recent_mouse_top_speed - params[selected_mode].offset));

    ImPlot::SetNextAxesLimits(0, PLOT_X_RANGE, 0, 4);
    /* ---------------------------- FUNCTION PLOT ---------------------------- */
    if (ImPlot::BeginPlot("Function Plot [Input / Output]", {-1, avail.y - 70})) {
        ImPlot::SetupAxis(ImAxis_X1, "Input Speed [counts / ms]");
        ImPlot::SetupAxis(ImAxis_Y1, "Output / Input Speed Ratio");

        // Display currently applied parameters in the background
        if (was_initialized) {
            ImPlot::SetNextLineStyle(ImVec4(0.3, 0.3, 0.3, 1));
            ImPlot::PlotLine("Function in use", functions[0].values, PLOT_POINTS, functions[0].x_stride);
        }

        if (params[selected_mode].use_anisotropy) {
            ImPlot::SetNextLineStyle(ImVec4(0.3, 0.3, 0.8, 1), 2);
            ImPlot::PlotLine("Active Mode Y##ActivePlotY", functions[selected_mode].values_y, PLOT_POINTS,
                             functions[selected_mode].x_stride);
        }

        ImPlot::SetNextLineStyle(ImColor(0.3f, 0.5f, 0.7f, 1.0f), 2);

        if (selected_mode == AccelMode_CustomCurve) {
            bool is_hovered = false, is_pressed = false, is_held = false;
            bool is_interacting_with_points = false;
            bool modified = false;

            static ImVec2 held_point_start_pos = {0, 0};
            static int held_point = -1;
            static int last_held_point = -1;
            int hovered_point = -1;

            auto& points = params[selected_mode].customCurve.points;
            auto& control_points = params[selected_mode].customCurve.control_points;

            if (!points.empty()) {
                // Draw lines between control and Bezier points
                if (show_custom_curve_control_points) {
                    ImPlot::PushPlotClipRect();
                    for (int i = 0; i < points.size()-1; ++i) {
                        auto& p = points[i];
                        ImVec2 p1 = ImPlot::PlotToPixels(p);
                        ImVec2 p2 = ImPlot::PlotToPixels(points[(i + 1) % points.size()]);
                        ImVec2 pc1 = ImPlot::PlotToPixels(control_points[i][0]);
                        ImVec2 pc2 = ImPlot::PlotToPixels(control_points[i][1]);

                        ImPlot::GetPlotDrawList()->AddLine(p1, pc1, ImColor(0.7, 0.1f, 0.8, 0.8));
                        ImPlot::GetPlotDrawList()->AddLine(p2, pc2, ImColor(0.7, 0.1f, 0.8, 0.8));
                    }
                    ImPlot::PopPlotClipRect();
                }

                // Draw Bezier points
                for (int i = 0; i < points.size(); ++i) {
                    auto& p = points[i];
                    float p_min = i > 0 ? points[i-1].x + 0.5f : 0;
                    float p_max = i < points.size() - 1 ? points[i+1].x - 0.5f : 1000;
                    // char _buf[12];
                    // sprintf(_buf, "P%i", i);
                    // ImPlot::PlotText(_buf, p.x, p.y);
                    // unique even id
                    ImGui::PushID(i*1512 + 22);
                    modified |= ImPlot::DragPoint(i*2,&p.x,&p.y, ImVec4(0,0.9f,0,1),4, ImPlotDragToolFlags_None, &is_pressed, &is_hovered, &is_held);
                    is_interacting_with_points |= is_pressed || is_held || is_hovered;

                    if (is_held) {
                        p.x = std::clamp(p.x, p_min, p_max);
                    }

                    if (is_hovered)
                        hovered_point = i;

                    if (is_held)
                        held_point = i;
                    else if (held_point == i)
                        held_point = -1;

                    auto old_cursor_pos = ImGui::GetCursorPos();
                    ImVec2 pos = ImPlot::PlotToPixels(p.x,p.y,IMPLOT_AUTO,IMPLOT_AUTO);
                    ImGui::SetCursorPos(pos - ImVec2(5, 5));
                    ImGui::InvisibleButton("#invBt", {10, 10});
                    ImGui::SetCursorPos(old_cursor_pos);

                    if (ImGui::BeginPopupContextItem()) {
                        char buf[12];
                        sprintf(buf, "P #%d", i);
                        ImGui::SeparatorText(buf);
                        bool drag_changed = false;
                        ImVec2 p_before = p;
                        ImGui::Checkbox("Lock control", &p.is_locked);
                        ImGui::SetItemTooltip("Control points won't be updated when smoothing");
                        ImGui::BeginGroup();
                        ImGui::PushMultiItemsWidths(2, ImGui::GetContentRegionAvail().x);
                        drag_changed |= ImGui::DragFloat("##pos1x", &p.x, 0.5, p_min, p_max);
                        ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);
                        drag_changed |= ImGui::DragFloat("##pos1y", &p.y, 0.01, 0, 10);
                        ImGui::PopItemWidth();
                        ImGui::EndGroup();
                        modified |= drag_changed;
                        if (drag_changed) {
                            ImVec2 drag = p - p_before;

                            //printf("held drag = (%.2f, %.2f)\n", drag.x, drag.y);
                            // Apply the Bezier point drag to it's control points
                            if (i == 0) {
                                control_points[0][0] += drag;
                            }
                            else if (i == points.size() - 1) {
                                control_points[i-1][1] += drag;
                            }
                            else {
                                control_points[i][0] += drag;
                                control_points[i-1][1] += drag;
                            }

                        }
                        if (points.size() > 1)
                            for (int j = (i == points.size() - 1 || i == 0) ? 0 : 1; j >= 0; j--) {
                                ImGui::PushID(j);
                                ImGui::BeginGroup();
                                ImGui::PushMultiItemsWidths(2, ImGui::GetContentRegionAvail().x);
                                auto& p1 = control_points[i == points.size() - 1 ? (i-1) : (i-j)][i == 0 ? 0 : i == points.size() - 1 ? 1 : j];
                                // p.x = std::clamp(p.x, i > 0 ? points[i-1].x + 0.5f : 0, i < points.size() - 1 ? points[i+1].x - 0.5f : 1000);
                                modified |= ImGui::DragFloat("##pos2x", &p1.x, 0.5, p_min, p_max);
                                ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);
                                modified |= ImGui::DragFloat("##pos2y", &p1.y, 0.01, 0, 10);
                                ImGui::PopItemWidth();
                                ImGui::EndGroup();
                                ImGui::PopID();
                            }
                        if (ImGui::Button("Remove", {-1, 0})) {
                            points.erase(points.begin()+i);
                            if (!points.empty()) {
                                if (i > 0 && i < points.size()) {
                                    // Swap control points before erasing, because of how control points are stored
                                    std::swap(control_points[i-1][0], control_points[i][0]);
                                }
                                if (i > 0)
                                    control_points.erase(control_points.begin()+i-1);
                                else
                                    control_points.erase(control_points.begin()+i);
                            }
                            modified = true;
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::PopID();
                }


                // Move control points with main points
                if (held_point >= 0) {
                    ImVec2 drag = points[held_point] - held_point_start_pos;
                    held_point_start_pos = points[held_point];
                    if (last_held_point != -1) {
                        //printf("held drag = (%.2f, %.2f)\n", drag.x, drag.y);

                        if (move_control_points_along) {
                            // Apply the Bezier point drag to it's control points
                            if (held_point == 0) {
                                control_points[0][0] += drag;
                            }
                            else if (held_point == points.size() - 1) {
                                control_points[held_point-1][1] += drag;
                            }
                            else {
                                control_points[held_point][0] += drag;
                                control_points[held_point-1][1] += drag;
                            }
                        }

                        // Apply constraints to the control point
                        // if (held_point < points.size() - 1)
                        //     control_points[held_point][0].x = std::clamp(control_points[held_point][0].x, points[held_point].x + CURVE_POINTS_MARGIN, held_point < points.size() - 1 ? points[held_point+1].x - CURVE_POINTS_MARGIN : 1000);
                        // if (held_point > 0)
                        //     control_points[held_point-1][1].x = std::clamp(control_points[held_point-1][1].x, points[held_point-1].x + CURVE_POINTS_MARGIN, held_point < points.size() - 1 ? points[held_point].x - CURVE_POINTS_MARGIN : 1000);

                        if (held_point < points.size() - 1)
                            control_points[held_point][0].x = std::clamp(control_points[held_point][0].x,
                                points[held_point].x + CURVE_POINTS_MARGIN, points[held_point+1].x - CURVE_POINTS_MARGIN);
                        if (held_point > 0)
                            control_points[held_point-1][1].x = std::clamp(control_points[held_point-1][1].x,
                                points[held_point-1].x + CURVE_POINTS_MARGIN,  points[held_point].x - CURVE_POINTS_MARGIN);
                    }
                }

                // Draw control points
                if (show_custom_curve_control_points && points.size() > 1) {
                    for (int i = 0; i < control_points.size(); ++i) {
                        for (int j = 0; j < 2; j++) {
                            bool is_point_hovered = hovered_point != -1 && ((i == hovered_point && j == 0) || (i == hovered_point - 1 && j == 1));
                            is_point_hovered |= held_point != -1 && ((i == held_point && j == 0) || (i == held_point - 1 && j == 1));
                            bool is_point_locked = points[i+j].is_locked;//(points[i].is_locked && j == 0) || (points[i+1].is_locked && j == 1);
                            auto& p = control_points[i][j];
                            // char _buf[12];
                            // sprintf(_buf, "P(%d, %d)", i, j);
                            // ImPlot::PlotText(_buf, p.x, p.y);
                            // unique odd id

                            modified |= ImPlot::DragPoint(i * 4 + j * 2 + 1,&p.x,&p.y,
                                is_point_locked ? ImVec4(is_point_hovered ? 0.4 : 0.3, 0.7f, 0.8, 1) : ImVec4(is_point_hovered ? 0.3 : 0,0.5f,1, 1),
                                is_point_hovered ? 5 : 4, ImPlotDragToolFlags_None, &is_pressed, &is_hovered, &is_held);
                            is_interacting_with_points |= is_pressed || is_held || is_hovered;

                            if (is_held) {
                                if (j == 1)
                                    p.x = std::clamp(p.x, points[i].x + CURVE_POINTS_MARGIN, i < points.size() - 1 ? points[i+1].x - CURVE_POINTS_MARGIN : 1000);
                                else
                                    p.x = std::clamp(p.x, points[i].x + CURVE_POINTS_MARGIN, i < points.size() - 1 ? points[i+1].x - CURVE_POINTS_MARGIN : 1000);
                            }
                        }
                    }
                }
            }

            static ImVec2 mouse_pos = {0, 0};
            if (ImGui::IsMouseClicked(0)) {
                mouse_pos = ImGui::GetMousePos();
            }

            if (ImGui::IsMouseReleased(0) && !is_interacting_with_points && !ImGui::IsMouseDragging(0) && !ImPlot::GetCurrentPlot()->Held && mouse_pos == ImGui::GetMousePos() && ImPlot::IsPlotHovered()) {
                auto m_pos = ImPlot::GetPlotMousePos();

                int best_idx = -1;
                if (points.size() >= 1) {
                    for (int i = points.size()-1; i >= 0; i--) {
                        if (points[i].x < m_pos.x) {
                            best_idx = i;
                            break;
                        }
                    }
                }

                points.insert(points.begin() + best_idx + 1, static_cast<Ex_Vec2>(m_pos));

                if (points.size() > 1) {

                    if (best_idx == -1) {
                        control_points.insert(control_points.begin(), {m_pos + ImPlotPoint(4, 0), points[1] - ImPlotPoint(4, 0)});
                    }
                    else if (points.size() >= 2 && best_idx == points.size() - 2) {  // points.size() - 2 because we changed points' size right before
                        control_points.insert(control_points.begin() + best_idx, {points[best_idx] + ImPlotPoint(4, 0), m_pos - ImPlotPoint(4, 0)});
                        // std::swap(control_points[best_idx][0], control_points[best_idx][1]);
                        // control_points[best_idx][0] = points[best_idx] + ImPlotPoint(10, 0);
                    }
                    else {
                        auto cur_point = points[best_idx+1];
                        auto prev_point = points[best_idx] - cur_point;
                        auto next_point = points[best_idx+2] - cur_point;

                        auto len_prev = std::sqrt(ImLengthSqr(prev_point)), len_next = std::sqrt(ImLengthSqr(next_point));

                        auto dir = next_point * (len_prev / len_next) - prev_point;
                        dir = dir / std::sqrt(ImLengthSqr(dir));

                        control_points.insert(control_points.begin() + best_idx, {m_pos + (-dir * (len_prev / 3)), m_pos + dir * (len_next / 3)});

                        std::swap(control_points[best_idx][0], control_points[best_idx + 1][0]);
                        std::swap(control_points[best_idx + 1][0], control_points[best_idx][1]);
                    }

                    params[selected_mode].customCurve.ApplyCurveConstraints();
                }

                modified = true;
            }

            if (modified) {
                params[selected_mode].customCurve.ApplyCurveConstraints();
                params[selected_mode].LUT_size = params[selected_mode].customCurve.ExportCurveToLUT(params[selected_mode].LUT_data_x, params[selected_mode].LUT_data_y);
                params[selected_mode].customCurve.UpdateLUT();
                functions[selected_mode].PreCacheFunc();
            }

            // Draw the curve
            if (points.size() > 1) {

                ImPlot::PlotLine("##bez",&params[selected_mode].customCurve.LUT_points[0].x, &params[selected_mode].customCurve.LUT_points[0].y, (points.size() - 1) * BEZIER_FRAG_SEGMENTS, 0, 0, sizeof(ImPlotPoint));

                if (show_custom_curve_LUT_points) {
                    ImPlot::SetNextMarkerStyle(-1, 2);
                    // Draw LUT points
                    ImPlot::PlotScatterG("LUT points", [](int idx, void* ud) {
                        double x = static_cast<Parameters*>(ud)->LUT_data_x[idx];
                        double y = static_cast<Parameters*>(ud)->LUT_data_y[idx];
                        return ImPlotPoint(x, y);
                    }, &params[selected_mode], params[selected_mode].LUT_size);

                    ImPlot::PlotLine("##ActivePlot", functions[selected_mode].values, PLOT_POINTS,
                        functions[selected_mode].x_stride);
                }
            }

            // ImPlot::PlotLine("##ActivePlot", functions[selected_mode].values, PLOT_POINTS,
            //              functions[selected_mode].x_stride);

            last_held_point = held_point;
        }
        else
            ImPlot::PlotLine("##ActivePlot", functions[selected_mode].values, PLOT_POINTS,
                         functions[selected_mode].x_stride);

        ImPlot::PlotScatterG("Mouse Speed", [](int idx, void *data) { return *(ImPlotPoint *) data; }, &mousePoint_main,
                             1);
        ImPlot::SetNextMarkerStyle(IMPLOT_AUTO, IMPLOT_AUTO /* * !is_record_old*/,
                                   ImVec4(180 / 255.f, 70 / 255.f, 80 / 255.f, 1), 2,
                                   ImVec4(180 / 255.f, 70 / 255.f, 80 / 255.f, 1));
        ImPlot::PlotScatterG("Mouse Top Speed", [](int idx, void *data) { return *(ImPlotPoint *) data; },
                             &mousePoint_topSpeed, 1);

        ImPlot::SetNextLineStyle(IMPLOT_AUTO_COL, 2);

        if (hovered_mode != -1 && selected_mode != hovered_mode) {
            ImPlot::SetNextLineStyle(ImVec4(0.7, 0.7, 0.3, 1));
            ImPlot::PlotLine("##Hovered Function", functions[hovered_mode].values, PLOT_POINTS,
                             functions[hovered_mode].x_stride);
        }

        ImPlot::EndPlot();
    }

    /* ---------------------------- BOTTOM BUTTONS ---------------------------- */
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {10, 10});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {10, 10});
    if (ImGui::BeginChild("EndButtons", ImVec2(-1, -1), ImGuiChildFlags_FrameStyle)) {
        ImGui::PopStyleColor();

        ImGui::SetWindowFontScale(1.2f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.975, 0.9, 1).Value);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.975, 0.82, 1).Value);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(0.975, 0.75, 1).Value);
        if (ImGui::Button("Reset", {avail.x / 2 - 15, -1})) {
            ResetParameters();
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        // Disable Apply button for 1.1 second after clicking it th(is is a driver "limitation")
        ImGui::BeginDisabled(!has_privilege || !was_initialized ||
                             duration_cast<milliseconds>(steady_clock::now() - last_apply_clicked).count() < 1100 ||
                             (selected_mode == AccelMode_Lut /* LUT */ && params[selected_mode].LUT_size == 0) ||
                             !functions[selected_mode].isValid);

        if (ImGui::Button("Apply", {-1, -1})) {
            params[selected_mode].SaveAll();
            functions[0] = functions[selected_mode];
            params[0] = params[selected_mode];
            used_mode = selected_mode;
            last_apply_clicked = steady_clock::now();
        }

        ImGui::EndDisabled();

        if (!functions[selected_mode].isValid) {
            ImGui::SetItemTooltip("Invalid parameters");
        }

        ImGui::SetWindowFontScale(1.f);
    } else
        ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    ImGui::EndChild();

    ImGui::EndGroup();

    if (!has_privilege)
        ImGui::GetForegroundDrawList()->AddText(ImVec2(10, ImGui::GetWindowHeight() - 40),
                                                ImColor::HSV(0.975, 0.9, 1).operator ImU32(),
                                                "Running without root privileges.\nSome functions will not be available");

    if (!was_initialized)
        ImGui::GetForegroundDrawList()->AddText(ImVec2(10, 55),
                                                ImColor::HSV(0.975, 0.9, 1).operator ImU32(),
                                                "Could not read and initialize driver parameters, working on dummy data");

    return 0;
}

Parameters start_params;

void ResetParameters(void) {
    for (int mode = 0; mode < NUM_MODES; mode++) {
        params[mode] = start_params;
        params[mode].accelMode = static_cast<AccelMode>(mode == 0 ? used_mode : mode);
        if (mode == AccelMode_CustomCurve) {
            params[mode].accelMode = AccelMode_Lut; // Custom curve is saved just like LUT, the only distinction is on the GUI side
        }

        if (mode == AccelMode_Lut) {
            memcpy(params[mode].LUT_data_x, start_params.LUT_data_x,
                   start_params.LUT_size * sizeof(params[selected_mode].LUT_data_x[0]));
            memcpy(params[mode].LUT_data_y, start_params.LUT_data_y,
                   start_params.LUT_size * sizeof(params[selected_mode].LUT_data_y[0]));
        }

        if (mode == AccelMode_Linear)
            params[mode].accel = fminf(0.1, params[mode].accel);

        if (mode == AccelMode_Classic)
            params[mode].exponent = fmaxf(fminf(params[mode].exponent, 5), 2.1);

        if (mode == AccelMode_Natural)
            params[mode].exponent = fmaxf(fminf(params[mode].exponent, 1), 0.01);

        if (mode == AccelMode_Jump)
            params[mode].exponent = fmaxf(fminf(params[mode].exponent, 1), 0.01);

        if (mode == AccelMode_Power)
            params[mode].exponent = fmaxf(fminf(params[mode].exponent, 1), 0.1);

        if (mode == AccelMode_CustomCurve) {
            params->customCurve.ApplyCurveConstraints();
            params[mode].LUT_size = params[mode].customCurve.ExportCurveToLUT(params[mode].LUT_data_x, params[mode].LUT_data_y);
            params[mode].customCurve.UpdateLUT();
        }

        functions[mode] = CachedFunction(((float) PLOT_X_RANGE) / PLOT_POINTS, &params[mode]);
        //printf("stride = %f\n", functions[mode].x_stride);
        bool old_use_ani = functions[mode].params->use_anisotropy;
        functions[mode].params->use_anisotropy = true;
        functions[mode].PreCacheFunc();
        functions[mode].params->use_anisotropy = old_use_ani;
    }
}

int main() {
    GUI::Setup(OnGui);
    ImPlot::CreateContext();

    ImGui::GetIO().IniFilename = nullptr;

    if (getuid()) {
        fprintf(stderr, "You are not a root!\n");
        has_privilege = false;
        //return 1;
    } else
        has_privilege = true;


    if (!DriverHelper::ValidateDirectory()) {
        fprintf(stderr,
                "YeetMouse directory doesnt exist!\nInstall the driver first, or check the parameters path.\n");
        return 2;
    }

    int fixed_num = 0;
    if (!DriverHelper::CleanParameters(fixed_num) && fixed_num != 0 && !has_privilege) {
        fprintf(stderr, "Could not setup driver params\n");
    } else {
        // Read driver parameters to a dummy aggregate
        DriverHelper::GetParameterF("Sensitivity", start_params.sens);
        DriverHelper::GetParameterF("SensitivityY", start_params.sensY);
        DriverHelper::GetParameterF("OutputCap", start_params.outCap);
        DriverHelper::GetParameterF("InputCap", start_params.inCap);
        DriverHelper::GetParameterF("Offset", start_params.offset);
        DriverHelper::GetParameterF("Acceleration", start_params.accel);
        DriverHelper::GetParameterF("Exponent", start_params.exponent);
        DriverHelper::GetParameterF("Midpoint", start_params.midpoint);
        DriverHelper::GetParameterF("PreScale", start_params.preScale);
        DriverHelper::GetParameterI("AccelerationMode", reinterpret_cast<int &>(start_params.accelMode));
        DriverHelper::GetParameterB("UseSmoothing", start_params.useSmoothing);
        DriverHelper::GetParameterI("LutSize", start_params.LUT_size);
        DriverHelper::GetParameterF("RotationAngle", start_params.rotation);
        start_params.rotation /= DEG2RAD;
        DriverHelper::GetParameterF("AngleSnap_Threshold", start_params.as_threshold);
        start_params.as_threshold /= DEG2RAD;
        DriverHelper::GetParameterF("AngleSnap_Angle", start_params.as_angle);
        start_params.as_angle /= DEG2RAD;
        //DriverHelper::GetParameterF("LutStride", start_params.LUT_stride);
        std::string Lut_dataBuf;
        DriverHelper::GetParameterS("LutDataBuf", Lut_dataBuf);
        Lut_dataBuf.copy(LUT_user_data, sizeof(LUT_user_data), 0);
        DriverHelper::ParseDriverLutData(Lut_dataBuf.c_str(), start_params.LUT_data_x, start_params.LUT_data_y);

        start_params.use_anisotropy = start_params.sensY != start_params.sens;

        used_mode = start_params.accelMode;

        selected_mode = static_cast<AccelMode>(start_params.accelMode % NUM_MODES);

        was_initialized = true;
    }


    ResetParameters();


    while (true) {
        if (GUI::RenderFrame())
            break;
    }

    GUI::ShutDown();

    return 0;
}
