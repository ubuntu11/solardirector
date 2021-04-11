
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __SMANET_COMMAND_H
#define __SMANET_COMMAND_H

#define CMD_GET_NET		0x01	/* Request for network configuration */
#define CMD_SEARCH_DEV		0x02	/* Find Devices */
#define CMD_CFG_NETADR		0x03	/* Assign network address */
#define CMD_SET_GRPADR		0x04	/* Assign group address */
#define CMD_DEL_GRPADR		0x05	/* Delete group address */
#define CMD_GET_NET_START	0x06	/* Start of configuration */

#define CMD_GET_CINFO		0x09	/* Request for channel information */
#define CMD_SYN_ONLINE		0x0A	/* Synchronize online data */
#define CMD_GET_DATA		0x0B	/* Data request */
#define CMD_SET_DATA		0x0C	/* Transmit data */
#define CMD_GET_SINFO		0x0D	/* Query storge config */
#define CMD_SET_MPARA		0x0F	/* Parameterize data acquisition */

#define CMD_GET_MTIME		0x14	/* Read storage intervals */
#define CMD_SET_MTIME		0x15	/* Set storage intervals */

#define CMD_GET_BINFO		0x1E	/* Request binary range information */
#define CMD_GET_BIN		0x1F	/* Binary data request */
#define CMD_SET_BIN		0x20	/* Send binary data */

#define CMD_TNR_VERIFY		0x32	/* Verify participant number */
#define CMD_VAR_VALUE		0x33	/* Request system variables */
#define CMD_VAR_FIND		0x34	/* Owner of a variable */
#define CMD_VAR_STATUS_OUT	0x35	/* Allocation variable - channel */
#define CMD_VAR_DEFINE_OUT	0x36	/* Allocate variable - channel */
#define CMD_VAR_STATUS_IN	0x37	/* Allocation input parameter - status */
#define CMD_VAR_DEFINE_IN	0x38	/* Allocate input parameter - variable */

#define CMD_PDELIMIT		0x28	/* Limitation of device power */
#define CMD_TEAM_FUNCTION	0x3C	/* Team function for PV inverters */

int command_getlen(int);

#endif
