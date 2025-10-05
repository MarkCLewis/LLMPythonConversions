#include "vector.h"
#include <math.h>

F64x3 vec_add(F64x3 a, F64x3 b) {
    return (F64x3){a.x + b.x, a.y + b.y, a.z + b.z};
}

F64x3 vec_sub(F64x3 a, F64x3 b) {
    return (F64x3){a.x - b.x, a.y - b.y, a.z - b.z};
}

F64x3 vec_mul_scalar(F64x3 a, double s) {
    return (F64x3){a.x * s, a.y * s, a.z * s};
}

F64x3 vec_div_scalar(F64x3 a, double s) {
    return (F64x3){a.x / s, a.y / s, a.z / s};
}

double vec_dot(F64x3 a, F64x3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

F64x3 vec_min(F64x3 a, F64x3 b) {
    return (F64x3){fmin(a.x, b.x), fmin(a.y, b.y), fmin(a.z, b.z)};
}

F64x3 vec_max(F64x3 a, F64x3 b) {
    return (F64x3){fmax(a.x, b.x), fmax(a.y, b.y), fmax(a.z, b.z)};
}
