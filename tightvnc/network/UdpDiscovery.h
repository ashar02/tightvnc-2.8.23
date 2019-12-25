// Copyright (C) 2009,2010,2011,2012 GlavSoft LLC.
// All rights reserved.
//
//-------------------------------------------------------------------------
// This file is part of the TightVNC software.  Please visit our Web site:
//
//                       http://www.tightvnc.com/
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//-------------------------------------------------------------------------
//

#ifndef _MULTI_THREAD_UDP_DISCOVERY_H_
#define _MULTI_THREAD_UDP_DISCOVERY_H_

#include "thread/Thread.h"
#include "util/Exception.h"
#include "network/socket/SocketUdpv4.h"

#define MODE_SERVER 0
#define MODE_CLIENT 1

class UdpDiscovery : private Thread
{
public:
	UdpDiscovery(const TCHAR *bindHost, unsigned short bindPort, bool autoStart, int mode) throw(Exception);
	virtual ~UdpDiscovery();
	const TCHAR *getBindHost() const;
	unsigned short getBindPort() const;

protected:
	virtual void start();
	virtual void execute();

private:
	SocketUdpv4 m_socket;
	StringStorage m_bindHost;
	unsigned short m_bindPort;
	int m_mode;
};

#endif
