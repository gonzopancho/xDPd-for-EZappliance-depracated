xDPd-for-EZappliance
====================

The forwarding module for EZappliance NP-3 network processor platform

## Brief

The content of this folder is the implementation of the EZappliance (AFA-compliant) forwarding module. The EZappliance forwarding module is controlling hardware datapath of NP-3 network processor as well as providing additionally software datapath (complementing limitation of NP-3 processor) based on GNU/Linux forwarding module.

## Usage

<pre>
./autogen.sh
cd ./build
../configure -enable-ezappliance
make
</pre>

## Running

<pre>
cd ./build/src/xdpd
./xdpd -c <i>config.cfg</i>
</pre>

## More info about xDPd:
https://www.codebasin.net/redmine/projects/xdpd/wiki
