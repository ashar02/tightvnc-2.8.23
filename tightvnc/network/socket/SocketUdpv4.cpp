// Copyright (C) 2008,2009,2010,2011,2012 GlavSoft LLC.
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

#include <stdlib.h>
#include "SocketAddressIPv4.h"
#include "SocketAddressIPv4.h"
#include "SocketUdpv4.h"

#include "thread/AutoLock.h"

#include <crtdbg.h>

SocketUdpv4::SocketUdpv4() : m_localAddr(NULL), m_wsaStartup(1, 2)
{
  m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (m_socket == INVALID_SOCKET) {
    throw SocketException();
  }
}

SocketUdpv4::~SocketUdpv4()
{
  if (m_socket) {
#ifdef _WIN32
  ::closesocket(m_socket);
#else
  ::close(m_socket);
#endif
  }
  AutoLock l(&m_mutex);
  if (m_localAddr) {
    delete m_localAddr;
  }
}

int SocketUdpv4::available()
{
  int result;
  int err = ::ioctlsocket(m_socket, FIONREAD, reinterpret_cast<u_long*>(&result));
  if (err != 0) {
	return 0;
  }
  return result;
}

void SocketUdpv4::close()
{
  if (m_socket) {
#ifdef _WIN32
    ::closesocket(m_socket);
#else
	::close(m_socket);
#endif
  }
  m_socket = -1;
}

void SocketUdpv4::bind(const TCHAR *bindHost, unsigned int bindPort)
{
  SocketAddressIPv4 address(bindHost, bindPort);
  bind(address);
}

void SocketUdpv4::bind(const SocketAddressIPv4 &addr)
{
  struct sockaddr_in bindSockaddr = addr.getSockAddr();
  if (::bind(m_socket, (const sockaddr *)&bindSockaddr, addr.getAddrLen()) == SOCKET_ERROR) {
    throw SocketException();
  }
  AutoLock l(&m_mutex);
  if (m_localAddr) {
    delete m_localAddr;
  }
  m_localAddr = new SocketAddressIPv4(*(struct sockaddr_in*)&bindSockaddr);
}

void SocketUdpv4::set(SOCKET socket)
{
  AutoLock l(&m_mutex);
#ifdef _WIN32
  ::closesocket(m_socket);
#else
  ::close(m_socket);
#endif
  m_socket = socket;
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(struct sockaddr_in);
  if (getsockname(socket, (struct sockaddr *)&addr, &addrlen) == 0) {
    m_localAddr = new SocketAddressIPv4(addr);
  }
}

int SocketUdpv4::sendTo(const TCHAR *toHost, unsigned int toPort, const char *data, int size, int flags)
{
  SocketAddressIPv4 address(toHost, toPort);
  struct sockaddr_in toAddr = address.getSockAddr();
  int result;
  result = ::sendto(m_socket, data, size, 0, (const sockaddr *)&toAddr, address.getAddrLen());
  return result;
}

int SocketUdpv4::recvFrom(char *buffer, int size, int flags, char *fromHost, unsigned int *fromPort)
{
  int result;
  struct sockaddr_in fromAddr;
  memset(&fromAddr, 0, sizeof(fromAddr));
  int len = sizeof(fromAddr);
  result = recvfrom(m_socket, buffer, size, flags, (sockaddr *)&fromAddr, &len);
  if (result && (fromHost || fromPort)) {
	  struct sockaddr_in *sin = (struct sockaddr_in *)&fromAddr;
	  if (fromHost) {
		  strcpy(fromHost, inet_ntoa(sin->sin_addr));
	  }
	  if (fromPort) {
		  (*fromPort) = htons(sin->sin_port);
	  }
  }
  return result;
}

bool SocketUdpv4::getLocalAddr(SocketAddressIPv4 *addr)
{
  AutoLock l(&m_mutex);
  if (m_localAddr == 0) {
    return false;
  }
  *addr = *m_localAddr;
  return true;
}

void SocketUdpv4::setSocketOptions(int level, int name, void *value, socklen_t len)
{
  if (setsockopt(m_socket, level, name, (char*)value, len) == SOCKET_ERROR) {
    throw SocketException();
  }
}

void SocketUdpv4::getSocketOptions(int level, int name, void *value, socklen_t *len)
{
  if (getsockopt(m_socket, level, name, (char*)value, len) == SOCKET_ERROR) {
    throw SocketException();
  }
}

void SocketUdpv4::setExclusiveAddrUse()
{
  int val = 1;
  setSocketOptions(SOL_SOCKET, SO_EXCLUSIVEADDRUSE, &val, sizeof(val));
}
