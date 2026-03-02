#ifndef CUSTOMCURVE_H
#define CUSTOMCURVE_H

#include "External/ImGui/imgui.h"
#include <deque>
#include <array>
#include <string>
#include <vector>

#include "External/ImGui/implot.h"

#define CURVE_POINTS_MARGIN 0.2f
#define BEZIER_FRAG_SEGMENTS 50
#define CURVE_EXPORT_PRECISION 3 // Decimal points precision for exporting Custom Curves

struct Ex_Vec2 : ImVec2 {
    bool is_locked = false;
    bool use_polar_coordinates = false;

    Ex_Vec2(float x, float y) : ImVec2(x, y) {}
    Ex_Vec2(ImVec2 vec) : ImVec2(vec) {}
    Ex_Vec2() : ImVec2(0, 0) {}
};

class CustomCurve {
public:
    std::deque<Ex_Vec2> points{{5, 1}, {50, 2}}; // actual points
    std::deque<std::array<ImVec2, 2> > control_points{std::array<ImVec2, 2>({ImVec2{40, 1}, ImVec2{20, 2}})};
    std::vector<ImPlotPoint> LUT_points{};

    CustomCurve() = default;

    // Constraints the curve to be aligned with the "mathematical" definition of a function x -> f(x)
    void ApplyCurveConstraints();

    // Tries to optimally distribute the points for the exported LUT
    int ExportCurveToLUT(double *LUT_data_x, double *LUT_data_y) const;

    // Exports the custom curve points raw (not as a LUT)
    std::string ExportCustomCurve() const;

    // Exports the custom curve points
    bool ImportCustomCurve(const std::string& data);

    // Makes first and second derivative continuous
    void SmoothBezier();

    // Builds a LUT for a fast curve plotting (stored in LUT_points), no "fancy" algorithms here
    void UpdateLUT();
};


#endif //CUSTOMCURVE_H
