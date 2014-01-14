/*
 * xmp.h
 *
 *  Created on: 11.01.2014
 *      Author: andreas
 */

#ifndef XDPD_MANAGER_H_
#define XDPD_MANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <inttypes.h>
#ifdef __cplusplus
}
#endif

#include "rofl/common/csocket.h"
#include "../../switch_manager.h"
#include "../../port_manager.h"
#include "../../plugin_manager.h"

#include "cxmpmsg.h"

namespace xdpd {
namespace mgmt {
namespace protocol {

class xmp :
		public ciosrv,
		public csocket_owner,
		public plugin
{
	csocket					socket;		// listening socket
	std::string				udp_addr;	// binding address
	uint16_t				udp_port;	// listening UDP port

#define MGMT_PORT_UDP_ADDR	"127.0.0.1"
#define MGMT_PORT_UDP_PORT	8444

public:

	xmp();

	virtual ~xmp();

	virtual void init(int args, char** argv);

	virtual std::string get_name(void){
		return std::string("xmp");
	};

protected:

	/*
	 * overloaded from ciosrv
	 */

	virtual void
	handle_timeout(
			int opaque);

protected:

	/*
	 * overloaded from csocket_owner
	 */

	virtual void
	handle_accepted(csocket *socket, int newsd, caddress const& ra) {};

	virtual void
	handle_connected(csocket *socket, int sd) {};

	virtual void
	handle_connect_refused(csocket *socket, int sd) {};

	virtual void
	handle_read(csocket *socket, int sd);

	virtual void
	handle_closed(csocket *socket, int sd) {};

private:

	void
	handle_request(
			cxmpmsg& msg);

	void
	handle_port_attach(
			cxmpmsg& msg);

	void
	handle_port_detach(
			cxmpmsg& msg);

	void
	handle_port_enable(
			cxmpmsg& msg);

	void
	handle_port_disable(
			cxmpmsg& msg);
};

}; // end of namespace protocol
}; // end of namespace mgmt
}; // end of namespace xdpd



#endif /* XDPD_MANAGER_H_ */
