#ifndef MICROROS_RTTHREAD_NET_TRANSPORT_H_
#define MICROROS_RTTHREAD_NET_TRANSPORT_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>


// For GNU C < 6.0.0 __attribute__((deprecated(msg))) is not supported for enums, used in rmw/types.h
#define aux__attribute__(x) __attribute__(x)
#define __attribute__(x)

#include <microros_allocators.h>
#include <rmw_microros/rmw_microros.h>

#undef __attribute__
#define __attribute__(x) aux__attribute__(x)

bool rtthread_net_transport_open(struct uxrCustomTransport * transport);
bool rtthread_net_transport_close(struct uxrCustomTransport * transport);
size_t rtthread_net_transport_write(
  struct uxrCustomTransport * transport, const uint8_t * buf,
  size_t len, uint8_t * errcode);
size_t rtthread_net_transport_read(
  struct uxrCustomTransport * transport, uint8_t * buf, size_t len,
  int timeout, uint8_t * errcode);

typedef struct custom_net_transport_args {
    char* agent_ip_address;
    uint16_t agent_port;
} custom_net_transport_args;

static inline void set_microros_net_transport(char * ip_address, uint16_t agent_port) {

    // Initialize micro-ROS allocator
    set_microros_allocators();

    custom_net_transport_args remote_addr = {
        .agent_ip_address=ip_address,
        .agent_port=agent_port
    };

    rmw_uros_set_custom_transport(
        false,
        (void *) &remote_addr,
        rtthread_net_transport_open,
        rtthread_net_transport_close,
        rtthread_net_transport_write,
        rtthread_net_transport_read);
}

#endif  // MICROROS_RTTHREAD_NET_TRANSPORT_H_