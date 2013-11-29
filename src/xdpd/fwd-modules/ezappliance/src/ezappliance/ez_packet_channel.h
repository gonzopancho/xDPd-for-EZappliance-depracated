/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef EZ_PACKET_CHANNEL_H
#define EZ_PACKET_CHANNEL_H 

#include <unistd.h>
#include <rofl/datapath/pipeline/common/datapacket.h>

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
	virtual unsigned int write(unsigned int q_id, unsigned int num_of_buckets);
        virtual void start();

protected:
    
    int connect_to_ezproxy_packet_interface();
    
    //TCP connection to EZ-PROXY for purpose of network packet exchange between xdpd and NP-3 network processor
    int ez_packets_socket;
};


rofl_result_t launch_ez_packet_channel();
rofl_result_t stop_ez_packet_channel();

}// namespace xdpd::gnu_linux 
}// namespace xdpd


#endif /* EZ_PACKET_CHANNEL_H */
