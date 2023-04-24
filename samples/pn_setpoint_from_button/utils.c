#include "utils.h"

bool are_arrays_equal (
   const uint8_t arr1[],
   int size1,
   const uint8_t arr2[],
   int size2)
{
   if (size1 != size2)
   {
      return false;
   }

   for (int i = 0; i < size1; i++)
   {
      if (arr1[i] != arr2[i])
      {
         return false;
      }
   }

   return true;
}

uint32_t combine_bytes (
   uint8_t byte1,
   uint8_t byte2,
   uint8_t byte3,
   uint8_t byte4)
{
   uint32_t result = 0;

   result |= (uint32_t)byte1 << 24;
   result |= (uint32_t)byte2 << 16;
   result |= (uint32_t)byte3 << 8;
   result |= (uint32_t)byte4;

   return result;
}