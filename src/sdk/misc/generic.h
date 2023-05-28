#ifndef BROWNIANBOT_GENERIC_H
#define BROWNIANBOT_GENERIC_H


#define DEFINE_ARRAY(TType) typedef struct {TType* items; int length;} Array##TType


#endif
