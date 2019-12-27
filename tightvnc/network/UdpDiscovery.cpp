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

#include <Windows.h>
#include <iphlpapi.h>
#include "atlstr.h"

#include "thread/AutoLock.h"

#define BUFFER_MAX_LENGTH 500
#define MSG_TYPE_QUERY_INFO 0
#define MSG_TYPE_MY_INFO 1
#define MSG_TYPE_REMOVE_INFO 2
#define MSG_RESEND_COUNT 2

UdpDiscovery::UdpDiscovery(const TCHAR *bindHost, unsigned short bindPort, unsigned short otherPort, bool autoStart, int mode, unsigned short sharePort) : m_bindHost(bindHost), m_bindPort(bindPort), m_otherPort(otherPort), m_mode(mode), m_sharePort(sharePort), m_lastTimestamp(0)
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
	if (m_mode == MODE_SERVER) {
		sendMsg(MSG_TYPE_REMOVE_INFO);
	}
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

map<string, SingleDiscovery> UdpDiscovery::getDiscovery() {
	AutoLock l(&m_mutex);
	map<string, SingleDiscovery>::iterator iterator = m_discoveryMap.begin();
	while (iterator != m_discoveryMap.end()) {
		SingleDiscovery singleDiscovery = iterator->second;
		time_t current = time(NULL);
		if ((current - singleDiscovery.timestamp) > 90) {
			iterator = m_discoveryMap.erase(iterator);
		} else {
			++iterator;
		}
	}
	return m_discoveryMap;
}

void UdpDiscovery::start()
{
	if (m_mode == MODE_CLIENT) {
		sendMsg(MSG_TYPE_QUERY_INFO);
	} else if (m_mode == MODE_SERVER) {
		sendMsg(MSG_TYPE_MY_INFO);
	}
	resume();
}

void UdpDiscovery::execute()
{
	char buffer[BUFFER_MAX_LENGTH] = { 0 };
	char fromHost[ADDRESS_MAX_LENGTH] = { 0 };
	unsigned int fromPort = 0;
	while (!isTerminating()) {
		if (m_mode == MODE_CLIENT) {
		} else if (m_mode == MODE_SERVER) {
			time_t currentTimestamp = time(NULL);
			if ((currentTimestamp - m_lastTimestamp) >= 25) {
				sendMsg(MSG_TYPE_MY_INFO);
			}
		}
		while (m_socket.available()) {
			int recvLength = m_socket.recvFrom(buffer, BUFFER_MAX_LENGTH, 0, fromHost, &fromPort);
			if (recvLength) {
				processMsg(buffer, recvLength, fromHost, fromPort);
			}
			if (m_mode == MODE_CLIENT) {
			} else if (m_mode == MODE_SERVER) {
				time_t currentTimestamp = time(NULL);
				if ((currentTimestamp - m_lastTimestamp) >= 25) {
					sendMsg(MSG_TYPE_MY_INFO);
				}
			}
			Thread::sleep(10);
		}
		Thread::sleep(250);
	}
}

void UdpDiscovery::processMsg(char *buffer, int size, char *fromHost, unsigned int fromPort) {
	//TCHAR str[500];
	//swprintf(str, _T("%S%S:%d\n"), buffer, fromHost, fromPort);
	//OutputDebugString(str);
	char* app = getValueFromMsg("app=", buffer, size);
	if (!app) {
		return; //discard
	}
	free(app);
	if (fromPort != m_otherPort) {
		return; //discard
	}
	char* type = getValueFromMsg("type=", buffer, size);
	if (!type) {
		return; //discard
	}
	if (m_mode == MODE_CLIENT) {
		if (stricmp(type, "my-info") == 0) {
			char* name = getValueFromMsg("name=", buffer, size);
			if (name) {
				char* address = getValueFromMsg("address=", buffer, size);
				if (address) {
					std::string addressStr(address);
					SingleDiscovery singleDiscovery;
					memset(&singleDiscovery, 0, sizeof(SingleDiscovery));
					strcpy(singleDiscovery.name, name);
					strcpy(singleDiscovery.address, address);
					singleDiscovery.timestamp = time(NULL);
					AutoLock l(&m_mutex);
					m_discoveryMap[addressStr] = singleDiscovery;
					free(address);
				}
				free(name);
			}
		} else if (stricmp(type, "remove-info") == 0) {
			char* address = getValueFromMsg("address=", buffer, size);
			if (address) {
				std::string addressStr(address);
				AutoLock l(&m_mutex);
				m_discoveryMap.erase(addressStr);
				free(address);
			}
		}
	} else if (m_mode == MODE_SERVER) {
		if (stricmp(type, "query-info") == 0) {
			sendMsg(MSG_TYPE_MY_INFO);
		}
	}
	free(type);
}

char* UdpDiscovery::getValueFromMsg(char *key, char *buffer, int size) {
	char *start = strstr(buffer, key);
	if (!start) {
		return NULL;
	}
	int keyLength = strlen(key);
	start = start + keyLength;
	if (start >= (buffer + size)) {
		return NULL;
	}
	char *end = strstr(start, "\n");
	if (!end) {
		return NULL;
	}
	int valueLength = end - start;
	char *value = (char*) malloc(valueLength + 1);
	if (!value) {
		return NULL;
	}
	memset(value, 0, valueLength + 1);
	strncpy(value, start, valueLength);
	return value;
}

void UdpDiscovery::sendMsg(int type) {
	IP_ADAPTER_INFO *adapterInfo = (IP_ADAPTER_INFO *) malloc(sizeof(IP_ADAPTER_INFO));
	ULONG adapterInfoLength = sizeof(IP_ADAPTER_INFO);  
	if (GetAdaptersInfo(adapterInfo, &adapterInfoLength) != ERROR_SUCCESS) {
		free(adapterInfo);
		adapterInfo = (IP_ADAPTER_INFO*) malloc(adapterInfoLength);
	}
	if (GetAdaptersInfo(adapterInfo, &adapterInfoLength) != ERROR_SUCCESS) {
		return;
	}
	PIP_ADAPTER_INFO pAdapter = adapterInfo;
	while (pAdapter) {
		unsigned long ip = inet_addr(pAdapter->IpAddressList.IpAddress.String);
		unsigned long mask = inet_addr(pAdapter->IpAddressList.IpMask.String);
		unsigned long broadcast = ip | ~mask;
		struct in_addr addr;
		addr.S_un.S_addr = broadcast;
		char* broadcastAddr = inet_ntoa(addr);
		//if (strcmp(broadcastAddr, "255.255.255.255") == 0 || strcmp(pAdapter->IpAddressList.IpAddress.String, "0.0.0.0") == 0 || strcmp(pAdapter->IpAddressList.IpAddress.String, "127.0.0.1") == 0) {
		//	pAdapter = pAdapter->Next;
		//	continue;
		//}
		if (type == MSG_TYPE_MY_INFO) {
			sendMyInfo(pAdapter->IpAddressList.IpAddress.String, broadcastAddr, m_sharePort);
		} else if (type == MSG_TYPE_QUERY_INFO) {
			sendQueryInfo(pAdapter->IpAddressList.IpAddress.String, broadcastAddr);
		}
		//TCHAR str[500];
		//swprintf(str, _T("%S: %S %S\n"), pAdapter->IpAddressList.IpAddress.String, pAdapter->IpAddressList.IpMask.String, broadcastAddr);
		//OutputDebugString(str);
		pAdapter = pAdapter->Next;
	}
	if (adapterInfo) {
		free(adapterInfo);
	}
}

void UdpDiscovery::sendMyInfo(char *ip, char *broadcast, unsigned short sharePort) {
	USES_CONVERSION;
	TCHAR name[NAME_MAX_LENGTH] = { 0 };
	DWORD length = NAME_MAX_LENGTH;
	if (!GetComputerName(name, &length)) {
		return;
	}
	char myInfoMsg[BUFFER_MAX_LENGTH] = "app=hype-local\ntype=my-info\nname=";
	sprintf(myInfoMsg, "%s%ws\naddress=%s:%d\n", myInfoMsg, name, ip, sharePort);
	for (int index = 0; index < MSG_RESEND_COUNT; index++) {
		sleep(5);
		m_socket.sendTo(A2T(broadcast), m_otherPort, myInfoMsg, strlen(myInfoMsg), 0);
	}
	m_lastTimestamp = time(NULL);
}

void UdpDiscovery::sendRemoveInfo(char *ip, char *broadcast, unsigned short sharePort) {
	USES_CONVERSION;
	char removeInfoMsg[BUFFER_MAX_LENGTH] = "app=hype-local\ntype=remove-info\n";
	sprintf(removeInfoMsg, "%saddress=%s:%d\n", removeInfoMsg, ip, sharePort);
	for (int index = 0; index < MSG_RESEND_COUNT; index++) {
		sleep(5);
		m_socket.sendTo(A2T(broadcast), m_otherPort, removeInfoMsg, strlen(removeInfoMsg), 0);
	}
}

void UdpDiscovery::sendQueryInfo(char *ip, char *broadcast) {
	USES_CONVERSION;
	char queryInfoMsg[] = "app=hype-local\ntype=query-info\n";
	for (int index = 0; index < MSG_RESEND_COUNT; index++) {
		sleep(5);
		m_socket.sendTo(A2T(broadcast), m_otherPort, queryInfoMsg, strlen(queryInfoMsg), 0);
	}
}



/*DWORD size;
DWORD errorCode = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_INCLUDE_PREFIX, NULL, NULL, &size);
if (errorCode != ERROR_BUFFER_OVERFLOW) {
return;
}
PIP_ADAPTER_ADDRESSES adapterAddresses = (PIP_ADAPTER_ADDRESSES) malloc(size);
errorCode = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_INCLUDE_PREFIX, NULL, adapterAddresses, &size);
if (errorCode != ERROR_SUCCESS) {
free(adapterAddresses);
return;
}
for (PIP_ADAPTER_ADDRESSES adapterAddress = adapterAddresses; adapterAddress != NULL; adapterAddress = adapterAddress->Next) {
for (PIP_ADAPTER_PREFIX_XP adapterAddressPrefixXp = adapterAddress->FirstPrefix; adapterAddressPrefixXp != NULL; adapterAddressPrefixXp = adapterAddressPrefixXp->Next) {
char buffer[BUFSIZ] = { 0 };
if (adapterAddressPrefixXp->Address.lpSockaddr->sa_family != AF_INET) {
continue;
}
getnameinfo(adapterAddressPrefixXp->Address.lpSockaddr, adapterAddressPrefixXp->Address.iSockaddrLength, buffer, sizeof(buffer), NULL, 0, NI_NUMERICHOST);
TCHAR str[500];
swprintf(str, _T("%lS: %S\n"), adapterAddress->FriendlyName, buffer);
OutputDebugString(str);
}
}
free(adapterAddresses);*/

// 3. periodically send info from server end
// 1. filter old record and display in drop down -- done
// 2. refresh the data even drop down is open
// 4. write code in server project as well
// 5. test using different systems
// 6. append port beside ip from server end      -- done
// 7. server remove info message                 -- done
// 8. msi certificate
