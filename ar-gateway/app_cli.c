/***************************************************************************//**
 * @file app_cli.c
 * @brief app_cli.c
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
#include <string.h>
#include PLATFORM_HEADER
#include "em_chip.h"
#include "stack/include/ember.h"
#include "hal/hal.h"
#include "sl_cli.h"
#include "sl_flex_assert.h"
#include "sl_app_common.h"

// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------
/// Literal constant for print operations
#define ENABLED  "enabled"
#define DISABLED "disabled"
/// The destination endpoint of the outgoing message
#define DATA_ENDPOINT           1
#define TX_TEST_ENDPOINT        2

// -----------------------------------------------------------------------------
//                          Static Function Declarations
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------
/// Connect security key (default)
EmberKeyData security_key = { .contents = SENSOR_SINK_SECURITY_KEY };

// -----------------------------------------------------------------------------
//                                Static Variables
// -----------------------------------------------------------------------------
/// Sink TX power set by CLI command
static int16_t tx_power = SENSOR_SINK_TX_POWER;

// -----------------------------------------------------------------------------
//                          Public Function Definitions
// -----------------------------------------------------------------------------
/******************************************************************************
 * CLI - form command
 * Form a network on the specified channel.
 *****************************************************************************/
void cli_form(sl_cli_command_arg_t *arguments)
{
  EmberStatus status;
  EmberNetworkParameters parameters;

  // Initialize the security key to the default key prior to forming the
  // network.
  emberSetSecurityKey(&security_key);

  MEMSET(&parameters, 0, sizeof(EmberNetworkParameters));
  parameters.radioTxPower = tx_power;
  parameters.radioChannel = sl_cli_get_argument_uint8(arguments, 0);
  parameters.panId = SENSOR_SINK_PAN_ID;

  status = emberFormNetwork(&parameters);

  APP_INFO("form 0x%02X\n", status);
}

/******************************************************************************
 * CLI - permit join
 * Allows sensors to join the sink for a given seconds (or unlimited = 0xff)
 *****************************************************************************/
void cli_pjoin(sl_cli_command_arg_t *arguments)
{
  EmberStatus status;
  uint8_t duration = (uint8_t)sl_cli_get_argument_uint8(arguments, 0);
  size_t length = 0;
  uint8_t *contents = NULL;

  if (sl_cli_get_argument_count(arguments) > 1) {
    contents = sl_cli_get_argument_hex(arguments, 1, &length);
    status = emberSetSelectiveJoinPayload(length, contents);
  } else {
    emberClearSelectiveJoinPayload();
  }

  emberPermitJoining(duration);
}

/******************************************************************************
 * CLI - set TX power
 * Set antenna power in 0.1 dBm resolution
 *****************************************************************************/
void cli_set_tx_power(sl_cli_command_arg_t *arguments)
{
  bool save_power = false;
  tx_power = sl_cli_get_argument_int16(arguments, 0);

  // check optional parameter for storing the value we're about to set
  if (sl_cli_get_argument_count(arguments) > 1) {
    save_power =  sl_cli_get_argument_int8(arguments, 1);
  }

  if (emberSetRadioPower(tx_power, save_power) == EMBER_SUCCESS) {
    APP_INFO("TX power set: %d\n", (int16_t)emberGetRadioPower());
  } else {
    APP_INFO("TX power set failed\n");
  }
}

/******************************************************************************
 * CLI - set TX options
 * Set the option bits for ACKs, security and priority via the binary value given.
 *****************************************************************************/
void cli_set_tx_options(sl_cli_command_arg_t *arguments)
{
  tx_options = sl_cli_get_argument_uint8(arguments, 0);
  APP_INFO("TX options set: MAC acks %s, security %s, priority %s\n",
           ((tx_options & EMBER_OPTIONS_ACK_REQUESTED) ? "enabled" : "disabled"),
           ((tx_options & EMBER_OPTIONS_SECURITY_ENABLED) ? "enabled" : "disabled"),
           ((tx_options & EMBER_OPTIONS_HIGH_PRIORITY) ? "enabled" : "disabled"));;
}

/******************************************************************************
 * CLI - remove child
 * Remove a specific sensor from the child list.
 *****************************************************************************/
void cli_remove_child(sl_cli_command_arg_t *arguments)
{
  EmberStatus status;
  EmberMacAddress address;
  size_t hex_length = 0;
  uint8_t* child_id;

  address.mode = sl_cli_get_argument_uint8(arguments, 0);

  if (address.mode == EMBER_MAC_ADDRESS_MODE_SHORT) {
    address.addr.shortAddress = sl_cli_get_argument_uint8(arguments, 1);
  } else {
    child_id = sl_cli_get_argument_hex(arguments, 1, &hex_length);
    memcpy(&address.addr.longAddress, child_id, hex_length);
  }

  status = emberRemoveChild(&address);

  APP_INFO("Child removal 0x%02X\n", status);
}

/******************************************************************************
 * CLI - advertise
 * Advertise the Sink to Sensors.
 *****************************************************************************/
void cli_advertise(sl_cli_command_arg_t *arguments)
{
  (void) arguments;
  emberEventControlSetActive(*advertise_control);
}

/******************************************************************************
 * CLI - sensors
 * Prints the values of the child sensors.
 *****************************************************************************/
void cli_sensors(sl_cli_command_arg_t *arguments)
{
  (void) arguments;
  uint8_t i;
  APP_INFO("### Sensors table ###\n");
  for (i = 0; i < SENSOR_TABLE_SIZE; i++) {
    if (sensors[i].node_id != EMBER_NULL_NODE_ID) {
      APP_INFO("entry:%d id:0x%04X eui64:(>)%02X%02X%02X%02X%02X%02X%02X%02X last report:0x%04X\n",
               (uint16_t)i,
               sensors[i].node_id,
               sensors[i].node_eui64[7], sensors[i].node_eui64[6],
               sensors[i].node_eui64[5], sensors[i].node_eui64[4],
               sensors[i].node_eui64[3], sensors[i].node_eui64[2],
               sensors[i].node_eui64[1], sensors[i].node_eui64[0],
               sensors[i].last_report_ms);
    }
  }
}

/******************************************************************************
 * CLI - info command
 * It lists the main attributes of the current state of the node
 *****************************************************************************/
void cli_info(sl_cli_command_arg_t *arguments)
{
  (void) arguments;
  char* is_ack = ((tx_options & EMBER_OPTIONS_ACK_REQUESTED) ? ENABLED : DISABLED);
  char* is_security = ((tx_options & EMBER_OPTIONS_SECURITY_ENABLED) ? ENABLED : DISABLED);
  char* is_high_prio = ((tx_options & EMBER_OPTIONS_HIGH_PRIORITY) ? ENABLED : DISABLED);

  APP_INFO("Info:\n");
  APP_INFO("         MCU Id: 0x%llX\n", SYSTEM_GetUnique());
  APP_INFO("  Network state: 0x%02X\n", emberNetworkState());
  APP_INFO("      Node type: 0x%02X\n", emberGetNodeType());
  APP_INFO("        Node id: 0x%04X\n", emberGetNodeId());
  APP_INFO("         Pan id: 0x%04X\n", emberGetPanId());
  APP_INFO("        Channel: %d\n", (uint16_t)emberGetRadioChannel());
  APP_INFO("          Power: %d\n", (int16_t)emberGetRadioPower());
  APP_INFO("     TX options: MAC acks %s, security %s, priority %s\n", is_ack, is_security, is_high_prio);
}

/******************************************************************************
 * CLI - leave command
 * By this API call the node forgets the current network and reverts to
 * a network status of EMBER_NO_NETWORK
 *****************************************************************************/
void cli_leave(sl_cli_command_arg_t *arguments)
{
  (void) arguments;
  emberResetNetworkState();
}

/******************************************************************************
 * CLI - data command
 * The node sends message to the given destination ID
 *****************************************************************************/
void cli_data(sl_cli_command_arg_t *arguments)
{
  EmberStatus status;
  EmberNodeId destination = sl_cli_get_argument_uint16(arguments, 0);
  uint8_t *hex_value = 0;
  size_t hex_length = 0;
  hex_value = sl_cli_get_argument_hex(arguments, 1, &hex_length);

  status = emberMessageSend(destination,
                            DATA_ENDPOINT,
                            0, // messageTag
                            hex_length,
                            hex_value,
                            tx_options);

  APP_INFO("TX: Data to 0x%04X:{", destination);
  for (uint8_t i = 0; i < hex_length; i++) {
    APP_INFO("%02X ", hex_value[i]);
  }
  APP_INFO("}: status=0x%02X\n", status);
}

/******************************************************************************
 * CLI - set_channel command
 * Sets the channel
 *****************************************************************************/
void cli_set_channel(sl_cli_command_arg_t *arguments)
{
  uint8_t channel = sl_cli_get_argument_uint8(arguments, 0);
  EmberStatus status = emberSetRadioChannel(channel);
  if (status == EMBER_SUCCESS) {
    APP_INFO("Radio channel set, status=0x%02X\n", status);
  } else {
    APP_INFO("Setting radio channel failed, status=0x%02X\n", status);
  }
}

/******************************************************************************
 * CLI - set_tx_options command
 * Sets the tx options
 *****************************************************************************/
void cli_set_tx_option(sl_cli_command_arg_t *arguments)
{
  tx_options = sl_cli_get_argument_uint8(arguments, 0);
  char* is_ack = ((tx_options & EMBER_OPTIONS_ACK_REQUESTED) ? ENABLED : DISABLED);
  char* is_security = ((tx_options & EMBER_OPTIONS_SECURITY_ENABLED) ? ENABLED : DISABLED);
  char* is_high_prio = ((tx_options & EMBER_OPTIONS_HIGH_PRIORITY) ? ENABLED : DISABLED);

  APP_INFO("TX options set: MAC acks %s, security %s, priority %s", is_ack, is_security, is_high_prio);
}

/******************************************************************************
 * CLI - reset command
 * Resets the hardware
 *****************************************************************************/
void cli_reset(sl_cli_command_arg_t *arguments)
{
  (void) arguments;
  halReboot();
}

/******************************************************************************
 * CLI - toggle_radio command
 * The command turnes on and of the radio (for Rx)
 *****************************************************************************/
void cli_toggle_radio(sl_cli_command_arg_t *arguments)
{
  bool radio_on = (sl_cli_get_argument_uint8(arguments, 0) > 0);

  EmberStatus status = emberSetRadioPowerMode(radio_on);

  if (status == EMBER_SUCCESS) {
    APP_INFO("Radio is turned %s\n", (radio_on) ? "ON" : "OFF");
  } else {
    APP_INFO("Radio toggle is failed, status=0x%02X\n", status);
  }
}

/******************************************************************************
 * CLI - start_energy_scan command
 * The command scans the energy level on the given channel, e.g.
 * "start_energy_scan 0 10" results in 10 RSSI samples collected on channel 0.
 *****************************************************************************/
void cli_start_energy_scan(sl_cli_command_arg_t *arguments)
{
  EmberStatus status;
  uint8_t channel = sl_cli_get_argument_uint8(arguments, 0);
  uint8_t sample_num = sl_cli_get_argument_uint8(arguments, 1);
  status = emberStartEnergyScan(channel, sample_num);

  if (status == EMBER_SUCCESS) {
    APP_INFO("Start energy scanning: channel %d, samples %d\n", channel, sample_num);
  } else {
    APP_INFO("Start energy scanning failed, status=0x%02X\n", status);
  }
}

/******************************************************************************
 * CLI - set_security_key command
 * The command sets the security key
 *****************************************************************************/
void cli_set_security_key(sl_cli_command_arg_t *arguments)
{
  size_t key_len = 0;
  const char* const key_input_string = sl_cli_get_argument_hex(arguments, 0, &key_len);
  if (key_len == EMBER_ENCRYPTION_KEY_SIZE) {
    EmberKeyData key;
    memcpy(key.contents, key_input_string, key_len);
    if (emberSetSecurityKey(&key) == EMBER_SUCCESS) {
      APP_INFO("Security key set: {");
      for (uint8_t i = 0; i < EMBER_ENCRYPTION_KEY_SIZE; i++) {
        if (i == (EMBER_ENCRYPTION_KEY_SIZE - 1)) {
          APP_INFO("%02X", key.contents[i]);
        } else {
          APP_INFO("%02X ", key.contents[i]);
        }
      }
      APP_INFO("}\n");
    } else {
      APP_INFO("Security key set failed\n");
    }
  } else {
    APP_INFO("Security key length must be: %d bytes\n", EMBER_ENCRYPTION_KEY_SIZE);
  }
}

/******************************************************************************
 * CLI - counter command
 * The command prints out the passed stack counter
 *****************************************************************************/
void cli_counter(sl_cli_command_arg_t *arguments)
{
  uint8_t counter_type = sl_cli_get_argument_uint8(arguments, 0);
  uint32_t counter;
  EmberStatus status = emberGetCounter(counter_type, &counter);

  if (status == EMBER_SUCCESS) {
    APP_INFO("Counter type=0x%02X: %d\n", counter_type, counter);
  } else {
    APP_INFO("Get counter failed, status=0x%02X\n", status);
  }
}
