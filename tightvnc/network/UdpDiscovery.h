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
#define NAME_MAX_LENGTH 300
#define ADDRESS_MAX_LENGTH 30

struct SingleDiscovery {
	char name[NAME_MAX_LENGTH];
	char address[ADDRESS_MAX_LENGTH];
	time_t timestamp;
};

class UdpDiscovery : private Thread
{
public:
	UdpDiscovery(const TCHAR *bindHost, unsigned short bindPort, unsigned short otherPort, bool autoStart, int mode, unsigned short sharePort) throw(Exception);
	virtual ~UdpDiscovery();
	const TCHAR *getBindHost() const;
	unsigned short getBindPort() const;
	map<string, SingleDiscovery> getDiscovery();

protected:
	virtual void start();
	virtual void execute();

private:
	void processMsg(char *buffer, int size, char *fromHost = NULL, unsigned int fromPort = 0);
	char* getValueFromMsg(char *key, char *buffer, int size);
	void sendMsg(int type);
	void sendMyInfo(char *ip, char *broadcast, unsigned short sharePort);
	void sendRemoveInfo(char *ip, char *broadcast, unsigned short sharePort);
	void sendQueryInfo(char *ip, char *broadcast);

	SocketUdpv4 m_socket;
	StringStorage m_bindHost;
	unsigned short m_bindPort;
	unsigned short m_otherPort;
	unsigned short m_sharePort;
	int m_mode;
	map<string, SingleDiscovery> m_discoveryMap;
	LocalMutex m_mutex;
	time_t m_lastTimestamp;
};

#endif
