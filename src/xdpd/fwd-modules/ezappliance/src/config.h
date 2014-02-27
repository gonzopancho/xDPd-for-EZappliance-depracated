/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef XDPD_GNU_LINUX_CONFIG_H 
#define XDPD_GNU_LINUX_CONFIG_H 

/**
* @file config.h
*
* @author Damian Parniewicz<damianp (at) man.poznan.pl>
* @author Marc Sune<marc.sune (at) bisdn.de>
*
* @brief Configuration file for the xDPd EZappliance forwarding module.
* 
*/

//---------------------------------------------------------//

/*
* General parameters
*/

//None for the moment

/*
* I/O subsystem parameters
*/

// Address and port of EZappliance TCP server for exchanging network frames
#define EZ_IP "10.134.0.4"
#define EZ_PORT 7777

#define EZ_STRUCT_IOR "/tmp/ior/EZapi_struct.ior"
#define EZ_MONITOR_IOR "/tmp/ior/EZapi_monitor.ior"

//Buffer storage(PKT_IN) max buffers
#define IO_PKT_IN_STORAGE_MAX_BUF 512
//Buffer storage(PKT_IN) expiration time (seconds)
#define IO_PKT_IN_STORAGE_EXPIRATION_S 180

/*
* Processing subsystem parameters
*/

//Num of processing threads per Logical Switch Instance (Tlsi)
#define PROCESSING_THREADS_PER_LSI 2

//Per thread input queue to the switch
//Align to a power of 2
#define PROCESSING_INPUT_QUEUE_SLOTS 1024 

//Per thread input queue to the switch
//Align to a power of 2
//WARNING: do not over-size it or congestion can be created
#define PROCESSING_PKT_IN_QUEUE_SLOTS 64 


//---------------------------------------------------------//

#endif //XDPD_GNU_LINUX_CONFIG_H
