/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef EZ_CORBA_STRUCTURES_H
#define EZ_CORBA_STRUCTURES_H 

//#include <stdint.h>

#include <rofl.h>
#include <rofl/datapath/pipeline/openflow/openflow1x/pipeline/of1x_flow_entry.h>

#include "../idl/Proxy_Adapter.hh"

#define EZ_FLOWTABLE_STRUCTURE_NUM 0
#define EZ_FLOWTABLE_KEY_SIZE 38
#define EZ_FLOWTABLE_RESULT_SIZE 8

#define EZ_BIT_VLAN_TAG_FLAG 12

enum ez_bits_control {
        EZ_BIT_CONTROL_VALID,
        EZ_BIT_CONTROL_MATCH
};

enum ez_bits_actions {
        EZ_BIT_FORWARD_ALL_PORTS,
        EZ_BIT_FORWARD_SINGLE_PORT,
        EZ_BIT_DROP
};

#pragma pack(push)
#pragma pack(1)
typedef struct EZFlowTableKey {
        uint8_t reserved;
        uint16_t priority;
        uint8_t src_mac[6];
        uint8_t dst_mac[6];
        uint16_t vlan_tag;
        uint16_t ether_type;
        uint32_t src_ipv4;
        uint32_t dst_ipv4;
        uint8_t ip_protocol;
        uint16_t tp_src_port;
        uint16_t tp_dst_port;
} EZFlowTableKey_t; 
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
typedef struct EZFlowTableResult {
        uint8_t control;
        uint8_t actions;
        uint8_t port_number;
} EZFlowTableResult_t; 
#pragma pack(pop)

rofl_result_t set_ez_struct_key(of1x_flow_entry_t* entry, Proxy_Adapter::EZvalue& _key, Proxy_Adapter::EZvalue& _mask);
rofl_result_t set_ez_struct_result(of1x_flow_entry_t* entry, Proxy_Adapter::EZvalue& _result);
rofl_result_t check_if_match_list_empty(of1x_flow_entry_t* entry);
void set_ez_flow_entry(of1x_flow_entry_t* entry);
void del_ez_flow_entry(of1x_flow_entry_t* entry);
void show_ez_flow_entries();
void del_all_ez_flow_entries();

#endif /* EZ_CORBA_STRUCTURES_H */
 
