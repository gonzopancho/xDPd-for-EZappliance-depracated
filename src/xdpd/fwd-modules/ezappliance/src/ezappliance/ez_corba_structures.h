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

typedef struct EZFlowTableKey {
        uint8_t reserved;
        uint16_t priority;
        uint8_t src_mac[6];
        uint8_t dst_mac[6];
        uint16_t vlan_tag;
} EZFlowTableKey_t;  

typedef struct EZFlowTableResult {
        uint8_t control;
        uint8_t actions;
        uint8_t port_number;
} EZFlowTableResult_t; 


rofl_result_t set_ez_struct_key(of1x_flow_entry_t* entry, Proxy_Adapter::EZvalue& _key, Proxy_Adapter::EZvalue& _mask);
rofl_result_t set_ez_struct_result(of1x_flow_entry_t* entry, Proxy_Adapter::EZvalue& _result);
void set_ez_flow_entry(of1x_flow_entry_t* entry);
void del_ez_flow_entry(of1x_flow_entry_t* entry);

#endif /* EZ_CORBA_STRUCTURES_H */
 
