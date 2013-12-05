/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef EZ_PACKET_CHANNEL_H
#define EZ_PACKET_CHANNEL_H 

#include <unistd.h>
#include <rofl.h>
#include "../io/datapacketx86.h"
#include <rofl/datapath/pipeline/openflow/of_switch.h>

namespace xdpd {
namespace gnu_linux {

/**
* @brief TCP channel to EZappliance NP-3 
* 
* Allows for exchange of network packets between xdpd software pipeline and NP-3 procesor
*/
class ez_packet_channel{

public:
	ez_packet_channel();
	virtual ~ez_packet_channel();

	virtual datapacket_t* read(void);
	virtual rofl_result_t write(datapacket_t* pkt, uint8_t output_port);
        virtual void put_packet_to_pipeline(datapacket_t* pkt);
        virtual void start();
        
        
        // logical switch contains OF pipeline
        of_switch_t* logical_switch;

protected:
    
    int connect_to_ezproxy_packet_interface();
    
    //TCP connection to EZ-PROXY for purpose of network packet exchange between xdpd and NP-3 network processor
    int ez_packets_socket;
};

}// namespace xdpd::gnu_linux 
}// namespace xdpd


rofl_result_t launch_ez_packet_channel();
rofl_result_t stop_ez_packet_channel();

// retieve a single existing instance of EZ packet channel
void* get_ez_packet_channel();

// create a reference to a logical switch instance 
void set_lsw_for_ez_packet_channel(of_switch_t* sw);

// packet is pushed to EZ NP-3 and send via network port
rofl_result_t send_packet_via_ez_packet_channel(datapacket_t* pkt, uint32_t output_port);

#endif /* EZ_PACKET_CHANNEL_H */
