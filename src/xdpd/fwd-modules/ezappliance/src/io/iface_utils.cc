#include <linux/ethtool.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <linux/sockios.h>
//#include <netpacket/packet.h>
#include <netinet/in.h>
//#include <net/if.h>
#include <stdio.h>
#include <string>
#include <map>
#include <unistd.h>
#include <cassert>

//Prototypes
#include <rofl/common/utils/c_logger.h>
#include <rofl/datapath/pipeline/platform/memory.h>
#include <rofl/datapath/pipeline/openflow/of_switch.h>
#include <rofl/datapath/pipeline/common/datapacket.h>
#include <rofl/datapath/pipeline/physical_switch.h>
#include <rofl/datapath/afa/cmm.h>
#include "iface_utils.h" 

#include "../ezappliance/ez_packet_channel.h"
#include "../ezappliance/ez_corba_client.h"

using namespace xdpd::gnu_linux;

/*
*
* Port management
*
* All of the functions related to physical port management 
*
*/


static port_state_t translate_port_state_corba_to_of(bool status) {

       if (status == true)
               return PORT_STATE_LIVE;
       else
               return PORT_STATE_LINK_DOWN;
}

rofl_result_t update_port_status(char * name, port_state_t new_port_status) {
 
	switch_port_t *port;
	
	//Update all ports
	//if(update_physical_ports() != ROFL_SUCCESS){
	//	ROFL_ERR("Update physical ports failed \n");
	//	assert(0);
	//}

	port = fwd_module_get_port_by_name(name);
	if(!port)
		return ROFL_SUCCESS; //Port deleted
  
        if (port->state == new_port_status)
                return ROFL_SUCCESS; //Port status not changed

        port->state = new_port_status;

	//port_status message needs to be created if the port id attached to switch
	if(port->attached_sw != NULL){
		cmm_notify_port_status_changed(port);
	}
	
	return ROFL_SUCCESS;
}

void update_ports_statuses() {

        Proxy_Adapter::EZport ports = get_ez_ports();
        char* port_name;
        bool port_status;
        
        for (uint32_t port_id=0; port_id<ports.length(); port_id++) {
                
                port_name = get_ez_port_name(port_id);
                port_status = get_ez_port_status(port_id);
                update_port_status(port_name,
                                   translate_port_state_corba_to_of(port_status));
        }
}

static port_features_t translate_medium_corba_to_of(Proxy_Adapter::EZapiPort_Medium medium) {
        
        switch (medium) {
                case Proxy_Adapter::EZapiPort_Medium_Fiber:
                        return PORT_FEATURE_FIBER;
                case Proxy_Adapter::EZapiPort_Medium_Copper:
                        return PORT_FEATURE_COPPER;
        }
        return PORT_FEATURE_COPPER;
}

static port_features_t translate_rate_corba_to_of(Proxy_Adapter::EZapiPort_Rate rate) {
        
        switch (rate) {
                case Proxy_Adapter::EZapiPort_Rate_10MB:
                        return PORT_FEATURE_10MB_FD;
                case Proxy_Adapter::EZapiPort_Rate_100MB:
                        return PORT_FEATURE_100MB_FD;          
                case Proxy_Adapter::EZapiPort_Rate_1GB:
                        return PORT_FEATURE_1GB_FD;
                case Proxy_Adapter::EZapiPort_Rate_10GB:
                        return PORT_FEATURE_10GB_FD;
                case Proxy_Adapter::EZapiPort_Rate_40GB:
                        return PORT_FEATURE_40GB_FD;
                case Proxy_Adapter::EZapiPort_Rate_100GB:
                        return PORT_FEATURE_100GB_FD;
        }
        return PORT_FEATURE_1GB_FD;
}


static switch_port_t* fill_port(uint32_t port_id) {
	
        //Init the port
        char* name = get_ez_port_name(port_id);
        bool port_status = get_ez_port_status(port_id);
        port_state_t _port_status = translate_port_state_corba_to_of(port_status);

        switch_port_t* port = switch_port_init(name, true/*will be overriden afterwards*/, PORT_TYPE_PHYSICAL, _port_status);
        if(!port)
                return NULL;

        Proxy_Adapter::MacAddress mac = get_ez_port_mac(port_id);
        for(int j=0;j<6;j++)
                port->hwaddr[j] = mac[j];
        
        
        Proxy_Adapter::EZapiPort_Medium medium;
        Proxy_Adapter::EZapiPort_Rate rate;
        
        get_ez_port_features(port_id, medium, rate); 

        switch_port_set_current_speed(port, translate_rate_corba_to_of(rate));

        switch_port_add_capabilities(&port->curr, translate_medium_corba_to_of(medium));
        switch_port_add_capabilities(&port->curr, PORT_FEATURE_AUTONEG);

        port->advertised = port->supported = port->curr;

        return port;
}

/*
 * Looks in the system physical ports and fills up the switch_port_t sructure with them
 *
 */
rofl_result_t discover_physical_ports() {
    
        switch_port_t* port;

        //Fill ports
        Proxy_Adapter::EZport ports = get_ez_ports();

        ROFL_DEBUG("Number of retrieved EZ ports is %d \n", ports.length());
        
        for (uint32_t i=0; i<ports.length(); i++) {
                
                port = fill_port(ports[i]);
                
                if( physical_switch_add_port(port) != ROFL_SUCCESS ) {
                        ROFL_ERR("<%s:%d> All physical port slots are occupied\n",__func__, __LINE__);
                        assert(0);
                        return ROFL_FAILURE;
                }
        }
        return ROFL_SUCCESS;
}




rofl_result_t destroy_port(switch_port_t* port) {

	if(!port)
		return ROFL_FAILURE;

	//Delete port from the pipeline library
	if(physical_switch_remove_port(port->name) == ROFL_FAILURE)
		return ROFL_FAILURE;

	return ROFL_SUCCESS;
}


/*
 * Discovers platform physical ports and fills up the switch_port_t sructures
 *
 */
rofl_result_t destroy_ports() {

	unsigned int max_ports, i;
	switch_port_t** array;	

	array = physical_switch_get_physical_ports(&max_ports);
	for(i=0; i<max_ports ; i++){
		if(array[i] != NULL){
			destroy_port(array[i]);
		}
	}

	array = physical_switch_get_virtual_ports(&max_ports);
	for(i=0; i<max_ports ; i++){
		if(array[i] != NULL){
			destroy_port(array[i]);
		}
	}

	//TODO: add tun

	return ROFL_SUCCESS;
}
/*
* Port attachement/detachment 
*/

/*
 * Discover new platform ports (usually triggered by bg task manager)
 */
rofl_result_t update_physical_ports() {

	switch_port_t *port, **ports;
	int sock;
	struct ifaddrs *ifaddr, *ifa;
	unsigned int i, max_ports;
	std::map<std::string, struct ifaddrs*> system_ifaces;
	std::map<std::string, switch_port_t*> pipeline_ifaces;
	
	ROFL_DEBUG_VERBOSE("Trying to update the list of physical interfaces...\n");	
	
	/*real way to find interfaces*/
	//getifaddrs(&ifap); -> there are examples on how to get the ip addresses
	if (getifaddrs(&ifaddr) == -1){
		return ROFL_FAILURE;	
	}
	
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		return ROFL_FAILURE;	
    	}
	
	//Call the forwarding module to list the ports
	ports = fwd_module_get_physical_ports(&max_ports);

	if(!ports){
		close(sock);
		return ROFL_FAILURE;	
	}

	//Generate some helpful vectors
	for(i=0;i<max_ports;i++){
		if(ports[i])
			pipeline_ifaces[std::string(ports[i]->name)] = ports[i];	
			
	}
	for(ifa = ifaddr; ifa != NULL; ifa=ifa->ifa_next){
		if(ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_PACKET)
			continue;
			
		system_ifaces[std::string(ifa->ifa_name)] = ifa;	
	}

	//Validate the existance of the ports in the pipeline and remove
	//the ones that are no longer there. If they still exist remove them from ifaces
	for (std::map<std::string, switch_port_t*>::iterator it = pipeline_ifaces.begin(); it != pipeline_ifaces.end(); ++it){
		if (system_ifaces.find(it->first) == system_ifaces.end() ) {
			//Interface has been deleted. Detach and remove
			port = it->second;
			if(!port)
				continue;

			ROFL_INFO("Interface %s has been removed from the system. The interface will now be detached from any logical switch it is attached to (if any), and removed from the list of physical interfaces.\n", it->first.c_str());
			//Detach
			if(port->attached_sw && (fwd_module_detach_port_from_switch(port->attached_sw->dpid, port->name) != AFA_SUCCESS) ){
				ROFL_WARN("WARNING: unable to detach port %s from switch. This can lead to an unknown behaviour\n", it->first.c_str());
				assert(0);
			}
			//Destroy and remove from the list of physical ports
			destroy_port(port);	
		} 
		system_ifaces.erase(it->first);
	}
	
	//Add remaining "new" interfaces (remaining interfaces in system_ifaces map 
	for (std::map<std::string, struct ifaddrs*>::iterator it = system_ifaces.begin(); it != system_ifaces.end(); ++it){
		//Fill port
		port = NULL; //fill_port(sock,  it->second);  // DAMIAN fixhack
		if(!port){
			ROFL_ERR("Unable to initialize newly discovered interface %s\n", it->first.c_str());
			continue;
		}		

		//Adding the 
		if( physical_switch_add_port(port) != ROFL_SUCCESS ){
			ROFL_ERR("<%s:%d> Unable to add port %s to physical switch. Not enough slots?\n", it->first.c_str());
			freeifaddrs(ifaddr);
			continue;	
		}

	}
	
	ROFL_DEBUG_VERBOSE("Update of interfaces done.\n");	

	freeifaddrs(ifaddr);
	close(sock);
	
	return ROFL_SUCCESS;
}
