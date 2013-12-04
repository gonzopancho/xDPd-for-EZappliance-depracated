/*
 * @section LICENSE
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 * 
 * @author: msune, akoepsel, tjungel, valvarez, 
 * 
 * @section DESCRIPTION 
 * 
 * GNU/Linux forwarding_module dispatching routines. This file contains primary AFA driver hooks
 * for CMM to call forwarding module specific functions (e.g. bring up port, or create logical switch).
 * Openflow version dependant hooks are under openflow/ folder. 
*/


#include <stdio.h>
#include <rofl/datapath/afa/fwd_module.h>
#include <rofl/common/utils/c_logger.h>
#include <rofl/datapath/afa/cmm.h>
#include <rofl/datapath/pipeline/platform/memory.h>
#include <rofl/datapath/pipeline/physical_switch.h>
#include <rofl/datapath/pipeline/openflow/openflow1x/of1x_switch.h>
#include "../io/bufferpool.h"
#include "../bg_taskmanager.h"

#include "../io/iface_utils.h"
#include "../ezappliance/ez_packet_channel.h"

//only for Test
#include <stdlib.h>
#include <string.h>
#include <rofl/datapath/pipeline/openflow/of_switch.h>
#include <rofl/datapath/pipeline/common/datapacket.h>


#define NUM_ELEM_INIT_BUFFERPOOL 2048 //This is cache for fast port addition

using namespace xdpd::gnu_linux;

/*
* @name    fwd_module_init
* @brief   Initializes driver. Before using the AFA_DRIVER routines, higher layers must allow driver to initialize itself
* @ingroup fwd_module_management
*/
afa_result_t fwd_module_init(){

	ROFL_INFO("[AFA] Initializing EZappliance forwarding module...\n");
        
	//Init the ROFL-PIPELINE phyisical switch
	if(physical_switch_init() != ROFL_SUCCESS)
		return AFA_FAILURE;

	//create bufferpool
	bufferpool::init(NUM_ELEM_INIT_BUFFERPOOL);
        
        //Initialize packet channel to EZ
        if(launch_ez_packet_channel() != ROFL_SUCCESS){
                return AFA_FAILURE;
        }
        
	if(discover_physical_ports() != ROFL_SUCCESS)
		return AFA_FAILURE;

	//Initialize Background Tasks Manager
	if(launch_background_tasks_manager() != ROFL_SUCCESS){
		return AFA_FAILURE;
	}
	return AFA_SUCCESS; 
}

/*
* @name    fwd_module_destroy
* @brief   Destroy driver state. Allows platform state to be properly released. 
* @ingroup fwd_module_management
*/
afa_result_t fwd_module_destroy(){
    
        ROFL_DEBUG("[AFA] fwd_module_destroy\n");

        //Stop 
        stop_ez_packet_channel();
        
	//Stop the bg manager
	stop_background_tasks_manager();

	//Destroy interfaces
	destroy_ports();

	//Destroy physical switch (including ports)
	physical_switch_destroy();
	
	// destroy bufferpool
	bufferpool::destroy();
	
	ROFL_INFO("[AFA] EZappliance forwarding module destroyed.\n");
	
	return AFA_SUCCESS; 
}

/*
* Switch management functions
*/

/*
* @name    fwd_module_create_switch 
* @brief   Instruct driver to create an OF logical switch 
* @ingroup logical_switch_management
* @retval  Pointer to of_switch_t instance 
*/
of_switch_t* fwd_module_create_switch(char* name, uint64_t dpid, of_version_t of_version, unsigned int num_of_tables, int* ma_list){
	
        ROFL_DEBUG("[AFA] fwd_module_create_switch (name: %s, dpid: %d, tables: %d)\n", name, dpid, num_of_tables);
	of_switch_t* sw;
	
	sw = (of_switch_t*)of1x_init_switch(name, of_version, dpid, num_of_tables, (enum of1x_matching_algorithm_available*) ma_list);

	//Add switch to the bank	
	physical_switch_add_logical_switch(sw);
        
        // Add switch (with pipeline) to EZ-packet-channel
        set_lsw_for_ez_packet_channel(sw);
	
	return sw;
}

/*
* @name    fwd_module_get_switch_by_dpid 
* @brief   Retrieve the switch with the specified dpid  
* @ingroup logical_switch_management
* @retval  Pointer to of_switch_t instance or NULL 
*/
of_switch_t* fwd_module_get_switch_by_dpid(uint64_t dpid){
	
        ROFL_DEBUG("[AFA] fwd_module_init (dpid: %d)\n", dpid);
	//Call directly the bank
	return physical_switch_get_logical_switch_by_dpid(dpid); 
}

/*
* @name    fwd_module_destroy_switch_by_dpid 
* @brief   Instructs the driver to destroy the switch with the specified dpid 
* @ingroup logical_switch_management
*/
afa_result_t fwd_module_destroy_switch_by_dpid(const uint64_t dpid){

        ROFL_DEBUG("[AFA] fwd_module_destroy_switch_by_dpid (dpid: %d)\n", dpid);
	unsigned int i;
	
	//Try to retrieve the switch
	of_switch_t* sw = physical_switch_get_logical_switch_by_dpid(dpid);
	
	if(!sw)
		return AFA_FAILURE;

	//Stop all ports and remove it from being scheduled by I/O first
	for(i=0;i<sw->max_ports;i++){

		if(sw->logical_ports[i].attachment_state == LOGICAL_PORT_STATE_ATTACHED && sw->logical_ports[i].port){

		}
	}
	
	//Detach ports from switch. Do not feed more packets to the switch
	if(physical_switch_detach_all_ports_from_logical_switch(sw)!=ROFL_SUCCESS)
		return AFA_FAILURE;
	
	//Remove switch from the switch bank
	if(physical_switch_remove_logical_switch(sw)!=ROFL_SUCCESS)
		return AFA_FAILURE;
	
	return AFA_SUCCESS;
}

/*
* Port management 
*/

/*
* @name    fwd_module_list_platform_ports
* @brief   Retrieve the list of ports of the platform 
* @ingroup port_management
* @retval  Pointer to the first port. 
*/
switch_port_t* fwd_module_list_platform_ports(){
	/* TODO FIXME */
        ROFL_DEBUG("[AFA] fwd_module_list_platform_ports\n");
	return NULL;
}

/*
 * @name fwd_module_get_port_by_name
 * @brief Get a reference to the port by its name 
 * @ingroup port_management
 */
switch_port_t* fwd_module_get_port_by_name(const char *name){
        ROFL_DEBUG("[AFA] fwd_module_get_port_by_name (name: %s)\n", name);
	return physical_switch_get_port_by_name(name);
}

/*
* @name    fwd_module_get_physical_ports_ports
* @brief   Retrieve the list of the physical ports of the switch
* @ingroup port_management
* @retval  Pointer to the first port. 
*/
switch_port_t** fwd_module_get_physical_ports(unsigned int* num_of_ports){
        ROFL_DEBUG("[AFA] fwd_module_get_physical_ports\n");
	return physical_switch_get_physical_ports(num_of_ports);
}

/*
* @name    fwd_module_get_virtual_ports
* @brief   Retrieve the list of virtual ports of the platform
* @ingroup port_management
* @retval  Pointer to the first port. 
*/
switch_port_t** fwd_module_get_virtual_ports(unsigned int* num_of_ports){
        ROFL_DEBUG("[AFA] fwd_module_get_virtual_ports\n");
	return physical_switch_get_virtual_ports(num_of_ports);
}

/*
* @name    fwd_module_get_tunnel_ports
* @brief   Retrieve the list of tunnel ports of the platform
* @ingroup port_management
* @retval  Pointer to the first port. 
*/
switch_port_t** fwd_module_get_tunnel_ports(unsigned int* num_of_ports){
        ROFL_DEBUG("[AFA] fwd_module_get_tunnel_ports\n");
	return physical_switch_get_tunnel_ports(num_of_ports);
}
/*
* @name    fwd_module_attach_physical_port_to_switch
* @brief   Attemps to attach a system's port to switch, at of_port_num if defined, otherwise in the first empty OF port number.
* @ingroup management
*
* @param dpid Datapath ID of the switch to attach the ports to
* @param name Port name (system's name)
* @param of_port_num If *of_port_num is non-zero, try to attach to of_port_num of the logical switch, otherwise try to attach to the first available port and return the result in of_port_num
*/
afa_result_t fwd_module_attach_port_to_switch(uint64_t dpid, const char* name, unsigned int* of_port_num){

        ROFL_DEBUG("[AFA] fwd_module_attach_port_to_switch (dpid: %d, name: %s)\n", dpid, name);
    
	switch_port_t* port;
	of_switch_t* lsw;

	//Check switch existance
	lsw = physical_switch_get_logical_switch_by_dpid(dpid);
	if(!lsw){
		return AFA_FAILURE;
	}
	
	//Check if the port does exist
	port = physical_switch_get_port_by_name(name);
	if(!port)
		return AFA_FAILURE;

	//Update pipeline state
	if(*of_port_num == 0){
		//no port specified, we assign the first available
		if(physical_switch_attach_port_to_logical_switch(port,lsw,of_port_num) == ROFL_FAILURE){
			assert(0);
			return AFA_FAILURE;
		}
	}else{

		if(physical_switch_attach_port_to_logical_switch_at_port_num(port,lsw,*of_port_num) == ROFL_FAILURE){
			assert(0);
			return AFA_FAILURE;
		}
	}

	//notify port attached
	if(cmm_notify_port_add(port)!=AFA_SUCCESS){
		//return AFA_FAILURE; //Ignore
	}
	
	return AFA_SUCCESS;
}

/**
* @name    fwd_module_connect_switches
* @brief   Attemps to connect two logical switches via a virtual port. Forwarding module may or may not support this functionality. 
* @ingroup management
*
* @param dpid_lsi1 Datapath ID of the LSI1
* @param dpid_lsi2 Datapath ID of the LSI2 
*/
afa_result_t fwd_module_connect_switches(uint64_t dpid_lsi1, switch_port_t** port1, uint64_t dpid_lsi2, switch_port_t** port2){

        ROFL_DEBUG("[AFA] fwd_module_connect_switches (dpid_1: %d, dpid_2: %d)\n", dpid_lsi1, dpid_lsi2);
	of_switch_t *lsw1, *lsw2;

	//Check existance of the dpid
	lsw1 = physical_switch_get_logical_switch_by_dpid(dpid_lsi1);
	lsw2 = physical_switch_get_logical_switch_by_dpid(dpid_lsi2);

	if(!lsw1 || !lsw2){
		assert(0);
		return AFA_FAILURE;
	}
	

	return AFA_SUCCESS; 
}

/*
* @name    fwd_module_detach_port_from_switch
* @brief   Detaches a port from the switch 
* @ingroup port_management
*
* @param dpid Datapath ID of the switch to detach the ports
* @param name Port name (system's name)
*/
afa_result_t fwd_module_detach_port_from_switch(uint64_t dpid, const char* name){

        ROFL_DEBUG("[AFA] fwd_module_detach_port_from_switch (dpid: %d, name: %s)\n", dpid, name);
	of_switch_t* lsw;
	switch_port_t* port;
	
	lsw = physical_switch_get_logical_switch_by_dpid(dpid);
	if(!lsw)
		return AFA_FAILURE;

	port = physical_switch_get_port_by_name(name);

	//Check if the port does exist and is really attached to the dpid
	if( !port || port->attached_sw->dpid != dpid)
		return AFA_FAILURE;

	if(physical_switch_detach_port_from_logical_switch(port,lsw) != ROFL_SUCCESS)
		return AFA_FAILURE;
	
	//notify port dettached
	if(cmm_notify_port_delete(port) != AFA_SUCCESS){
		///return AFA_FAILURE; //ignore
	}

	//If it is virtual remove also the data structures associated
	if(port->type == PORT_TYPE_VIRTUAL){
		//Remove from the pipeline and delete
		if(physical_switch_remove_port(port->name) != ROFL_SUCCESS){
			ROFL_ERR("Error removing port from the physical_switch. The port may become unusable...\n");
			assert(0);
			return AFA_FAILURE;
			
		}
	}
	
	return AFA_SUCCESS; 
}


/*
* @name    fwd_module_detach_port_from_switch_at_port_num
* @brief   Detaches port_num of the logical switch identified with dpid 
* @ingroup port_management
*
* @param dpid Datapath ID of the switch to detach the ports
* @param of_port_num Number of the port (OF number) 
*/
afa_result_t fwd_module_detach_port_from_switch_at_port_num(uint64_t dpid, const unsigned int of_port_num){

        ROFL_DEBUG("[AFA] fwd_module_detach_port_from_switch_at_port_num (dpid: %d, port: %d)\n", dpid, of_port_num);
	of_switch_t* lsw;
	
	lsw = physical_switch_get_logical_switch_by_dpid(dpid);
	if(!lsw)
		return AFA_FAILURE;

	//Check if the port does exist.
	if(!of_port_num || of_port_num >= LOGICAL_SWITCH_MAX_LOG_PORTS || !lsw->logical_ports[of_port_num].port)
		return AFA_FAILURE;

	return fwd_module_detach_port_from_switch(dpid, lsw->logical_ports[of_port_num].port->name);
}


//Port admin up/down stuff

/*
* Port administrative management actions (ifconfig up/down like)
*/

/*
* @name    fwd_module_enable_port
* @brief   Brings up a system port. If the port is attached to an OF logical switch, this also schedules port for I/O and triggers PORTMOD message. 
* @ingroup port_management
*
* @param name Port system name 
*/
afa_result_t fwd_module_enable_port(const char* name){
    
        ROFL_DEBUG("[AFA] fwd_module_enable_port (name: %s)\n", name);

	switch_port_t* port = physical_switch_get_port_by_name(name);

	if(!port)
		return AFA_FAILURE;
        
        // Assing channel for packet exchange from/to EZ
        port->platform_port_state = (platform_port_state_t*)get_ez_packet_channel();

	if(cmm_notify_port_status_changed(port) != AFA_SUCCESS)
		return AFA_FAILURE;
	
	return AFA_SUCCESS;
}

/*
* @name    fwd_module_disable_port
* @brief   Shutdowns (brings down) a system port. If the port is attached to an OF logical switch, this also de-schedules port and triggers PORTMOD message. 
* @ingroup port_management
*
* @param name Port system name 
*/
afa_result_t fwd_module_disable_port(const char* name){

        ROFL_DEBUG("[AFA] fwd_module_disable_port (name: %s)\n", name);
        
	switch_port_t* port = physical_switch_get_port_by_name(name);
        
	//Check if the port does exist
	if(!port || !port->platform_port_state)
		return AFA_FAILURE;
        
        port->platform_port_state = NULL;

	if(cmm_notify_port_status_changed(port) != AFA_SUCCESS)
		return AFA_FAILURE;
	
	return AFA_SUCCESS;
}

/*
* @name    fwd_module_enable_port_by_num
* @brief   Brings up a port from an OF logical switch (and the underlying physical interface). This function also triggers the PORTMOD message 
* @ingroup port_management
*
* @param dpid DatapathID 
* @param port_num OF port number
*/
afa_result_t fwd_module_enable_port_by_num(uint64_t dpid, unsigned int port_num){

        ROFL_DEBUG("[AFA] fwd_module_enable_port_by_num (dpid: %d, port: %d\n", dpid, port_num);
	of_switch_t* lsw;
	
	lsw = physical_switch_get_logical_switch_by_dpid(dpid);
	if(!lsw)
		return AFA_FAILURE;

	//Check if the port does exist and is really attached to the dpid
	if( !lsw->logical_ports[port_num].port || lsw->logical_ports[port_num].attachment_state != LOGICAL_PORT_STATE_ATTACHED || lsw->logical_ports[port_num].port->attached_sw->dpid != dpid)
		return AFA_FAILURE;

        lsw->logical_ports[port_num].port->platform_port_state = (platform_port_state_t*)get_ez_packet_channel();
	
	if(cmm_notify_port_status_changed(lsw->logical_ports[port_num].port)!=AFA_SUCCESS)
		return AFA_FAILURE;
	
	return AFA_SUCCESS;
}

/*
* @name    fwd_module_disable_port_by_num
* @brief   Brings down a port from an OF logical switch (and the underlying physical interface). This also triggers the PORTMOD message.
* @ingroup port_management
*
* @param dpid DatapathID 
* @param port_num OF port number
*/
afa_result_t fwd_module_disable_port_by_num(uint64_t dpid, unsigned int port_num){

        ROFL_DEBUG("[AFA] fwd_module_disable_port_by_num (dpid: %d, port: %d\n", dpid, port_num);
	of_switch_t* lsw;
	
	lsw = physical_switch_get_logical_switch_by_dpid(dpid);
	if(!lsw)
		return AFA_FAILURE;

	//Check if the port does exist and is really attached to the dpid
	if( !lsw->logical_ports[port_num].port || lsw->logical_ports[port_num].attachment_state != LOGICAL_PORT_STATE_ATTACHED || lsw->logical_ports[port_num].port->attached_sw->dpid != dpid)
		return AFA_FAILURE;

        lsw->logical_ports[port_num].port->platform_port_state = NULL;
	
	if(cmm_notify_port_status_changed(lsw->logical_ports[port_num].port)!=AFA_SUCCESS)
		return AFA_FAILURE;
	
	return AFA_SUCCESS;
}

/**
 * @brief get a list of available matching algorithms
 * @ingroup fwd_module_management
 *
 * @param of_version
 * @param name_list
 * @param count
 * @return
 */
afa_result_t fwd_module_list_matching_algorithms(of_version_t of_version, const char * const** name_list, int *count){
        ROFL_DEBUG("[AFA] fwd_module_list_matching_algorithms\n");
	return (afa_result_t)of_get_switch_matching_algorithms(of_version, name_list, count);
}
