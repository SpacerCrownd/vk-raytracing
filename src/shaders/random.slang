// Random functions from https://blog.demofox.org/
#define PI 3.14159265359
#define TWOPI 2.0 * PI

uint wang_hash(inout uint seed) {
    seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> 4);
    seed *= uint(0x27d4eb2d);
    seed = seed ^ (seed >> 15);
    return seed;
}

float randomFloat01(inout uint state) {
    return float(wang_hash(state)) / 4294967296.0;
}

uint rewRandomSeed(uint v0, uint v1, uint v2) {
    return uint(v0 * uint(1973) + v1 * uint(9277) + v2 * uint(26699)) | uint(1);
}

float3 cosineSampleHemisphere(float r1, float r2)
{
    float r = sqrt(r1);
    float phi = TWOPI * r2;
    float3 dir;
    dir.x = r * cos(phi);
    dir.y = r * sin(phi);
    dir.z = sqrt(max(0.F, 1.F - dir.x * dir.x - dir.y * dir.y));
    return dir;
}