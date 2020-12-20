/***************************************************************************//**
 * @brief Connect Debug Print component configuration header.
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

// <h>Connect Debug Print configuration

// <q CONNECT_DEBUG_PRINTS_ENABLED> Prints enabled
// <i> Default: 1
// <i> If this option is enabled, all enabled debug groups shall be enabled, otherwise all debug prints shall be disabled.
#define CONNECT_DEBUG_PRINTS_ENABLED                                         (1)

// <q CONNECT_DEBUG_STACK_GROUP_ENABLED> Stack group enabled
// <i> Default: 1
// <i> If this option is enabled, prints belonging to the "stack" group shall be enabled.
#define CONNECT_DEBUG_STACK_GROUP_ENABLED                                    (1)

// <q CONNECT_DEBUG_CORE_GROUP_ENABLED> Core group enabled
// <i> Default: 1
// <i> If this option is enabled, prints belonging to the "core" group shall be enabled.
#define CONNECT_DEBUG_CORE_GROUP_ENABLED                                     (1)

// <q CONNECT_DEBUG_APP_GROUP_ENABLED> App group enabled
// <i> Default: 1
// <i> If this option is enabled, prints belonging to the "app" group shall be enabled.
#define CONNECT_DEBUG_APP_GROUP_ENABLED                                      (1)

// </h>

// <<< end of configuration section >>>
