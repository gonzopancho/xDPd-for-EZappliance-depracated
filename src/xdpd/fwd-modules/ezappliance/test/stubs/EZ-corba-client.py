import sys, os, time

sys.path.append(os.getcwd()+"/../../src/idl/") # add directory with corba stub to python modules path
import Proxy_Adapter
del sys.path[-1]

from corbaUtils import corbaClient

def test_DevMonitor():
    ref = corbaClient(Proxy_Adapter.DevMonitor, iorFile='/tmp/ior/EZapi_monitor.ior')
    print "DevMonitor.testDevMonitor result:", ref.testDevMonitor(1)
    print "DevMonitor.getPorts result:", ref.getPorts([])
    print "DevMonitor.getPortStatus result:", ref.getPortStatus(1)
    print "DevMonitor.getPortName result:", ref.getPortName(1) 
    print "DevMonitor.getPortMac result:", ref.getPortMac(1, "000000") 
    print "DevMonitor.getPortFeatures result:", ref.getPortFeatures(1)
    


def test_StructConf():
    ref = corbaClient(Proxy_Adapter.StructConf, iorFile='/tmp/ior/EZapi_struct.ior')
    #print "StructConf.testStructConf result:", ref.testStructConf(1,1)
    print "StructConf.setStructEntry result:", ref.setStructEntry(Proxy_Adapter.EzapiSearch1, 0, 38, 8, "\x01"*38, "\x03"+7*"\x02", "\xFF"*38)
    #print "StructConf.getStructEntry result:", ref.getStructEntry(Proxy_Adapter.EzapiParse2, 1, 2, 3, 4, "5", "6", "7")
    #print "StructConf.getStructResult result:", ref.getStructResult(Proxy_Adapter.EzapiParse2, 1, 2, "3", 4, "5")
    #print "StructConf.getStructLimit result:", ref.getStructLimit(Proxy_Adapter.EzapiParse2, 1)
    #print "StructConf.getStructLength result:", ref.getStructLength(Proxy_Adapter.EzapiParse2, 1)
    #print "StructConf.delStructEntry result:", ref.delStructEntry(Proxy_Adapter.EzapiParse2, 1, 2, 3, "4", "5", "6")
    #print "StructConf.delStructAllEntries result:", ref.delStructAllEntries(Proxy_Adapter.EzapiParse2, 1)
    

#test_DevMonitor()
test_StructConf()
