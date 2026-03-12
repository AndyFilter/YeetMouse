// Acceleration Mode
#define ACCELERATION_MODE 1 // AccelMode_Linear (value needs to be inlined because the preprocessor runs before the compiler)

// Global Parameters
#define SENSITIVITY 1
#define RATIO_YX 1 // Sensitivity ratio Y/X
#define OUTPUT_CAP 0
#define INPUT_CAP 0
#define OFFSET 0
#define PRESCALE 1

// Mode-specific parameters
#define ACCELERATION 0.1
#define MIDPOINT 1.3
#define MOTIVITY 1.5
#define EXPONENT 1.8
#define USE_SMOOTHING 1 // 1 - True, 0 - False

// Angle Snapping (in radians)
#define ANGLE_SNAPPING_THRESHOLD 0 // 0 deg. in rad.
#define ANGLE_SNAPPING_ANGLE 0 // 1.5708 - 90 deg. in rad.

// Rotation (in radians)
#define ROTATION_ANGLE 0

// LUT settings
#define LUT_SIZE 0
#define LUT_DATA 0

// Custom Curve (Not used on the driver side)
#define CC_DATA_AGGREGATE