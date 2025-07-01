#include "Tests.h"

#include <array>
#include <cmath>

#include "TestManager.h"
#include "driver/accel_modes.h"

#include "../gui/FunctionHelper.h"
#include "driver/config.h"
#include "shared_definitions.h"

//static CachedFunction functions[AccelMode_Count];

void Tests::Initialize() {
    // for (int mode = 1; mode < AccelMode_Count; mode++) {
    //     auto* params = new Parameters;
    //     params->sens = 1;
    //     params->sensY = 1;
    //     params->preScale = 1;
    //     params->accelMode = static_cast<AccelMode>(mode);
    //     functions[mode] = CachedFunction(0.1, params);
    //     functions[mode].PreCacheConstants();
    // }
}

bool Tests::TestAccelLinear(float range_min, float range_max) {
    bool result = true;

    TestManager::SetAccelMode(AccelMode_Linear);
    TestManager::SetAcceleration(0.0001f);
    TestManager::UpdateModesConstants();

    printf("Running test #1 for Linear Mode\n");

    for (int i = 0; i < BASIC_TEST_STEPS; i++) {
        float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
        auto res = TestManager::AccelLinear(value);

        result &= IsAccelValueGood(res);
        result &= IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value));
    }

    printf("Test #1: %s\n" RESET, result ? GREEN "Passed" : RED "Failed");

    printf("\n");
    return true;
}

bool Tests::TestAccelPower(float range_min, float range_max) {
    bool result = true;

    TestManager::SetAccelMode(AccelMode_Power);
    try {

        bool temp_res = true;
        printf("Running test #1 for Power Mode\n");

        TestManager::SetAcceleration(5.0f);
        TestManager::SetExponent(0.01f);
        TestManager::SetMidpoint(1.0f);
        TestManager::UpdateModesConstants();

        if (!TestManager::ValidateConstants()) {
            fprintf(stderr, "Invalid constants\n");
            temp_res = false;
        }

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelPower(value);
            //printf("%f, %f, %f\n", value, FP64_ToFloat(res), TestManager::EvalFloatFunc(value));

            temp_res &= IsAccelValueGood(res);
            temp_res &= IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value));
        }

        printf("Test #1: %s\n" RESET, temp_res ? GREEN "Passed" : RED "Failed");
        result &= temp_res;
        temp_res = true;

        printf("Running test #2 for Power Mode\n");

        TestManager::SetAccelMode(AccelMode_Power);
        TestManager::SetAcceleration(5.0f);
        TestManager::SetExponent(1.0f);
        TestManager::SetMidpoint(5.0f);
        TestManager::UpdateModesConstants();

        if (!TestManager::ValidateConstants()) {
            fprintf(stderr, "Invalid constants\n");
            temp_res = false;
        }

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelPower(value);
            //printf("%f, %f, %f\n", value, FP64_ToFloat(res), TestManager::EvalFloatFunc(value));

            temp_res &= IsAccelValueGood(res);
            temp_res &= IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value));
        }

        printf("Test #2: %s\n" RESET, temp_res ? GREEN "Passed" : RED "Failed");
        result &= temp_res;
        temp_res = true;

        printf("Running test #3 for Power Mode\n");

        // Technically bad values (not possible through GUI)
        TestManager::SetAccelMode(AccelMode_Power);
        TestManager::SetAcceleration(50.f);
        TestManager::SetExponent(0.001f);
        TestManager::SetMidpoint(5.0f);
        TestManager::UpdateModesConstants();

        if (TestManager::ValidateConstants()) {
            fprintf(stderr, "Valid constants (Should be invalid)\n");
            temp_res = false;
        }

        // GUI Validation should reject these settings
        if (TestManager::ValidateFunctionGUI())
            temp_res = false;

        printf("Test #3: %s\n" RESET, temp_res ? GREEN "Passed" : RED "Failed");
        result &= temp_res;
        temp_res = true;


        printf("Running test #4 for Power Mode\n");

        TestManager::SetAccelMode(AccelMode_Power);

        for (int i = 0; i < BASIC_TEST_STEPS_REDUCED; i++) {
            float t1 = static_cast<float>(i) / BASIC_TEST_STEPS_REDUCED;
            for (int j = 0; j < BASIC_TEST_STEPS_REDUCED; j++) {
                float t2 = static_cast<float>(j) / BASIC_TEST_STEPS_REDUCED;
                float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS_REDUCED;
                auto res = TestManager::AccelPower(value, 1, lerp(0.1f, 1, t1), lerp(0.01f, 8, t2));
                // printf("arg_a: %f, arg_b: %f\n", lerp(0.1f, 1, t1), lerp(0.01f, 8, t2));
                // printf("%f, %f, %f\n", value, FP64_ToFloat(res), TestManager::EvalFloatFunc(value));

                // auto modes_const = TestManager::GetModesConstants();
                //printf("offset_x: %f, power_const: %f\n", FP64_ToFloat(modes_const.offset_x), FP64_ToFloat(modes_const.power_constant));

                temp_res &= IsAccelValueGood(res);
                temp_res &= IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value));
            }
        }

        printf("Test #4: %s\n" RESET, temp_res ? GREEN "Passed" : RED "Failed");
        result &= temp_res;
    }
    catch (std::exception &ex) {
        fprintf(stderr, "Exception: %s, in Power mode\n", ex.what());
        result = false;
    }

    printf("\n");
    return result;
}

bool Tests::TestAccelClassic(float range_min, float range_max) {
    bool result = true;
    TestManager::SetAccelMode(AccelMode_Classic);
    TestManager::SetAcceleration(0.01f);
    TestManager::SetExponent(4.f);
    TestManager::UpdateModesConstants();

    printf("Running test #1 for Classic Mode\n");

    for (int i = 0; i < BASIC_TEST_STEPS; i++) {
        float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
        auto res = TestManager::AccelClassic(value);

        result &= IsAccelValueGood(res);
        result &= IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value));
    }

    printf("Test #1: %s\n" RESET, result ? GREEN "Passed" : RED "Failed");

    printf("\n");
    return result;
}

bool Tests::TestAccelMotivity(float range_min, float range_max) {
    bool result = true;

    try {
        TestManager::SetAccelMode(AccelMode_Motivity);
        TestManager::SetAcceleration(4.f);
        TestManager::SetMidpoint(0.f);
        TestManager::UpdateModesConstants();

        printf("Running test #1 for Motivity Mode\n");

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelMotivity(value);

            result &= IsAccelValueGood(res);
            result &= IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value));
        }

        printf("Test #1: %s\n" RESET, result ? GREEN "Passed" : RED "Failed");
    }
    catch (std::exception &ex) {
        fprintf(stderr, "Exception: %s, in Motivity mode\n", ex.what());
        result = false;
    }

    printf("\n");
    return result;
}

bool Tests::TestAccelNatural(float range_min, float range_max) {
    bool result = true;

    try {
        TestManager::SetAccelMode(AccelMode_Natural);
        TestManager::SetAcceleration(1.02f);
        TestManager::SetExponent(5.f);
        TestManager::UpdateModesConstants();
	TestManager::SetMidpoint(0.f);
	TestManager::SetUseSmoothing(false);

        printf("Running test #1 for Natural Mode\n");

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelNatural(value);

            result &= IsAccelValueGood(res);
            result &=
                IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value));
	    printf("res: %f\n", FP64_ToFloat(res));
            printf("val function: %f\n", TestManager::EvalFloatFunc(value));

        }

        printf("Test #1: %s\n" RESET, result ? GREEN "Passed" : RED "Failed");

        // Test 2
        result = true;
        TestManager::SetAccelMode(AccelMode_Natural);
        TestManager::SetAcceleration(0.041f);
        TestManager::SetExponent(3.f);
        TestManager::SetMidpoint(6.f);
        TestManager::SetUseSmoothing(true);
        TestManager::UpdateModesConstants();

        printf("Running test #2 for Natural Mode\n");

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelNatural(value);

            result &= IsAccelValueGood(res);
            result &=
                IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value));
            // printf("res: %f\n", FP64_ToFloat(res));
            // printf("val function: %f\n", TestManager::EvalFloatFunc(value));

        }

        printf("Test #2: %s\n" RESET, result ? GREEN "Passed" : RED "Failed");

        // Test 3
        result = true;
        TestManager::SetAccelMode(AccelMode_Natural);
        TestManager::SetAcceleration(0.2f);
        TestManager::SetExponent(5.f);
        TestManager::SetMidpoint(0.f);
        TestManager::SetUseSmoothing(false);
        TestManager::UpdateModesConstants();

        printf("Running test #3 for Natural Mode\n");

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelNatural(value);

            result &= IsAccelValueGood(res);
            result &=
                IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value));
        }

        printf("Test #3: %s\n" RESET, result ? GREEN "Passed" : RED "Failed");


    }
    catch (std::exception &ex) {
        fprintf(stderr, "Exception: %s, in Natural mode\n", ex.what());
        result = false;
    }

    printf("\n");
    return result;
}

bool Tests::TestAccelJump(float range_min, float range_max) {
    bool result = true;

    try {
        TestManager::SetAccelMode(AccelMode_Jump);
        TestManager::SetAcceleration(4.f);
        TestManager::SetMidpoint(0.01f);
        TestManager::SetExponent(0.95f); // Smoothness
        TestManager::UpdateModesConstants();

        printf("Running test #1 for Jump Mode\n");
        bool temp_res = true;

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelJump(value);

            temp_res &= IsAccelValueGood(res);
            temp_res &= IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value));
        }

        printf("Test #1: %s\n" RESET, temp_res ? GREEN "Passed" : RED "Failed");
        result &= temp_res;
        temp_res = true;

        TestManager::SetAccelMode(AccelMode_Jump);
        TestManager::SetAcceleration(4.f);
        TestManager::SetMidpoint(0.01f);
        TestManager::SetExponent(0.01f); // Smoothness
        TestManager::UpdateModesConstants();

        printf("Running test #2 for Jump Mode\n");

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelJump(value);

            temp_res &= IsAccelValueGood(res);
            temp_res &= IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value));
        }

        printf("Test #2: %s\n" RESET, temp_res ? GREEN "Passed" : RED "Failed");
        result &= temp_res;
        temp_res = true;


        TestManager::SetAccelMode(AccelMode_Jump);
        TestManager::SetMidpoint(0.00f);
        TestManager::SetExponent(0.01f); // Smoothness
        TestManager::UpdateModesConstants();

        printf("Running test #3 for Jump Mode\n");

        if (TestManager::ValidateConstants()) { // Should be invalid!
            fprintf(stderr, "Valid constants (should be invalid)\n");
            temp_res = false;
        }

        printf("Test #3: %s\n" RESET, temp_res ? GREEN "Passed" : RED "Failed");
        result &= temp_res;
        temp_res = true;


        TestManager::SetAccelMode(AccelMode_Jump);
        TestManager::SetMidpoint(0.01f);
        TestManager::SetExponent(0.0f); // Smoothness
        TestManager::UpdateModesConstants();

        printf("Running test #4 for Jump Mode\n");

        if (TestManager::ValidateConstants()) { // Should be invalid!
            fprintf(stderr, "Valid constants (should be invalid)\n");
            temp_res = false;
        }

    }
    catch (std::exception &ex) {
        fprintf(stderr, "Exception: %s, in Jump mode\n", ex.what());
        return false;
    }

    printf("\n");
    return result;
}

bool Tests::TestAccelLUT(float range_min, float range_max) {
    bool result = true;
    try {
        TestManager::SetAccelMode(AccelMode_Lut);
        float values_x[4] = {1, 20, 20, 40};
        float values_y[4] = {1, 1, 2, 2};
        TestManager::SetLutData(values_x, values_y, 4);
        TestManager::UpdateModesConstants();

        bool temp_res = true;

        printf("Running test #1 for LUT Mode\n");

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelLUT(value);

            //printf("%f, %f, %f\n", value, FP64_ToFloat(res), TestManager::EvalFloatFunc(value));

            temp_res &= IsAccelValueGood(res);
            temp_res &= IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value));
        }

        printf("Test #1: %s\n" RESET, temp_res ? GREEN "Passed" : RED "Failed");
        result &= temp_res;
        temp_res = true;

        float values_x2[] = {1, 20, 20, 40, 40};
        float values_y2[] = {1, 1, 2, 2, 3};
        TestManager::SetAccelMode(AccelMode_Lut);
        TestManager::SetLutData(values_x2, values_y2, sizeof(values_x2) / sizeof(float));
        TestManager::UpdateModesConstants();

        printf("Running test #2 for LUT Mode\n");

        if (TestManager::ValidateConstants()) { // Should be invalid!
            fprintf(stderr, "Valid constants (should be invalid)\n");
            temp_res = false;
        }

        printf("Test #2: %s\n" RESET, temp_res ? GREEN "Passed" : RED "Failed");
        result &= temp_res;
        temp_res = true;

        float values_x3[] = {1, 20, 20, 40, 40};
        float values_y3[] = {1, 1, 2, 2, 2};
        TestManager::SetAccelMode(AccelMode_Lut);
        TestManager::SetLutData(values_x3, values_y3, sizeof(values_x3) / sizeof(float));
        TestManager::UpdateModesConstants();

        printf("Running test #3 for LUT Mode\n");

        if (TestManager::ValidateConstants()) { // Should be invalid!
            fprintf(stderr, "Valid constants (should be invalid)\n");
            temp_res = false;
        }

        printf("Test #3: %s\n" RESET, temp_res ? GREEN "Passed" : RED "Failed");
        result &= temp_res;
        temp_res = true;


        float values_x4[] = {1, 20, 40, 20, 40};
        float values_y4[] = {1, 1, 3, 2, 2};
        TestManager::SetAccelMode(AccelMode_Lut);
        TestManager::SetLutData(values_x4, values_y4, sizeof(values_x4) / sizeof(float));
        TestManager::UpdateModesConstants();

        printf("Running test #4 for LUT Mode\n");

        if (TestManager::ValidateConstants()) { // Should be invalid!
            fprintf(stderr, "Valid constants (should be invalid)\n");
            temp_res = false;
        }

        printf("Test #4: %s\n" RESET, temp_res ? GREEN "Passed" : RED "Failed");
        result &= temp_res;
        temp_res = true;

        float values_x5[] = {1};
        float values_y5[] = {2};
        TestManager::SetAccelMode(AccelMode_Lut);
        TestManager::SetLutData(values_x5, values_y5, sizeof(values_x5) / sizeof(float));
        TestManager::UpdateModesConstants();

        printf("Running test #5 for LUT Mode\n");

        if (TestManager::ValidateConstants()) { // Should be invalid!
            fprintf(stderr, "Valid constants (should be invalid)\n");
            temp_res = false;
        }

        printf("Test #5: %s\n" RESET, temp_res ? GREEN "Passed" : RED "Failed");
        result &= temp_res;
    }
    catch (std::exception &ex) {
        fprintf(stderr, "Exception: %s, in LUT mode\n", ex.what());
        return false;
    }


    printf("\n");
    return result;
}

bool Tests::TestAccelMode(AccelMode mode, float range_min, float range_max) {
    switch (mode) {
        case AccelMode_Linear:
            return TestAccelLinear(range_min, range_max);
        case AccelMode_Power:
            return TestAccelPower(range_min, range_max);
        case AccelMode_Classic:
            return TestAccelClassic(range_min, range_max);
        case AccelMode_Motivity:
            return TestAccelMotivity(range_min, range_max);
        case AccelMode_Natural:
            return TestAccelNatural(range_min, range_max);
        case AccelMode_Jump:
            return TestAccelJump(range_min, range_max);
        case AccelMode_Lut:
            return TestAccelLUT(range_min, range_max);
        case AccelMode_CustomCurve:
            return true; // Can't really test custom curves on the driver side, as it's just a LUT
        default:
            fprintf(stderr, "Unknown mode (%i), skipping!\n", mode);
    }

    return true;
}

std::array<bool, AccelMode_Count> Tests::TestAllBasic(float range_min, float range_max) {
    std::array<bool, AccelMode_Count> results {true}; // AccelMode_Current is always true

    for (int mode = 1; mode < AccelMode_Count; mode++) {
        results[mode] = TestAccelMode(static_cast<AccelMode>(mode), range_min, range_max);
    }

    return results;
}

bool Tests::IsAccelValueGood(FP_LONG value) {
    if (value >= FP64_FromInt(100000) || value < 0)
        return false;

    return true;
}

bool Tests::IsCloseEnough(FP_LONG value1, float value2, float tolerance) {
    return std::abs(FP64_ToFloat(value1) - value2) < tolerance;
}

bool Tests::IsCloseEnoughRelative(FP_LONG value1, float value2, float tolerance) {
    return std::abs(FP64_ToFloat(value1) - value2) / std::abs(value2) < tolerance;
}

float Tests::lerp(float a, float b, float t) {
    return a + t * (b - a);
}
