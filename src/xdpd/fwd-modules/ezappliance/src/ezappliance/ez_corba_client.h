/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef EZ_CORBA_CLIENT_H
#define EZ_CORBA_CLIENT_H 

#include <stdint.h>

#include "../idl/Proxy_Adapter.hh"


Proxy_Adapter::EZport get_ez_ports();
char* get_ez_port_name(uint32_t port_id);

#endif /* EZ_CORBA_CLIENT_H */
 
