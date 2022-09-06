#include <rtthread.h>
#include <rtdbg.h>

#include <microros_rtthread_net_transport.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <std_msgs/msg/int32.h>

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){ LOG_E("micro-ROS error line %d ! \r\n", __LINE__); return; }}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){LOG_W("micro-ROS error on line %d, Continuing\r\n", __LINE__);}}

static rcl_subscription_t subscriber;
static std_msgs__msg__Int32 msg;

static rclc_executor_t executor;
static rclc_support_t support;
static rcl_allocator_t allocator;

static rcl_node_t node;

static void subscription_callback(const void * msgin)
{
    const std_msgs__msg__Int32 * msg = (const std_msgs__msg__Int32 *)msgin;
    LOG_I("[microros] Received data: %d", msg->data);
}

static void microros_sub_thread_entry(void *parameter)
{
    (void) parameter;

    while(1)
    {
        RCSOFTCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(1000)));
        rt_thread_mdelay(100);
    }
}

static void microros_sub(int argc, char* argv[])
{
    if (3 > argc || 0 == atoi(argv[2]))
    {
        LOG_E("Expected arguments: IP Port\n");
        return;
    }

    // Configure network transport
    char * agent_address = argv[1];
    uint16_t agent_port = (uint16_t)atoi(argv[2]);
    set_microros_net_transport(agent_address, agent_port);

    allocator = rcl_get_default_allocator();

    // Initialize micro-ROS
    RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));
    LOG_D("[microros] Connected to agent");

    // Create node
    RCCHECK(rclc_node_init_default(&node, "microros_sub_node", "", &support));
    LOG_D("[microros] Node created");

    // Create subscriber
    RCCHECK(rclc_subscription_init_default(
      &subscriber,
      &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
      "microros_sub_topic"));
    LOG_D("[microros] Subscriber created");

    // create executor
    RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
    RCCHECK(rclc_executor_add_subscription(&executor, &subscriber, &msg, &subscription_callback, ON_NEW_DATA));
    LOG_D("[microros] Executor created");

    rt_thread_t thread = rt_thread_create("microros_sub", microros_sub_thread_entry, RT_NULL, 2048, 25, 10);
    if(thread != RT_NULL)
    {
        LOG_I("[microros] Start executor thread");
        rt_thread_startup(thread);
    }
    else
    {
        LOG_E("[microros] Failed to create executor thread");
    }
}
MSH_CMD_EXPORT(microros_sub, microros subscriber example)