import sys, os, time

sys.path.append(os.getcwd()+"/../../src/idl/") # add directory with corba stub to python modules path
import Proxy_Adapter
del sys.path[-1]

from corbaUtils import corbaClient

def clear_flow_table():
    ref = corbaClient(Proxy_Adapter.StructConf, iorFile='/tmp/ior/EZapi_struct.ior')
    print "StructConf.delStructAllEntries result:", ref.delStructAllEntries(Proxy_Adapter.EzapiSearch1, 0)

clear_flow_table()
