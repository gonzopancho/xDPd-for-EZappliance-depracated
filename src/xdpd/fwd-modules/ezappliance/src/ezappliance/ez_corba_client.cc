#include "ez_corba_client.h"
#include <fstream>
#include <iostream>
#include <unistd.h>

#include <rofl.h>
#include <rofl/common/utils/c_logger.h>
#include "../config.h"

Proxy_Adapter::DevMonitor_var deviceMonitorProxy;
Proxy_Adapter::StructConf_var structConfProxy;

//========================================================================================

static bool corba_fetch_ior(std::string file_name, std::string& ior) {

        try {
                std::ifstream file_stream;

                file_stream.open(file_name.c_str());
                file_stream >> ior;
                file_stream.close();
        } catch (...) {
                return false;
        }
        return true;
}

static bool corba_init(CORBA::ORB_var& orb)
{
        int    argc = 0;
        char** argv = NULL;

        try {
                orb = CORBA::ORB_init(argc, argv, "omniORB4");
                if (!orb) {
                        throw std::string("Cannot initialize ORB");
                }

                CORBA::Object_var obj;

                obj = orb->resolve_initial_references("RootPOA");
                if (!obj) {
                        throw std::string("Cannot find RootPOA");
                }

        } catch (CORBA::SystemException & e) {
                ROFL_ERR("Caught CORBA::SystemException\n");
                return false;
        } catch (CORBA::Exception & e) {
                ROFL_ERR("Caught CORBA::Exception\n");
                return 0;
        } catch (omniORB::fatalException & e) {
                ROFL_ERR("Caught omniORB::fatalException:\n");
                ROFL_ERR("  file: %s\n", e.file());
                ROFL_ERR("  line: %d\n", e.line());
                ROFL_ERR("  mesg: %s\n", e.errmsg());
                return false;
        } catch (std::string & e) {
                ROFL_ERR("Caught library exception: %s\n", e.c_str());
        } catch (...) {
                ROFL_ERR("Caught unknown exception\n");
                return false;
        }

        return true;
}

//========================================================================================

static bool devMonitorConnect(bool force_new_connection=false) {
        
        // check if corba object reference created
        if (!force_new_connection && !CORBA::is_nil(deviceMonitorProxy))
                return true; 
        
        ROFL_DEBUG("[EZ-CORBA] Connecting DeviceMonitor object\n");
        try {
                CORBA::ORB_var orb;
                corba_init(orb);
                if (CORBA::is_nil(orb)) {
                        ROFL_ERR("[EZ-CORBA] Cannot get ORB\n");
                        return false;
                }
        
                std::string ior;
                if (!corba_fetch_ior(EZ_MONITOR_IOR, ior)) {
                        ROFL_ERR("[EZ-CORBA] Cannot fetch DeviceMonitor IOR\n");
                        return false;
                }

                CORBA::Object_var obj;
                obj = orb->string_to_object(ior.c_str());

                if (CORBA::is_nil(obj)) {
                        ROFL_ERR("[EZ-CORBA] Cannot get DeviceMonitor object\n");
                        return false;
                }

                deviceMonitorProxy = Proxy_Adapter::DevMonitor::_narrow(obj);
                if (CORBA::is_nil(deviceMonitorProxy)) {
                        ROFL_ERR("[EZ-CORBA] Cannot invoke on a nil DeviceMonitor object reference\n");
                        return false;
                }

                ROFL_DEBUG("[EZ-CORBA] Proxy_Adapter::DevMonitor object connected\n");
                return true;
        } 
        catch (CORBA::UNKNOWN) {
                ROFL_ERR("[EZ-CORBA] unknown exception in devMonitorConnect\n");
        }
        return false;
}

Proxy_Adapter::EZport get_ez_ports() {
        
        ROFL_DEBUG("[EZ-CORBA] Calling get_ez_ports method\n");
        Proxy_Adapter::EZport ports;
        try {
                if (devMonitorConnect()) {
                        deviceMonitorProxy->getPorts(ports);
                }
        } 
        catch (CORBA::UNKNOWN) {
                ROFL_ERR("[EZ-CORBA] unknown exception in get_ez_ports\n");
        }
        return ports;
}

char* get_ez_port_name(uint32_t port_id) {
        
        ROFL_DEBUG("[EZ-CORBA] Calling get_ez_port_name method\n");
        CORBA::String_var name;
        try {
                if (devMonitorConnect())
                        deviceMonitorProxy->getPortName((Proxy_Adapter::EZuint)port_id, name);
        } 
        catch (CORBA::UNKNOWN) {
                ROFL_ERR("[EZ-CORBA] unknown exception in get_ez_port_name\n");
        }
        ROFL_DEBUG("Getting port name %s\n", CORBA::string_dup(name));
        return CORBA::string_dup(name);
}

Proxy_Adapter::MacAddress  get_ez_port_mac(uint32_t port_id) {

        ROFL_DEBUG("[EZ-CORBA] Calling get_ez_port_mac method\n");
        CORBA::Octet ptr[6];
        Proxy_Adapter::MacAddress mac(6, ptr);
        
        try {
                if (devMonitorConnect()) {
                        deviceMonitorProxy->getPortMac((Proxy_Adapter::EZuint)port_id, mac);
                }
        }
        catch (CORBA::UNKNOWN) {
                ROFL_ERR("[EZ-CORBA] unknown exception in get_ez_port_mac\n");
        }
        return mac;
}

bool get_ez_port_status(uint32_t port_id) {
        
        ROFL_DEBUG("[EZ-CORBA] Calling get_ez_port_status method\n");
        bool port_status = false;
        try {
                if (devMonitorConnect())
                        deviceMonitorProxy->getPortStatus((Proxy_Adapter::EZuint)port_id, port_status);
                ROFL_DEBUG("[EZ-CORBA] port %d operational status is %d\n", port_id, port_status);
        } 
        catch (CORBA::UNKNOWN) {
                ROFL_ERR("[EZ-CORBA] unknown exception in get_ez_port_status\n");
        }
        return port_status;
}

void get_ez_port_features(uint32_t port_id, Proxy_Adapter::EZapiPort_Medium& medium, Proxy_Adapter::EZapiPort_Rate& rate) {
        
        ROFL_DEBUG("[EZ-CORBA] Calling get_ez_port_features method\n");
        try {
                if (devMonitorConnect())
                        deviceMonitorProxy->getPortFeatures((Proxy_Adapter::EZuint)port_id, medium, rate);
        } 
        catch (CORBA::UNKNOWN) {
                ROFL_ERR("[EZ-CORBA] unknown exception in get_ez_port_features\n");
        }
}

//========================================================================================

static bool structConfConnect(bool force_new_connection=false) {
        
        
        // check if corba object reference created
        if (!force_new_connection && !CORBA::is_nil(structConfProxy))
                return true; 
        
        ROFL_DEBUG("[EZ-CORBA] Connecting StructConf object\n");
        try {
                CORBA::ORB_var orb;
                corba_init(orb);
                if (CORBA::is_nil(orb)) {
                        ROFL_ERR("[EZ-CORBA] Cannot get ORB\n");
                        return false;
                }
                
                std::string ior;
                if (!corba_fetch_ior(EZ_STRUCT_IOR, ior)) {
                        ROFL_ERR("[EZ-CORBA] Cannot fetch StructConf IOR\n");
                        return false;
                }

                CORBA::Object_var obj;
                obj = orb->string_to_object(ior.c_str());
                if (CORBA::is_nil(obj)) {
                        ROFL_ERR("[EZ-CORBA] Cannot get StructConf object\n");
                        return false;
                }

                structConfProxy = Proxy_Adapter::StructConf::_narrow(obj);
                if (CORBA::is_nil(structConfProxy)) {
                        ROFL_ERR("[EZ-CORBA] Cannot invoke on a nil StructConf object reference\n");
                        return false;
                }

                ROFL_DEBUG("[EZ-CORBA] Proxy_Adapter::StructConf object connected\n");
                return true;
        } 
        catch (CORBA::UNKNOWN) {
                ROFL_ERR("[EZ-CORBA] unknown exception in structConfConnect\n");
        }
        return false;
}

static void show_EZvalue(Proxy_Adapter::EZvalue& value, const char* name) {
        
        ROFL_DEBUG("  -> %s is: ", name);
        for (uint32_t i=0; i<value.length(); i++)
                ROFL_DEBUG("%d:0x%x ", i, value[i]);
        ROFL_DEBUG("\n");
}

void set_ez_struct(Proxy_Adapter::EZStruct_type struct_type,
                   uint32_t struct_num,
                   uint32_t k_length,
                   uint32_t r_length,
                   Proxy_Adapter::EZvalue key,
                   Proxy_Adapter::EZvalue result,
                   Proxy_Adapter::EZvalue mask) {
                   
        ROFL_DEBUG("[EZ-CORBA] Calling set_ez_struct method\n");
        
        show_EZvalue(key, "key");
        show_EZvalue(mask, "mask");
        show_EZvalue(result, "result");
        
        try {
                if (structConfConnect())
                        structConfProxy->setStructEntry(struct_type, struct_num, k_length, r_length, key, result, mask);
        } 
        catch (CORBA::UNKNOWN) {
                ROFL_ERR("[EZ-CORBA] unknown exception in set_ez_struct\n");
        }
}

void del_ez_struct(Proxy_Adapter::EZStruct_type struct_type,
                   uint32_t struct_num,
                   uint32_t k_length,
                   uint32_t r_length,
                   Proxy_Adapter::EZvalue key,
                   Proxy_Adapter::EZvalue result,
                   Proxy_Adapter::EZvalue mask) {
                   
        ROFL_DEBUG("[EZ-CORBA] Calling del_ez_struct method\n");
        
        show_EZvalue(key, "key");
        show_EZvalue(mask, "mask");
        show_EZvalue(result, "result");
        
        try {
                if (structConfConnect())
                        structConfProxy->delStructEntry(struct_type, struct_num, k_length, r_length, key, result, mask);
        } 
        catch (CORBA::UNKNOWN) {
                ROFL_ERR("[EZ-CORBA] unknown exception in del_ez_struct\n");
        }
}

void del_all_ez_struct_entries(Proxy_Adapter::EZStruct_type struct_type,
                               uint32_t struct_num) {
                   
        ROFL_DEBUG("[EZ-CORBA] Calling del_all_ez_struct_entries method\n");
        
        try {
                if (structConfConnect())
                        structConfProxy->delStructAllEntries(struct_type, struct_num);
        } 
        catch (CORBA::UNKNOWN) {
                ROFL_ERR("[EZ-CORBA] unknown exception in del_all_ez_struct_entries\n");
        }
}

uint32_t get_ez_struct_length(Proxy_Adapter::EZStruct_type struct_type, uint32_t struct_num) {

        ROFL_DEBUG("[EZ-CORBA] Calling get_ez_struct_length method (struct type: %d, struct id: %d) \n", struct_type, struct_num);
        
        uint32_t length;
        try {
                if (structConfConnect())
                        structConfProxy->getStructLength(struct_type, struct_num, length);
        } 
        catch (CORBA::TRANSIENT) {
                ROFL_ERR("[EZ-CORBA] TRANSIENT exception in get_ez_struct_length\n");
                return 0;
        }
        catch (CORBA::UNKNOWN) {
                ROFL_ERR("[EZ-CORBA] unknown exception in get_ez_struct_length\n");
        }
        ROFL_DEBUG("\n\n[CORBA] There is %d entries in NP-3 flow table\n\n\n", length);
        return length;
}

void get_ez_struct(Proxy_Adapter::EZStruct_type struct_type,
                   uint32_t struct_num,
                   uint32_t index,
                   uint32_t& k_length,
                   uint32_t& r_length,
                   Proxy_Adapter::EZvalue& key,
                   Proxy_Adapter::EZvalue& result,
                   Proxy_Adapter::EZvalue& mask) {
                   
        ROFL_DEBUG("[EZ-CORBA] Calling get_ez_struct method\n");
        
        try {
                if (structConfConnect())
                        structConfProxy->getStructEntry(struct_type, struct_num, index, k_length, r_length, key, result, mask);
        } 
        catch (CORBA::UNKNOWN) {
                ROFL_ERR("[EZ-CORBA] unknown exception in get_ez_struct\n");
        }

        show_EZvalue(key, "key");
        show_EZvalue(mask, "mask");
        show_EZvalue(result, "result");
        
}
