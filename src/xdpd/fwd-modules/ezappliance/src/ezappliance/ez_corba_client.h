/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef EZ_CORBA_CLIENT_H
#define EZ_CORBA_CLIENT_H 

#include <stdint.h>

#include "../idl/Proxy_Adapter.hh"

Proxy_Adapter::EZport get_ez_ports();

char* get_ez_port_name(uint32_t port_id);

Proxy_Adapter::MacAddress get_ez_port_mac(uint32_t port_id);

bool get_ez_port_status(uint32_t port_id);

void get_ez_port_features(uint32_t port_id, 
                          Proxy_Adapter::EZapiPort_Medium& medium, 
                          Proxy_Adapter::EZapiPort_Rate&   rate);


void set_ez_struct(Proxy_Adapter::EZStruct_type struct_type, 
                   uint32_t struct_num, 
                   uint32_t k_length, 
                   uint32_t r_length, 
                   Proxy_Adapter::EZvalue key, 
                   Proxy_Adapter::EZvalue result, 
                   Proxy_Adapter::EZvalue mask);

void del_ez_struct(Proxy_Adapter::EZStruct_type struct_type,
                   uint32_t struct_num,
                   uint32_t k_length,
                   uint32_t r_length,
                   Proxy_Adapter::EZvalue key,
                   Proxy_Adapter::EZvalue result,
                   Proxy_Adapter::EZvalue mask);

void del_all_ez_struct_entries(Proxy_Adapter::EZStruct_type struct_type,
                               uint32_t struct_num);

uint32_t get_ez_struct_length(Proxy_Adapter::EZStruct_type struct_type, uint32_t struct_num);


void get_ez_struct(Proxy_Adapter::EZStruct_type struct_type,
                   uint32_t struct_num,
                   uint32_t index,
                   uint32_t& k_length,
                   uint32_t& r_length,
                   Proxy_Adapter::EZvalue& key,
                   Proxy_Adapter::EZvalue& result,
                   Proxy_Adapter::EZvalue& mask);

#endif /* EZ_CORBA_CLIENT_H */
 
