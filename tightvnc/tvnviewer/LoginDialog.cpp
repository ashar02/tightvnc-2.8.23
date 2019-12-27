// Copyright (C) 2011,2012 GlavSoft LLC.
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

#include "LoginDialog.h"
#include "NamingDefs.h"
#include "OptionsDialog.h"

#include "win-system/Shell.h"
#include "atlstr.h"

const TCHAR LoginDialog::DEFAULT_HOST_DISCOVERY[] = _T("0.0.0.0");

LoginDialog::LoginDialog(TvnViewer *viewer)
: BaseDialog(IDD_LOGINDIALOG),
  m_viewer(viewer),
  m_isListening(false),
  m_udpDiscoveryClient(DEFAULT_HOST_DISCOVERY, DEFAULT_PORT_DISCOVERY_CLIENT, DEFAULT_PORT_DISCOVERY_SERVER, true, MODE_CLIENT, 0),
  m_udpDiscoveryServer(DEFAULT_HOST_DISCOVERY, DEFAULT_PORT_DISCOVERY_SERVER, DEFAULT_PORT_DISCOVERY_CLIENT, true, MODE_SERVER, 5900)
{
}

LoginDialog::~LoginDialog()
{
}

BOOL LoginDialog::onInitDialog()
{
  setControlById(m_server, IDC_CSERVER);
  setControlById(m_listening, IDC_LISTENING);
  setControlById(m_ok, IDOK);
  updateHistory();
  SetForegroundWindow(getControl()->getWindow());
  m_server.setFocus();
  if (m_isListening) {
    m_listening.setEnabled(false);
  }

  int adjustment = -176;
  moveDialog(0, 0, 0, adjustment, TRUE);

  return TRUE;
}

void LoginDialog::enableConnect()
{
  StringStorage str;
  int iSelected = m_server.getSelectedItemIndex();
  if (iSelected == -1) {
    m_server.getText(&str);
    m_ok.setEnabled(!str.isEmpty());
  } else {
    m_ok.setEnabled(true);
  }
}

void LoginDialog::updateHistory()
{
  USES_CONVERSION;
  //ConnectionHistory *conHistory;

  StringStorage currentServer;
  m_server.getText(&currentServer);
  m_server.removeAllItems();
  //conHistory = ViewerConfig::getInstance()->getConnectionHistory();
  //conHistory->load();
  //for (size_t i = 0; i < conHistory->getHostCount(); i++) {
  //  m_server.insertItem(static_cast<int>(i), conHistory->getHost(i));
  //}
  map<string, SingleDiscovery> discoveryMap = m_udpDiscoveryClient.getDiscovery();
  int index = 0;
  for (map<string, SingleDiscovery>::iterator iterator = discoveryMap.begin(); iterator != discoveryMap.end(); iterator++)
  {
	SingleDiscovery singleDiscovery = iterator->second;
	char item[NAME_MAX_LENGTH + ADDRESS_MAX_LENGTH + 1] = { 0 };
	sprintf(item, "%s -- %s", singleDiscovery.name, singleDiscovery.address);
	m_server.insertItem(static_cast<int>(index), A2T(item));
	index++;
  }
  m_server.setText(currentServer.getString());
  if (m_server.getItemsCount()) {
    if (currentServer.isEmpty()) {
      m_server.setSelectedItem(0);
    }
	StringStorage server;
	int selectedItemIndex = m_server.getSelectedItemIndex();
	m_server.getItemText(selectedItemIndex, &server);
	const TCHAR *item = server.getString();
	if (item) {
		const TCHAR *subStr = wcsstr(item, _T(" -- "));
		if (subStr) {
			subStr += 4;
			m_server.setText(subStr, selectedItemIndex);
			server = subStr;
		}
	}
    //m_server.getText(&server);
    ConnectionConfigSM ccsm(RegistryPaths::VIEWER_PATH,
                            server.getString());
    m_connectionConfig.loadFromStorage(&ccsm);
  }
}

void LoginDialog::onConnect()
{
  ConnectionHistory *conHistory = ViewerConfig::getInstance()->getConnectionHistory();

  m_server.getText(&m_serverHost);

  conHistory->load();
  conHistory->addHost(m_serverHost.getString());
  conHistory->save();

  if (!m_serverHost.isEmpty()) {
    ConnectionConfigSM ccsm(RegistryPaths::VIEWER_PATH, m_serverHost.getString());
    m_connectionConfig.saveToStorage(&ccsm);
  }

  m_viewer->newConnection(&m_serverHost, &m_connectionConfig);
}

void LoginDialog::onConfiguration()
{
  m_viewer->postMessage(TvnViewer::WM_USER_CONFIGURATION);
}

BOOL LoginDialog::onOptions()
{
  OptionsDialog dialog;
  dialog.setConnectionConfig(&m_connectionConfig);
  dialog.setParent(getControl());
  if (dialog.showModal() == 1) {
    StringStorage server;
    m_server.getText(&server);
    if (server.isEmpty()) {
      ConnectionConfigSM ccsm(RegistryPaths::VIEWER_PATH,
                              server.getString());
      m_connectionConfig.saveToStorage(&ccsm);
    }
    return FALSE;
  }
  return TRUE;
}

void LoginDialog::onOrder()
{
  openUrl(StringTable::getString(IDS_URL_LICENSING_FVC));
}

void LoginDialog::openUrl(const TCHAR *url)
{
  // TODO: removed duplicated code (see AboutDialog.h)
  try {
    Shell::open(url, 0, 0);
  } catch (const SystemException &sysEx) {
    StringStorage message;

    message.format(StringTable::getString(IDS_FAILED_TO_OPEN_URL_FORMAT), sysEx.getMessage());

    MessageBox(m_ctrlThis.getWindow(),
               message.getString(),
               StringTable::getString(IDS_MBC_TVNVIEWER),
               MB_OK | MB_ICONEXCLAMATION);
  }
}

void LoginDialog::setListening(bool isListening)
{
  m_isListening = isListening;
  if (isListening) {
    m_listening.setEnabled(false);
  } else {
    m_listening.setEnabled(true);
  }
}

void LoginDialog::onListening()
{
  ConnectionConfigSM ccsm(RegistryPaths::VIEWER_PATH,
                          _T(".listen"));
  m_connectionConfig.loadFromStorage(&ccsm);

  m_listening.setEnabled(false);
  m_viewer->startListening(ViewerConfig::getInstance()->getListenPort());
}

void LoginDialog::onAbout()
{
  m_viewer->postMessage(TvnViewer::WM_USER_ABOUT);
}

BOOL LoginDialog::onCommand(UINT controlID, UINT notificationID)
{
  switch (controlID) {
  case IDC_CSERVER:
    switch (notificationID) {
    case CBN_DROPDOWN:
      updateHistory();
      break;

    // select item in ComboBox with list of history
    case CBN_SELENDOK:
      {
        int selectedItemIndex = m_server.getSelectedItemIndex();
        if (selectedItemIndex < 0) {
          return FALSE;
        }
        StringStorage server;
        m_server.getItemText(selectedItemIndex, &server);
		const TCHAR *item = server.getString();
		if (item) {
			const TCHAR *subStr = wcsstr(item, _T(" -- "));
			if (subStr) {
				subStr += 4;
				m_server.setText(subStr, selectedItemIndex);
				server = subStr;
			}
		}
        ConnectionConfigSM ccsm(RegistryPaths::VIEWER_PATH,
                                server.getString());
        m_connectionConfig.loadFromStorage(&ccsm);
        break;
      }
    }

    enableConnect();
    break;

  // click "Connect"
  case IDOK:
    onConnect();
    kill(0);
    break;

  // cancel connection
  case IDCANCEL:
    kill(0);
    break;

  case IDC_BCONFIGURATION:
    onConfiguration();
    break;

  case IDC_BOPTIONS:
    return onOptions();

  case IDC_LISTENING:
    onListening();
    kill(0);
    break;

  case IDC_ORDER_SUPPORT_BUTTON:
    onOrder();
    break;

  case IDC_BABOUT:
    onAbout();
    break;

  default:
    _ASSERT(true);
    return FALSE;
  }
  return TRUE;
}

StringStorage LoginDialog::getServerHost()
{
  return m_serverHost;
}
