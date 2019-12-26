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

#ifndef SOCKET_UDPV4_H
#define SOCKET_UDPV4_H

#include "sockdefs.h"

#include "SocketUdpv4.h"
#include "SocketAddressIPv4.h"
#include "SocketException.h"

#include "io-lib/Channel.h"
#include "io-lib/IOException.h"
#include "win-system/WsaStartup.h"
#include "thread/LocalMutex.h"

class SocketUdpv4
{
public:
  SocketUdpv4();
  virtual ~SocketUdpv4();
  
  void close();
  void bind(const TCHAR *bindHost, unsigned int bindPort);
  void bind(const SocketAddressIPv4 &addr) throw(SocketException);
  int sendTo(const TCHAR *toHost, unsigned int toPort, const char *data, int size, int flags = 0);
  int recvFrom(char *buffer, int size, int flags = 0, char *fromHost = NULL, unsigned int *fromPort = NULL);
  int available();
  bool getLocalAddr(SocketAddressIPv4 *addr);
  void setSocketOptions(int level, int name, void *value, socklen_t len) throw(SocketException);
  void getSocketOptions(int level, int name, void *value, socklen_t *len) throw(SocketException);
  void setExclusiveAddrUse() throw(SocketException);

private:
  WsaStartup m_wsaStartup;

protected:
  void set(SOCKET socket);

  LocalMutex m_mutex;
  SOCKET m_socket;
  SocketAddressIPv4 *m_localAddr;
};

#endif
