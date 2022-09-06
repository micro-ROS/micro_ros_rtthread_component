#include <microros_rtthread_net_transport.h>

#include <rtthread.h>
#include <sys/socket.h>
#include "netdb.h"

#include <rtdbg.h>

int sock = -1;
struct sockaddr_in server_addr = {};
bool initialized = false;

bool rtthread_net_transport_open(struct uxrCustomTransport * transport)
{
    if (initialized) {
        return initialized;
    }

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        LOG_E("Create socket error");
        return false;
    }

    // Save agent address
    custom_net_transport_args * args = (custom_net_transport_args*) transport->args;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(args->agent_port);
    inet_aton(args->agent_ip_address, &server_addr.sin_addr.s_addr);
    rt_memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));

    initialized = true;
    return true;
}

bool rtthread_net_transport_close(struct uxrCustomTransport * transport)
{
  return true;
}

size_t rtthread_net_transport_write(
  struct uxrCustomTransport * transport, const uint8_t * buf,
  size_t len, uint8_t * errcode)
{
    int ret = sendto(sock, buf, len, 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
    return (ret > 0) ? ret : 0;
}

size_t rtthread_net_transport_read(
  struct uxrCustomTransport * transport, uint8_t * buf, size_t len,
  int timeout, uint8_t * errcode)
{
    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(sock, &readset);

    timeout = (timeout == 0) ? 1 : timeout;

    struct timeval net_timeout = {.tv_sec = timeout / 1000, .tv_usec = (timeout % 1000) * 1000};

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &net_timeout, sizeof(net_timeout));

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(struct sockaddr);
    int bytes_read = recvfrom(sock, buf, len, 0, (struct sockaddr *)&client_addr, &addr_len);

    return (bytes_read > 0) ? bytes_read : 0;
}