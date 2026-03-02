#include "Tests.h"

#include <array>
#include <cmath>

#include "TestManager.h"
#include "driver/accel_modes.h"

#include "../gui/FunctionHelper.h"
#include "driver/config.h"

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
    TestSupervisor supervisor{"Linear Mode"};

    try {
        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Linear);
        TestManager::SetAcceleration(0.0001f);
        TestManager::SetUseSmoothing(false);
        TestManager::SetMidpoint(0.f);
        TestManager::UpdateModesConstants();

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelLinear(value);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
        }

        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Linear);
        TestManager::SetAcceleration(0.0001f);
        TestManager::SetUseSmoothing(true);
        TestManager::SetMidpoint(0.f);
        TestManager::UpdateModesConstants();

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelLinear(value);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
        }

        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Linear);
        TestManager::SetAcceleration(0.5f);
        TestManager::SetUseSmoothing(true);
        TestManager::SetMidpoint(2.f);
        TestManager::UpdateModesConstants();

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelLinear(value);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
        }

        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Linear);
        TestManager::SetAcceleration(1.f);
        TestManager::SetUseSmoothing(true);
        TestManager::SetMidpoint(1.5f);
        TestManager::UpdateModesConstants();

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelLinear(value);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
        }

        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Linear);
        TestManager::SetAcceleration(5.f);
        TestManager::SetUseSmoothing(true);
        TestManager::SetMidpoint(6.f);
        TestManager::UpdateModesConstants();

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelLinear(value);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
        }

        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Linear);
        TestManager::SetAcceleration(0.0f);
        TestManager::SetUseSmoothing(true);
        TestManager::SetMidpoint(2.f);
        TestManager::UpdateModesConstants();

        if (TestManager::ValidateConstants()) {
            // Should be invalid!
            fprintf(stderr, "Valid constants (should be invalid)\n");
            supervisor.result = false;
        }
    } catch (std::exception &ex) {
        fprintf(stderr, "Exception: %s, in Linear mode\n", ex.what());
        supervisor.result = false;
    }

    return supervisor.GetResult();
}

bool Tests::TestAccelPower(float range_min, float range_max) {
    TestSupervisor supervisor{"Power Mode"};

    try {
        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Power);
        TestManager::SetAcceleration(5.0f);
        TestManager::SetExponent(0.01f);
        TestManager::SetMidpoint(1.0f);
        TestManager::SetUseSmoothing(false);
        TestManager::UpdateModesConstants();

        if (!TestManager::ValidateConstants()) {
            fprintf(stderr, "Invalid constants\n");
            supervisor.result = false;
        }

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelPower(value);
            //printf("%f, %f, %f\n", value, FP64_ToFloat(res), TestManager::EvalFloatFunc(value));

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
        }

        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Power);
        TestManager::SetAcceleration(5.0f);
        TestManager::SetExponent(1.0f);
        TestManager::SetMidpoint(5.0f);
        TestManager::SetUseSmoothing(false);
        TestManager::UpdateModesConstants();

        if (!TestManager::ValidateConstants()) {
            fprintf(stderr, "Invalid constants\n");
            supervisor.result = false;
        }

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelPower(value);
            //printf("%f, %f, %f\n", value, FP64_ToFloat(res), TestManager::EvalFloatFunc(value));

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
        }

        supervisor.NextTest();

        // Technically bad values (not possible through GUI)
        TestManager::SetAccelMode(AccelMode_Power);
        TestManager::SetAcceleration(50.f);
        TestManager::SetExponent(0.001f);
        TestManager::SetMidpoint(5.0f);
        TestManager::SetUseSmoothing(false);
        TestManager::UpdateModesConstants();

        if (TestManager::ValidateConstants()) {
            fprintf(stderr, "Valid constants (Should be invalid)\n");
            supervisor.result = false;
        }

        // GUI Validation should reject these settings
        if (TestManager::ValidateFunctionGUI())
            supervisor.result = false;

        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Power);

        for (int i = 0; i < BASIC_TEST_STEPS_REDUCED; i++) {
            float t1 = static_cast<float>(i) / BASIC_TEST_STEPS_REDUCED;
            for (int j = 0; j < BASIC_TEST_STEPS_REDUCED; j++) {
                float t2 = static_cast<float>(j) / BASIC_TEST_STEPS_REDUCED;
                float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS_REDUCED;
                auto res = TestManager::AccelPower(value, 1, lerp(0.1f, 1, t1), lerp(0.01f, 8, t2), 0, false);
                // printf("arg_a: %f, arg_b: %f\n", lerp(0.1f, 1, t1), lerp(0.01f, 8, t2));
                // printf("%f, %f, %f\n", value, FP64_ToFloat(res), TestManager::EvalFloatFunc(value));

                // auto modes_const = TestManager::GetModesConstants();
                //printf("offset_x: %f, power_const: %f\n", FP64_ToFloat(modes_const.offset_x), FP64_ToFloat(modes_const.power_constant));

                supervisor.Validate(IsAccelValueGood(res));
                supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
            }
        }

        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Power);
        TestManager::SetAcceleration(5.0f);
        TestManager::SetExponent(1.0f);
        TestManager::SetMidpoint(0.1f);
        TestManager::SetMotivity(1.5f);
        TestManager::SetUseSmoothing(true);
        TestManager::UpdateModesConstants();

        if (!TestManager::ValidateConstants()) {
            fprintf(stderr, "Invalid constants\n");
            supervisor.result = false;
        }

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelPower(value);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
        }

        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Power);
        TestManager::SetAcceleration(8.0f);
        TestManager::SetExponent(0.2f);
        TestManager::SetMidpoint(0.1f);
        TestManager::SetMotivity(1.5f);
        TestManager::SetUseSmoothing(true);
        TestManager::UpdateModesConstants();

        if (!TestManager::ValidateConstants()) {
            fprintf(stderr, "Invalid constants\n");
            supervisor.result = false;
        }

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelPower(value);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
        }

        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Power);
        TestManager::SetAcceleration(0.12f);
        TestManager::SetExponent(0.21f);
        TestManager::SetMidpoint(0.9f);
        TestManager::SetMotivity(0.92f);
        TestManager::SetUseSmoothing(true);
        TestManager::UpdateModesConstants();

        if (!TestManager::ValidateConstants()) {
            fprintf(stderr, "Invalid constants\n");
            supervisor.result = false;
        }

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelPower(value);
            // printf("x: %f, Fixed Point: %f, Floating Point: %f, error: %f, value%i\n", value, FP64_ToFloat(res), TestManager::EvalFloatFunc(value), FP64_ToFloat(res) - TestManager::EvalFloatFunc(value), IsAccelValueGood(res));

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
            // printf("Validation: %i\n", supervisor.result);
        }

        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Power);
        TestManager::SetAcceleration(0.12f);
        TestManager::SetExponent(0.21f);
        TestManager::SetMidpoint(0.93f);
        TestManager::SetMotivity(0.92f);
        TestManager::SetUseSmoothing(true);
        TestManager::UpdateModesConstants();

        if (TestManager::ValidateConstants()) {
            // Should be invalid!
            fprintf(stderr, "Valid constants (should be invalid)\n");
            supervisor.result = false;
        }

        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Power);
        TestManager::SetAcceleration(0.12f);
        TestManager::SetExponent(0.21f);
        TestManager::SetMidpoint(0.93f);
        TestManager::SetMotivity(0.92f);
        TestManager::SetUseSmoothing(false);
        TestManager::UpdateModesConstants();

        if (!TestManager::ValidateConstants()) {
            fprintf(stderr, "Invalid constants\n");
            supervisor.result = false;
        }

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelPower(value);
            // printf("x: %f, Fixed Point: %f, Floating Point: %f, error: %f, value%i\n", value, FP64_ToFloat(res), TestManager::EvalFloatFunc(value), FP64_ToFloat(res) - TestManager::EvalFloatFunc(value), IsAccelValueGood(res));

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
            // printf("Validation: %i\n", supervisor.result);
        }

        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Power);
        TestManager::SetAcceleration(0.f);
        TestManager::SetExponent(0.21f);
        TestManager::SetMidpoint(0.93f);
        TestManager::SetMotivity(0.92f);
        TestManager::SetUseSmoothing(true);
        TestManager::UpdateModesConstants();

        if (TestManager::ValidateConstants()) {
            // Should be invalid!
            fprintf(stderr, "Valid constants (should be invalid)\n");
            supervisor.result = false;
        }
    } catch (std::exception &ex) {
        fprintf(stderr, "Exception: %s, in Power mode\n", ex.what());
        supervisor.result = false;
    }

    return supervisor.GetResult();
}

bool Tests::TestAccelClassic(float range_min, float range_max) {
    TestSupervisor supervisor{"Classic Mode"};
    try {
        supervisor.NextTest();
        TestManager::SetAccelMode(AccelMode_Classic);
        TestManager::SetAcceleration(0.01f);
        TestManager::SetExponent(4.f);
        TestManager::SetUseSmoothing(false);
        TestManager::UpdateModesConstants();

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelClassic(value);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
        }

        supervisor.NextTest();
        TestManager::SetAccelMode(AccelMode_Classic);
        TestManager::SetAcceleration(0.01f);
        TestManager::SetExponent(4.f);
        TestManager::SetMidpoint(6.f);
        TestManager::SetUseSmoothing(true);
        TestManager::UpdateModesConstants();

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelClassic(value);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
        }

        supervisor.NextTest();
        TestManager::SetAccelMode(AccelMode_Classic);
        TestManager::SetAcceleration(0.1f);
        TestManager::SetExponent(3.f);
        TestManager::SetMidpoint(5.f);
        TestManager::SetUseSmoothing(true);
        TestManager::UpdateModesConstants();

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelClassic(value);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
        }

        supervisor.NextTest();
        TestManager::SetAccelMode(AccelMode_Classic);
        TestManager::SetAcceleration(0.1f);
        TestManager::SetExponent(3.f);
        TestManager::SetMidpoint(0.f);
        TestManager::SetUseSmoothing(true);
        TestManager::UpdateModesConstants();

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelClassic(value);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
        }

        supervisor.NextTest();
        TestManager::SetAccelMode(AccelMode_Classic);
        TestManager::SetAcceleration(0.04f);
        TestManager::SetExponent(9.f);
        TestManager::SetMidpoint(5.f);
        TestManager::SetUseSmoothing(true);
        TestManager::UpdateModesConstants();

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelClassic(value);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
        }

        supervisor.NextTest();
        TestManager::SetAccelMode(AccelMode_Classic);
        TestManager::SetAcceleration(0.001f);
        TestManager::SetExponent(4.f);
        TestManager::SetMidpoint(0.f);
        TestManager::SetUseSmoothing(true);
        TestManager::UpdateModesConstants();

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelClassic(value);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
        }

        supervisor.NextTest();
        TestManager::SetAccelMode(AccelMode_Classic);
        TestManager::SetAcceleration(0.4f);
        TestManager::SetExponent(0.f);
        TestManager::SetMidpoint(0.f);
        TestManager::SetUseSmoothing(true);
        TestManager::UpdateModesConstants();

        if (TestManager::ValidateConstants()) {
            // Should be invalid!
            fprintf(stderr, "Valid constants (should be invalid)\n");
            supervisor.result = false;
        }

        supervisor.NextTest();
        TestManager::SetAccelMode(AccelMode_Classic);
        TestManager::SetAcceleration(0.4f);
        TestManager::SetExponent(1.f);
        TestManager::SetMidpoint(0.f);
        TestManager::SetUseSmoothing(true);
        TestManager::UpdateModesConstants();

        if (TestManager::ValidateConstants()) {
            // Should be invalid!
            fprintf(stderr, "Valid constants (should be invalid)\n");
            supervisor.result = false;
        }
    } catch (std::exception &ex) {
        fprintf(stderr, "Exception: %s, in Classic mode\n", ex.what());
        supervisor.result = false;
    }

    return supervisor.GetResult();
}

bool Tests::TestAccelMotivity(float range_min, float range_max) {
    TestSupervisor supervisor{"Motivity Mode"};

    try {
        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Motivity);
        TestManager::SetAcceleration(4.f);
        TestManager::SetMidpoint(0.f);
        TestManager::UpdateModesConstants();

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelMotivity(value);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
        }
    } catch (std::exception &ex) {
        fprintf(stderr, "Exception: %s, in Motivity mode\n", ex.what());
        supervisor.result = false;
    }

    return supervisor.GetResult();
}

bool Tests::TestAccelSynchronous(float range_min, float range_max) {
    TestSupervisor supervisor{"Synchronous Mode"};

    try {
        supervisor.NextTest();

        /* Parameter mapping (Rawaccel -> YeetMouse):
         * smooth -> midpoint
         * sync_speed -> accel
         * gamma -> exponent
        */
        TestManager::SetAccelMode(AccelMode_Synchronous);
        TestManager::SetExponent(20.f);
        TestManager::SetMidpoint(4.f);
        TestManager::SetMotivity(1.9f);
        TestManager::SetAcceleration(6.f);
        TestManager::SetUseSmoothing(false);
        TestManager::UpdateModesConstants();

        for (int i = 1; i <= BASIC_TEST_STEPS; i++) {
            float x = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelSynchronous(x);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(x)));

            //printf("x: %f, res: %f, float: %f\n", x, FP64_ToFloat(res), TestManager::EvalFloatFunc(x));
        }

        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Synchronous);
        TestManager::SetExponent(2.f);
        TestManager::SetMidpoint(0.5f);
        TestManager::SetMotivity(1.75f);
        TestManager::SetAcceleration(5.f);
        TestManager::SetUseSmoothing(true);
        TestManager::UpdateModesConstants();

        for (int i = 1; i <= BASIC_TEST_STEPS; i++) {
            float x = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelSynchronous(x);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(x)));

            //printf("x: %f, res: %f, float: %f\n", x, FP64_ToFloat(res), TestManager::EvalFloatFunc(x));
        }

        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Synchronous);
        TestManager::SetExponent(20.f);
        TestManager::SetMidpoint(4.f);
        TestManager::SetMotivity(1.9f);
        TestManager::SetAcceleration(6.f);
        TestManager::SetUseSmoothing(false);
        TestManager::UpdateModesConstants();

        for (int i = 1; i <= BASIC_TEST_STEPS; i++) {
            float x = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelSynchronous(x);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(x)));
        }

        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Synchronous);
        TestManager::SetExponent(20.f);
        TestManager::SetMidpoint(4.f);
        TestManager::SetMotivity(1.f);
        TestManager::SetAcceleration(6.f);
        TestManager::SetUseSmoothing(false);
        TestManager::UpdateModesConstants();

        if (TestManager::ValidateConstants()) {
            fprintf(stderr, "Valid constants (Should be invalid)\n");
            supervisor.result = false;
        }
    } catch (std::exception &ex) {
        fprintf(stderr, "Exception: %s, in Synchronous mode\n", ex.what());
        supervisor.result = false;
    }

    return supervisor.GetResult();
}

bool Tests::TestAccelNatural(float range_min, float range_max) {
    TestSupervisor supervisor{"Natural Mode"};

    try {
        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Natural);
        TestManager::SetAcceleration(1.02f);
        TestManager::SetExponent(5.f);
        TestManager::UpdateModesConstants();
        TestManager::SetMidpoint(0.f);
        TestManager::SetUseSmoothing(false);

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelNatural(value);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
            //printf("res: %f\n", FP64_ToFloat(res));
            //printf("val function: %f\n", TestManager::EvalFloatFunc(value));
        }

        supervisor.NextTest();

        // Test 2
        TestManager::SetAccelMode(AccelMode_Natural);
        TestManager::SetAcceleration(0.041f);
        TestManager::SetExponent(3.f);
        TestManager::SetMidpoint(6.f);
        TestManager::SetUseSmoothing(true);
        TestManager::UpdateModesConstants();

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelNatural(value);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
            // printf("res: %f\n", FP64_ToFloat(res));
            // printf("val function: %f\n", TestManager::EvalFloatFunc(value));
        }

        supervisor.NextTest();

        // Test 3
        TestManager::SetAccelMode(AccelMode_Natural);
        TestManager::SetAcceleration(0.2f);
        TestManager::SetExponent(5.f);
        TestManager::SetMidpoint(0.f);
        TestManager::SetUseSmoothing(false);
        TestManager::UpdateModesConstants();

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelNatural(value);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
        }

        supervisor.NextTest();

        // Test 4
        TestManager::SetAccelMode(AccelMode_Natural);
        TestManager::SetAcceleration(0.2f);
        TestManager::SetExponent(1.f);
        TestManager::SetMidpoint(0.5f);
        TestManager::SetUseSmoothing(false);
        TestManager::UpdateModesConstants();

        if (TestManager::ValidateConstants()) {
            // Should be invalid!
            fprintf(stderr, "Valid constants (should be invalid)\n");
            supervisor.result = false;
        }

        supervisor.NextTest();

        // Test 5
        TestManager::SetAccelMode(AccelMode_Natural);
        TestManager::SetAcceleration(0.f);
        TestManager::SetExponent(2.5f);
        TestManager::SetMidpoint(0.5f);
        TestManager::SetUseSmoothing(false);
        TestManager::UpdateModesConstants();

        if (TestManager::ValidateConstants()) {
            // Should be invalid!
            fprintf(stderr, "Valid constants (should be invalid)\n");
            supervisor.result = false;
        }
    } catch (std::exception &ex) {
        fprintf(stderr, "Exception: %s, in Natural mode\n", ex.what());
        supervisor.result = false;
    }

    return supervisor.GetResult();
}

bool Tests::TestAccelJump(float range_min, float range_max) {
    TestSupervisor supervisor{"Jump Mode"};

    try {
        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Jump);
        TestManager::SetAcceleration(4.f);
        TestManager::SetMidpoint(0.01f);
        TestManager::SetExponent(0.95f); // Smoothness
        TestManager::UpdateModesConstants();

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelJump(value);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
        }

        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Jump);
        TestManager::SetAcceleration(4.f);
        TestManager::SetMidpoint(0.01f);
        TestManager::SetExponent(0.01f); // Smoothness
        TestManager::UpdateModesConstants();

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelJump(value);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
        }

        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Jump);
        TestManager::SetAcceleration(4.f);
        TestManager::SetMidpoint(0.00f);
        TestManager::SetExponent(0.01f); // Smoothness
        TestManager::UpdateModesConstants();

        if (TestManager::ValidateConstants()) {
            // Should be invalid!
            fprintf(stderr, "Valid constants (should be invalid)\n");
            supervisor.result = false;
        }

        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Jump);
        TestManager::SetAcceleration(4.f);
        TestManager::SetMidpoint(0.01f);
        TestManager::SetExponent(0.0f); // Smoothness
        TestManager::UpdateModesConstants();

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelJump(value);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
        }

        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Jump);
        TestManager::SetAcceleration(4.f);
        TestManager::SetMidpoint(0.01f);
        TestManager::SetExponent(0.0f); // Smoothness
        TestManager::SetUseSmoothing(true);
        TestManager::UpdateModesConstants();

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelJump(value);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
        }

        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Jump);
        TestManager::SetAcceleration(4.f);
        TestManager::SetMidpoint(0.01f);
        TestManager::SetExponent(0.0f); // Smoothness
        TestManager::SetUseSmoothing(true);
        TestManager::UpdateModesConstants();

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelJump(value);

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
        }

        supervisor.NextTest();

        for (int i = 0; i < BASIC_TEST_STEPS_REDUCED; i++) {
            float t1 = static_cast<float>(i) / BASIC_TEST_STEPS_REDUCED;
            for (int j = 0; j < BASIC_TEST_STEPS_REDUCED; j++) {
                float t2 = static_cast<float>(j) / BASIC_TEST_STEPS_REDUCED;
                float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS_REDUCED;

                auto res = TestManager::AccelJump(value, 5, LERP(0, 1, t1), LERP(0.01, 50, t2), i % 2 == 0);

                // printf("exp = %f, midpoint = %f\n", LERP(0, 1, t1), LERP(0.01, 50, t2));
                // printf("x: %f, res: %f, float: %f\n", value, FP64_ToFloat(res), TestManager::EvalFloatFunc(value));

                supervisor.Validate(IsAccelValueGood(res));
                supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
            }
        }
    } catch (std::exception &ex) {
        fprintf(stderr, "Exception: %s, in Jump mode\n", ex.what());
        return false;
    }

    return supervisor.GetResult();
}

bool Tests::TestAccelLUT(float range_min, float range_max) {
    TestSupervisor supervisor{"LUT Mode"};
    try {
        supervisor.NextTest();

        TestManager::SetAccelMode(AccelMode_Lut);
        float values_x[4] = {1, 20, 20, 40};
        float values_y[4] = {1, 1, 2, 2};
        TestManager::SetLutData(values_x, values_y, 4);
        TestManager::UpdateModesConstants();

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float value = range_min + static_cast<float>(i) * (range_max - range_min) / BASIC_TEST_STEPS;
            auto res = TestManager::AccelLUT(value);

            //printf("%f, %f, %f\n", value, FP64_ToFloat(res), TestManager::EvalFloatFunc(value));

            supervisor.Validate(IsAccelValueGood(res));
            supervisor.Validate(IsCloseEnoughRelative(res, TestManager::EvalFloatFunc(value)));
        }

        supervisor.NextTest();

        float values_x2[] = {1, 20, 20, 40, 40};
        float values_y2[] = {1, 1, 2, 2, 3};
        TestManager::SetAccelMode(AccelMode_Lut);
        TestManager::SetLutData(values_x2, values_y2, sizeof(values_x2) / sizeof(float));
        TestManager::UpdateModesConstants();

        if (TestManager::ValidateConstants()) {
            // Should be invalid!
            fprintf(stderr, "Valid constants (should be invalid)\n");
            supervisor.result = false;
        }

        supervisor.NextTest();

        float values_x3[] = {1, 20, 20, 40, 40};
        float values_y3[] = {1, 1, 2, 2, 2};
        TestManager::SetAccelMode(AccelMode_Lut);
        TestManager::SetLutData(values_x3, values_y3, sizeof(values_x3) / sizeof(float));
        TestManager::UpdateModesConstants();

        if (TestManager::ValidateConstants()) {
            // Should be invalid!
            fprintf(stderr, "Valid constants (should be invalid)\n");
            supervisor.result = false;
        }

        supervisor.NextTest();

        float values_x4[] = {1, 20, 40, 20, 40};
        float values_y4[] = {1, 1, 3, 2, 2};
        TestManager::SetAccelMode(AccelMode_Lut);
        TestManager::SetLutData(values_x4, values_y4, sizeof(values_x4) / sizeof(float));
        TestManager::UpdateModesConstants();

        if (TestManager::ValidateConstants()) {
            // Should be invalid!
            fprintf(stderr, "Valid constants (should be invalid)\n");
            supervisor.result = false;
        }

        supervisor.NextTest();

        float values_x5[] = {1};
        float values_y5[] = {2};
        TestManager::SetAccelMode(AccelMode_Lut);
        TestManager::SetLutData(values_x5, values_y5, sizeof(values_x5) / sizeof(float));
        TestManager::UpdateModesConstants();

        if (TestManager::ValidateConstants()) {
            // Should be invalid!
            fprintf(stderr, "Valid constants (should be invalid)\n");
            supervisor.result = false;
        }
    } catch (std::exception &ex) {
        fprintf(stderr, "Exception: %s, in LUT mode\n", ex.what());
        return false;
    }

    return supervisor.GetResult();
}

bool Tests::TestAccelMode(AccelMode mode, float range_min, float range_max) {
    static_assert(AccelMode_Count == 10);

    // Just some random global parameters to make sure all the curves work with them (this might make debugging a hell)
    TestManager::SetSensitivity(0.75f);
    TestManager::SetOffset(2.f);
    TestManager::SetPreScale(0.9f);

    switch (mode) {
        case AccelMode_Linear:
            return TestAccelLinear(range_min, range_max);
        case AccelMode_Power:
            return TestAccelPower(range_min, range_max);
        case AccelMode_Classic:
            return TestAccelClassic(range_min, range_max);
        case AccelMode_Motivity:
            return TestAccelMotivity(range_min, range_max);
        case AccelMode_Synchronous:
            return TestAccelSynchronous(range_min, range_max);
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
    std::array<bool, AccelMode_Count> results{true}; // AccelMode_Current is always true

    for (int mode = 1; mode < AccelMode_Count; mode++) {
        results[mode] = TestAccelMode(static_cast<AccelMode>(mode), range_min, range_max);
    }

    return results;
}

bool Tests::TestFixedPointArithmetic() {
    TestSupervisor supervisor{"Arithmetic Test"};

    try {
        supervisor.NextTest();

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float x = -30 + static_cast<float>(i) * 60 / BASIC_TEST_STEPS; // Range -10, 10
            auto val = FP64_Tanh(FP64_FromFloat(x));

            supervisor.Validate(IsCloseEnough(val, std::tanh(x), 1e-4));

            //printf("%f, %f,%f,%f\n", x, FP64_ToFloat(val), std::tanh(x), FP64_ToFloat(val) - std::tanh(x));
        }

        supervisor.NextTest();

        for (int i = 0; i < BASIC_TEST_STEPS; i++) {
            float x = -375 + static_cast<float>(i) * 750 / BASIC_TEST_STEPS;

            auto val = FP64_Ilogb(FP64_FromFloat(x));

            supervisor.Validate(IsCloseEnough(FP64_FromInt(val), std::ilogb(x), 1e-1));

            //printf("%f, %i,%i,%i\n", x, val, std::ilogb(x), val - std::ilogb(x));
        }

        supervisor.NextTest();

        for (int i = 0; i < BASIC_TEST_STEPS_REDUCED; i++) {
            float x1 = -1000 + static_cast<float>(i) * 2000 / BASIC_TEST_STEPS_REDUCED;
            for (int j = 0; j < 20; j++) {
                int x2 = -10 + j * 20 / 20;
                auto val = FP64_Scalbn(FP64_FromFloat(x1), x2);

                //supervisor.Validate(IsAccelValueGood(val));
                supervisor.Validate(IsCloseEnough(val, std::scalbln(x1, x2), 1e-4));

                //printf("(%f, %i), %f,%f,%f\n", x1, x2, FP64_ToFloat(val), std::scalbln(x1, x2), FP64_ToFloat(val) - std::scalbln(x1, x2));
            }
        }
    } catch (std::exception &ex) {
        fprintf(stderr, "Exception: %s during arithmetic\n", ex.what());
        supervisor.result = false;
    }

    return supervisor.GetResult();
}

void Tests::TestSupervisor::Validate(bool res) {
    if (result && !res) // Prints only on the first occurrence
        printf(RED "Test failed!\n" RESET);

    result &= res;
}

void Tests::TestSupervisor::NextTest() {
    TestPass();

    _result &= result;
    result = true;

    printf("Running test #%d for %s\n", test_idx, test_name);

    test_idx++;

    start_time = std::chrono::steady_clock::now();
}

Tests::TestSupervisor::~TestSupervisor() {
    TestPass();
    printf("\n");
}

void Tests::TestSupervisor::TestPass() {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    auto seconds = duration.count() / 1000000;
    auto millis = duration.count() / 1000;

    if (test_idx > 1) {
        printf("Test #%d: %s (%lds %ldms %lld\xC2\xB5s)\n" RESET, test_idx - 1, result ? GREEN "Passed" : RED "Failed",
               seconds, millis, duration.count() % 1000);
    }
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
