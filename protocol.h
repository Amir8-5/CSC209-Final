#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

typedef struct {
    float weight;
    float bias;
} model_t;

typedef struct {
    float sqft;
    float bias;
} HouseData

#endif
