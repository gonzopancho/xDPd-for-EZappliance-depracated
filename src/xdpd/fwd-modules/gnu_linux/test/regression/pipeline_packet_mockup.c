#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <rofl/datapath/pipeline/common/datapacket.h>
#include <rofl/datapath/pipeline/openflow/of_switch.h>
#include <rofl/datapath/pipeline/platform/packet.h>

#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "io/datapacketx86_c_wrapper.h"
#include "io/bufferpool_c_wrapper.h"

#include "ls_internal_state.h"
#include "io/datapacket_storage_c_wrapper.h"
#include <rofl/datapath/afa/openflow/openflow12/of12_cmm.h>


/*
* Getters
*/

uint32_t
platform_packet_get_size_bytes(datapacket_t * const pkt)
{
	 return 1500;
}
uint32_t
platform_packet_get_port_in(datapacket_t * const pkt)
{
	 return 0x1; //0x0;
}

uint32_t
platform_packet_get_phy_port_in(datapacket_t * const pkt)
{
	 return 0x1; //0x0;
}

uint64_t
platform_packet_get_eth_dst(datapacket_t * const pkt)
{
	 return 0x0;
}

uint64_t
platform_packet_get_eth_src(datapacket_t * const pkt)
{
	 return 0x0;
}

uint16_t
platform_packet_get_eth_type(datapacket_t * const pkt)
{
	 return 0x0;
}

uint16_t
platform_packet_get_vlan_vid(datapacket_t * const pkt)
{
	 return 0x0;
}

uint8_t
platform_packet_get_vlan_pcp(datapacket_t * const pkt)
{
	 return 0x0;
}

uint8_t
platform_packet_get_ip_proto(datapacket_t * const pkt)
{
	 return 0x0;
}
uint8_t
platform_packet_get_ip_ecn(datapacket_t * const pkt)
{
	 return 0x0;
}
uint8_t
platform_packet_get_ip_dscp(datapacket_t * const pkt)
{
	 return 0x0;
}

uint32_t
platform_packet_get_ipv4_src(datapacket_t * const pkt)
{
	 return 0x0;
}

uint32_t
platform_packet_get_ipv4_dst(datapacket_t * const pkt)
{
	 return 0x0;
}

uint16_t
platform_packet_get_tcp_dst(datapacket_t * const pkt)
{
	 return 0x0;
}

uint16_t
platform_packet_get_tcp_src(datapacket_t * const pkt)
{
	 return 0x0;
}

uint16_t
platform_packet_get_udp_dst(datapacket_t * const pkt)
{
	 return 0x0;
}

uint16_t
platform_packet_get_udp_src(datapacket_t * const pkt)
{
	 return 0x0;
}

uint8_t
platform_packet_get_icmpv4_type(datapacket_t * const pkt)
{
	 return 0x0;
}

uint8_t
platform_packet_get_icmpv4_code(datapacket_t * const pkt)
{
	 return 0x0;
}

uint32_t
platform_packet_get_mpls_label(datapacket_t * const pkt)
{
	 return 0x0;
}

uint8_t
platform_packet_get_mpls_tc(datapacket_t * const pkt)
{
	 return 0x0;
}

uint8_t
platform_packet_get_pppoe_code(datapacket_t * const pkt)
{
	 return 0x0;
}

uint8_t
platform_packet_get_pppoe_type(datapacket_t * const pkt)
{
	 return 0x0;
}

uint16_t
platform_packet_get_pppoe_sid(datapacket_t * const pkt)
{
	 return 0x0;
}

uint16_t
platform_packet_get_ppp_proto(datapacket_t * const pkt)
{
	 return 0x0;
}


/*
* Actions
*/

void
platform_packet_copy_ttl_in(datapacket_t* pkt)
{
	fprintf(stderr,"COPY TTL IN\n");
	dpx86_copy_ttl_in(pkt);
}

void
platform_packet_pop_vlan(datapacket_t* pkt)
{
	fprintf(stderr,"POP VLAN\n");
	dpx86_pop_vlan(pkt);
}

void
platform_packet_pop_mpls(datapacket_t* pkt, uint16_t ether_type)
{
	fprintf(stderr,"POP MPLS\n");
	dpx86_pop_mpls(pkt, ether_type);
}

void
platform_packet_pop_pppoe(datapacket_t* pkt, uint16_t ether_type)
{
	fprintf(stderr,"POP PPPOE\n");
	dpx86_pop_pppoe(pkt, ether_type);
}

void
platform_packet_push_pppoe(datapacket_t* pkt, uint16_t ether_type)
{
	fprintf(stderr,"PUSH PPPOE\n");
	dpx86_push_pppoe(pkt, ether_type);
}

void
platform_packet_push_mpls(datapacket_t* pkt, uint16_t ether_type)
{
	fprintf(stderr,"PUSH MPLS\n");
	dpx86_push_mpls(pkt, ether_type);
}

void
platform_packet_push_vlan(datapacket_t* pkt, uint16_t ether_type)
{
	fprintf(stderr,"PUSH VLAN\n");
	dpx86_push_vlan(pkt, ether_type);
}

void
platform_packet_copy_ttl_out(datapacket_t* pkt)
{
	fprintf(stderr,"COPY TTL OUT\n");
	dpx86_copy_ttl_out(pkt);
}

void
platform_packet_dec_nw_ttl(datapacket_t* pkt)
{
	fprintf(stderr,"DEC NW TTL\n");
	dpx86_dec_nw_ttl(pkt);
}

void
platform_packet_dec_mpls_ttl(datapacket_t* pkt)
{
	fprintf(stderr,"DEC MPLS TTL\n");
	dpx86_dec_mpls_ttl(pkt);
}

void
platform_packet_set_mpls_ttl(datapacket_t* pkt, uint8_t new_ttl)
{
	fprintf(stderr,"SET MPLS TTL\n");
	dpx86_set_mpls_ttl(pkt, new_ttl);
}

void
platform_packet_set_nw_ttl(datapacket_t* pkt, uint8_t new_ttl)
{
	fprintf(stderr,"SET NW TTL\n");
	dpx86_set_nw_ttl(pkt, new_ttl);
}

void
platform_packet_set_queue(datapacket_t* pkt, uint32_t queue)
{
	fprintf(stderr,"SET QUEUE\n");
	dpx86_set_queue(pkt, queue);
}

#if HAVE_METADATA_PROCESSING // todo this has to be implemented
void
platform_packet_set_metadata(datapacket_t* pkt, uint64_t metadata)
{
	fprintf(stderr,"SET METADATA\n");
}
#endif

void
platform_packet_set_eth_dst(datapacket_t* pkt, uint64_t eth_dst)
{
	fprintf(stderr,"SET ETH DST\n");
	dpx86_set_eth_dst(pkt, eth_dst);
}

void
platform_packet_set_eth_src(datapacket_t* pkt, uint64_t eth_src)
{
	fprintf(stderr,"SET ETH SRC\n");
	dpx86_set_eth_src(pkt, eth_src);
}

void
platform_packet_set_eth_type(datapacket_t* pkt, uint16_t eth_type)
{
	fprintf(stderr,"SET ETH TYPE\n");
	dpx86_set_eth_type(pkt, eth_type);
}

void
platform_packet_set_vlan_vid(datapacket_t* pkt, uint16_t vlan_vid)
{
	fprintf(stderr,"SET VLAN VID\n");
	dpx86_set_vlan_vid(pkt, vlan_vid);
}

void
platform_packet_set_vlan_pcp(datapacket_t* pkt, uint8_t vlan_pcp)
{
	fprintf(stderr,"SET VLAN PCP\n");
	dpx86_set_vlan_pcp(pkt, vlan_pcp);
}

void
platform_packet_set_ip_dscp(datapacket_t* pkt, uint8_t ip_dscp)
{
	fprintf(stderr,"SET IP DSCP\n");
	dpx86_set_ip_dscp(pkt, ip_dscp);
}

void
platform_packet_set_ip_ecn(datapacket_t* pkt, uint8_t ip_ecn)
{
	fprintf(stderr,"SET IP ECN\n");
	dpx86_set_ip_ecn(pkt, ip_ecn);
}

void
platform_packet_set_ip_proto(datapacket_t* pkt, uint8_t ip_proto)
{
	fprintf(stderr,"SET IP PROTO\n");
	dpx86_set_ip_proto(pkt, ip_proto);
}

void
platform_packet_set_ipv4_src(datapacket_t* pkt, uint32_t ip_src)
{
	fprintf(stderr,"SET IPv4 SRC\n");
	dpx86_set_ipv4_src(pkt, ip_src);
}

void
platform_packet_set_ipv4_dst(datapacket_t* pkt, uint32_t ip_dst)
{
	fprintf(stderr,"SET IPv4 DST\n");
	dpx86_set_ipv4_dst(pkt, ip_dst);
}

void
platform_packet_set_tcp_src(datapacket_t* pkt, uint16_t tcp_src)
{
	fprintf(stderr,"SET TCP SRC\n");
	dpx86_set_tcp_src(pkt, tcp_src);
}

void
platform_packet_set_tcp_dst(datapacket_t* pkt, uint16_t tcp_dst)
{
	fprintf(stderr,"SET TCP DST\n");
	dpx86_set_tcp_dst(pkt, tcp_dst);
}

void
platform_packet_set_udp_src(datapacket_t* pkt, uint16_t udp_src)
{
	fprintf(stderr,"SET UDP SRC\n");
	dpx86_set_udp_src(pkt, udp_src);
}

void
platform_packet_set_udp_dst(datapacket_t* pkt, uint16_t udp_dst)
{
	fprintf(stderr,"SET UDP DST\n");
	dpx86_set_udp_dst(pkt, udp_dst);
}

void
platform_packet_set_icmpv4_type(datapacket_t* pkt, uint8_t type)
{
	fprintf(stderr,"SET ICMPv4 TYPE\n");
	dpx86_set_icmpv4_type(pkt, type);
}

void
platform_packet_set_icmpv4_code(datapacket_t* pkt, uint8_t code)
{
	fprintf(stderr,"SET ICMPv4 CODE\n");
	dpx86_set_icmpv4_code(pkt, code);
}

void
platform_packet_set_mpls_label(datapacket_t* pkt, uint32_t label)
{
	fprintf(stderr,"SET MPLS LABEL\n");
	dpx86_set_mpls_label(pkt, label);
}

void
platform_packet_set_mpls_tc(datapacket_t* pkt, uint8_t tc)
{
	fprintf(stderr,"SET MPLS TC\n");
	dpx86_set_mpls_tc(pkt, tc);
}

void
platform_packet_set_pppoe_type(datapacket_t* pkt, uint8_t type)
{
	fprintf(stderr,"SET PPPOE TYPE\n");
	dpx86_set_pppoe_type(pkt, type);
}

void
platform_packet_set_pppoe_code(datapacket_t* pkt, uint8_t code)
{
	fprintf(stderr,"SET PPPOE CODE\n");
	dpx86_set_pppoe_code(pkt, code);
}

void
platform_packet_set_pppoe_sid(datapacket_t* pkt, uint16_t sid)
{
	fprintf(stderr,"SET PPPOE SID\n");
	dpx86_set_pppoe_sid(pkt, sid);
}

void
platform_packet_set_ppp_proto(datapacket_t* pkt, uint16_t proto)
{
	fprintf(stderr,"SET PPP PROTO\n");
	dpx86_set_ppp_proto(pkt, proto);
}

void
platform_packet_drop(datapacket_t* pkt)
{
	fprintf(stderr,"PAKCET DROP!\n");
	//Release buffer
	bufferpool_release_buffer_wrapper(pkt);

}

void
platform_packet_output(datapacket_t* pkt, switch_port_t* port)
{
	fprintf(stderr,"OUTPUT #%p\n",port);
	dpx86_output_packet(pkt, port);
}

datapacket_t* platform_packet_replicate(datapacket_t* pkt){

	return NULL;	
}

