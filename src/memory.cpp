// #include "memory.h"
#include <assert.h>
#include <cstring>
#include <stdint.h>
#include <stddef.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define Bytes(n)      (n)
#define Kilobytes(n)  (n << 10)
#define Megabytes(n)  (n << 20)
#define Gigabytes(n)  (((U64)n) << 30)
#define Terabytes(n)  (((U64)n) << 40)

// Linear Allocator
// Credits to gingerBill and RyanFleury
struct Arena
{
  unsigned char* mem_base;
  size_t mem_length;
  size_t curr_offset;
};

Arena* arena_alloc(size_t capacity)
{

#ifdef _WIN32
  Arena* arena = (Arena*)VirtualAlloc(0,capacity, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
#else
  Arena* arena = (Arena*) malloc(capacity);
#endif
  arena->mem_base = (unsigned char*)(arena + sizeof(arena));
  arena->mem_length = capacity;
  arena->curr_offset = sizeof(arena);
  return arena;
}

void arena_release(Arena* arena)
{
}

static bool is_power_of_two(uintptr_t x)
{
  return (x&(x-1)) == 0;
}

static uintptr_t align_forward(uintptr_t ptr, size_t align)
{
  uintptr_t p, a, modulo;
  assert(is_power_of_two(align));

  p = ptr;
  a = (uintptr_t)align;
  // Same as (p % a) but faster as 'a' is a power of two
  modulo = p & (a-1);
  if(modulo != 0)
  {
    // If 'p' address is not aligned, push the address to the
    // next value which is aligned
    p += a - modulo;
  }
  return p;
}

void* arena_push_align(Arena* arena, size_t size, size_t align)
{
  uintptr_t curr_ptr = (uintptr_t)arena->mem_base + (uintptr_t)arena->curr_offset;
  uintptr_t offset = align_forward(curr_ptr, align);
  offset -= (uintptr_t)arena->mem_base;

  if( offset + size <= arena->mem_length)
  {
    void* memory = &arena->mem_base[offset];
    arena->curr_offset = offset + size;
    // Zero new memory by default
    memset(memory, 0, size);

    return memory;
  }
  return NULL;
}

#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT (2*sizeof(void*))
#endif

void* arena_push(Arena* arena, size_t size)
{
  return arena_push_align(arena, size, DEFAULT_ALIGNMENT);
}
