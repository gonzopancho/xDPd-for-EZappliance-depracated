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

import sys, os, logging, time
import socket, errno
import struct, binascii
from optparse import OptionParser
from threading import Lock, Thread
import thread

from deamon import Daemon


##############################################

MODULE_NAME = 'EZ_tcp_server'
__version__ = '0.1'

##############################################

def generateMessage(input_port, frame):
    n = len(frame)
    return struct.pack("!BH%ds" % n, input_port, len(frame), frame)

def sendMessage(conn, delay, message):
    try:
        logger.info('Event registered for %d seconds', delay)
        time.sleep(delay)
        conn.sendall(message)
        logger.info("Message sent")
    except socket.error as err:
        if err.errno == errno.EBADF:
            logger.error("Socket no longer exists")
        else:
            logger.error("Generic error", exc_info=True)

##############################################
class TcpServer(Thread):
    
    def __init__(self, config):
        '''contructor method required for access to common data model'''
        Thread.__init__(self)
        self.config = config
        self.is_active = True
            
    def run(self):
        """Called when server is starting"""
        try:
            HOST = ''                 
            PORT = 8080
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            s.bind((HOST, PORT))
            s.listen(1)

            i = 1
            logger.info("Listing on port %d", PORT)
            while self.is_active:
                logger.info("%d: Waiting for connection...", i)
                conn, addr = s.accept()
                thread.start_new_thread(sendMessage, (conn, 7, generateMessage(2, "C"*3000)))
                logger.info("%d: Connected by %s", i, addr)
                while self.is_active:
                    logger.info('wait for data')
                    try: 
                        data = conn.recv(4096)
                        if not data:
                            break
                        self.handle_data(data, conn)
                    except socket.error as err:
                        if err.errno == errno.ECONNRESET:
                            logger.error("Client disconnected")
                            break
                        else:
                            raise
                conn.close()
                logger.info("%d: Connection closed", i)
                i += 1
            s.close()
        except:
            logger.error("Generic error", exc_info=True)
            
    def handle_data(self, data, conn):
        logger.debug("Data received: %s", data)
        if len(data) > 1:
            logger.debug("Output port %d, length is %d" % struct.unpack("!BH", data[:3]))
        
    def stop(self):
        self.is_active = False
    

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
            self.tcpServer = TcpServer(self.options)
            self.tcpServer.start()
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

 
