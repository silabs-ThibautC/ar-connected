/***************************************************************************//**
 * @brief Connect Poll component configuration header.
 *
 *******************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
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

// <<< Use Configuration Wizard in Context Menu >>>

// <h>Connect Poll configuration

// <o EMBER_AF_PLUGIN_POLL_SHORT_POLL_INTERVAL_QS> Short Poll Interval in quarter seconds<1-255>
// <i> Default: 4
// <i> The amount of time (in quarter seconds) the device will wait between MAC data polls when it is expecting data.
#define EMBER_AF_PLUGIN_POLL_SHORT_POLL_INTERVAL_QS        (4)

// <o EMBER_AF_PLUGIN_POLL_LONG_POLL_INTERVAL_S> Long Poll Interval in quarter seconds<1-65535>
// <i> Default: 600
// <i> The amount of time (in seconds) the device will wait between MAC data polls to ensure connectivity with its parent.
#define EMBER_AF_PLUGIN_POLL_LONG_POLL_INTERVAL_S          (600)

// </h>

// <<< end of configuration section >>>
