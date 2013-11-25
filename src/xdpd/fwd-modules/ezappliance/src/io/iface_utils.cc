#include <linux/ethtool.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <linux/sockios.h>
//#include <netpacket/packet.h>
#include <netinet/in.h>
//#include <net/if.h>
#include <stdio.h>
#include <map>
#include <unistd.h>

//Prototypes
#include <rofl/common/utils/c_logger.h>
#include <rofl/datapath/pipeline/platform/memory.h>
#include <rofl/datapath/pipeline/openflow/of_switch.h>
#include <rofl/datapath/pipeline/common/datapacket.h>
#include <rofl/datapath/pipeline/physical_switch.h>
#include <rofl/datapath/afa/cmm.h>
#include "iface_utils.h" 
#include "iomanager.h"

#include "ports/ioport.h" 
#include "ports/mockup/ioport_mockup.h"
#include "ports/ez_packet_channel/ioport_ez_packet_channel.h"
#include "ports/mmap/ioport_mmap.h" 
#include "ports/mmap/ioport_mmapv2.h" 
#include "ports/vlink/ioport_vlink.h" 

using namespace xdpd::gnu_linux;

/*
*
* Port management
*
* All of the functions related to physical port management 
*
*/

rofl_result_t update_port_status(char * name){

	struct ifreq ifr;
	int sd, rc;
 
	switch_port_t *port;
	
	//Update all ports
	if(update_physical_ports() != ROFL_SUCCESS){
		ROFL_ERR("Update physical ports failed \n");
		assert(0);
	}

	port = fwd_module_get_port_by_name(name);
	if(!port)
		return ROFL_SUCCESS; //Port deleted

	ioport* io_port = ((ioport*)port->platform_port_state);
	
	if ((sd = socket(AF_PACKET, SOCK_RAW, 0)) < 0){
		return ROFL_FAILURE;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, port->name);

	if ((rc = ioctl(sd, SIOCGIFINDEX, &ifr)) < 0){
		return ROFL_FAILURE;
	}

	//Make sure there are no race conditions between ioctl() calls
	pthread_rwlock_rdlock(&io_port->rwlock);
		
	//Recover flags
	if ((rc = ioctl(sd, SIOCGIFFLAGS, &ifr)) < 0){ 
		close(sd);
		pthread_rwlock_unlock(&io_port->rwlock);
		return ROFL_FAILURE;
	}
	
	//Release mutex
	pthread_rwlock_unlock(&io_port->rwlock);
	
	ROFL_DEBUG("[bg] Interface %s is %s, and link is %s\n", name,( ((IFF_UP & ifr.ifr_flags) > 0) ? "up" : "down"), ( ((IFF_RUNNING & ifr.ifr_flags) > 0) ? "detected" : "not detected"));

	//Update link state
	io_port->set_link_state( ((IFF_RUNNING & ifr.ifr_flags) > 0) );

	if(IFF_UP & ifr.ifr_flags){
		iomanager::bring_port_up(io_port);
	}else{
		iomanager::bring_port_down(io_port);
	}
	
	//port_status message needs to be created if the port id attached to switch
	if(port->attached_sw != NULL){
		cmm_notify_port_status_changed(port);
	}
	
	return ROFL_SUCCESS;
}


static void fill_port_queues(switch_port_t* port, ioport* io_port){

	unsigned int i;
	char queue_name[PORT_QUEUE_MAX_LEN_NAME];

	//Filling one-by-one the queues that ioport has	
	for(i=0;i<io_port->get_num_of_queues();i++){
		snprintf(queue_name, PORT_QUEUE_MAX_LEN_NAME, "%s%d", "queue", i);
		switch_port_add_queue(port, i, (char*)&queue_name, io_port->get_queue_size(i), 0, 0);
	}

}


static switch_port_t* fill_port(char* name){
	
    switch_port_t* port;
    
    //Init the port
    port = switch_port_init(name, true/*will be overriden afterwards*/, PORT_TYPE_PHYSICAL, PORT_STATE_NONE);
    if(!port)
        return NULL;
    
      for(int j=0;j<6;j++)
        port->hwaddr[j] = 0x00; //FIXME: get MAC from EZ-port
        
    //port->platform_port_state = NULL;
    
    //Initialize MMAP-based port
    //Change this line to use another ioport...
    //ioport* io_port = new ioport_mmapv2(port);
    //iport* io_port = new ioport_mmap(port);
    ioport* io_port = new ioport_ez_packet_channel(port);

    port->platform_port_state = (platform_port_state_t*)io_port;
    
    //Fill port queues
    fill_port_queues(port, (ioport*)port->platform_port_state);

    //TODO: set rest of switch port attributes; See: rofl-core\src\rofl\datapath\pipeline\switch_port.h
    return port;
}

/*
 * Looks in the system physical ports and fills up the switch_port_t sructure with them
 *
 */
rofl_result_t discover_physical_ports(){
    
    switch_port_t* port;
    
    //Fill ports
    
    port = fill_port((char*)"ez0");
    if( physical_switch_add_port(port) != ROFL_SUCCESS ){
        ROFL_ERR("<%s:%d> All physical port slots are occupied\n",__func__, __LINE__);
        assert(0);
        return ROFL_FAILURE;
    }
        
    port = fill_port((char*)"ez1");
    if( physical_switch_add_port(port) != ROFL_SUCCESS ){
        ROFL_ERR("<%s:%d> All physical port slots are occupied\n",__func__, __LINE__);
        assert(0);
        return ROFL_FAILURE;
    }
    return ROFL_SUCCESS;
}

/*
 * Creates a virtual port pair between two switches
 */
rofl_result_t create_virtual_port_pair(of_switch_t* lsw1, ioport** vport1, of_switch_t* lsw2, ioport** vport2){

	//Names are composed following vlinkX-Y
	//Where X is the virtual link number (0... N-1)
	//Y is the edge 0 (left) 1 (right) of the connectio
	static unsigned int num_of_vlinks=0;
	char port_name[PORT_QUEUE_MAX_LEN_NAME];
	switch_port_t *port1, *port2;	
	uint64_t port_capabilities=0x0;
	//uint64_t mac_addr;
	uint16_t randnum = 0;

	//Init the pipeline ports
	snprintf(port_name,PORT_QUEUE_MAX_LEN_NAME, "vlink%u_%u", num_of_vlinks, 0);
	port1 = switch_port_init(port_name, true, PORT_TYPE_VIRTUAL, PORT_STATE_NONE);

	snprintf(port_name,PORT_QUEUE_MAX_LEN_NAME, "vlink%u_%u", num_of_vlinks, 1);
	port2 = switch_port_init(port_name, true, PORT_TYPE_VIRTUAL, PORT_STATE_NONE);

	if(!port1 || !port2){
		free(port1);
		assert(0);
		ROFL_ERR("<%s:%d> Not enough memory\n",__func__, __LINE__);
		return ROFL_FAILURE;
	}

	//Create two ioports
	*vport1 = new ioport_vlink(port1);
	
	if(!*vport1){
		free(port1);
		free(port2);
		ROFL_ERR("<%s:%d> Not enough memory\n",__func__, __LINE__);
		assert(0);
		return ROFL_FAILURE;
	}
	
	*vport2 = new ioport_vlink(port2);

	if(!*vport2){
		free(port1);
		free(port2);
		delete *vport1;
		ROFL_ERR("<%s:%d> Not enough memory\n",__func__, __LINE__);
		assert(0);
		return ROFL_FAILURE;
	}

	//Initalize port features(Marking as 1G)
	port_capabilities |= PORT_FEATURE_1GB_FD;
	
	switch_port_add_capabilities(&port1->curr, (port_features_t)port_capabilities);	
	switch_port_add_capabilities(&port1->advertised, (port_features_t)port_capabilities);	
	switch_port_add_capabilities(&port1->supported, (port_features_t)port_capabilities);	
	switch_port_add_capabilities(&port1->peer, (port_features_t)port_capabilities);	
	//mac_addr = 0x0200000000 | (rand() % (sizeof(int)-1));

	randnum = (uint16_t)rand();
	port1->hwaddr[0] = ((uint8_t*)&randnum)[0];
	port1->hwaddr[1] = ((uint8_t*)&randnum)[1];
	randnum = (uint16_t)rand();
	port1->hwaddr[2] = ((uint8_t*)&randnum)[0];
	port1->hwaddr[3] = ((uint8_t*)&randnum)[1];
	randnum = (uint16_t)rand();
	port1->hwaddr[4] = ((uint8_t*)&randnum)[0];
	port1->hwaddr[5] = ((uint8_t*)&randnum)[1];

	port1->hwaddr[0] &= 0xfe;

	//memcpy(port1->hwaddr, &mac_addr, sizeof(port1->hwaddr));

	switch_port_add_capabilities(&port2->curr, (port_features_t)port_capabilities);	
	switch_port_add_capabilities(&port2->advertised, (port_features_t)port_capabilities);	
	switch_port_add_capabilities(&port2->supported, (port_features_t)port_capabilities);	
	switch_port_add_capabilities(&port2->peer, (port_features_t)port_capabilities);	
	//mac_addr = 0x0200000000 | (rand() % (sizeof(int)-1));

	randnum = (uint16_t)rand();
	port2->hwaddr[0] = ((uint8_t*)&randnum)[0];
	port2->hwaddr[1] = ((uint8_t*)&randnum)[1];
	randnum = (uint16_t)rand();
	port2->hwaddr[2] = ((uint8_t*)&randnum)[0];
	port2->hwaddr[3] = ((uint8_t*)&randnum)[1];
	randnum = (uint16_t)rand();
	port2->hwaddr[4] = ((uint8_t*)&randnum)[0];
	port2->hwaddr[5] = ((uint8_t*)&randnum)[1];

	port2->hwaddr[0] &= 0xfe;

	//memcpy(port2->hwaddr, &mac_addr, sizeof(port1->hwaddr));

	//Add output queues
	fill_port_queues(port1, *vport1);
	fill_port_queues(port2, *vport2);


	//Store platform state on switch ports
	port1->platform_port_state = (platform_port_state_t*)*vport1;
	port2->platform_port_state = (platform_port_state_t*)*vport2;
	
	//Set cross link 
	((ioport_vlink*)*vport1)->set_connected_port((ioport_vlink*)*vport2);
	((ioport_vlink*)*vport2)->set_connected_port((ioport_vlink*)*vport1);

	//Add them to the platform
	if( physical_switch_add_port(port1) != ROFL_SUCCESS ){
		free(port1);
		free(port2);
		delete *vport1;
		delete *vport2;
		ROFL_ERR("<%s:%d> Unable to add vlink port1 to the physical switch; out of slots?\n",__func__, __LINE__);
		assert(0);
		return ROFL_FAILURE;
	}
	if( physical_switch_add_port(port2) != ROFL_SUCCESS ){
		free(port1);
		free(port2);
		delete *vport1;
		delete *vport2;
		ROFL_ERR("<%s:%d> Unable to add vlink port2 to the physical switch; out of slots?\n",__func__, __LINE__);
		assert(0);
		return ROFL_FAILURE;
	}

	//Increment counter and return
	num_of_vlinks++; //TODO: make this atomic jic

	return ROFL_SUCCESS;	
}

switch_port_t* get_vlink_pair(switch_port_t* port){
	if(((ioport_vlink*)port->platform_port_state)->connected_port)
		return ((ioport_vlink*)port->platform_port_state)->connected_port->of_port_state; 
	else 
		return NULL;
}




rofl_result_t destroy_port(switch_port_t* port){
	
	ioport* ioport_instance;

	if(!port)
		return ROFL_FAILURE;
	
	//Destroy the inner driver-dependant structures
	ioport_instance = (ioport*)port->platform_port_state;	
	delete ioport_instance;

	//Delete port from the pipeline library
	if(physical_switch_remove_port(port->name) == ROFL_FAILURE)
		return ROFL_FAILURE;

	return ROFL_SUCCESS;
}


/*
 * Discovers platform physical ports and fills up the switch_port_t sructures
 *
 */
rofl_result_t destroy_ports(){

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
* Enable port (direct call to ioport)
*/
rofl_result_t enable_port(platform_port_state_t* ioport_instance){

	ioport* port = (ioport*)ioport_instance; 

	if(port->enable() == ROFL_FAILURE)
		return ROFL_FAILURE;

	return ROFL_SUCCESS;
}

/*
* Disable port (direct call to ioport)
*/
rofl_result_t disable_port(platform_port_state_t* ioport_instance){

	class ioport* port = (ioport*)ioport_instance; 

	if(port->disable() == ROFL_FAILURE)
		return ROFL_FAILURE;

	return ROFL_SUCCESS;
}

/*
 * Discover new platform ports (usually triggered by bg task manager)
 */
rofl_result_t update_physical_ports(){

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
