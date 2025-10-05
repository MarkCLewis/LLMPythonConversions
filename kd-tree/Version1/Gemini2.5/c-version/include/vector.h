#ifndef VECTOR_H
#define VECTOR_H

typedef struct {
    double x, y, z;
} F64x3;

F64x3 vec_add(F64x3 a, F64x3 b);
F64x3 vec_sub(F64x3 a, F64x3 b);
F64x3 vec_mul_scalar(F64x3 a, double s);
F64x3 vec_div_scalar(F64x3 a, double s);
double vec_dot(F64x3 a, F64x3 b);
F64x3 vec_min(F64x3 a, F64x3 b);
F64x3 vec_max(F64x3 a, F64x3 b);

#endif // VECTOR_H
