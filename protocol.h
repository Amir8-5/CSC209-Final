#ifndef PROTOCOL_H
#define PROTOCOL_H

#define MAX_CLIENTS 3
#include <stdint.h>

typedef struct {
    float weight;
    float bias;
} model_t;

typedef struct {
    float sqft;
    float price;
} HouseData;

typedef struct {
    HouseData *data;
    int size;
    int capacity;
} HouseDataPacket;

#endif
