/***************************************************************************//**
 * @brief Connect Stack Common component configuration header.
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

// <h>Connect Stack Parent Support configuration

// <o EMBER_CHILD_TABLE_SIZE> Child Table Size <1-64>
// <i> Default: 16
// <i> The maximum numbers of children the coordinator can manage. While the maximum size is 64, for range extenders the child table size can not exceed 32 entries.
#define EMBER_CHILD_TABLE_SIZE                                      (16)

// <o EMBER_CHILD_TIMEOUT_SEC> Child Timeout in seconds<1-0x20C400>
// <i> Default: 3600
// <i> The time in seconds after which the parent shall remove the child from its child table if it did not hear from it. Range extenders periodically exchange network-level commands with the coordinator. End devices and sleepy end devices can periodically poll as keep alive mechanism.
#define EMBER_CHILD_TIMEOUT_SEC                                     (3600)

// <o EMBER_INDIRECT_QUEUE_SIZE>Maximum number of packets in the indirect queue<1-16>
// <i> Default: 8
// <i> The maximum number of outgoing packets that can be enqueued at the parent indirect queue.
#define EMBER_INDIRECT_QUEUE_SIZE                                   (16)

// <o EMBER_INDIRECT_TRANSMISSION_TIMEOUT_MS> Indirect Queue Timeout in milliseconds<1-0x80000000>
// <i> Default: 8000
// <i> The maximum amount of time (in milliseconds) that the MAC will hold a message for indirect transmission to a child.
#define EMBER_INDIRECT_TRANSMISSION_TIMEOUT_MS                      (8000)

// <o EMBER_COORDINATOR_FIRST_SHORT_ID_TO_BE_ASSIGNED> First Assigned Short ID <1-32767>
// <i> Default: 1
// <i> A star coordinator assigns short IDs to other nodes in the star network sequentially starting from this short ID. This option provides a simple effective way to reserve an interval of short addresses.
#define EMBER_COORDINATOR_FIRST_SHORT_ID_TO_BE_ASSIGNED             (1)

// <o EMBER_NWK_RANGE_EXTENDER_UPDATE_PERIOD_SEC> Range extender update period in seconds <1-0x7FFFFFFF>
// <i> Default: 60
// <i> The period in seconds a range extender sends an update command to the coordinator containing the list of its children. Only used by range extenders.
#define EMBER_NWK_RANGE_EXTENDER_UPDATE_PERIOD_SEC                  (60)

// </h>

// <<< end of configuration section >>>
