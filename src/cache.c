//========================================================//
//  cache.c                                               //
//  Source file for the Cache Simulator                   //
//                                                        //
//  Implement the I-cache, D-Cache and L2-cache as        //
//  described in the README                               //
//========================================================//
#include <math.h>
#include "cache.h"

//
// TODO:Student Information
//
const char *studentName = "Dongchen Yang";
const char *studentID   = "A59003267";
const char *email       = "doy001@ucsd.edu";

//------------------------------------//
//        Cache Configuration         //
//------------------------------------//

uint32_t icacheSets;     // Number of sets in the I$
uint32_t icacheAssoc;    // Associativity of the I$
uint32_t icacheHitTime;  // Hit Time of the I$

uint32_t dcacheSets;     // Number of sets in the D$
uint32_t dcacheAssoc;    // Associativity of the D$
uint32_t dcacheHitTime;  // Hit Time of the D$

uint32_t l2cacheSets;    // Number of sets in the L2$
uint32_t l2cacheAssoc;   // Associativity of the L2$
uint32_t l2cacheHitTime; // Hit Time of the L2$
uint32_t inclusive;      // Indicates if the L2 is inclusive

uint32_t blocksize;      // Block/Line size
uint32_t memspeed;       // Latency of Main Memory

//------------------------------------//
//          Cache Statistics          //
//------------------------------------//

uint64_t icacheRefs;       // I$ references
uint64_t icacheMisses;     // I$ misses
uint64_t icachePenalties;  // I$ penalties

uint64_t dcacheRefs;       // D$ references
uint64_t dcacheMisses;     // D$ misses
uint64_t dcachePenalties;  // D$ penalties

uint64_t l2cacheRefs;      // L2$ references
uint64_t l2cacheMisses;    // L2$ misses
uint64_t l2cachePenalties; // L2$ penalties

//------------------------------------//
//        Cache Data Structures       //
//------------------------------------//

//
//TODO: Add your Cache data structures here
//
//here I referred to the implementation of 
//https://github.com/Fnjn


struct Block
{
  uint32_t value;
  struct Block *previous, *next;
};
typedef struct Block Block;


struct Set
{
  uint32_t size;
  Block *front, *back;
};
typedef struct Set Set;

Block* create_Block(uint32_t value)
{
  // define the block
  Block *block = (Block*)malloc(sizeof(Block));  
  //default to be null, will change later if we push
  block->value = value;
  block->next = NULL;
  block->previous = NULL;  
  return block;
}

void setPush(Set* set,  Block *block)
{
  // If size of the set greater than 0
  if(set->size > 0)
  { 
    (set->back)->next = block;
    (block->previous) = (set->back);
    (set->back) = block;
  }
  // If size = 0, which means it is empty
  else if ((set->size) ==0)
  { 
    (set->front) = block;
    (set->back) = block;
  }
  else
  {
    (set->front) = block;
    (set->back) = block;
  }
  (set->size)++;
}

// Pop front
void setPop(Set* set){
  // If set is empty
  if((set->size) == 0)
    return;
//pop out the first one
  Block *b_pop = set->front;
  (set->front) = (b_pop->next);

  if(set->front)
    ((set->front)->previous) = NULL;
  (set->size)--;
  free(b_pop);
}

Block* setPopIndex(Set* set, int idx){
 // pop out a specific idx from cache
  if(idx > (set->size) || idx<0)
  //invalid input return null
    return NULL;
  Block *b_pop = (set->front);
  if((set->size) == 1){
    (set->front) = NULL;
    (set->back) = NULL;
  }
  else if (idx == 0)
  {
    (set->front) = (b_pop->next);
    ((set->front)->previous) = NULL;
  }
  else if (idx == ((set->size) - 1))
  {
    b_pop = (set->back);
    (set->back) = ((set->back)->previous);
    ((set->back)->next) = NULL;
  }
  else{
    for(int i=0; i<idx; i++)
      b_pop = (b_pop->next);
    ((b_pop->previous)->next) = (b_pop->next);
    ((b_pop->next)->previous) = (b_pop->previous);
  }

  (b_pop->next) = NULL;
  (b_pop->previous) = NULL;

  (set->size)--;
  //Block ownership transfer to caller
  return b_pop;
}






uint32_t offset_size;
uint32_t offset_mask;

Set *icache;
uint32_t icache_idx_mask;
uint32_t icache_idx_size;

Set *dcache;
uint32_t dcache_idx_mask;
uint32_t dcache_idx_size;

Set *l2cache;
uint32_t l2cache_idx_mask;
uint32_t l2cache_idx_size;


//------------------------------------//
//          Cache Functions           //
//------------------------------------//

// Initialize the Cache Hierarchy
//
void 
init_cache()
{
  // Initialize cache stats
  icacheRefs        = 0;
  icacheMisses      = 0;
  icachePenalties   = 0;
  dcacheRefs        = 0;
  dcacheMisses      = 0;
  dcachePenalties   = 0;
  l2cacheRefs       = 0;
  l2cacheMisses     = 0;
  l2cachePenalties  = 0;
  
  //
  //TODO: Initialize Cache Simulator Data Structures
  //

  icache = (Set*)malloc(icacheSets * sizeof(Set));
  dcache = (Set*)malloc(dcacheSets * sizeof(Set) );
  l2cache = (Set*)malloc(l2cacheSets * sizeof(Set));

//init the sets

  for(int i=0; i <icacheSets; i++)
  {
    icache[i].size = 0;
    icache[i].front = NULL;
    icache[i].back = NULL;
  }
//init the sets
  for(int i=0; i <dcacheSets; i++)
  {
    dcache[i].size = 0;
    dcache[i].front = NULL;
    dcache[i].back = NULL;
  }
//init the sets
  for(int i=0; i <l2cacheSets; i++)
  {
    l2cache[i].size = 0;
    l2cache[i].front = NULL;
    l2cache[i].back = NULL;
  }

  offset_size = (uint32_t)log2(blocksize);
  if ((1<<offset_size)==blocksize)
  {
    offset_size += 0;
  }
  else
  {
    offset_size += 1;
  }
  offset_mask = (1 << offset_size) - 1;
  icache_idx_size = (uint32_t)log2(icacheSets);

  if ((1<<icache_idx_size) == icacheSets){
      icache_idx_size += 0;
  }
  else
  {
    icache_idx_size += 1;
  }
 

  dcache_idx_size = (uint32_t)log2(dcacheSets);

  if ((1<<dcache_idx_size) == dcacheSets){
      dcache_idx_size += 0;
  }
  else
  {
    dcache_idx_size += 1;
  }

  l2cache_idx_size = (uint32_t)log2(l2cacheSets);


  if ((1<<l2cache_idx_size) == l2cacheSets){
      l2cache_idx_size += 0;
  }
  else
  {
    l2cache_idx_size += 1;
  }
  // 0s for non masked
  icache_idx_mask = ((1 << icache_idx_size) - 1) << offset_size;
  dcache_idx_mask = ((1 << dcache_idx_size) - 1) << offset_size;
  l2cache_idx_mask = ((1 << l2cache_idx_size) - 1) << offset_size;

}




// Perform a memory access through the icache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
icache_access(uint32_t addr)
{
  //
  //TODO: Implement I$
  //
// If not intialized, get it in l2
  if(icacheSets==0){
    return l2cache_access(addr);
  }
  // If intialized, go see if it is a hit

  icacheRefs++;

  // get offset, idx, and tag trough masking and shifting
  uint32_t offset = offset_mask & addr;
  uint32_t idx = (icache_idx_mask & addr) >> offset_size;
  uint32_t tag = addr >> (icache_idx_size + offset_size);

  Block *block_pop = icache[idx].front;
  //see if it is a hit
  for(int i=0; i<icache[idx].size; i++){
    if(block_pop->value == tag){ // Hit
      Block *b = setPopIndex(&icache[idx], i); // Get the hit block
      setPush(&icache[idx],  b); // move to end of set queue
      return icacheHitTime;
    }
    block_pop = block_pop->next;
  }
 //it is a miss
  icacheMisses++;

  uint32_t penalty = l2cache_access(addr);
  icachePenalties += penalty;

  // Miss replacement
  Block *block_new = create_Block(tag);

  if(icache[idx].size == icacheAssoc) // sest filled, pop out the last element
    setPop(&icache[idx]);
  setPush(&icache[idx],  block_new);

  return icacheHitTime + penalty;
}

// Perform a memory access through the dcache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
dcache_access(uint32_t addr)
{
  //
  //TODO: Implement D$
  //
// If not intialized, get in l2
  if(dcacheSets==0){
    return l2cache_access(addr);
  }
  dcacheRefs++ ;
 // get offset, idx, and tag trough masking and shifting
  uint32_t offset = offset_mask & addr;
  uint32_t idx = (dcache_idx_mask & addr) >> offset_size;
  uint32_t tag = addr >> (dcache_idx_size + offset_size);

  Block *block_pop = dcache[idx].front;

  for(int i=0; i<dcache[idx].size; i++){
    if(block_pop->value == tag){ // Hit
      Block *b = setPopIndex(&dcache[idx], i); 
      setPush(&dcache[idx],  b); // move to end of sets
      return dcacheHitTime;
    }
    block_pop = block_pop->next;
  }

  dcacheMisses += 1;


  uint32_t penalty = l2cache_access(addr);
  dcachePenalties += penalty;

  // Miss replacement - LRU
  Block *block_new = create_Block(tag);

  if(dcache[idx].size == dcacheAssoc) // set filled, replace LRU (front of set queue)
    setPop(&dcache[idx]);
  setPush(&dcache[idx],  block_new);

  return dcacheHitTime + penalty;
}

// Perform a memory access to the l2cache for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
l2cache_access(uint32_t addr)
{
  //
  //TODO: Implement L2$
  //

  if(l2cacheSets==0){
    return memspeed;
  }
  // If intialized, go
  l2cacheRefs ++;

  uint32_t offset = offset_mask & addr;
  uint32_t idx = (l2cache_idx_mask & addr) >> offset_size;
  uint32_t tag = addr >> (l2cache_idx_size + offset_size);

  Block *block_pop = l2cache[idx].front;

  for(int i=0; i<l2cache[idx].size; i++){
    if(block_pop->value == tag){ // Hit
      Block *b = setPopIndex(&l2cache[idx], i); // Get the hit block
      setPush(&l2cache[idx],  b); // move to end of set queue
      return l2cacheHitTime;
    }
    block_pop = block_pop->next;
  }

  l2cacheMisses += 1;

  // Miss replacement - LRU
  Block *block_new = create_Block(tag);
  
  if(l2cache[idx].size == l2cacheAssoc){ // set filled, replace LRU 

    setPop(&l2cache[idx]);
    // else suppose it's a non-inclusive cache as for the requirement
  }
  setPush(&l2cache[idx], block_new);

  l2cachePenalties += memspeed;
  return l2cacheHitTime + memspeed;
}
