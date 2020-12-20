/***************************************************************************//**
 * @file app_init.c
 * @brief app_init.c
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
#include "sl_flex_assert.h"
#include "sl_app_common.h"
#include "sl_si70xx.h"
#include "sl_i2cspm_instances.h"
#include "sl_sleeptimer.h"
#include "app_process.h"
#include "app_framework_common.h"
// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                          Static Function Declarations
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                                Static Variables
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                          Public Function Definitions
// -----------------------------------------------------------------------------
/******************************************************************************
* Application framework init callback
******************************************************************************/
void emberAfInitCallback(void)
{
  EmberStatus status;
  uint8_t device_id = 0;
  // init temperature sensor
  if (!sl_si70xx_present(sl_i2cspm_sensor, SI7021_ADDR, &device_id)) {
    // wait a bit before re-trying
    // the si7021 sensor can take up to 80 ms (25 ms @25 deg C) to start up
    sl_sleeptimer_delay_millisecond(80);
    // init temperature sensor (2nd attempt)
    if (!sl_si70xx_present(sl_i2cspm_sensor, SI7021_ADDR, &device_id)) {
      APP_INFO("Failed to initialize temperature sensor!\n");
    }
  }

  emberAfAllocateEvent(&report_control, &report_handler);
  // CLI info message
  APP_INFO("\nSensor\n");

  emberSetSecurityKey(&security_key);
  status = emberNetworkInit();
  APP_INFO("Network status 0x%02X\n", status);

#if defined(EMBER_AF_PLUGIN_BLE)
  bleConnectionInfoTableInit();
#endif
}
// -----------------------------------------------------------------------------
//                          Static Function Definitions
// -----------------------------------------------------------------------------
