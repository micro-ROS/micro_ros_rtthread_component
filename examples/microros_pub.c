#include <rtthread.h>
#include <rtdbg.h>

#include <microros_rtthread_net_transport.h>
#include <microros_allocators.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <std_msgs/msg/int32.h>

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){ LOG_E("micro-ROS error line %d ! \r\n", __LINE__); return; }}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){LOG_W("micro-ROS error on line %d, Continuing\r\n", __LINE__);}}

static rcl_publisher_t publisher;
static std_msgs__msg__Int32 msg;

static rclc_executor_t executor;
static rclc_support_t support;
static rcl_allocator_t allocator;

static rcl_node_t node;
static rcl_timer_t timer;

static void timer_callback(rcl_timer_t * timer, int64_t last_call_time)
{
    (void) last_call_time;

    if (timer != NULL)
    {
        RCSOFTCHECK(rcl_publish(&publisher, &msg, NULL));
        LOG_I("[microros] Publish data: %d", msg.data);
        msg.data++;
    }
}

static void microros_pub_thread_entry(void *parameter)
{
    (void) parameter;

    while(1)
    {
        RCSOFTCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100)));
        rt_thread_mdelay(100);
    }
}

static void microros_pub(int argc, char* argv[])
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
    RCCHECK(rclc_node_init_default(&node, "microros_pub_node", "", &support));
    LOG_D("[microros] Node created");

    // Create publisher
    RCCHECK(rclc_publisher_init_default(
      &publisher,
      &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
      "microros_pub"));

    LOG_D("[microros] Publisher created");

    // create timer
    const unsigned int timer_timeout = 1000;
    RCCHECK(rclc_timer_init_default(
      &timer,
      &support,
      RCL_MS_TO_NS(timer_timeout),
      timer_callback));

    LOG_D("[microros] Timer created");

    // create executor
    RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
    RCCHECK(rclc_executor_add_timer(&executor, &timer));

    LOG_D("[microros] Executor created");

    msg.data = 0;

    rt_thread_t thread = rt_thread_create("microros_pub", microros_pub_thread_entry, RT_NULL, 2048, 25, 10);
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
MSH_CMD_EXPORT(microros_pub, microros publisher example)
