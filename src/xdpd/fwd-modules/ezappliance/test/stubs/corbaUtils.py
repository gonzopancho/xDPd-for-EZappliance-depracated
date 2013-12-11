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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

import sys, os
from omniORB import CORBA
import CosNaming
import traceback
from threading import Thread

import logging
logger = logging.getLogger('corba')

class CorbaException(Exception): pass

class CorbaServant(Thread):
    """Inherit this class if you want create Corba Servant for the interface"""
    def __init__(self, servantClass, data, iorDir, iorName=None):
        """
        Constructor of the class with arguments:
            servantClass - a class implementing the interface
            dataModels - object containing data
            iorDir - path to the directory where ior file will be created
            iorName - ior filename - otherwise IDL interface name is used
        """
        Thread.__init__(self)
        self.servantClass = servantClass
        self.interfaceName = iorName or servantClass.__name__
        self.data = data
        logger.info('initializing corba enviroment for %s' % self.interfaceName)
        try:
            self.orb = CORBA.ORB_init([], CORBA.ORB_ID)
            rootPoa = self.orb.resolve_initial_references("RootPOA")
            poaManager = rootPoa._get_the_POAManager()
            poaManager.activate()
            self.iorFile = iorDir + '/%s.ior' % self.interfaceName

            servantInstance = self.servantClass(self.data)
            servantObject = servantInstance._this()
            #logger.debug("Look at servant %s", servantObject)
            self.servantObject = servantObject
            self.ior = self.orb.object_to_string(servantObject)
        except:
            logger.error("Exception" + traceback.format_exc())
            raise CorbaException("Corba enviroment initialization failed" + traceback.format_exc())
     
    #-------------------------     
    def run(self):
        """Called when Servant is starting"""
        logger.info('starting corba servant')
        try:        
            # unrem if you would like try to use Corba Namespace
            #try:
            #    nameRoot    = self.orb.resolve_initial_references("NameService")
            #    rootContext = nameRoot._narrow(CosNaming.NamingContext)
            #    name        = [CosNaming.NameComponent("Default", "Object")]
            #    rootContext.rebind(name, servantObject)
            #except:
            #    logger.error('Binding to NameSerive unsuccessful !')
            #    logger.error("Exception" + traceback.format_exc())
                
            #writing ior of the interface object to the file
            file(self.iorFile,'w').write(self.ior)
            self.orb.run()
        except:
            #traceback.print_exc()
            logger.error("Exception" + traceback.format_exc())
            raise CorbaException("Corba servant initialization failed")
    #-------------------------  
    def clean(self):
        """removing ior file"""
        if os.path.exists(self.iorFile):
            os.remove(self.iorFile)

#=====================================================

def corbaClient(objectClass, iorDir=None, iorFile=None):
    """
    Creates reference to interface object running in the servant processes.
    Arguments are:
        servantClass - a class implementing the interface
        iorDir - path to the directory where ior files are existing 
                   (exact ior file name is concluded from objectClass name)
        iorFile - direct path the the ior file with reference to the object"""
    logger.info('starting corba client')
    if  not iorDir and not iorFile:
        logger.error("iorDir or iorFile must be specified!")
        raise CorbaException("Corba client wrong configuration")
        
    try:
        orb = CORBA.ORB_init([], CORBA.ORB_ID)
        if not iorFile:
            #concluding the name of ior file from interface object class name
            iorFile = iorDir + '/%s.ior' % objectClass.__name__
        ior = file(iorFile).readline()
        obj = orb.string_to_object(ior)
        objectReference = obj._narrow(objectClass)
        # reference to the remote object is available
        return objectReference
    except:
        traceback.print_exc()
        logger.error("Exception" + traceback.format_exc())
        raise CorbaException("Corba connection to %s failed!" % objectClass.__name__)

#=====================================================

def corbaClient2(objectClass):       
    try:
        orb = CORBA.ORB_init([], CORBA.ORB_ID)
        obj = orb.resolve_initial_references("NameService")
        rootContext = obj._narrow(CosNaming.NamingContext)

        # Resolve the name
        name = [CosNaming.NameComponent("Default", "Object")]
        try:
            obj = rootContext.resolve(name)
        except CosNaming.NamingContext.NotFound, ex:
            traceback.print_exc()
            logger.error("Name not found")
            return

        objectReference = obj._narrow(objectClass)
        # reference to the remote object is available
        return objectReference
    except:
        traceback.print_exc()
        logger.error("Exception" + traceback.format_exc())
        raise CorbaException("Corba client initialization failed")
