/***************************************************************************//**
 * @file app_callbacks.c
 * @brief app_callbacks.c
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
#include "sl_app_common.h"
#include "app_framework_common.h"
#include "sl_simple_led_instances.h"

// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------
/// Table of sensors paired with the sink.
sensor sensors[SENSOR_TABLE_SIZE];
/// TX options set up for the network
EmberMessageOptions tx_options = EMBER_OPTIONS_ACK_REQUESTED;
/// Advertising period event control
EmberEventControl *advertise_control;
/// Data dump period  event control
EmberEventControl *data_report_control;

// -----------------------------------------------------------------------------
//                                Static Variables
// -----------------------------------------------------------------------------
/// Current contents of the data sent with send()
static uint8_t message[SENSOR_SINK_MAXIMUM_LENGTH];
/// Length of the message data to eb sent with send()
static EmberMessageLength message_length;

// -----------------------------------------------------------------------------
//                          Static Function Declarations
// -----------------------------------------------------------------------------
/**************************************************************************//**
 * Housekeeping init of the sensor data.
 *****************************************************************************/
static void sink_init(void);

/**************************************************************************//**
 * Helper function to send messages to sensors.
 *
 * @param node_id is the destination sink node ID
 * @param command_id is the command that is being sent to the sink node
 * @param *buffer is a piece of data related to the command
 * @param buffer_length is the length of the buffer
 * @returns Returns an EMBER_SUCCESS if successful or the reason of failure.
 *****************************************************************************/
static EmberStatus send(EmberNodeId node_id,
                        sensor_sink_command_id command_id,
                        uint8_t *buffer,
                        uint8_t buffer_length);

// -----------------------------------------------------------------------------
//                          Public Function Definitions
// -----------------------------------------------------------------------------
/**************************************************************************//**
 * An advertisement message consists of the sensor/sink protocol id, the
 * advertisement command id, and the long and short ids of the sink.  Each sink
 * on the network periodically broadcasts its advertisement to all other nodes.
 *****************************************************************************/
void advertise_handler(void)
{
  // If the sink is not on the network, the periodic event is cancelled and
  // advertisements are not set.
  if (!emberStackIsUp()) {
    emberEventControlSetInactive(*advertise_control);
  } else {
    EmberStatus status = send(EMBER_BROADCAST_ADDRESS,
                              SENSOR_SINK_COMMAND_ID_ADVERTISE,
                              NULL,
                              0);
    APP_INFO("TX: Advertise to 0x%04X: 0x%02X\n",
             EMBER_BROADCAST_ADDRESS,
             status);
    emberEventControlSetDelayMS(*advertise_control,
                                SINK_ADVERTISEMENT_PERIOD_MS);
  }
}

/**************************************************************************//**
 * Here we print out the first two bytes reported by the sinks as a little
 * endian 16-bits decimal.
 *****************************************************************************/
void data_report_handler(void)
{
  // If the sink is not on the network, the periodic event is cancelled and
  // sensors data is no longer printed out.
  if (!emberStackIsUp()) {
    emberEventControlSetInactive(*data_report_control);
  } else {
    uint8_t i;
    for (i = 0; i < SENSOR_TABLE_SIZE; i++) {
      if (sensors[i].node_id != EMBER_NULL_NODE_ID
          && sensors[i].reported_data_length >= 2) {
        // Temperature is sampled in "millicelsius".
        int32_t temperature = emberFetchLowHighInt32u(sensors[i].reported_data);
        APP_INFO("< %02X%02X%02X%02X%02X%02X%02X%02X , %d.%d%d >\n",
                 sensors[i].node_eui64[7], sensors[i].node_eui64[6],
                 sensors[i].node_eui64[5], sensors[i].node_eui64[4],
                 sensors[i].node_eui64[3], sensors[i].node_eui64[2],
                 sensors[i].node_eui64[1], sensors[i].node_eui64[0],
                 temperature / 1000,
                 (temperature % 1000) / 100,
                 (temperature % 100) / 10);
      }
    }
    emberEventControlSetDelayMS(*data_report_control,
                                SINK_DATA_DUMP_PERIOD_MS);
  }
}

/******************************************************************************
* Application framework init callback
******************************************************************************/
void emberAfInitCallback(void)
{
  emberAfAllocateEvent(&advertise_control, &advertise_handler);
  emberAfAllocateEvent(&data_report_control, &data_report_handler);
  // CLI info message
  APP_INFO("Sink\n");

  emberSetSecurityKey(&security_key);
  sink_init();
  emberNetworkInit();

#if defined(EMBER_AF_PLUGIN_BLE)
  bleConnectionInfoTableInit();
#endif
}

/**************************************************************************//**
 * This function is called when a message is received.
 *****************************************************************************/
void emberAfIncomingMessageCallback(EmberIncomingMessage *message)
{
  if (message->length < SENSOR_SINK_MINIMUM_LENGTH
      || (emberFetchLowHighInt16u(message->payload + SENSOR_SINK_PROTOCOL_ID_OFFSET)
          != SENSOR_SINK_PROTOCOL_ID)) {
    return;
  }

  switch (message->payload[SENSOR_SINK_COMMAND_ID_OFFSET]) {
    case SENSOR_SINK_COMMAND_ID_ADVERTISE_REQUEST:
    {
      EmberStatus status;
      APP_INFO("RX: Advertise Request from 0x%04X\n", message->source);

      // We received an advertise request from a sensor, unicast back an advertise
      // command.
      status = send(message->source,
                    SENSOR_SINK_COMMAND_ID_ADVERTISE,
                    NULL,
                    0);
      APP_INFO("TX: Advertise to 0x%04X: 0x%02X\n",
               message->source,
               status);
    }
    break;
    case SENSOR_SINK_COMMAND_ID_ADVERTISE:
      APP_INFO("RX: Advertise from 0x%04X\n", message->source);
      break;
    case SENSOR_SINK_COMMAND_ID_PAIR_REQUEST:
    {
      uint8_t i;
      APP_INFO("RX: Pair Request from 0x%04X\n", message->source);
      // Check whether the sensor is already present in the table first.
      for (i = 0; i < SENSOR_TABLE_SIZE; i++) {
        if (sensors[i].node_id != EMBER_NULL_NODE_ID
            && MEMCOMPARE(sensors[i].node_eui64,
                          message->payload + SENSOR_SINK_EUI64_OFFSET,
                          EUI64_SIZE) == 0) {
          break;
        }
      }

      // Find an empty entry in the table.
      if (i == SENSOR_TABLE_SIZE) {
        for (i = 0; i < SENSOR_TABLE_SIZE; i++) {
          if (sensors[i].node_id == EMBER_NULL_NODE_ID) {
            break;
          }
        }
      }

      // Add or update the entry in the table.
      if (i < SENSOR_TABLE_SIZE) {
        EmberStatus status = send(message->source,
                                  SENSOR_SINK_COMMAND_ID_PAIR_CONFIRM,
                                  NULL,
                                  0);
        APP_INFO("TX: Pair Confirm to 0x%04X: 0x%02X\n",
                 message->source,
                 status);
        if (status == EMBER_SUCCESS) {
          sensors[i].node_id = message->source;

          MEMCOPY(sensors[i].node_eui64,
                  message->payload + SENSOR_SINK_EUI64_OFFSET,
                  EUI64_SIZE);

          sensors[i].last_report_ms = halCommonGetInt32uMillisecondTick();
        }
      }
    }
    break;
    case SENSOR_SINK_COMMAND_ID_PAIR_CONFIRM:
      APP_INFO("RX: Pair Confirm from 0x%04X\n", message->source);
      break;
    case SENSOR_SINK_COMMAND_ID_DATA:
    {
      uint8_t i, j;
      for (i = 0; i < SENSOR_TABLE_SIZE; i++) {
        if (MEMCOMPARE(sensors[i].node_eui64,
                       message->payload + SENSOR_SINK_EUI64_OFFSET,
                       EUI64_SIZE) == 0) {
          APP_INFO("RX: Data from 0x%04X:", message->source);
          for (j = SENSOR_SINK_DATA_OFFSET; j < message->length; j++) {
            APP_INFO(" %02X", message->payload[j]);
          }
          APP_INFO("\n");

          sensors[i].reported_data_length = message->length - SENSOR_SINK_DATA_OFFSET;

          MEMCOPY(sensors[i].reported_data,
                  message->payload + SENSOR_SINK_DATA_OFFSET,
                  sensors[i].reported_data_length);

          sensors[i].last_report_ms = halCommonGetInt32uMillisecondTick();
          break;
        }
      }
    }
    break;
    default:
      APP_INFO("RX: Unknown from 0x%04X\n", message->source);
      break;
  }
}

/**************************************************************************//**
 * This function is called to indicate whether an outgoing message was
 * successfully transmitted or to indicate the reason of failure.
 *****************************************************************************/
void emberAfMessageSentCallback(EmberStatus status,
                                EmberOutgoingMessage *message)
{
  if (status != EMBER_SUCCESS) {
    APP_INFO("TX: 0x%02X\n", status);
  }
}

/**************************************************************************//**
 * This function is called by the application framework from the stack status
 * handler.
 *****************************************************************************/
void emberAfStackStatusCallback(EmberStatus status)
{
  switch (status) {
    case EMBER_NETWORK_UP:
      APP_INFO("Network up\n");

      emberEventControlSetActive(*advertise_control);
      emberEventControlSetActive(*data_report_control);
      break;
    case EMBER_NETWORK_DOWN:
      APP_INFO("Network down\n");
      sink_init();
      break;
    default:
      APP_INFO("Stack status: 0x%02X\n", status);
      break;
  }
}

/**************************************************************************//**
 * This function is called in each iteration of the main application loop and
 * can be used to perform periodic functions.
 *****************************************************************************/
void emberAfTickCallback(void)
{
  // Time out sensors that have not reported in a while.
  uint32_t nowMs = halCommonGetInt32uMillisecondTick();
  uint8_t i;
  for (i = 0; i < SENSOR_TABLE_SIZE; i++) {
    if (sensors[i].node_id != EMBER_NULL_NODE_ID) {
      if (SENSOR_TIMEOUT_MS
          < elapsedTimeInt32u(sensors[i].last_report_ms, nowMs)) {
        APP_INFO("EVENT: timed out sensor 0x%04X\n",
                 sensors[i].node_id);
        sensors[i].node_id = EMBER_NULL_NODE_ID;
      }
    }
  }

  if (emberStackIsUp()) {
    sl_led_turn_on(&sl_led_led0);
  } else {
    sl_led_turn_off(&sl_led_led0);
  }
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

// -----------------------------------------------------------------------------
//                          Static Function Definitions
// -----------------------------------------------------------------------------s
/**************************************************************************//**
 * Housekeeping init of the sensor data.
 *****************************************************************************/
static void sink_init(void)
{
  uint8_t i;
  for (i = 0; i < SENSOR_TABLE_SIZE; i++) {
    sensors[i].node_id = EMBER_NULL_NODE_ID;
  }
}

/**************************************************************************//**
   Helper function to send messages to sensors.
 *****************************************************************************/
static EmberStatus send(EmberNodeId node_id,
                        sensor_sink_command_id command_id,
                        uint8_t *buffer,
                        uint8_t buffer_length)
{
  message_length = 0;
  emberStoreLowHighInt16u(message + message_length, SENSOR_SINK_PROTOCOL_ID);
  message_length += 2;
  message[message_length++] = command_id;
  MEMCOPY(message + message_length, emberGetEui64(), EUI64_SIZE);
  message_length += EUI64_SIZE;
  emberStoreLowHighInt16u(message + message_length, emberGetNodeId());
  message_length += 2;
  if (buffer_length != 0) {
    MEMCOPY(message + message_length, buffer, buffer_length);
    message_length += buffer_length;
  }
  return emberMessageSend(node_id,
                          0, // endpoint
                          0, // messageTag
                          message_length,
                          message,
                          tx_options);
}
