/*
 * cxmpclient.h
 *
 *  Created on: 12.01.2014
 *      Author: andreas
 */

#ifndef CXMPCLIENT_H_
#define CXMPCLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <inttypes.h>
#ifdef __cplusplus
}
#endif

#include "rofl/common/ciosrv.h"
#include "rofl/common/cmemory.h"
#include "rofl/common/csocket.h"
#include "../cxmpmsg.h"

namespace xdpd {
namespace mgmt {
namespace protocol {

class cxmpclient :
	public rofl::ciosrv,
	public rofl::csocket_owner
{
	rofl::csocket		socket;
	rofl::caddress		laddr;
	rofl::caddress		raddr;

	enum cxmpclient_timer_t {
		TIMER_XMPCLNT_EXIT 		= 1,
	};

public:

	/**
	 *
	 */
	cxmpclient();

	/**
	 *
	 */
	virtual
	~cxmpclient();

public:

	/**
	 *
	 */
	void
	port_attach(
			uint64_t dpid, std::string const& portname);

	/**
	 *
	 */
	void
	port_detach(
			uint64_t dpid, std::string const& portname);

	/**
	 *
	 */
	void
	port_enable(
			std::string const& portname);

	/**
	 *
	 */
	void
	port_disable(
			std::string const& portname);

protected:

	virtual void
	handle_timeout(int opaque);

protected:

	virtual void
	handle_accepted(rofl::csocket *socket, int newsd, rofl::caddress const& ra) {};

	virtual void
	handle_connected(rofl::csocket *socket, int sd) {};

	virtual void
	handle_connect_refused(rofl::csocket *socket, int sd) {};

	virtual void
	handle_read(rofl::csocket *socket, int sd) {};

	virtual void
	handle_closed(rofl::csocket *socket, int sd) {};
};

}; // end of namespace protocol
}; // end of namespace mgmt
}; // end of namespace xdpd



#endif /* CXMPCLIENT_H_ */
