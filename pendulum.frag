#version 330 core

#define PI 3.14159265359

in vec2 vTexCoord;
out vec4 FragColor;

uniform float uGrav; // number of squares across screen
uniform float uDamping;
uniform float uArm1Length;
uniform float uArm2Length;

uniform float uScaleX;
uniform float uScaleY;
uniform float uCamX;
uniform float uCamY;

uniform float uTime_step;
uniform float uTime_total;

struct Arm {
    float alpha; // angular acceleration (derivative of omega)
    float omega; // angular velocity (derivative of theta)
    float theta; // rotation
};

struct Pendulum {
    Arm arm1;
    Arm arm2;
};


Pendulum calculate_pendulum(Pendulum p, float delta) {
    float g = uGrav;
    float L1 = uArm1Length;
    float L2 = uArm2Length;
    float th1 = p.arm1.theta;
    float th2 = p.arm2.theta;
    float w1  = p.arm1.omega;
    float w2  = p.arm2.omega;
    float d1  = uDamping;
    float d2  = uDamping;

    float diff = th1 - th2;
    float denom = 2.0 - cos(2.0*diff);

    float a1 = (-g*(2.0*sin(th1) + sin(th1 - 2.0*th2))
                - 2.0*sin(diff)*(w2*w2*L2 + w1*w1*L1*cos(diff))) / (L1*denom);
    float a2 = (2.0*sin(diff)*(w1*w1*L1 + g*cos(th1) + w2*w2*L2*cos(diff))) / (L2*denom);

    // float diff = th1 - th2;
    // float denom = 2.0f * m1 + m2 - m2 * cos(2.0f * delta);
    // if (abs(denom) < 1e-6f) denom = (denom < 0.0f ? -1e-6f : 1e-6f);

    // float num1 =
    //     -g * (2.0f * m1 + m2) * sin(th1)
    //     - m2 * g * sin(th1 - 2.0f * th2)
    //     - 2.0f * sin_d * m2 * (w2*w2 * L2 + w1*w1 * L1 * cos_d);

    // float a1 = num1 / (L1 * denom);

    // float num2 =
    //     2.0f * sin_d * (
    //         w1*w1 * L1 * (m1 + m2)
    //         + g * (m1 + m2) * cos(th1)
    //         + w2*w2 * L2 * m2 * cos_d
    //     );

    // float a2 = num2 / (L2 * denom);
    // Semi-implicit Euler
    w1 = w1 + (a1 - d1*w1) * delta;
    w2 = w2 + (a2 - d2*w2) * delta;
    th1 = th1 + w1 * delta;
    th2 = th2 + w2 * delta;

    Arm newArm1 = Arm(a1, w1, th1);
    Arm newArm2 = Arm(a2, w2, th2);

    Pendulum newPendulum = Pendulum(newArm1, newArm2);
    return newPendulum;
}

vec3 value_to_gradient(float value) {
    vec3 palette[5];

palette[0] = vec3(5, 5, 30)/255.0;       // Darkest Indigo
palette[1] = vec3(20, 30, 60)/255.0;     // Deep Navy
palette[2] = vec3(140, 20, 30)/255.0;  // Pale Ice Blue (Accent)
palette[3] = vec3(50, 70, 100)/255.0;    // Dark Denim Blue
palette[4] = vec3(5, 5, 30)/255.0;       // Darkest Indigo (Repeat)
    float indexF = value * 4.0;
    int index = int(indexF);
    
    // Calculate the local 't' value within that segment (0.0 to 1.0)
    float t_local = fract(indexF);

    // Handle the edge case for t=1.0. When t=1.0, indexF=4.0, index=4, t_local=0.0.
    // We want the last segment G3 to G4.
    if (index >= 4) {
        return palette[4]; // Return the last color G4 directly
    }

    // Linear interpolation (lerp) between the start and end color of the segment
    // GLSL's mix(a, b, t) is equivalent to a * (1-t) + b * t
    return mix(palette[index], palette[index + 1], t_local);
}

void main() {
    float arm1_rot = vTexCoord.x * uScaleX + uCamX;
    float arm2_rot = vTexCoord.y * uScaleY + uCamY;

    Pendulum pendulum = Pendulum(Arm(0.0, 0.0, arm1_rot), Arm(0.0, 0.0, arm2_rot));
    for (float i = 0.0; i < uTime_total; i += uTime_step) {
        float dt = uTime_step;
        if (i + uTime_step > uTime_total) {
            dt = uTime_total - i;
        }
        pendulum = calculate_pendulum(pendulum, dt);
    }

    float value = 0.5 + 0.5 * sin(pendulum.arm1.theta + pendulum.arm2.theta);
    FragColor = vec4(value_to_gradient(value), 1.0);
}
