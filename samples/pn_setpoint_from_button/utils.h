#ifndef PROFINET_UTILS_H
#define PROFINET_UTILS_H

#include <stdint.h>
#include <stdbool.h>

bool are_arrays_equal (
   const uint8_t arr1[],
   int size1,
   const uint8_t arr2[],
   int size2);

uint32_t combine_bytes (
   uint8_t byte1,
   uint8_t byte2,
   uint8_t byte3,
   uint8_t byte4);

#endif /* PROFINET_UTILS_H */