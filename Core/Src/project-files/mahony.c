#include "project.h"

#include <math.h>
#include <stdint.h>

// -------------------------------------------------------
// Mahony Filter State
// -------------------------------------------------------

// Tuning parameters — adjust these for your device
// Kp: higher = trust accelerometer/magnetometer more, less gyro drift but noisier
// Ki: higher = correct gyro bias faster
#define MAHONY_KP  2.0f
#define MAHONY_KI  0.005f

// Call this once at startup
void Mahony_Init(MahonyState *state)
{
    state->q0 = 1.0f;
    state->q1 = 0.0f;
    state->q2 = 0.0f;
    state->q3 = 0.0f;
    state->integralFBx = 0.0f;
    state->integralFBy = 0.0f;
    state->integralFBz = 0.0f;
}

// -------------------------------------------------------
// Mahony Filter Update
// Call this at a fixed rate (e.g. 100Hz from a timer)
//
// gx, gy, gz  — gyroscope  (radians/sec)
// ax, ay, az  — accelerometer (any unit, will be normalised)
// mx, my, mz  — magnetometer  (any unit, will be normalised)
// dt          — time delta in seconds (e.g. 0.01 for 100Hz)
// -------------------------------------------------------

void Mahony_Update(MahonyState *s,
                   float gx, float gy, float gz,
                   float ax, float ay, float az,
                   float mx, float my, float mz,
                   float dt)
{
    float q0 = s->q0, q1 = s->q1, q2 = s->q2, q3 = s->q3;
    float recipNorm;
    float hx, hy, bx, bz;
    float halfvx, halfvy, halfvz;
    float halfwx, halfwy, halfwz;
    float halfex, halfey, halfez;

    // Normalise accelerometer
    float accNorm = sqrtf(ax*ax + ay*ay + az*az);
    if (accNorm == 0.0f) return;
    recipNorm = 1.0f / accNorm;
    ax *= recipNorm;
    ay *= recipNorm;
    az *= recipNorm;

    // Normalise magnetometer
    float magNorm = sqrtf(mx*mx + my*my + mz*mz);
    if (magNorm == 0.0f) return;
    recipNorm = 1.0f / magNorm;
    mx *= recipNorm;
    my *= recipNorm;
    mz *= recipNorm;

    // Reference direction of Earth's magnetic field
    float q0q0 = q0*q0, q0q1 = q0*q1, q0q2 = q0*q2, q0q3 = q0*q3;
    float q1q1 = q1*q1, q1q2 = q1*q2, q1q3 = q1*q3;
    float q2q2 = q2*q2, q2q3 = q2*q3;
    float q3q3 = q3*q3;

    hx = 2.0f * (mx * (0.5f - q2q2 - q3q3) + my * (q1q2 - q0q3) + mz * (q1q3 + q0q2));
    hy = 2.0f * (mx * (q1q2 + q0q3) + my * (0.5f - q1q1 - q3q3) + mz * (q2q3 - q0q1));
    bx = sqrtf(hx*hx + hy*hy);
    bz = 2.0f * (mx * (q1q3 - q0q2) + my * (q2q3 + q0q1) + mz * (0.5f - q1q1 - q2q2));

    // Estimated direction of gravity and magnetic field
    halfvx =  q1q3 - q0q2;
    halfvy =  q0q1 + q2q3;
    halfvz =  q0q0 - 0.5f + q3q3;

    halfwx = bx * (0.5f - q2q2 - q3q3) + bz * (q1q3 - q0q2);
    halfwy = bx * (q1q2 - q0q3)         + bz * (q0q1 + q2q3);
    halfwz = bx * (q1q3 + q0q2)         + bz * (0.5f - q1q1 - q2q2);

    // Error is cross product of estimated and measured direction
    halfex = (ay * halfvz - az * halfvy) + (my * halfwz - mz * halfwy);
    halfey = (az * halfvx - ax * halfvz) + (mz * halfwx - mx * halfwz);
    halfez = (ax * halfvy - ay * halfvx) + (mx * halfwy - my * halfwx);

    // Apply integral feedback (gyro bias correction)
    s->integralFBx += MAHONY_KI * halfex * dt;
    s->integralFBy += MAHONY_KI * halfey * dt;
    s->integralFBz += MAHONY_KI * halfez * dt;
    gx += MAHONY_KP * halfex + s->integralFBx;
    gy += MAHONY_KP * halfey + s->integralFBy;
    gz += MAHONY_KP * halfez + s->integralFBz;

    // Integrate rate of change of quaternion
    q0 += (-q1*gx - q2*gy - q3*gz) * (0.5f * dt);
    q1 += ( q0*gx + q2*gz - q3*gy) * (0.5f * dt);
    q2 += ( q0*gy - q1*gz + q3*gx) * (0.5f * dt);
    q3 += ( q0*gz + q1*gy - q2*gx) * (0.5f * dt);

    // Normalise quaternion
    recipNorm = 1.0f / sqrtf(q0*q0 + q1*q1 + q2*q2 + q3*q3);
    s->q0 = q0 * recipNorm;
    s->q1 = q1 * recipNorm;
    s->q2 = q2 * recipNorm;
    s->q3 = q3 * recipNorm;
}

// -------------------------------------------------------
// Extract heading 0-359 from quaternion
// -------------------------------------------------------

float Mahony_GetHeading(MahonyState *s)
{
    float q0 = s->q0, q1 = s->q1, q2 = s->q2, q3 = s->q3;

    // Yaw from quaternion (tilt-compensated heading)
    float yaw = atan2f(2.0f * (q1*q2 + q0*q3),
                       q0*q0 + q1*q1 - q2*q2 - q3*q3);

    // Convert radians to degrees
    float heading = yaw * (180.0f / M_PI);

    // Normalize to 0-359
    if (heading < 0.0f)   heading += 360.0f;
    if (heading >= 360.0f) heading -= 360.0f;

    return heading;
}

