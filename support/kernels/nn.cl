typedef struct latLong
  {
    float lat;
    float lng;
  } LatLong;

/*#ifndef SIMD_LANES
  #define SIMD_LANES 16
#endif
  
#ifndef COMPUTE_UNITS
  #define COMPUTE_UNITS 3
#endif

__attribute__((reqd_work_group_size(64,1,1)))
__attribute__((num_simd_work_items(SIMD_LANES)))
__attribute__((num_compute_units(COMPUTE_UNITS)))
*/
__kernel void NearestNeighbor(__global LatLong* restrict d_locations,
            __global float*   restrict d_distances,
            const    int               numRecords,
            const    float             lat,
            const    float             lng,
            const    int               iterations,
            const    int               offset
            )
{
  int index    =get_global_id(0);
  int globalId = index+offset;
  
  if (globalId < numRecords)
  {
    __global LatLong *latLong = d_locations + index;
    __global float *dist = d_distances + index;
    *dist = (float)sqrt( (lat - latLong->lat) * (lat - latLong->lat) + (lng - latLong->lng) * (lng - latLong->lng) );
  }
}
