#
# The Geysers project (work funded by European Commission).
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USASA

import uuid, httplib, sys, os, thread

sys.path.append(os.getcwd()+"/../") # add directory with corba stub to python modules path
import Proxy_Adapter__POA as Proxy_Adapter
import Proxy_Adapter as Proxy
import _GlobalIDL as glob
del sys.path[-1]

from corbaUtils import CorbaServant, corbaClient

import logging

logging.basicConfig(filename = "EZ-corba-stub.log", level = logging.DEBUG, 
                           format = "%(levelname)s - %(asctime)s - %(name)s - %(message)s")
logger = logging.getLogger('EZ-corba-stub')


class DevMonitor (Proxy_Adapter.DevMonitor):
     
    def testDevMonitor(self, testVal):
        print 'DevMonitor.testDevMonitor() called'
        return 0

    def getPorts(self, ports):
        print 'DevMonitor.getPorts() called'
        return [1,2,3]
        
    def getPortStatus(self, port_number):
        print 'DevMonitor.getPortStatus() called'
        return True
        
    def getPortName(self, port_number):
        print 'DevMonitor.getPortName() called'
        return "eth0"

if __name__ == '__main__':
    # processed when module is started as a standlone application
    for servant in [DevMonitor]:
        server = CorbaServant(servant, None, '/tmp/')
        server.start()

