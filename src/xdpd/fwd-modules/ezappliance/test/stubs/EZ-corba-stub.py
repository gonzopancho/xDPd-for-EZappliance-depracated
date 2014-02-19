#!/usr/bin/env python

#
# The ALIEN project (work funded by European Commission).
#
# Copyright (C) 2012  Poznan Supercomputing and Network Center
# Authors:
#   Damian Parniewicz (PSNC) <damianp_at_man.poznan.pl>
# 
# This software is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3 of the License, or (at your option) any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

import sys, os, logging
import socket, errno
from optparse import OptionParser
from threading import Lock, Thread
import thread, time
from functools import wraps

from deamon import Daemon
from corbaUtils import CorbaServant

sys.path.append(os.getcwd()+"/../../src/idl/") # add directory with corba stub to python modules path
import Proxy_Adapter__POA
import Proxy_Adapter
del sys.path[-1]

##############################################

MODULE_NAME = 'EZ-corba-stub'
__version__ = '0.1'

##############################################

def exception_handler(f):
    'intercepting all corba calls and checking for exceptions'
    wraps(f)
    def wrapper(*args, **kwargs):
        try:
            v = f(*args, **kwargs)
            return v
        except:
            import traceback
            logger.error("Exception" + traceback.format_exc())
    return wrapper

##############################################

class EZapi_monitor (Proxy_Adapter__POA.DevMonitor):
        
    def __init__(self, data):
        pass
     
    @exception_handler
    def testDevMonitor(self, testVal):
        logger.debug('DevMonitor.testDevMonitor(%s) called', testVal)
        return 0, 0

    @exception_handler
    def getPorts(self, ports):
        logger.debug('DevMonitor.getPorts(%s) called', ports)
        return 0, [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16]
     
    @exception_handler
    def getPortStatus(self, port_number):
        logger.debug('DevMonitor.getPortStatus(%d) called', port_number)
        return 0, True
    
    @exception_handler
    def getPortName(self, port_number):
        logger.debug('DevMonitor.getPortName(%d) called', port_number)
        return 0, "eth%d" % port_number
        
    @exception_handler
    def getPortMac(self, port_number, mac):
        logger.debug('DevMonitor.getPortMac(%d) called', port_number)
        return 0, "12345%d" % (port_number%10)
        
    @exception_handler
    def getPortFeatures(self, port_number):
        logger.debug('DevMonitor.getPortFeatures(%d) called', port_number)
        return 0, Proxy_Adapter.EZapiPort_Medium_Fiber, Proxy_Adapter.EZapiPort_Rate_1GB

##############################################

import struct

def show_flowtable_key(key, mask):
    KEY_FORMAT = "=BH6B6BHHIIBHH"
    key_length = struct.calcsize(KEY_FORMAT)
    def show(value, name):
        if len(value) < key_length:
            logger.debug("-> Too short flow entry")
            logger.debug("-> %s length is %d", name, len(value))
            return
        values = struct.unpack(KEY_FORMAT, value[:key_length])
        logger.debug(" %s " % name + "-> reserved: 0x%x, priority: %d, src_mac: %x:%x:%x:%x:%x:%x, dst_mac: %x:%x:%x:%x:%x:%x, vlan_tag: 0x%x, ether_type: 0x%x, ipv4_src: 0x%x, ipv4_dst: 0x%x, ip_proto: %d, src_port: %d, dst_port: %d" % values)
        vlan_id = values[14] & 0x0FFF
        vlan_pcp = (values[14] & 0xE000) >> 13
        vlan_flag = (values[14] & 0x1000) >> 12
        if name == "key":
            logger.debug("  %s:  vlan_id: %d, vlan_flag: %d, vlan_pcp: %d", name, vlan_id, vlan_flag, vlan_pcp)
        else:
            logger.debug("  %s:  vlan_id: 0x%x, vlan_flag: 0x%x, vlan_pcp: 0x%x", name, vlan_id, vlan_flag, vlan_pcp)    
    show(key, "key")
    show(mask, "mask")
   
def show_flowtable_result(result):
    RESULT_FORMAT = "BBB"
    result_length = struct.calcsize(RESULT_FORMAT)
    if len(result) < result_length:
        logger.debug("-> Too short flow result")
        logger.debug("-> Result length is %d", len(result))
        return
    values = struct.unpack(RESULT_FORMAT, result[:result_length])
    logger.debug(" result -> control: 0x%x, actions: 0x%x, port_number: %d" % values)
        

class EZapi_struct (Proxy_Adapter__POA.StructConf):
        
    def __init__(self, data):
        pass
     
    @exception_handler
    def testStructConf(self, testVal):
        logger.debug('StructConf.testStructConf(%s) called', testVal)
        return 0, 0
        
    @exception_handler
    def setStructEntry(self, struct_type, struct_num, k_length, r_length, key, result, mask):
        logger.debug('StructConf.setStruct called (struct_type: %s, struct_num: %d, k_length: %d, r_length: %d, key: %s, result: %s, mask: %s)', struct_type, struct_num, k_length, r_length, key, result, mask)
        
        if struct_type == Proxy_Adapter.EzapiSearch1 and struct_num == 0:
                show_flowtable_key(key, mask)
                show_flowtable_result(result)
        
        return 0
        
    @exception_handler
    def getStructEntry(self, struct_type, struct_num, index, k_length, r_length, key, result, mask):
        logger.debug('StructConf.getStruct called (struct_type: %s, struct_num: %d, index: %d, k_length: %d, r_length: %d, key: %s, result: %s, mask: %s)', struct_type, struct_num, index, k_length, r_length, key, result, mask)
        return 0, k_length, r_length, key, result, mask

    @exception_handler
    def getStructResult(self, struct_type, struct_num, k_length, key, r_length, result):
        logger.debug('StructConf.getStructResult called (struct_type: %s, struct_num: %d, k_length: %d, r_length: %d, key: %s, result: %s)', struct_type, struct_num, k_length, r_length, key, result)
        return 0, result
        
    @exception_handler
    def getStructLimit(self, struct_type, struct_num):
        logger.debug('StructConf.getStructLimit called (struct_type: %s, struct_num: %d)', struct_type, struct_num)
        return 0, 10

    @exception_handler
    def getStructLength(self, struct_type, struct_num):
        logger.debug('StructConf.getStructLength called (struct_type: %s, struct_num: %d)', struct_type, struct_num)
        return 0, 0
 
    @exception_handler
    def delStructEntry(self, struct_type, struct_num, k_length, r_length, key, result, mask):
        logger.debug('StructConf.delStruct called (struct_type: %s, struct_num: %d, k_length: %d, r_length: %d, key: %s, result: %s, mask: %s)', struct_type, struct_num, k_length, r_length, key, result, mask)
        
        if struct_type == Proxy_Adapter.EzapiSearch1 and struct_num == 0:
                show_flowtable_key(key, mask)
                show_flowtable_result(result)
                
        return 0
        
    @exception_handler
    def delStructAllEntries(self, struct_type, struct_num):
        logger.debug('StructConf.delStructAllEntries called (struct_type: %s, struct_num: %d)', struct_type, struct_num)
        return 0

##############################################
    
class CorbaServers(Thread):
    
    def __init__(self, config):
        '''contructor method required for access to common data model'''
        Thread.__init__(self)
        self.config = config
            
    def run(self):
        """Called when server is starting"""
        for servant in [EZapi_monitor, EZapi_struct]:
                server = CorbaServant(servant, None, '/tmp/ior')
                server.start()       

##############################################

class ModuleDaemon(Daemon):
    def __init__(self, moduleName, options):
        self.moduleName=moduleName
        self.options = options
        self.logger = logging.getLogger(self.__class__.__name__)
        pidFile = "%s/%s.pid" % (self.options.pidDir, self.moduleName)
        Daemon.__init__(self, pidFile)

    #---------------------
    def run(self):
        """
        Method called when starting the daemon. 
        """
        try:
            # starting interfaces threads
            self.servers = CorbaServers(self.options)
            self.servers.start()
        except:
            import traceback
            self.logger.error("Exception" + traceback.format_exc())
   

##############################################

if __name__ == "__main__":
    
    # optional command-line arguments processing
    usage="usage: %prog start|stop|restart [options]"
    parser = OptionParser(usage=usage, version="%prog " + __version__)
    parser.add_option("-p", "--pidDir", dest="pidDir", default='/tmp', help="directory for pid file")
    parser.add_option("-l", "--logDir", dest="logDir", default='.', help="directory for log file")
    options, args = parser.parse_args()

    if 'start' in args[0]:
        # clear log file
        try:
            os.remove("%s/%s.log" % (options.logDir, MODULE_NAME))
        except: 
            pass          

    # creation of logging infrastructure
    logging.basicConfig(filename = "%s/%s.log" % (options.logDir, MODULE_NAME),
                        level    = logging.DEBUG,
                        format   = "%(levelname)s - %(asctime)s - %(name)s - %(message)s")
    logger = logging.getLogger(MODULE_NAME)

    # starting module's daemon
    daemon = ModuleDaemon(MODULE_NAME, options)
    
    # mandatory command-line arguments processing
    if len(args) == 0:
        print usage
        sys.exit(2)
    if 'start' == args[0]:
        logger.info('starting the module')
        daemon.start()
    elif 'stop' == args[0]:
        logger.info('stopping the module')
        daemon.stop()
    elif 'restart' == args[0]:
        logger.info('restarting the module')
        daemon.stop()
        daemon.start()
    else:
        print "Unknown command"
        print usage
        sys.exit(2)
    sys.exit(0)

 
