#ifndef MICROROS_ALLOCATORS_H_
#define MICROROS_ALLOCATORS_H_

#include <rtthread.h>

#include <stdlib.h>
#include <stdint.h>

#include <rcl/rcl.h>

static void * custom_allocate(size_t size, void * state) { return rt_malloc(size); };
static void custom_deallocate(void * pointer, void * state){ return rt_free(pointer); };
static void * custom_reallocate(void * pointer, size_t size, void * state){ return rt_realloc(pointer, size); };
static void * custom_zero_allocate(size_t number_of_elements, size_t size_of_element, void * state){ return rt_calloc(number_of_elements, size_of_element); };

static inline void set_microros_allocators() {

    rcl_allocator_t allocator = rcutils_get_zero_initialized_allocator();
    allocator.allocate = custom_allocate;
    allocator.deallocate = custom_deallocate;
    allocator.reallocate = custom_reallocate;
    allocator.zero_allocate = custom_zero_allocate;

    (void)! rcutils_set_default_allocator(&allocator);
}

#endif  // MICROROS_ALLOCATORS_H_