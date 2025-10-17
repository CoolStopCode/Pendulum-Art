#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;

uniform float uGrav; // number of squares across screen

uniform float uArm1_change;
uniform float uArm2_change;
uniform float uArm1_start;
uniform float uArm2_start;

uniform float uTime_step;
uniform float uTime_total;

struct Arm {
    float alpha; // angular acceleration (derivative of omega)
    float omega; // angular velocity (derivative of theta)
    float theta; // rotation
    float length;
    float damping;
};

struct Pendulum {
    Arm arm1;
    Arm arm2;
};


Pendulum calculate_pendulum(Pendulum p, float delta) {
    float g = uGrav;
    float L1 = p.arm1.length;
    float L2 = p.arm2.length;
    float th1 = p.arm1.theta;
    float th2 = p.arm2.theta;
    float w1  = p.arm1.omega;
    float w2  = p.arm2.omega;
    float d1  = p.arm1.damping;
    float d2  = p.arm2.damping;

    float diff = th1 - th2;
    float denom = 2.0 - cos(2.0*diff);

    float a1 = (-g*(2.0*sin(th1) + sin(th1 - 2.0*th2))
                - 2.0*sin(diff)*(w2*w2*L2 + w1*w1*L1*cos(diff))) / (L1*denom);
    float a2 = (2.0*sin(diff)*(w1*w1*L1 + g*cos(th1) + w2*w2*L2*cos(diff))) / (L2*denom);

    // Semi-implicit Euler
    w1 = w1 + (a1 - d1*w1) * delta;
    w2 = w2 + (a2 - d2*w2) * delta;
    th1 = th1 + w1 * delta;
    th2 = th2 + w2 * delta;

    Arm newArm1 = Arm(a1, w1, th1, L1, d1);
    Arm newArm2 = Arm(a2, w2, th2, L2, d2);

    Pendulum newPendulum = Pendulum(newArm1, newArm2);
    return newPendulum;
}

void main() {
    float arm1_rot = vTexCoord.x * uArm1_change + uArm1_start;
    float arm2_rot = vTexCoord.y * uArm2_change + uArm2_start;

    Pendulum pendulum = Pendulum(Arm(0.0, 0.0, arm1_rot, 1.0, 0.0), Arm(0.0, 0.0, arm2_rot, 1.0, 0.0));
    for (float i = 0.0; i < uTime_total; i += uTime_step) {
        pendulum = calculate_pendulum(pendulum, uTime_step);
    }

    FragColor = vec4(0.0, mod(pendulum.arm2.theta / 6.28, 1.0), mod(pendulum.arm1.theta / 6.28, 1.0), 1.0);
}
