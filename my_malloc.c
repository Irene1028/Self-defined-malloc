#include<stdio.h>
#include<stdlib.h>
#include "my_malloc.h"
#include<stdint.h>
#include<stdbool.h>
#include<unistd.h>
#include <pthread.h>

/************* Thread Safe Definations *************/
pthread_mutex_t mymutex=PTHREAD_MUTEX_INITIALIZER;
/**************************************************/
/*
__thread int has_init=0;
__thread void *block_head = NULL;
__thread unsigned long seg=0;
__thread unsigned long freeSeg=0;
*/
int has_init=0;
void *block_head = NULL;
unsigned long seg=0;
unsigned long freeSeg=0;

void init_list(){
  block *head = (block *)sbrk(sizeof(block));
  head->size = sizeof(block);
  head->isFree = false;
  head->next = NULL;
  head->prev = NULL;
  head->dataSeg = block_head + sizeof(block);
  block_head = head;
  has_init = 1;
}


void init_list_nolock(){
  pthread_mutex_lock(&mymutex);  
  block *head = (block *)sbrk(sizeof(block));
  pthread_mutex_unlock(&mymutex);   
  head->size = sizeof(block);
  head->isFree = false;
  head->next = NULL;
  head->prev = NULL;
  head->dataSeg = block_head + sizeof(block);
  block_head = head;
  has_init = 1;
  
}

void split_block(block * big_block, size_t size){
  //Here is split function
  block *small_blk = (block*)((void*)big_block + size);
  small_blk->size = big_block->size - size;
  small_blk->isFree = true;
  small_blk->next = big_block->next;
  small_blk->prev = big_block;
  small_blk->dataSeg = (void *)((void *)small_blk + sizeof(block));
  big_block->size = size;
  big_block->isFree = false;
  //link next->prev
  if(big_block->next!=NULL){
    big_block->next->prev = small_blk;
  }
  big_block->next = small_blk;
}
block * create_new_block(size_t size){
  block *new_block = (void *)sbrk(size);//=sbrk(0)
  seg += size;
  new_block->dataSeg = (void *)new_block + sizeof(block);
  new_block->size = size;
  new_block->isFree = false;
  new_block->next = NULL;
  new_block->prev = NULL;
  return new_block;
}

block * create_new_block_nolock(size_t size){
  pthread_mutex_lock(&mymutex);  
  block *new_block = (void *)sbrk(size);//=sbrk(0)
  pthread_mutex_unlock(&mymutex);   
  seg += size;
  new_block->dataSeg = (void *)new_block + sizeof(block);
  new_block->size = size;
  new_block->isFree = false;
  new_block->next = NULL;
  new_block->prev = NULL;
  return new_block;
}

/********************* Best Fit *************************/

void * ts_malloc_lock(size_t size){
  pthread_mutex_lock(&mymutex);  
  if(!has_init){
    init_list();
  }
  block *curr = block_head;
  block *prev = block_head;

  // Here we add size
  size += sizeof(block);
  size_t mini_size = 0;  
  block * mini_target = NULL;
  while (curr->next != NULL){
    if(prev == curr){
      //the block_head base case
      curr = curr->next;
      continue;
    }
    if((curr->isFree) && (curr->size >= size)){
      //available space
      if((curr->size == size)){
	//if available space equals to requested space
	//Exact_blk condition
	curr->isFree = false;
	freeSeg -= curr->size;

	pthread_mutex_unlock(&mymutex);   
	return curr->dataSeg;
      }
      else{
	if(mini_size == 0){
	  mini_size = curr->size;
	  mini_target = curr;
	}
	else if(curr->size < mini_size){
	  mini_size = curr->size;
	  mini_target = curr;
	}
      }
    }
    prev = curr;
    curr = curr->next;
  }
  /******** Additional Loop ***********/
  if((curr->isFree) && (curr->size >= size)){
    //available space
    if((curr->size == size)){
      //if available space equals to requested space
      //printf("Here is exact_blk condition\n");
      curr->isFree = false;
      freeSeg -= curr->size;
      pthread_mutex_unlock(&mymutex);   
      return  curr->dataSeg;
    }
    else{
      if(mini_size == 0){
	mini_size = curr->size;
	mini_target = curr;
      }
      else if(curr->size < mini_size){
	mini_size = curr->size;
	mini_target = curr;
      }
    }
  }
  /********* Final Alloc **********/
  if(mini_size == 0){
    block * new_block = create_new_block(size);
    new_block->prev = curr;
    curr->next = new_block;
    pthread_mutex_unlock(&mymutex);   
    return new_block->dataSeg;
  }
  else{
    if(mini_target->size - size >= sizeof(block)){
      split_block(mini_target, size);
      freeSeg -= mini_target->size;
      pthread_mutex_unlock(&mymutex);   
      return mini_target->dataSeg;
    }
    else{
      mini_target->isFree = false;
      freeSeg -= mini_target->size;
      pthread_mutex_unlock(&mymutex);   
      return mini_target->dataSeg;
    }
  }
}


/*********************** Non-Lock  ***********************/

void * ts_malloc_nolock(size_t size){
  if(!has_init){
    init_list_nolock();
  }
  block *curr = block_head;
  block *prev = block_head;
  //  void * ret_addr = NULL;
  // Here we add size
  size += sizeof(block);
  size_t mini_size = 0;  
  block * mini_target = NULL;
  while (curr->next != NULL){
    if(prev == curr){
      //the block_head base case
      curr = curr->next;
      continue;
    }
    if((curr->isFree) && (curr->size >= size)){
      //available space
      if((curr->size == size)){
	//if available space equals to requested space
	//Exact_blk condition
	curr->isFree = false;
	freeSeg -= curr->size;
	return curr->dataSeg;
      }
      else{
	if(mini_size == 0){
	  mini_size = curr->size;
	  mini_target = curr;
	}
	else if(curr->size < mini_size){
	  mini_size = curr->size;
	  mini_target = curr;
	}
      }
    }
    prev = curr;
    curr = curr->next;
  }
  /******** Additional Loop ***********/
  if((curr->isFree) && (curr->size >= size)){
    //available space
    if((curr->size == size)){
      //if available space equals to requested space
      //printf("Here is exact_blk condition\n");
      curr->isFree = false;
      freeSeg -= curr->size;

      return curr->dataSeg;
    }
    else{
      if(mini_size == 0){
	mini_size = curr->size;
	mini_target = curr;
      }
      else if(curr->size < mini_size){
	mini_size = curr->size;
	mini_target = curr;
      }
    }
  }
  /********* Final Alloc **********/
  if(mini_size == 0){
    block * new_block = create_new_block_nolock(size);
    new_block->prev = curr;
    curr->next = new_block;
    return new_block->dataSeg;
  }
  else{
    if(mini_target->size - size >= sizeof(block)){
      split_block(mini_target, size);
      freeSeg -= mini_target->size;
      return mini_target->dataSeg;
    }
    else{
      mini_target->isFree = false;
      freeSeg -= mini_target->size;
      return mini_target->dataSeg;
    }
  }
}

/************************** Free *****************************/

void merge_block(block * curr){
  if(curr->next != NULL){
    if(curr->next->isFree){
      //Here is merge next
      curr->size += curr->next->size;
      if(curr->next->next!=NULL){
	curr->next->next->prev = curr;
      }
      curr->next = curr->next->next;
    }
  }
  if(curr->prev->isFree){
    //Here is merge prev
    curr->prev->size += curr->size;
    if(curr->next!=NULL){
      curr->next->prev = curr->prev;
    }
    curr->prev->next = curr->next;
  }
}

void merge_block_nolock(block * curr){
  
  if(curr->next != NULL){
    if(curr->next->isFree){
      if(curr + curr->size == curr->next){
	//in this thread, current and next are ajacent
  
	//Here is merge next
	curr->size += curr->next->size;
	if(curr->next->next!=NULL){
	  curr->next->next->prev = curr;
	}
	curr->next = curr->next->next;
      }
    }
  }
  if(curr->prev->isFree){
    if(curr == curr->prev + curr->prev->size){
      //Here is merge prev
      curr->prev->size += curr->size;
      if(curr->next!=NULL){
	curr->next->prev = curr->prev;
      }
      curr->prev->next = curr->next;
    }
  }

}

/*********** Lock Version ************/
void ts_free_lock(void * ptr){
  //Here is ff_free
  block *curr = (block *)(ptr-sizeof(block));
  curr->isFree = true;
  freeSeg += curr->size;
  //  merge_block(curr);
}
/*********** Non-Lock Version ************/
void ts_free_nolock(void * ptr){
  block *curr = (block *)(ptr-sizeof(block));
  curr->isFree = true;
  freeSeg += curr->size;
  //  merge_block_nolock(curr);
}

unsigned long get_data_segment_size(){
  return seg;
}

unsigned long get_data_segment_free_space_size(){
  return freeSeg;
}
