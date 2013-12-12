#include "ez_corba_client.h"
#include <fstream>
#include <iostream>
#include <unistd.h>

#include <rofl.h>
#include <rofl/common/utils/c_logger.h>
#include "../config.h"

Proxy_Adapter::DevMonitor_var deviceMonitorProxy;

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

static bool devMonitorConnect(bool force_new_connection=false) {
        
        // check if corba object reference created
        if (!force_new_connection && !CORBA::is_nil(deviceMonitorProxy))
                return true; 
        
        try {
                CORBA::ORB_var orb;
                corba_init(orb);
                if (CORBA::is_nil(orb)) {
                        ROFL_ERR("[CORBA] Cannot get ORB\n");
                        return false;
                }
                
                std::string ior;
                if (!corba_fetch_ior("/tmp/DevMonitor.ior", ior)) {
                        ROFL_ERR("[CORBA] Cannot fetch IOR\n");
                        return false;
                }

                CORBA::Object_var obj;
                obj = orb->string_to_object(ior.c_str());
                if (CORBA::is_nil(obj)) {
                        ROFL_ERR("[CORBA] Cannot get object\n");
                        return false;
                }

                deviceMonitorProxy = Proxy_Adapter::DevMonitor::_narrow(obj);
                if (CORBA::is_nil(deviceMonitorProxy)) {
                        ROFL_ERR("[CORBA] Cannot invoke on a nil object reference\n");
                        return false;
                }

                ROFL_DEBUG("[CORBA] Proxy_Adapter::DevMonitor object connected\n");
                return true;
        } 
        catch (CORBA::UNKNOWN) {
                ROFL_ERR("[CORBA] unknown exception in devMonitorConnect\n");
        }
        return false;
}

Proxy_Adapter::EZport get_ez_ports() {
        
        Proxy_Adapter::EZport ports;
        try {
                if (devMonitorConnect())
                        deviceMonitorProxy->getPorts(ports);
        } 
        catch (CORBA::UNKNOWN) {
                ROFL_ERR("[CORBA] unknown exception in get_ez_ports");
        }
        return ports;
}

char* get_ez_port_name(uint32_t port_id) {
        
        CORBA::String_var name;
        try {
                if (devMonitorConnect())
                        deviceMonitorProxy->getPortName((Proxy_Adapter::EZuint)port_id, name);
        } 
        catch (CORBA::UNKNOWN) {
                ROFL_ERR("[CORBA] unknown exception in get_ez_port_name");
        }
        return CORBA::string_dup(name);
}

Proxy_Adapter::MacAddress get_ez_port_mac(uint32_t port_id) {
        
        Proxy_Adapter::MacAddress_var mac;
        try {
                if (devMonitorConnect())
                        deviceMonitorProxy->getPortMac((Proxy_Adapter::EZuint)port_id, mac);
        } 
        catch (CORBA::UNKNOWN) {
                ROFL_ERR("[CORBA] unknown exception in get_ez_port_mac");
        }
        return mac;
}

void get_ez_port_features(uint32_t port_id, Proxy_Adapter::EZapiPort_Medium& medium, Proxy_Adapter::EZapiPort_Rate& rate) {
        
        try {
                if (devMonitorConnect())
                        deviceMonitorProxy->getPortFeatures((Proxy_Adapter::EZuint)port_id, medium, rate);
        } 
        catch (CORBA::UNKNOWN) {
                ROFL_ERR("[CORBA] unknown exception in get_ez_port_features");
        }
}