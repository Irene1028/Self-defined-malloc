#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<stdbool.h>

typedef struct data_block
{
  struct data_block *next, *prev;
  bool isFree;
  size_t size;
  void *dataSeg;
} data_block;

typedef struct data_block block;

/*First Fit*/
void * ts_malloc_lock(size_t size);
void ts_free_lock(void *ptr);

/*Best Fit*/
void * ts_malloc_nolock(size_t size);
void ts_free_nolock(void *ptr);

/*Additional Test Library Funcs*/
unsigned long get_data_segment_size();  //in bytes
unsigned long get_data_segment_free_space_size();  //in bytes
