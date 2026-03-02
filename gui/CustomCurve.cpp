#include "CustomCurve.h"

#include <algorithm>
#include <vector>

#include "DriverHelper.h"
#include "External/ImGui/imgui_internal.h"

void CustomCurve::ApplyCurveConstraints() {
    for (int i = 0; i < points.size(); i++) {
        float p_min = i > 0 ? points[i - 1].x + 0.5f : 0;
        float p_max = i < points.size() - 1 ? points[i + 1].x - 0.5f : 1000;

        points[i].x = std::clamp(points[i].x, p_min, p_max);

        if (i < points.size() - 1)
            control_points[i][0].x = std::clamp(control_points[i][0].x,
                                                points[i].x + CURVE_POINTS_MARGIN,
                                                points[i + 1].x - CURVE_POINTS_MARGIN);
        if (i > 0)
            control_points[i - 1][1].x = std::clamp(control_points[i - 1][1].x,
                                                    points[i - 1].x + CURVE_POINTS_MARGIN,
                                                    points[i].x - CURVE_POINTS_MARGIN);
    }
}

// Cubic Bezier derivatives
ImVec2 BezierFirstOrderDerivative(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t) {
    float u = (1 - t);
    float w1 = 3 * (u * u);
    float w2 = 6 * (u * t);
    float w3 = 3 * (t * t);
    return ((p1 - p0) * w1) + ((p2 - p1) * w2) + ((p3 - p2) * w3);
}

ImVec2 BezierSecondOrderDerivative(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t) {
    float u = (1 - t);
    float w1 = 6 * u;
    float w2 = 6 * t;
    return (p2 - p1 * 2 + p0) * w1 + (p3 - p2 * 2 + p1) * w2;
}

// Spreads points based on the rate of change and other things
// Sorry for the shear number of magic numbers in this function, there is just a lot to configure
int CustomCurve::ExportCurveToLUT(double *LUT_data_x, double *LUT_data_y) const {
    const float TIME_ADAPTIVE_FACTOR = 0.5;
    const int PRE_LUT_ARRAY_SIZE = 100;
    const float LENGTH_WEIGHT = 0.95;
    int LUT_size = 0;

    if (points.size() <= 1)
        return LUT_size;

    LUT_size = 0;

    std::vector<float> curve_arc_len((points.size() - 1) * PRE_LUT_ARRAY_SIZE);
    size_t idx = 0;
    std::vector<double> seg_len(points.size() - 1); // normalized
    double len_sum = 0;
    // Pre calculate each segments' length
    auto last_p = points[0];
    for (int i = 0; i < points.size() - 1; i++) {
        seg_len[i] = 0;
        curve_arc_len[idx++] = 0;
        for (int j = 1; j < PRE_LUT_ARRAY_SIZE; j++) {
            const double t = j / (PRE_LUT_ARRAY_SIZE - 1.0);
            ImVec2 p = ImBezierCubicCalc(points[i], control_points[i][0], control_points[i][1], points[i + 1], t);
            ImVec2 dist = p - last_p;
            dist.y *= 30; // The graph is stretched horizontally, this accounts for that
            curve_arc_len[idx] = curve_arc_len[idx - 1] + std::sqrt(ImLengthSqr(dist));
            seg_len[i] += std::sqrt(ImLengthSqr(dist));
            last_p = p;
            idx++;
        }
        len_sum += seg_len[i];
    }

    // Normalize lengths [0,1]
    for (auto &seg: seg_len)
        seg = seg / len_sum * (points.size() - 1);

    // Linearize Bezier's domain (http://www.planetclegg.com/projects/WarpingTextToSplines.html)
    // Make a LUT based on arc length of the Bezier curve for later lookup
    auto linear_map_t = [&curve_arc_len](float tg_arc_len, size_t start) {
        int l = 0, r = PRE_LUT_ARRAY_SIZE - 1;
        int index = 0;
        while (l < r) {
            index = (l + r) / 2;
            if (curve_arc_len[start + index] < tg_arc_len) {
                l = index + 1;
            } else {
                r = index;
            }
        }
        if (curve_arc_len[start + index] > tg_arc_len) {
            index--;
        }

        float lengthBefore = curve_arc_len[start + index];
        if (lengthBefore == tg_arc_len) {
            return (float) index / (PRE_LUT_ARRAY_SIZE - 1);
        }

        // interpolate
        return (index + (tg_arc_len - lengthBefore) / (curve_arc_len[start + index + 1] - lengthBefore)) / (
                   PRE_LUT_ARRAY_SIZE - 1);
    };

    // Using premade LUT
    double u = 0; // [0,1]
    constexpr double U_STEP_FACTOR = 0.1;
    constexpr double u_step = U_STEP_FACTOR / (MAX_LUT_ARRAY_SIZE - 1);
    float last_added_u = 0;
    double target_u = 0; // Point at which to actually add a point (dynamically changes)
    for (int i = 0; LUT_size < (MAX_LUT_ARRAY_SIZE - 1) && u < 1.f; i++) {
        float t = 0;
        //u = ((float)i / (MAX_LUT_ARRAY_SIZE - 1));

        int edge_idx = (int) (u * (points.size() - 1));

        float local_u = u * (points.size() - 1) - edge_idx;
        //printf("u = %f, edge_idx = %d, local_u = %f\n", u, edge_idx, local_u);
        float tg_arc_len = local_u * curve_arc_len[(edge_idx + 1) * PRE_LUT_ARRAY_SIZE - 1];

        int start = edge_idx * PRE_LUT_ARRAY_SIZE;
        t = linear_map_t(tg_arc_len, start);

        auto p = ImBezierCubicCalc(points[edge_idx], control_points[edge_idx][0], control_points[edge_idx][1],
                                   points[edge_idx + 1], t);
        auto dp = BezierFirstOrderDerivative(points[edge_idx], control_points[edge_idx][0], control_points[edge_idx][1],
                                             points[edge_idx + 1], t);
        auto d2p = BezierSecondOrderDerivative(points[edge_idx], control_points[edge_idx][0],
                                               control_points[edge_idx][1], points[edge_idx + 1], t);

        // rate of change = curvature of the curve at time t
        float rate_of_change = 1;
        if (i > 0) {
            rate_of_change = std::sqrt(
                std::fabs(dp.x * d2p.y - dp.y * d2p.x) / std::pow(dp.y * dp.y * 200 + dp.x * dp.x, 3.0 / 2.0) * 250);
            // Curvature with respect to t, + a sqrt to remap change the characteristics
        }
        float rate_of_change_clamped = ImClamp(rate_of_change, 0.f, 1.0f);

        if (u + 0.000002 >= target_u) {
            LUT_data_x[LUT_size] = p.x;
            LUT_data_y[LUT_size] = p.y;

            LUT_size++;

            last_added_u = u;
        }

        if (t <= 0.0001)
            u = (double) i / (MAX_LUT_ARRAY_SIZE - 1) * U_STEP_FACTOR + u_step;
            // recalibrate position, it's like with IMUs and GPS, this is the GPS
        else
            u += u_step;

        // Adaptive points distribution
        // Function f that determines how much points to put on a given edge depending on how close (slow) it is.
        // This function (f) has to satisfy the following statement Integral{0->1}f(x)dx = 1
        // For example f(x) = ax + (1-0.5a)
        double adaptive_time_step = (TIME_ADAPTIVE_FACTOR * p.x / points.back().x + (1.f - TIME_ADAPTIVE_FACTOR * 0.5f))
                                    * u_step;
        double next_u_step = adaptive_time_step / ImLerp(1., seg_len[edge_idx], LENGTH_WEIGHT);
        target_u = last_added_u + ImLerp(next_u_step * 4, next_u_step * 0.9, rate_of_change_clamped) / U_STEP_FACTOR;
        // Clamp u to the start of the next point [u = edge / (size-1)]
        if (u + 0.000002 > (edge_idx + 1.0f) / (points.size() - 1)) {
            target_u = (edge_idx + 1.0f) / (points.size() - 1);
            u = target_u + 0.00002;
        }
    }

    LUT_data_x[LUT_size] = points.back().x;
    LUT_data_y[LUT_size++] = points.back().y;

    return LUT_size;
}

// ctrl_p -> control_point
// Format: point[i].x;point[i].y;ctrl_p[i][0].x,ctrl_p[i][0].y;ctrl_p[i][1].x;ctrl_p[i][1].y;point[i+1].x;point[i+1].y;ctrl_p[i][0].x;ctrl_p[i][0].y;ctrl_p[i][1].x;ctrl_p[i][1].y;point[i+1].x.y;...point[n-1].x;point[n-1];
std::string CustomCurve::ExportCustomCurve() const {
    std::stringstream out_stream;

    if (points.size() < 2)
        return "";

    out_stream << std::fixed << std::setprecision(CURVE_EXPORT_PRECISION);

    try {
        for (int i = 0; i < points.size(); i++) {
            out_stream << points[i].x << ";" << points[i].y << ";";

            // Print control points
            if (i < points.size() - 1) {
                for (auto & cp : control_points[i]) {
                    out_stream << cp.x << ";" << cp.y << ";";
                }
                out_stream.seekp(-1, std::ios_base::cur); // remove the last comma
                out_stream << ";";
            }
        }
    }
    catch (std::exception& exception) {
        fprintf(stderr, "Error while exporting curve: %s\n", exception.what());
        return "";
    }

    if (out_stream.tellp() >= MAX_LUT_BUF_LEN)
        return "";

    return out_stream.str();
}

// Same format as the function above (CustomCurve::ExportCustomCurve)
bool CustomCurve::ImportCustomCurve(const std::string& data) {
    std::stringstream ss(data);
    points.clear();
    control_points.clear();
    size_t idx = 0;

    try {
        float p_x = 0, p_y = 0;
        double p = 0;
        while (idx < MAX_LUT_ARRAY_SIZE && ss >> p) {
            if (idx % 2 == 0)
                p_x = p;
            else {
                p_y = p;
                if (idx % 6 == 1) {
                    points.emplace_back(p_x, p_y);
                }
                else {
                    if (idx % 6 == 3)
                        control_points.emplace_back();
                    auto j = idx % 6 - 2;
                    control_points.back()[j / 2] = {p_x, p_y};
                }
            }


            char nextC = ss.peek();
            if (nextC == ';' || nextC == ',')
                ss.ignore();

            idx++;
        }
    }
    catch (std::exception& exception) {
        return false;
    }

    // 1 element is not enough for a linear interpolation
    if (idx <= 2 || idx % 6 + 2 != 0) {
        return false;
    }

    return true;
}

// https://www.codeproject.com/Articles/31859/Draw-a-Smooth-Curve-through-a-Set-of-2D-Points-wit
void CustomCurve::SmoothBezier() {
    //bezier_control_points.clear();
    auto &center_points = points;
    auto &bezier_control_points = control_points;

    int n = center_points.size() - 1;
    if (n < 1) {
        bezier_control_points.clear();
        return;
    }

    if (n == 1) {
        bezier_control_points.resize(2);
        // Special case: Bezier curve should be a straight line.

        // 3P1 = 2P0 + P3
        bezier_control_points[0][0] = (center_points[0] * 2 + center_points[1]) / 3;

        // P2 = 2P1 – P0
        bezier_control_points[0][1] = bezier_control_points[0][0] * 2 - center_points[0];

        return;
    }

    center_points.emplace_back(center_points[n] * 2 - center_points[n - 1]); // Push dummy point at the end

    n += 1;
    bezier_control_points.resize(n);

    // Calculate first Bezier control points
    // Right hand side vector
    ImVec2 *rhs = new ImVec2[n];

    // Set right hand side X values
    for (int i = 1; i < n - 1; ++i)
        rhs[i] = center_points[i] * 4 + center_points[i + 1] * 2;
    rhs[0] = center_points[0] + center_points[1] * 2;
    rhs[n - 1] = (center_points[n - 1] * 8 + center_points[n]) / 2.0;
    // Get first control points

    ImVec2 *x = new ImVec2[n]; // Solution vector.
    ImVec2 *tmp = new ImVec2[n]; // Temp workspace.

    ImVec2 b = {2.0, 2.0};
    x[0] = rhs[0] / b;
    for (int i = 1; i < n; i++) // Decomposition and forward substitution.
    {
        tmp[i] = ImVec2(1, 1) / b;
        b = (i < n - 1 ? ImVec2(4, 4) : ImVec2(3.5, 3.5)) - tmp[i];
        x[i] = (rhs[i] - x[i - 1]) / b;
    }
    for (int i = 1; i < n; i++)
        x[n - i - 1] -= tmp[n - i] * x[n - i]; // Backsubstitution.


    for (int i = 0; i < n; ++i) {
        // First control point
        if (!center_points[i].is_locked)
            bezier_control_points[i][0] = x[i];
        // Second control point
        if (i == n - 1 || !center_points[i + 1].is_locked) {
            if (i < n - 1) {
                bezier_control_points[i][1] = center_points[i + 1] * 2 - x[i + 1];
            } else
                bezier_control_points[i][1] = center_points[n] + x[n - 1] / 2;
        }
    }

    center_points.pop_back(); // Pop the back dummy
    bezier_control_points.pop_back();

    ApplyCurveConstraints();

    delete[] rhs;
    delete[] x;
    delete[] tmp;
}

void CustomCurve::UpdateLUT() {
    if (points.size() < 2)
        return;
    LUT_points.resize((points.size() - 1) * BEZIER_FRAG_SEGMENTS);
    ImVec2 last_point = points[0];
    for (int i = 0; i < points.size() - 1; i++) {
        for (int j = 0; j < BEZIER_FRAG_SEGMENTS; ++j) {
            float t = (float) j / (BEZIER_FRAG_SEGMENTS - 1);
            ImVec2 p = ImBezierCubicCalc(points[i], control_points[i][0], control_points[i][1], points[i + 1], t);
            LUT_points[i * BEZIER_FRAG_SEGMENTS + j] = p;
            if (last_point.x > p.x) {
                // Bad Curve
            }
            last_point = p;
        }
    }
}
