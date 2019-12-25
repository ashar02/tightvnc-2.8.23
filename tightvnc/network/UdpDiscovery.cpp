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

#include "UdpDiscovery.h"
#include "network/socket/SocketAddressIPv4.h"

UdpDiscovery::UdpDiscovery(const TCHAR *bindHost, unsigned short bindPort, bool autoStart, int mode) : m_bindHost(bindHost), m_bindPort(bindPort), m_mode(mode)
{
	SocketAddressIPv4 bindAddr = SocketAddressIPv4::resolve(bindHost, bindPort);
	BOOL broadcast = 1;
	m_socket.setSocketOptions(SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
	m_socket.bind(bindAddr);
	if (autoStart) {
		start();
	}
}

UdpDiscovery::~UdpDiscovery()
{	
	m_socket.close();
	if (isActive()) {
		Thread::terminate();
		Thread::wait();
	}
}

const TCHAR *UdpDiscovery::getBindHost() const
{
	return m_bindHost.getString();
}

unsigned short UdpDiscovery::getBindPort() const
{
	return m_bindPort;
}

void UdpDiscovery::start()
{
	if (m_mode == MODE_CLIENT) {
		char fetchListMsg[] = "app=hype-local\ntype=fetch-list\n";
		m_socket.sendTo(_T("192.168.100.255"), m_bindPort, fetchListMsg, strlen(fetchListMsg), 0);
	} else if (m_mode == MODE_SERVER) {
		char broadcastMeMsg[] = "app=hype-local\ntype=broadcast-me\nname=\naddress=\n";
		m_socket.sendTo(_T("192.168.100.255"), m_bindPort, broadcastMeMsg, strlen(broadcastMeMsg), 0);
	}
	resume();
}

void UdpDiscovery::execute()
{
	const int len = 100;
	char buffer[len];
	while (!isTerminating()) {
		Thread::sleep(200);
		while (m_socket.available()) {
			m_socket.recvFrom(buffer, len, 0);
			//process here
		}
	}
}
