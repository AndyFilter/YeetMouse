// Maximum number of packets allowed to be sent from the mouse at once. Linux's default value is 8, which at
// least causes EOVERFLOW for my mouse (SteelSeries Rival 600). Increase this, if 'dmesg -w' tells you to!
#define BUFFER_SIZE 16

// There values are just here to allow you to comfortably start the GUI and change them to your preferences.
#define SENSITIVITY 1 //0.85f
#define SENSITIVITY_Y 1
#define ACCELERATION 0.15
#define OUTPUT_CAP 0
#define OFFSET 0
//#define POST_SCALE_X 0.4f
//#define POST_SCALE_Y 0.4f
#define INPUT_CAP 0
#define MIDPOINT 6
#define MOTIVITY 1.5
#define PRESCALE 1
#define USE_SMOOTHING 1

// Rotation (in radians)
#define ROTATION_ANGLE 0

// Angle Snapping
#define ANGLE_SNAPPING_THRESHOLD 0 // 0 deg. in rad.
#define ANGLE_SNAPPING_ANGLE 0 // 1.5708 - 90 deg. in rad.

// LUT settings
#define LUT_SIZE 0
#define LUT_DATA 0


#define ACCELERATION_MODE AccelMode_Linear

// For exponential curves.
#define EXPONENT 0.2 //26214