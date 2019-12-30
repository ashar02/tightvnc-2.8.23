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

LoginDialog::LoginDialog(TvnViewer *viewer)
: BaseDialog(IDD_LOGINDIALOG),
  m_viewer(viewer),
  m_isListening(false),
  m_udpDiscoveryClient(DEFAULT_HOST_DISCOVERY, DEFAULT_PORT_DISCOVERY_CLIENT, DEFAULT_PORT_DISCOVERY_SERVER, MODE_CLIENT)//,
  //m_udpDiscoveryServer(DEFAULT_HOST_DISCOVERY, DEFAULT_PORT_DISCOVERY_SERVER, DEFAULT_PORT_DISCOVERY_CLIENT, MODE_SERVER)
{
  m_udpDiscoveryClient.setDiscoveryCB(this, &(LoginDialog::discoveryCB));
  m_udpDiscoveryClient.start();
  //m_udpDiscoveryServer.setSharePort(DEFAULT_PORT);
  //m_udpDiscoveryServer.start();
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
  m_server.setItemWidth(300);
  m_server.setItemHeight(0, 17);
  
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

ComboBox* LoginDialog::getServer() {
  return &m_server;
}

void LoginDialog::updateHistory()
{
  USES_CONVERSION;

  StringStorage currentServer;
  m_server.getText(&currentServer);
  m_server.removeAllItems();
  m_server.setText(currentServer.getString());
  m_discoveryMap = m_udpDiscoveryClient.getDiscovery();
  int index = 0;
  TCHAR str[500];
  swprintf(str, _T("Update history count %d\n"), m_discoveryMap.size());
  OutputDebugString(str);
  for (map<string, SingleDiscovery>::iterator iterator = m_discoveryMap.begin(); iterator != m_discoveryMap.end(); iterator++)
  {
	SingleDiscovery singleDiscovery = iterator->second;
	char item[NAME_MAX_LENGTH + ADDRESS_MAX_LENGTH + 1] = { 0 };
	sprintf(item, "%s -- %s", singleDiscovery.name, singleDiscovery.address);
	m_server.addItem(A2T(item));
	//TCHAR str[500];
	//swprintf(str, _T("Update history %S\n"), item);
	//OutputDebugString(str);
	index++;
  }
  m_server.setText(currentServer.getString());
  if (m_server.getItemsCount()) {
    if (currentServer.isEmpty()) {
      m_server.setSelectedItem(0);
    }
	StringStorage server;
	int selectedItemIndex = m_server.getSelectedItemIndex();
	if (selectedItemIndex >= 0) {
	  m_server.getItemText(selectedItemIndex, &server);
	  const TCHAR *item = server.getString();
	  if (!server.isEmpty()) {
		if (item) {
		  const TCHAR *subStr = _tcsstr(item, _T(" -- "));
		  if (subStr) {
			subStr += 4;
			updateServerAfterDelay(subStr);
			server = subStr;
		  }
		}
	  }
	}
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
	case CBN_SELCHANGE:
      {
        int selectedItemIndex = m_server.getSelectedItemIndex();
        if (selectedItemIndex < 0) {
          return FALSE;
        }
        StringStorage server;
        m_server.getItemText(selectedItemIndex, &server);
		if (!server.isEmpty()) {
		  const TCHAR *item = server.getString();
		  if (item) {
			const TCHAR *subStr = _tcsstr(item, _T(" -- "));
			if (subStr) {
			  subStr += 4;
			  updateServerAfterDelay(subStr);
			  server = subStr;
			}
		  }
		  ConnectionConfigSM ccsm(RegistryPaths::VIEWER_PATH,
								 server.getString());
		  m_connectionConfig.loadFromStorage(&ccsm);
		}
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

void LoginDialog::discoveryCB(void *obj) {
  LoginDialog *loginDialog = (LoginDialog*) obj;
  if (!loginDialog) {
	return;
  }
  ComboBox *server = loginDialog->getServer();
  if (!server) {
	return;
  }
  if (server->getDropDownState()) {
	server->showDropDown(false);
	loginDialog->updateHistory();
	server->showDropDown(true);
  }
}

void LoginDialog::updateServerAfterDelay(const TCHAR *text) {
  struct Param *param = (struct Param*) malloc(sizeof(struct Param));
  if (param) {
	param->obj = this;
	_tcscpy(param->text, text);
	HANDLE handle = CreateThread(NULL, 0, UpdateComboEdit, param, 0, NULL);
	if (!handle) {
      free(param);
	}
  }
}

DWORD LoginDialog::UpdateComboEdit(LPVOID lpParam) {
  struct Param *param = (struct Param*) lpParam;
  if (!param) {
	return 0;
  }
  LoginDialog *loginDialog = (LoginDialog*) param->obj;
  if (!loginDialog) {
	free(param);
	return 0;
  }
  ComboBox *server = loginDialog->getServer();
  if (!server) {
	free(param);
	return 0;
  }
  Thread::sleep(10);
  server->setEditText(param->text);
  free(param);
  return 0;
}
