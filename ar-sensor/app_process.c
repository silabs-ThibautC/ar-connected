/***************************************************************************//**
 * @file app_process.c
 * @brief app_process.c
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

// -----------------------------------------------------------------------------
//                                   Includes
// -----------------------------------------------------------------------------
#include PLATFORM_HEADER
#include "stack/include/ember.h"
#include "hal/hal.h"
#include "em_chip.h"
#include "sl_flex_assert.h"
#include "sl_si70xx.h"
#include "sl_i2cspm_instances.h"
#include "poll.h"
#include "sl_app_common.h"
#include "app_process.h"
#include "app_framework_common.h"
#if defined(SL_CATALOG_LED0_PRESENT)
#include "sl_simple_led_instances.h"
#endif

#include "sl_simple_button_instances.h"

// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------
#define MAX_TX_FAILURES     (10u)
// -----------------------------------------------------------------------------
//                          Static Function Declarations
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------
/// Global flag set by a button push to allow or disallow entering to sleep
bool enable_sleep = false;
/// report timing event control
EmberEventControl *report_control;
/// report timing period
uint16_t sensor_report_period_ms =  (1 * MILLISECOND_TICKS_PER_SECOND);
/// TX options set up for the network
EmberMessageOptions tx_options = EMBER_OPTIONS_ACK_REQUESTED | EMBER_OPTIONS_SECURITY_ENABLED;

// -----------------------------------------------------------------------------
//                                Static Variables
// -----------------------------------------------------------------------------
/// Destination of the currently processed sink node
static EmberNodeId sink_node_id = EMBER_NULL_NODE_ID;

// -----------------------------------------------------------------------------
//                          Public Function Definitions
// -----------------------------------------------------------------------------
void sl_button_on_change(const sl_button_t *handle)
{
  if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED) {
    enable_sleep = !enable_sleep;
  }
}

/**************************************************************************//**
 * Here we print out the first two bytes reported by the sinks as a little
 * endian 16-bits decimal.
 *****************************************************************************/
void report_handler(void)
{
  if (!emberStackIsUp()) {
    emberEventControlSetInactive(*report_control);
  } else {
    EmberStatus status;
    EmberStatus sensor_status = EMBER_SUCCESS;
    uint8_t buffer[SENSOR_SINK_DATA_LENGTH];
    int32_t temp_data = 0;
    uint32_t rh_data = 0;
    uint8_t i;

    // Sample temperature and humidity from sensors.
    // Temperature is sampled in "millicelsius".
    #ifndef UNIX_HOST
    if (sl_si70xx_measure_rh_and_temp(sl_i2cspm_sensor,
                                      SI7021_ADDR,
                                      &rh_data,
                                      &temp_data)) {
      sensor_status = EMBER_ERR_FATAL;
      APP_INFO("Warning! Invalid Si7021 reading: %lu %ld\n", rh_data, temp_data);
    }
    #endif

    if (sensor_status == EMBER_SUCCESS) {
      emberStoreLowHighInt32u(buffer, temp_data);
      emberStoreLowHighInt32u(buffer + 4, rh_data);

      status = emberMessageSend(sink_node_id,
                                0, // endpoint
                                0, // messageTag
                                SENSOR_SINK_DATA_LENGTH,
                                buffer,
                                tx_options);

      APP_INFO("TX: Data to 0x%04X:", sink_node_id);
      for (i = 0; i < SENSOR_SINK_DATA_LENGTH; i++) {
        APP_INFO(" %02X", buffer[i]);
      }
      APP_INFO(": 0x%02X\n", status);
      emberEventControlSetDelayMS(*report_control, sensor_report_period_ms);
    }
  }
}

/**************************************************************************//**
 * Entering sleep is approved or denied in this callback, depending on user
 * demand.
 *****************************************************************************/
bool emberAfCommonOkToEnterLowPowerCallback(bool enter_em2, uint32_t duration_ms)
{
  (void) enter_em2;
  (void) duration_ms;
  return enable_sleep;
}

/**************************************************************************//**
 * This function is called when a message is received.
 *****************************************************************************/
void emberAfIncomingMessageCallback(EmberIncomingMessage *message)
{
  uint8_t i;
  APP_INFO("RX: Data from 0x%04X:", message->source);
  for (i = SENSOR_SINK_DATA_OFFSET; i < message->length; i++) {
    APP_INFO(" %x", message->payload[i]);
  }
  APP_INFO("\n");
}

/**************************************************************************//**
 * This function is called to indicate whether an outgoing message was
 * successfully transmitted or to indicate the reason of failure.
 *****************************************************************************/
void emberAfMessageSentCallback(EmberStatus status,
                                EmberOutgoingMessage *message)
{
  (void) message;
  if (status != EMBER_SUCCESS) {
    APP_INFO("TX: 0x%02X\n", status);
  }
}

/**************************************************************************//**
 * This function is called when the stack status changes.
 *****************************************************************************/
void emberAfStackStatusCallback(EmberStatus status)
{
  switch (status) {
    case EMBER_NETWORK_UP:
      APP_INFO("Network up\n");
      APP_INFO("Joined to Sink with node ID: 0x%04X\n", emberGetNodeId());
      // Schedule start of periodic sensor reporting to the Sink
      emberEventControlSetDelayMS(*report_control, sensor_report_period_ms);
      break;
    case EMBER_NETWORK_DOWN:
      APP_INFO("Network down\n");
      break;
    case EMBER_JOIN_SCAN_FAILED:
      APP_INFO("Scanning during join failed\n");
      break;
    case EMBER_JOIN_DENIED:
      APP_INFO("Joining to the network rejected!\n");
      break;
    case EMBER_JOIN_TIMEOUT:
      APP_INFO("Join process timed out!\n");
      break;
    default:
      APP_INFO("Stack status: 0x%02X\n", status);
      break;
  }
}

/**************************************************************************//**
 * This callback is called in each iteration of the main application loop and
 * can be used to perform periodic functions.
 *****************************************************************************/
void emberAfTickCallback(void)
{
#if defined(SL_CATALOG_LED0_PRESENT)
  if (emberStackIsUp()) {
    sl_led_turn_on(&sl_led_led0);
  } else {
    sl_led_turn_off(&sl_led_led0);
  }
#endif
}

/**************************************************************************//**
 * This function is called when a frequency hopping client completed the start
 * procedure.
 *****************************************************************************/
void emberAfFrequencyHoppingStartClientCompleteCallback(EmberStatus status)
{
  if (status != EMBER_SUCCESS) {
    APP_INFO("FH Client sync failed, status=0x%02X\n", status);
  } else {
    APP_INFO("FH Client Sync Success\n");
  }
}

/**************************************************************************//**
 * This function is called when a requested energy scan is complete.
 *****************************************************************************/
void emberAfEnergyScanCompleteCallback(int8_t mean,
                                       int8_t min,
                                       int8_t max,
                                       uint16_t variance)
{
  APP_INFO("Energy scan complete, mean=%d min=%d max=%d var=%d\n",
           mean, min, max, variance);
}

#if defined(EMBER_AF_PLUGIN_MICRIUM_RTOS) && defined(EMBER_AF_PLUGIN_MICRIUM_RTOS_APP_TASK1)

/**************************************************************************//**
 * This function is called from the Micrium RTOS plugin before the
 * Application (1) task is created.
 *****************************************************************************/
void emberAfPluginMicriumRtosAppTask1InitCallback(void)
{
  APP_INFO("app task init\n");
}

#include <kernel/include/os.h>
#define TICK_INTERVAL_MS 1000

/**************************************************************************//**
 * This function implements the Application (1) task main loop.
 *****************************************************************************/
void emberAfPluginMicriumRtosAppTask1MainLoopCallback(void *p_arg)
{
  RTOS_ERR err;
  OS_TICK yield_time_ticks = (OSCfg_TickRate_Hz * TICK_INTERVAL_MS) / 1000;

  while (true) {
    APP_INFO("app task tick\n");

    OSTimeDly(yield_time_ticks, OS_OPT_TIME_DLY, &err);
  }
}

#endif // EMBER_AF_PLUGIN_MICRIUM_RTOS && EMBER_AF_PLUGIN_MICRIUM_RTOS_APP_TASK1
