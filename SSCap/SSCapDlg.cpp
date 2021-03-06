
// SSCapDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SSCap.h"
#include "afxdialogex.h"
#include "ssloader.h"
#include "RunConfig.h"
#include "Utils.h"
#include "SocketBase.h"
#include "SocksClient.h"
#include "SSClient.h"
#include "Listener.h"
#include "SSCapDlg.h"
#include "AboutDlg.h"
#include "msgdef.h"
#include "popmenu.h"
#include "BaseDef.h"
#include "NewSSNodeDlg.h"
#include "UIListCtrl.h"
#include "SreenCaptureDlg.h"
#include "AddSSFromLink.h"
#include "AddSSFromJsonDlg.h"
#include "zxing/QRParser.h"
#include "SystemConfigureDlg.h"
#include "APPConfig.h"
#include "SysProxy.h"
#include "Registry.h"
#include "EncyptionMgr.h"
#include "privoxy.h"
#include "version.h"
#include "dibtoddb.h"

#include "PacEditorDialog.h"
#include "pac.h"

#include "GenericHTTPClient.h"
#include "ShowQRCodeDialog.h"

#include "DownloadSelectorDialog.h"

#include "ZBarParser.h"

#include "SysWideProxy.h"
#include "tarobase64.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL CSSCapDlg::bHasShowedBalloonTip = FALSE;

/** @brief 取得主程序的主界面对话框的指针
*/
CSSCapDlg *GetMainDlg()
{
	return (CSSCapDlg  *)AfxGetApp()->GetMainWnd();
}
CListCtrl* GetSSListContainer()
{
	CSSCapDlg *pDlg = GetMainDlg();
	if( pDlg ){
		return pDlg->GetSSListContainer();
	}

	return NULL;
}
// CAboutDlg dialog used for App About

#define ID_MAIN_WND_TIMER 1690

// CSSCapDlg dialog
CSSCapDlg::CSSCapDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSSCapDlg::IDD, pParent)
	, nTrayIcon( IDR_MAINFRAME )
	, bUseAutoDetect( FALSE )
	, bUseAutoConfigUrl( FALSE )
	, bUseProxyServer( FALSE )
	, m_bRegisterHotKey( FALSE )
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_nMainWndTimer = 0;
	//m_nLastActiveItem = 0;
	pTestingCurrentProxyDlg = NULL;
	pWebLoaderTread = NULL;
	pCounterPage = NULL;
	bMainWindowShow = TRUE;

	memset( lpAutoConfigUrl, 0, sizeof( TCHAR ) * 1024 );
	memset( lpProxyServer, 0, sizeof( TCHAR ) * 100 );
	memset( lpByPass, 0, sizeof( TCHAR ) * 500 );
}
CSSCapDlg::~CSSCapDlg()
{
	EndGetNewVersionThread();

	if( pTestingCurrentProxyDlg )
	{
		delete pTestingCurrentProxyDlg;
		pTestingCurrentProxyDlg = NULL;
	}
}

void CSSCapDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_SSNODE, m_listSSNodes);
	DDX_Control(pDX, IDC_STATIC_LOCALSOCKS_PORT, m_staLocalSocksPortTip);
	DDX_Control(pDX, IDC_BUTTON_ADD, m_btnAdd);
	DDX_Control(pDX, IDC_BUTTON_ADD_QRCODE, m_btnAddByQRCode);
	DDX_Control(pDX, IDC_BUTTON_ADD_BATCH, m_btnAddByBatch);
	DDX_Control(pDX, IDC_BUTTON_ADD_LINK, m_btnAddByLink);
	DDX_Control(pDX, IDC_BUTTON_ACTIVE, m_btnActiveNode);
	DDX_Control(pDX, IDC_BUTTON_EDIT, m_btnEditNode);
	DDX_Control(pDX, IDC_BUTTON_DEL, m_btnDelNode);
	DDX_Control(pDX, IDC_BUTTON_EXIT, m_btnExit);
	DDX_Control(pDX, IDC_BUTTON_ABOUT, m_btnAbout);
	DDX_Control(pDX, IDC_BUTTON_MAINPAGE, m_btnMainPage);
	DDX_Control(pDX, IDC_BUTTON_SETTING, m_btnSetting);
	DDX_Control(pDX, IDC_BUTTON_CHECK, m_btnCheck);
	DDX_Control(pDX, IDC_BUTTON_REFRESH, m_btnRefresh);
	DDX_Control(pDX, IDC_STATIC_HOTKEY_FOR_ADDQRCODE, m_staHotKeyTips);
}

BEGIN_MESSAGE_MAP(CSSCapDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CSSCapDlg::OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_ADD_QRCODE, &CSSCapDlg::OnBnClickedButtonAddQRCode)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, &CSSCapDlg::OnBnClickedButtonEdit)
	ON_BN_CLICKED(IDC_BUTTON_DEL, &CSSCapDlg::OnBnClickedButtonDel)
	ON_BN_CLICKED(IDC_BUTTON_ADD_BATCH, &CSSCapDlg::OnBnClickedButtonAddBatch)
	ON_BN_CLICKED(IDC_BUTTON_ADD_LINK, &CSSCapDlg::OnBnClickedButtonAddLink)
	ON_BN_CLICKED(IDC_BUTTON_ACTIVE, &CSSCapDlg::OnBnClickedButtonActive)
	ON_WM_TIMER()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_BN_CLICKED(IDC_BUTTON_EXIT, &CSSCapDlg::OnBnClickedButtonExit)
	ON_MESSAGE(WM_MY_TRAY_NOTIFICATION, OnTrayNotification )
	ON_WM_DROPFILES()
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_SSNODE, &CSSCapDlg::OnNMDblclkListSsnode)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_SSNODE, &CSSCapDlg::OnNMRClickListSsnode)
	ON_NOTIFY(NM_CLICK, IDC_LIST_SSNODE, &CSSCapDlg::OnNMClickListSsnode)
	ON_BN_CLICKED(IDC_BUTTON_CHECK, &CSSCapDlg::OnBnClickedButtonCheck)
	ON_BN_CLICKED(IDC_BUTTON_SETTING, &CSSCapDlg::OnBnClickedButtonSetting)
	ON_BN_CLICKED(IDC_BUTTON_MAINPAGE, &CSSCapDlg::OnBnClickedButtonMainpage)
	ON_BN_CLICKED(IDC_BUTTON_ABOUT, &CSSCapDlg::OnBnClickedButtonAbout)
	ON_BN_CLICKED(IDC_BUTTON_REFRESH, &CSSCapDlg::OnBnClickedButtonRefresh)
	ON_MESSAGE( WM_MSG_FOUND_NEWVERSION, OnFoundNewVersion )

	ON_WM_WINDOWPOSCHANGING()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_HOTKEY()
END_MESSAGE_MAP()


// CSSCapDlg message handlers

BOOL CSSCapDlg::OnInitDialog()
{
	CSSConfigInfo *pCfg = GetConfigInfo();

	pCfg->bLoadSaveNodes = !CAPPConfig::IsValidConfig() || !CAPPConfig::IsDisableAddNodes();

	// 加载ss服务器列表
	LoadShadowsocksServer( );

	InitWebAd();

	InitializeDialog();

	// 注册热键
	RegisterHotKey(_T(""));

	pCfg->isPrivoxyRunned = RunPrivoxy( );

	RestartLocalSocketService( TRUE );

	CRegistry registry;
	if( pCfg->runAtStartup == 1 )
	{
		//写注册表
		if( registry.Open( _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run")) ){
			TCHAR szOutBuffer[MAX_PATH] = {0};
			_stprintf_s(szOutBuffer,MAX_PATH,_T("\"%s\""),CRunConfig::GetApplicationFullPathName() );
			registry.Write( SSCAP_NAME, szOutBuffer );
			registry.Close();
		}
	}
	else
	{
		if( registry.Open( _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run")) ){
			registry.DeleteValue( SSCAP_NAME );
			registry.Close();
		}
	}

	InitializeSSNodeList();
	InitializeButtons();

	ChangeControlSize();

	//////////////////////////////////////////////////////////////////////////
	// 先获得IE旧的配置,以便退出时恢复
	GetSystemProxyInfo( 
		bUseAutoDetect,
		bUseAutoConfigUrl, 
		lpAutoConfigUrl,
		1024,
		bUseProxyServer,
		lpProxyServer,
		100,
		lpByPass,
		500
		);
	//////////////////////////////////////////////////////////////////////////

	// 只有privoxy启动了, 才可以使用系统代理的相关功能.
	// 如果配置启用系统代理.
	if( !CAPPConfig::IsValidConfig() 
		&& pCfg->GetNodeSize() > 0  // 存在节点的情况下, 如果不存在节点, 则不应该启用代理, 否则会导至不能连网
		&& pCfg->isPrivoxyRunned 
		&& ( pCfg->enable 
				// 如果在我们启动之前, 用户就设置了IE代理,那么此时,我们会将用户之前设置的IE代理改为我们的,退出时我们会为用户恢复
				|| bUseAutoConfigUrl 
				|| bUseProxyServer 
				)
		)
	{
		// global ?
		EnableSysWideProxy( m_hWnd, TRUE, pCfg->global || bUseProxyServer, FALSE );
	}
	else 
	{ 
		// 否则禁止代理, 这里之所以要禁用一次,是因为有可能其它版本的SS客户端 启用了代理,或者用户自己启用了代理.
		DisableSystemProxy();
	}

	// 如果用户设置了启动自动缩小到系统托盘
	if( pCfg->startInSystray )
	{
		SetShowMainWindow( FALSE );
	}

	StartGetNewVersionThread( this );
	StartGetIPLocation( NULL );

	return TRUE;  // return TRUE  unless you set the focus to a control
}
/** @brief 注册热键
*/
BOOL CSSCapDlg::RegisterHotKey( CString strHotKey ,WORD wNewVirtualKeyCode, WORD wNewModifiers ,BOOL bSave)
{
	CSSConfigInfo *pCfg = GetConfigInfo();

	// 如果有快捷键, 这里注册它
	WORD wOldVirtualKeyCode = 0 ;
	WORD wOldModifiers = 0 ;

	// 获得以前的HOTKEY
	pCfg->GetHotKeyForAddQRCode( wOldModifiers, wOldVirtualKeyCode );

	if( wOldVirtualKeyCode != wNewVirtualKeyCode 
		|| wOldModifiers != wNewModifiers )
	{
		WORD wRegVirtualKeyCode = wNewVirtualKeyCode ;
		WORD wRegModifiers = wNewModifiers ;

		if( wNewVirtualKeyCode == 0)
			wRegVirtualKeyCode = wOldVirtualKeyCode;
		if( wNewModifiers == 0 )
			wRegModifiers = wOldModifiers;

		// CHotKeyCtrl的值转换RegisterHotKey
		wRegModifiers = HKF2SDK(wRegModifiers);

		::UnregisterHotKey( GetMainDlg()->GetSafeHwnd(), WM_HOTKEY_FOR_ADDFROMQRCODE );

		m_bRegisterHotKey = ::RegisterHotKey( m_hWnd, WM_HOTKEY_FOR_ADDFROMQRCODE, wRegModifiers, wRegVirtualKeyCode);

		if( m_bRegisterHotKey && bSave )
		{
			pCfg->SetHotKeyForAddQRCode( wNewModifiers,wNewVirtualKeyCode ,wstring(strHotKey.GetBuffer()));
		}
	}

	CString strHotKeyTip;
	if( m_bRegisterHotKey )
		strHotKeyTip.Format( lm_u82u16_s( _("Hot key for add from qrcode: %s") ),pCfg->strHotKey.c_str() );
	else 
		strHotKeyTip.Format( lm_u82u16_s( _("Hot key for add from qrcode: n/a") ) );

	m_staHotKeyTips.SetWindowText( strHotKeyTip );

	return m_bRegisterHotKey;
}

// 重启本地SOCKS服务
BOOL CSSCapDlg::RestartLocalSocketService(BOOL bNoticeSearchPort /** 允许提示搜索端口,只用在启动时, 在配置界面中是不允许提示的*/)
{
	CSSConfigInfo *pCfg = GetConfigInfo();
	CString strLocalSocks5,strPrivoxy;
	BOOL bRet = TRUE ;
	BOOL bInSearchPortMode = FALSE;
	char szMsg[1000] = {0};

	ssManager.StopServices();

CHECKPORT:
	if( CheckTcpPortValid( pCfg->localPort ) ) 
	{
		if( !ssManager.StartServices() )
		{
			//char szMsg[1000] = {0};
			sprintf_s( szMsg, 1000, _("Local socks 5 service starting error: %s"),ssManager.GetLastErrorString() );

			::MessageBox( m_hWnd, lm_u82u16_s( szMsg ),CAPPConfig::GetSoftName().c_str(), MB_OK | MB_ICONERROR );

			bRet = FALSE;
		}

		if( bInSearchPortMode )
		{
			sprintf_s( szMsg, 1000, _("Local socks 5 service was bind at port [%d]."),pCfg->localPort );

			::MessageBox( m_hWnd, lm_u82u16_s( szMsg ),CAPPConfig::GetSoftName().c_str(), MB_OK | MB_ICONINFORMATION );

			bRet = TRUE;
		}
	}
	else
	{
		// 允许搜索端口
		if( bNoticeSearchPort )
		{
			sprintf_s( szMsg, 1000, _("Local socks 5 service port [%d] is in use,do you need to bind an unsed port for you automatic?"),pCfg->localPort );

			if( ::MessageBox( m_hWnd, lm_u82u16_s( szMsg ),CAPPConfig::GetSoftName().c_str(), MB_YESNO | MB_ICONERROR ) == IDYES )
			{
				UINT nPort = SearchAnUnsedPort( pCfg->localPort + 1 , 100 );

				if( nPort == 0 )
				{
					sprintf_s( szMsg, 1000, _("Search port was failed, please specify an unsed port manually in the configure.") );
					::MessageBox( m_hWnd, lm_u82u16_s( szMsg ),CAPPConfig::GetSoftName().c_str(), MB_OK | MB_ICONERROR );

					bRet = FALSE;
				}
				else 
				{
					pCfg->localPort = nPort;
					bInSearchPortMode = TRUE;

					goto CHECKPORT;
				}
			}
		}
		else 
		{
			sprintf_s( szMsg, 1000, _("Local socks 5 service port [%d] is in use,change it to another port in the configure please."),pCfg->localPort );

			::MessageBox( m_hWnd, lm_u82u16_s( szMsg ),CAPPConfig::GetSoftName().c_str(), MB_OK | MB_ICONERROR );

			bRet = FALSE;
		}
	}

	if( ssManager.IsListenerStarted() )
	{
		strLocalSocks5.Format( lm_u82u16_s( _("Socks 5: [%d] ") ),pCfg->localPort );
	}
	else
	{
		strLocalSocks5 = CString( lm_u82u16_s( _("Socks 5 is stoped.")) );
	}
	if( IsPrivoxyStarted() )
	{
		strPrivoxy.Format( lm_u82u16_s( _("Http: [%d]")),GetPrivoxyListenPort() );
	}
	else 
	{
		strPrivoxy = CString( lm_u82u16_s( _("Http is stoped.")));
	}
	m_staLocalSocksPortTip.SetWindowText( strLocalSocks5 + strPrivoxy );

	_UpdateTrayIconMessage();

	if( bRet )
	{
		// 重新加载PAC文件, 因为PORT可能改变了
		LoadPacFile( TRUE );

		pCfg->isPrivoxyRunned = RunPrivoxy( );
	}
	
	SaveShadowsocksServer();

	return bRet;
}

/** @brief 更新tray icon提示信息
*/
void CSSCapDlg::_UpdateTrayIconMessage()
{
	CString strRemoteSS;
	char szRemotess[1000] = {0};
	CString strLocalSocks5;
	CString strPrivoxy;
	CSSConfigInfo *pCfg = GetConfigInfo();

	if( ssManager.IsListenerStarted() )
	{
		strLocalSocks5.Format( lm_u82u16_s( _("Local socks 5 working on port: [%d]") ),pCfg->localPort );
	}
	else
	{
		strLocalSocks5 = CString( lm_u82u16_s( _("Local socks 5 service is stoped.")) );
	}

	CSSNodeInfo *pNode = pCfg->GetActiveNodeInfo();
	if( pNode )
	{
		if( pNode->remarks.empty() )
			sprintf_s(szRemotess, 1000, "%s:%d", pNode->server.c_str(), pNode->server_port );
		else 
			sprintf_s(szRemotess, 1000, "%s (%s:%d)", pNode->remarks.c_str(), pNode->server.c_str(), pNode->server_port );
		strRemoteSS.Format( _T("%s"), lm_u82u16_s( szRemotess ) );
	}
	else 
	{
		strRemoteSS.Format( _T("%s"), lm_u82u16_s( _("Remote shadowsocks server is not available")) );
	}
	if( IsPrivoxyStarted() )
	{
		strPrivoxy.Format( lm_u82u16_s( _("Local http is working on: [%d]")),GetPrivoxyListenPort() );
	}
	else 
	{
		strPrivoxy = CString( lm_u82u16_s( _("Local tttp is stoped.")));
	}

	TCHAR szIconTips[2000] = {0};
	_stprintf_s( szIconTips,2000,_T("%s %s\r\n%s\r\n%s\r\nShadowsocks node: %s")
		,SSCAP_NAME,SSCAP_VERSION
		,strLocalSocks5.GetBuffer()
		,strPrivoxy.GetBuffer()
		,strRemoteSS.GetBuffer()
		);

	nTrayIcon.SetIcon(MAKEINTRESOURCE(IDR_MAINFRAME),szIconTips );

}

/** @brief 注册开机自动运行的注册表配置
*/
void CSSCapDlg::RegisterSysConfigAutoRun()
{
	CSSConfigInfo *pCfg = GetConfigInfo();
	if( pCfg->runAtStartup )
	{
		CRegistry registry;
		//写注册表
		if( registry.Open( _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run")) ){
			TCHAR szOutBuffer[MAX_PATH];
			_stprintf_s(szOutBuffer,MAX_PATH,_T("\"%s\""),CRunConfig::GetApplicationFullPathName() );
			registry.Write( SSCAP_NAME, szOutBuffer );
			registry.Close();
		}
	}
	return;
}
BOOL CSSCapDlg::InitWebAd()
{
	return TRUE;
}

BOOL CSSCapDlg::InitializeDialog()
{
	CDialogEx::OnInitDialog();

	if( !CAPPConfig::IsValidConfig() )
	{
		// IDM_ABOUTBOX must be in the system command range.
		ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
		ASSERT(IDM_ABOUTBOX < 0xF000);

		CMenu* pSysMenu = GetSystemMenu(FALSE);
		if (pSysMenu != NULL)
		{
			BOOL bNameValid;
			CString strAboutMenu;
			bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
			ASSERT(bNameValid);
			if (!strAboutMenu.IsEmpty())
			{
				pSysMenu->AppendMenu(MF_SEPARATOR);
				pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
			}
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	tooltip.Create( this );
	tooltip.AddTool( &m_btnAdd ,lm_u82u16_s(_("Add shadowsocks server.") ) );
	tooltip.AddTool( &m_btnAddByQRCode,lm_u82u16_s(_("Add shadowsocks server from QR Code.") ) );
	tooltip.AddTool( &m_btnAddByBatch,lm_u82u16_s(_("Add shadowsocks server from json format.") ) );
	tooltip.AddTool( &m_btnAddByLink,lm_u82u16_s(_("Add shadowsocks server from ss link.") ) );
	tooltip.AddTool( &m_btnActiveNode,lm_u82u16_s(_("Activate selected shadowsocks server.") ) );
	tooltip.AddTool( &m_btnEditNode,lm_u82u16_s(_("Edit selection.") ) );
	tooltip.AddTool( &m_btnDelNode,lm_u82u16_s(_("Delete selection.") ) );
	tooltip.AddTool( &m_btnCheck,lm_u82u16_s(_("Check selection.") ) );

	tooltip.AddTool( &m_btnRefresh,lm_u82u16_s(_("Refresh proxy nodes from website") ) );
	tooltip.AddTool( &m_btnSetting,lm_u82u16_s(_("Configuration") ) );
	tooltip.AddTool( &m_btnMainPage,lm_u82u16_s(_("Online Help") ) );
	CString strAbout, strExit;
	strAbout.Format( lm_u82u16_s(_("About %s") ), CAPPConfig::GetSoftName().c_str() );
	strExit.Format( lm_u82u16_s(_("Exit %s") ), CAPPConfig::GetSoftName().c_str() );
	tooltip.AddTool( &m_btnAbout, strAbout.GetBuffer());
	tooltip.AddTool( &m_btnExit, strExit.GetBuffer() );

	m_btnActiveNode.EnableWindow( FALSE );
	m_btnEditNode.EnableWindow( FALSE );
	m_btnDelNode.EnableWindow( FALSE );
	m_btnCheck.EnableWindow( FALSE );

	CString strTitle;
	strTitle.Format(_T("%s %s"),CAPPConfig::GetSoftName().c_str(), SSCAP_VERSION );
	SetWindowText( strTitle );

	m_staLocalSocksPortTip.SetTextColor( RGB(255,0,0));
	m_staHotKeyTips.SetTextColor( RGB(255,0,0));

	CSSConfigInfo *pCfg = GetConfigInfo();

	m_nMainWndTimer = SetTimer( ID_MAIN_WND_TIMER, 5000, NULL );

	nTrayIcon.SetNotificationWnd(this, WM_MY_TRAY_NOTIFICATION);

	RefreshSSNodeListCountTip();

	return TRUE;
}

BOOL CSSCapDlg::InitializeSSNodeList()
{
	// Create and attach image list
	m_ImageList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 0);
	m_ImageList.Add(AfxGetApp()->LoadIcon(IDI_ICON_PROXY_EMPTY ) );
	m_ImageList.Add(AfxGetApp()->LoadIcon(IDI_ICON_PROXY_INUSE ) );
	m_listSSNodes.SetImageList(&m_ImageList, LVSIL_SMALL);
	m_listSSNodes.SetCellMargin(1.2);

	UILC_InsertColumn( &m_listSSNodes );
	UILC_RebuildListCtrl( &m_listSSNodes );

	return TRUE;
}

void CSSCapDlg::InitializeButtons()
{
	m_btnAdd.SetIcon( IDI_ICON_ADDONE_HOVER, IDI_ICON_ADDONE ,(int)BTNST_AUTO_GRAY);
	m_btnAdd.SetBtnCursor( ::LoadCursor(NULL,IDC_HAND) );

	m_btnAddByQRCode.SetIcon( IDI_ICON_ADD_BYQRCODE_HOVER, IDI_ICON_ADD_BYQRCODE ,(int)BTNST_AUTO_GRAY);
	m_btnAddByQRCode.SetBtnCursor( ::LoadCursor(NULL,IDC_HAND) );

	m_btnAddByBatch.SetIcon( IDI_ICON_BATCH_HOVER, IDI_ICON_BATCH ,(int)BTNST_AUTO_GRAY);
	m_btnAddByBatch.SetBtnCursor( ::LoadCursor(NULL,IDC_HAND) );

	m_btnAddByLink.SetIcon( IDI_ICON_LINK_HOVER, IDI_ICON_LINK ,(int)BTNST_AUTO_GRAY);
	m_btnAddByLink.SetBtnCursor( ::LoadCursor(NULL,IDC_HAND) );

	m_btnActiveNode.SetIcon( IDI_ICON_ACTIVE_HOVER, IDI_ICON_ACTIVE ,(int)BTNST_AUTO_GRAY);
	m_btnActiveNode.SetBtnCursor( ::LoadCursor(NULL,IDC_HAND) );

	m_btnEditNode.SetIcon( IDI_ICON_EDIT_HOVER, IDI_ICON_EDIT ,(int)BTNST_AUTO_GRAY);
	m_btnEditNode.SetBtnCursor( ::LoadCursor(NULL,IDC_HAND) );

	m_btnDelNode.SetIcon( IDI_ICON_DEL_HOVER, IDI_ICON_DEL ,(int)BTNST_AUTO_GRAY);
	m_btnDelNode.SetBtnCursor( ::LoadCursor(NULL,IDC_HAND) );


	m_btnCheck.SetIcon( IDI_ICON_CHECK_HOVER, IDI_ICON_CHECK ,(int)BTNST_AUTO_GRAY);
	m_btnCheck.SetBtnCursor( ::LoadCursor(NULL,IDC_HAND) );

	if( CAPPConfig::IsValidConfig() )
	{
		m_btnRefresh.ShowWindow( SW_SHOW);
		m_btnRefresh.SetIcon( IDI_ICON_REFRESH_HOVER, IDI_ICON_REFRESH,(int)BTNST_AUTO_GRAY);
		m_btnRefresh.SetBtnCursor( ::LoadCursor(NULL,IDC_HAND) );

		if( CAPPConfig::IsDisableAddNodes() )
		{
			m_btnAdd.EnableWindow( FALSE );
			m_btnAddByQRCode.EnableWindow( FALSE );
			m_btnAddByBatch.EnableWindow( FALSE );
			m_btnAddByLink.EnableWindow( FALSE );
			m_btnEditNode.EnableWindow( FALSE );
			m_btnDelNode.EnableWindow( FALSE );
		}
	}
	else
	{
		m_btnRefresh.ShowWindow( SW_HIDE ) ;
	}

	m_btnSetting.SetIcon( IDI_ICON_SETTING_HOVER, IDI_ICON_SETTING,(int)BTNST_AUTO_GRAY);
	m_btnSetting.SetBtnCursor( ::LoadCursor(NULL,IDC_HAND) );

	m_btnMainPage.SetIcon( IDI_ICON_ONLINEHELP_HOVER, IDI_ICON_ONLINEHELP ,(int)BTNST_AUTO_GRAY);
	m_btnMainPage.SetBtnCursor( ::LoadCursor(NULL,IDC_HAND) );

	m_btnAbout.SetIcon( IDI_ICON_ABOUT_HOVER, IDI_ICON_ABOUT ,(int)BTNST_AUTO_GRAY);
	m_btnAbout.SetBtnCursor( ::LoadCursor(NULL,IDC_HAND) );
	if( CAPPConfig::IsValidConfig() )
		m_btnAbout.EnableWindow( FALSE );

	m_btnExit.SetIcon( IDI_ICON_EXIT_HOVER, IDI_ICON_EXIT ,(int)BTNST_AUTO_GRAY);
	m_btnExit.SetBtnCursor( ::LoadCursor(NULL,IDC_HAND) );
}

void CSSCapDlg::RefreshSSNodeListCountTip()
{
	char szText[1000] = {0};
	CSSConfigInfo *pCfg = GetConfigInfo();

	sprintf_s( szText, 1000, _("Server nodes: [%d]"), pCfg->GetNodeSize() );

	CString strText;
	strText.Format( _T("%s"),lm_u82u16_s( szText ) );
		
	GetDlgItem(IDC_STATIC_TIPS1)->SetWindowText( strText );
}

void CSSCapDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
	
	if( nID == SC_CLOSE || nID == SC_MINIMIZE )
	{
		if( nID == SC_CLOSE )
		{
			ShowWindow ( SW_MINIMIZE );
		}

		ShowWindow( SW_HIDE );
		SetShowMainWindow( FALSE );

		if( !CSSCapDlg::bHasShowedBalloonTip )
		{
			TCHAR szTitle[1024]  = {0};
			TCHAR szBody[1024] = {0};

			_stprintf_s( szTitle,lm_u82u16_s(_("%s is minimized to tray")), CAPPConfig::GetSoftName().c_str() );
			_stprintf_s( szBody, lm_u82u16_s(_("you can restore it with double-click system tray icon or context menu")));

			nTrayIcon.ShowBalloonTip(szBody, szTitle ,10 );

			CSSCapDlg::bHasShowedBalloonTip = TRUE;
		}
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CSSCapDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CSSCapDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

LRESULT CSSCapDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	nTrayIcon.WindowProc( message, wParam, lParam );

	return CDialogEx::WindowProc(message, wParam, lParam);
}


BOOL CSSCapDlg::DestroyWindow()
{
	//if( pWebLoaderTread )
	//{
// 		CWnd *pWnd = pWebLoaderTread->GetMainWnd();
// 		if( pWnd )
// 		{
// 			pWnd->SendMessage( WM_CLOSE,0,0 );
// 		}

		//pWebLoaderTread->PostThreadMessage( WM_QUIT, 0 , 0 );
	//}

	::UnregisterHotKey( m_hWnd, WM_HOTKEY_FOR_ADDFROMQRCODE );

	ssManager.StopServices();

	if( m_nMainWndTimer )
	{
		KillTimer( m_nMainWndTimer );
		m_nMainWndTimer = 0;
	}

	return CDialogEx::DestroyWindow();
}


void CSSCapDlg::OnTimer(UINT_PTR nIDEvent)
{
	if( nIDEvent == m_nMainWndTimer )
	{
		UILC_RefreshItems( &m_listSSNodes );
		//m_listSSNodes.Invalidate( TRUE );
	}

	CDialogEx::OnTimer(nIDEvent);
}


void CSSCapDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	ChangeControlSize();
}


void CSSCapDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
#define WND_MIN_WIDTH 785
#define WND_MIN_HEIGHT 377

	lpMMI->ptMinTrackSize.x = WND_MIN_WIDTH;
	lpMMI->ptMinTrackSize.y = WND_MIN_HEIGHT;


	CDialogEx::OnGetMinMaxInfo(lpMMI);
}


void CSSCapDlg::OnCancel()
{
	// TODO: Add your specialized code here and/or call the base class

	//CDialogEx::OnCancel();
}


void CSSCapDlg::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class

	//CDialogEx::OnOK();
}

void CSSCapDlg::ChangeControlSize()
{
	if( !IsWindow( m_listSSNodes.GetSafeHwnd() )
		|| !IsWindow( m_btnAdd.GetSafeHwnd() )
		|| !IsWindow( m_btnAddByQRCode.GetSafeHwnd() )
		|| !IsWindow( m_btnAddByBatch.GetSafeHwnd() )
		|| !IsWindow( m_btnAddByLink.GetSafeHwnd() )
		|| !IsWindow( m_btnActiveNode.GetSafeHwnd() )
		|| !IsWindow( m_btnEditNode.GetSafeHwnd() )
		|| !IsWindow( m_btnDelNode.GetSafeHwnd() )
		//|| !IsWindow( m_staBottomRange.GetSafeHwnd() )
		)
		return;

	CRect rcClient, rcList, rcBtnAdd, rcBtnAddByQRcode, rcBtnAddByBatch, rcBtnAddByLink,rcBtnActive,rcBtnEdit,rcBtnDel,rcCheck;
	CRect rcRefresh,rcSetting, rcOnlineHelp, rcAbout, rcExit;
	//CRect rcTopRange,rcBottomRange;
	CRect wndListCtrl;
	CRect wndAddBtn;
	CRect wndClient;

	GetClientRect( &rcClient );
	GetWindowRect( &wndClient );

	//m_listSSNodes.GetClientRect( &rcList );
	m_listSSNodes.GetWindowRect( &wndListCtrl );
	m_btnAdd.GetWindowRect( &wndAddBtn );

	wndListCtrl.right = wndClient.right;
	wndListCtrl.bottom = wndAddBtn.top - 5;

	//m_staTopRange.GetClientRect( &rcTopRange );
	//m_staBottomRange.GetClientRect( &rcBottomRange );

	m_btnAdd.GetClientRect( &rcBtnAdd );
	m_btnAddByQRCode.GetClientRect( &rcBtnAddByQRcode );
	m_btnAddByBatch.GetClientRect( &rcBtnAddByBatch );
	m_btnAddByLink.GetClientRect( &rcBtnAddByLink );
	m_btnActiveNode.GetClientRect( &rcBtnActive );
	m_btnEditNode.GetClientRect( &rcBtnEdit );
	m_btnDelNode.GetClientRect( &rcBtnDel );
	m_btnCheck.GetClientRect( &rcCheck );

	m_btnRefresh.GetClientRect( &rcRefresh );
	m_btnSetting.GetClientRect( &rcSetting );
	m_btnMainPage.GetClientRect( &rcOnlineHelp );
	m_btnAbout.GetClientRect( &rcAbout );
	m_btnExit.GetClientRect( &rcExit );

	ScreenToClient( &wndListCtrl );

	wndListCtrl.right = rcClient.right;
	m_listSSNodes.MoveWindow( wndListCtrl );
	//m_listSSNodes.SetWindowPos( NULL, 0,0, rcClient.Width(), rcClient.Height()- ( rcBottomRange.Height() ) /** bottom */ - ( rcTopRange.Height() ) /* top */ , SWP_NOMOVE );

	int left = 10;	// 第一个按扭隔左边的距离
	int btnW = 28;	// 按扭宽
	int btnH = 22;	// 按扭高
	int btnBottom = 4;	// 按扭离底边的距离
	int btnLeftSpace = 3; // 每个按扭的左边间距

	m_btnAdd.SetWindowPos( NULL, left, rcClient.Height() - (btnH + btnBottom ), btnW, btnH, SWP_SHOWWINDOW );
	left += btnW + btnLeftSpace;

	m_btnAddByQRCode.SetWindowPos( NULL, left, rcClient.Height() - (btnH + btnBottom ), btnW, btnH, SWP_SHOWWINDOW );
	left += btnW + btnLeftSpace;

	m_btnAddByBatch.SetWindowPos( NULL, left, rcClient.Height() - (btnH + btnBottom ), btnW, btnH, SWP_SHOWWINDOW );
	left += btnW + btnLeftSpace;

	m_btnAddByLink.SetWindowPos( NULL, left, rcClient.Height() - (btnH + btnBottom ), btnW, btnH, SWP_SHOWWINDOW );
	left += btnW + btnLeftSpace;

	m_btnActiveNode.SetWindowPos( NULL, left, rcClient.Height() - (btnH + btnBottom ), btnW, btnH, SWP_SHOWWINDOW );
	left += btnW + btnLeftSpace;

	m_btnEditNode.SetWindowPos( NULL, left, rcClient.Height() - (btnH + btnBottom ), btnW, btnH, SWP_SHOWWINDOW );
	left += btnW + btnLeftSpace;

	m_btnDelNode.SetWindowPos( NULL, left, rcClient.Height() - (btnH + btnBottom ), btnW, btnH, SWP_SHOWWINDOW );
	left += btnW + btnLeftSpace;

	m_btnCheck.SetWindowPos( NULL, left, rcClient.Height() - (btnH + btnBottom ), btnW, btnH, SWP_SHOWWINDOW );
	left += btnW + btnLeftSpace;

	//////////////////////////////////////////////////////////////////////////
	left = rcClient.Width() - 10 - btnW;

	m_btnExit.SetWindowPos( NULL , left , rcClient.Height() - (btnH + btnBottom ), btnW, btnH, SWP_SHOWWINDOW );
	left -= ( btnW + btnLeftSpace );

	m_btnAbout.SetWindowPos( NULL , left , rcClient.Height() - (btnH + btnBottom ), btnW, btnH, SWP_SHOWWINDOW );
	left -= ( btnW + btnLeftSpace );

	m_btnMainPage.SetWindowPos( NULL , left , rcClient.Height() - (btnH + btnBottom ), btnW, btnH, SWP_SHOWWINDOW );
	left -= ( btnW + btnLeftSpace );

	m_btnSetting.SetWindowPos( NULL , left , rcClient.Height() - (btnH + btnBottom ), btnW, btnH, SWP_SHOWWINDOW );
	left -= ( btnW + btnLeftSpace );

	if( CAPPConfig::IsValidConfig() )
	{
		m_btnRefresh.SetWindowPos( NULL , left , rcClient.Height() - (btnH + btnBottom ), btnW, btnH, SWP_SHOWWINDOW );
		left -= ( btnW + btnLeftSpace );
	}

	m_btnSetting.Invalidate( TRUE );
	m_btnMainPage.Invalidate( TRUE );
	m_btnAbout.Invalidate( TRUE );
	m_btnExit.Invalidate( TRUE );
}

BOOL CSSCapDlg::PreTranslateMessage(MSG* pMsg)
{
	if( pMsg->message == WM_MOUSEMOVE )
		tooltip.RelayEvent( pMsg );

	return CDialogEx::PreTranslateMessage(pMsg);
}

LRESULT CSSCapDlg::OnTrayNotification(WPARAM uID, LPARAM lEvent)
{
	if( uID== IDR_MAINFRAME ) 
	{
		if((UINT)lEvent==WM_RBUTTONDOWN)
		{
			CPoint aPos;
			GetCursorPos(&aPos);
			TrayIcon_TrackPopupMenu( aPos.x, aPos.y );
		}
		if((UINT)lEvent==WM_LBUTTONDBLCLK)
		{
			OnShowMainWnd();
		}
	}

	// let tray icon do default stuff
	//	return m_trayIcon.OnTrayNotification(uID, lEvent);
	return 0;
}

LRESULT CSSCapDlg::OnShowMainWnd( )
{
	SetShowMainWindow( TRUE );

	SendMessage( WM_SYSCOMMAND, SC_RESTORE, 0 );

	ShowWindow( SW_SHOW );

	//::SetActiveWindow( m_hWnd );
	::SetForegroundWindow( m_hWnd );
	::SetFocus( m_hWnd );
	//	::BringWindowToTop( m_hWnd );

	return 1;
}

void CSSCapDlg::OnDropFiles(HDROP hDropInfo)
{
	// TODO: Add your message handler code here and/or call default

	CDialogEx::OnDropFiles(hDropInfo);
}

BOOL CSSCapDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UINT uMsg=LOWORD(wParam);
	// 属于SS服务器节点消息
	if( uMsg >= ID_MENU_NODES_START && uMsg <= ID_MENU_NODES_END )
	{
		int nIndex = uMsg - ID_MENU_NODES_START;

		CSSConfigInfo *pCfg = GetConfigInfo();
		int nNodeSize = pCfg->GetNodeSize();
		CSSNodeInfo *pNode = pCfg->GetNodeByIndex( nIndex );

		if( nIndex >= 0 && nIndex < nNodeSize && pNode )
		{
			pCfg->ActiveSSNode( pNode->id );

			// save.
			SaveShadowsocksServer( );

			m_listSSNodes.Invalidate( TRUE );

			_UpdateTrayIconMessage();
		}
	}
	else 
	{
		switch( uMsg )
		{
		case ID_MENU_ENABLE_SYSPROXY:
			{
				CSSConfigInfo *pCfg = GetConfigInfo();

				EnableSysWideProxy( m_hWnd, pCfg->enable?FALSE:TRUE,pCfg->global,TRUE );

				if( !pCfg->enable )
				{
					// 在启动之前发现用户设置系统代理的话, 程序是会保存之前的系统代理设置在退出时恢复的.
					// 如果用户手动禁止了系统代理, 那么在退出时就不再恢复保存的系统代理了.
					bUseAutoConfigUrl = FALSE;
					bUseProxyServer = FALSE;
				}
			}
			break;
		case ID_MENU_SYSTEM_CONFIGURE:
			{
				OnBnClickedButtonSetting();
			}
			break;
		case ID_MENU_HELP_OFFICIAL_PAGE:
			{
				OnBnClickedButtonMainpage();
			}
			break;
		/*case ID_MENU_HELP_CONTACTME:
			{

			}
			break;*/
		case ID_MENU_HELP_FEEDBACK:
			{
				ShellExecute(NULL,_T("open"), CAPPConfig::GetFeedbackUrl().c_str() ,NULL,NULL,SW_SHOW);
			}
			break;
		case ID_MENU_EXIT:
			{
				OnBnClickedButtonExit();
			}
			break;
		case IDM_ABOUTBOX:
			{
				OnBnClickedButtonAbout();
			}
			break;
		case ID_MENU_ACTIVATE_SEELECT:
			{
				OnBnClickedButtonActive();
			}
			break;
		case ID_MENU_EDIT_SEELECT:
			{
				OnBnClickedButtonEdit();
			}
			break;
		case ID_MENU_DELETE_SEELECT:
			{
				OnBnClickedButtonDel();
			}
			break;
		case ID_MENU_DELETE_ALLNODES:
			{
				OnBnClickedButtonDelAll();
			}
			break;
		case ID_MENU_ADD_NEW:
			{
				OnBnClickedButtonAdd();
			}
			break;
		case ID_MENU_ADD_NEW_FROM_QRCODE:
			{
				OnBnClickedButtonAddQRCode();
			}
			break;
		case ID_MENU_ADD_NEW_FROM_LINK:
			{
				OnBnClickedButtonAddLink();
			}
			break;
		case ID_MENU_BATCH_ADDING:
			{
				OnBnClickedButtonAddBatch();
			}
			break;
		case ID_MENU_CHECK_SEELECT_TCP:
			{
				CheckSSNode( FALSE);
			}
			break;
		case ID_MENU_CHECK_ALL_NODES_TCP:
			{
				CheckAllSSNode( FALSE );
			}
			break;
		case ID_MENU_CHECK_SEELECT_UDP:
			{
				CheckSSNode( TRUE );
			}
			break;
		case ID_MENU_CHECK_PING:
			{
				PingSSNode();
			}
			break;
		case ID_MENU_DISABLE_SEELECT:
			{
				EnableSelectSSNode( false );
			}
			break;
		case ID_MENU_ENABLE_SEELECT:
			{
				EnableSelectSSNode( true );
			}
			break;
		case ID_MENU_UPDATE_SS_FROM_WEBSITE:
			{
				OnBnClickedButtonRefresh();
			}
			break;
		case ID_MENU_COPY_TO_QRCODE:
			{
				CopyToQRCode();
			}
			break;
		case ID_MENU_COPY_TO_JSON:
			{
				CopyToJosn();
			}
			break;
		case ID_MENU_COPY_TO_SSLINK:
			{
				CopyToSSlink();
			}
			break;
		case ID_MENU_COPY_TO_PLAIN_NODE_INFO:
			{
				CopyToPlainNodeInfo();
			}
			break;
		case ID_MENU_SYSPROXY_GLOBAL_MODE:
			{
				CSSConfigInfo *pCfg = GetConfigInfo();
				ChangeSysProxyMode( m_hWnd, TRUE );
			}
			break;
		case ID_MENU_SYSPROXY_PAC_MODE:
			{
				CSSConfigInfo *pCfg = GetConfigInfo();
				ChangeSysProxyMode( m_hWnd, FALSE );
			}
			break;
		case ID_MENU_SYSPROXY_EDIT_PACFILE:
			{
#if 0
				CPacEditorDialog PacEditor( this );
				PacEditor.DoModal();
#endif
			}
			break;
		case ID_MENU_SYSPROXY_UPDATE_PACFILE:
			{
#if 0
				UpdateOnlinePacFile();
#endif
			}
			break;
		case ID_MENU_SHOW_QRCODE:
			{
				ShowQRCode();
			}
			break;
		case ID_MENU_DISCONNECTALLCONNECTION:
			{
				OnDisconnectAllConnections();
			}
			break;
		case ID_MENU_CLEAR_ALL_TRAFFIC_DATA:
			{
				OnClearAllTrafficData();
			}
			break;
		}
	}

	return CDialogEx::OnCommand(wParam, lParam);
}

void CSSCapDlg::OnClearAllTrafficData()
{
	CSSConfigInfo *pCfg = GetConfigInfo();
	pCfg->ClearAllNodesTrafficData();

	SaveShadowsocksServer();
}

void CSSCapDlg::OnDisconnectAllConnections()
{
	int nSelected = m_listSSNodes.GetSelectionMark();
	if( nSelected == -1 ) return;

	CSSNodeInfo *pNode = ( CSSNodeInfo *) m_listSSNodes.GetItemData( nSelected );
	if( !pNode ) return;

	DisconnectAllConnection( pNode->id );
}

void CSSCapDlg::ShowQRCode()
{
	int nSelected = m_listSSNodes.GetSelectionMark();
	if( nSelected == -1 ) return;

	CSSNodeInfo *pNode = ( CSSNodeInfo *) m_listSSNodes.GetItemData( nSelected );
	if( !pNode ) return;

	string s = pNode->ToSSlink();
	if( s.empty() ) return;

	HBITMAP hBitmap = QRcode_encodeStrongToBitmap( s.c_str(), 0, QR_ECLEVEL_H, QR_MODE_8, 1 );
	if( hBitmap )
	{
		CShowQRCodeDialog showqrcode( hBitmap , this );
		showqrcode.DoModal();
	}
	return;
}

void CSSCapDlg::EnableSelectSSNode( bool bEnable )
{
	int nSelected = m_listSSNodes.GetSelectionMark();
	if( nSelected == -1 )
		return;

	CSSNodeInfo *pNode = (CSSNodeInfo *)m_listSSNodes.GetItemData( nSelected );
	if( !pNode ) return;

	//CSSConfigInfo *pCfg = GetConfigInfo();
	//pCfg->EnableNode( pNode->id, bEnable );
	pNode->enable = bEnable ;
	m_listSSNodes.Invalidate( TRUE );

	SaveShadowsocksServer();
}
void CSSCapDlg::OnNMDblclkListSsnode(NMHDR *pNMHDR, LRESULT *pResult)
{
	ASSERT( pNMHDR != NULL );
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	
	*pResult = 0;

	int nSelect = m_listSSNodes.GetSelectionMark();
	if( nSelect != -1 ){
		OnBnClickedButtonActive();
	}
}


void CSSCapDlg::OnNMRClickListSsnode(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	
	*pResult = 0;

	if ( pNMItemActivate->iItem == -1 )
	{
		// GetSelectionMark()可以返回点击空白时，之前被选择ITEM的索引
		// 这里就是为了解决这个问题, 如果点击空白时候,先将之前选中的置为未选. 否则右击空白调用GetSelectionMark仍然得到之前选中的项.
		int iMark = m_listSSNodes.GetSelectionMark();
		if( iMark != -1 ){
			//UINT nState = mProgramContainer.GetItemState( iMark, LVIS_SELECTED );
			//nState &= (~LVIS_SELECTED);
			m_listSSNodes.SetItemState(iMark,0,LVIS_SELECTED );
			m_listSSNodes.SetSelectionMark( -1 );
		}
	}

	CPoint pos;
	GetCursorPos( &pos );

	ListCtrl_TrackPopupMenu( pos.x, pos.y );
}


void CSSCapDlg::OnNMClickListSsnode(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if ( pNMItemActivate->iItem == -1 )
	{
		// GetSelectionMark()可以返回点击空白时，之前被选择ITEM的索引
		// 这里就是为了解决这个问题, 如果点击空白时候,先将之前选中的置为未选. 否则右击空白调用GetSelectionMark仍然得到之前选中的项.
		int iMark = m_listSSNodes.GetSelectionMark();
		if( iMark != -1 ){
			//UINT nState = mProgramContainer.GetItemState( iMark, LVIS_SELECTED );
			//nState &= (~LVIS_SELECTED);
			m_listSSNodes.SetItemState(iMark,0,LVIS_SELECTED );
			m_listSSNodes.SetSelectionMark( -1 );
		}
	}

	int nSelect = m_listSSNodes.GetSelectedCount();
	if( nSelect > 0 )
	{
		m_btnActiveNode.EnableWindow( TRUE );
		m_btnCheck.EnableWindow( TRUE );

		int nSelectMask = m_listSSNodes.GetSelectionMark();
		BOOL bIsOnlineNode = FALSE;
		CSSNodeInfo *pNode = ( CSSNodeInfo *) m_listSSNodes.GetItemData( nSelectMask );
		if( pNode && pNode->IsOnlineNode() )
			bIsOnlineNode = TRUE;

		if( ( CAPPConfig::IsValidConfig() && CAPPConfig::IsDisableAddNodes()) || bIsOnlineNode )
		{
			m_btnEditNode.EnableWindow( FALSE );
			m_btnDelNode.EnableWindow( FALSE );
		}
		else 
		{
			m_btnEditNode.EnableWindow( TRUE );
			m_btnDelNode.EnableWindow( TRUE );
		}

	}
	else 
	{
		m_btnActiveNode.EnableWindow( FALSE );
		m_btnEditNode.EnableWindow( FALSE );
		m_btnDelNode.EnableWindow( FALSE );
		m_btnCheck.EnableWindow( FALSE );

	}

	*pResult = 0;
}

void CSSCapDlg::OnBnClickedButtonAdd()
{
	CNewSSNodeDlg dlg( FALSE,this );
	dlg.DoModal();
	RefreshSSNodeListCountTip();
}

HBITMAP CSSCapDlg::QRcode_encodeStrongToBitmap(const char *string, int version, QRecLevel level, QRencodeMode hint, int casesensitive)
{
#define OUT_FILE_PIXEL_PRESCALER	8											// Prescaler (number of pixels in bmp file for each QRCode pixel, on each 
#define PIXEL_COLOR_R				0											// Color of bmp pixels
#define PIXEL_COLOR_G				0
#define PIXEL_COLOR_B				0xff

	QRcode* pQRC = QRcode_encodeString( string, version, level, hint, casesensitive );
	if( pQRC )
	{
		unsigned int	unWidth, x, y, l, n, unWidthAdjusted, unDataBytes;
		unsigned char* pSourceData;
		unsigned char* pDestData;
		unsigned char* pRGBData;
		HBITMAP hBitmap;

		unWidth = pQRC->width;
		unWidthAdjusted = unWidth * OUT_FILE_PIXEL_PRESCALER * 3;
		if (unWidthAdjusted % 4)
			unWidthAdjusted = (unWidthAdjusted / 4 + 1) * 4;
		unDataBytes = unWidthAdjusted * unWidth * OUT_FILE_PIXEL_PRESCALER;

		if (!(pRGBData = (unsigned char*)malloc(unDataBytes)))
		{
			QRcode_free(pQRC);
			return NULL;
		}
		memset(pRGBData, 0xff, unDataBytes);

		// Prepare bmp headers
		BITMAPFILEHEADER kFileHeader;
		kFileHeader.bfType = 0x4d42;  // "BM"
		kFileHeader.bfSize =	sizeof(BITMAPFILEHEADER) +
			sizeof(BITMAPINFOHEADER) +
			unDataBytes;
		kFileHeader.bfReserved1 = 0;
		kFileHeader.bfReserved2 = 0;
		kFileHeader.bfOffBits =	sizeof(BITMAPFILEHEADER) +
			sizeof(BITMAPINFOHEADER);

		//memset(&bitinfo, 0, sizeof(BITMAPINFO));

		BITMAPINFOHEADER kInfoHeader;
		kInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
		kInfoHeader.biWidth = unWidth * OUT_FILE_PIXEL_PRESCALER;
		kInfoHeader.biHeight = -((int)unWidth * OUT_FILE_PIXEL_PRESCALER);
		kInfoHeader.biPlanes = 1;
		kInfoHeader.biBitCount = 24;
		kInfoHeader.biCompression = BI_RGB;
		kInfoHeader.biSizeImage = 0;
		kInfoHeader.biXPelsPerMeter = 0;
		kInfoHeader.biYPelsPerMeter = 0;
		kInfoHeader.biClrUsed = 0;
		kInfoHeader.biClrImportant = 0;

		// Convert QrCode bits to bmp pixels
		pSourceData = pQRC->data;
		for(y = 0; y < unWidth; y++)
		{
			pDestData = pRGBData + unWidthAdjusted * y * OUT_FILE_PIXEL_PRESCALER;
			for(x = 0; x < unWidth; x++)
			{
				if (*pSourceData & 1)
				{
					for(l = 0; l < OUT_FILE_PIXEL_PRESCALER; l++)
					{
						for(n = 0; n < OUT_FILE_PIXEL_PRESCALER; n++)
						{
							*(pDestData +		n * 3 + unWidthAdjusted * l) =	PIXEL_COLOR_B;
							*(pDestData + 1 +	n * 3 + unWidthAdjusted * l) =	PIXEL_COLOR_G;
							*(pDestData + 2 +	n * 3 + unWidthAdjusted * l) =	PIXEL_COLOR_R;
						}
					}
				}
				pDestData += 3 * OUT_FILE_PIXEL_PRESCALER;
				pSourceData++;
			}
		}

		/*
		CString strFile;
		strFile.Format(_T("%s\\temp\\output_qrcode.bmp"),CRunConfig::GetAppWorkingDirectory() );
		HANDLE hFile = CreateFile(strFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);//创建位图文件		
		if( hFile )
		{
			DWORD dwWritten;
			WriteFile(hFile, (LPCVOID)&kFileHeader, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
			WriteFile(hFile, (LPCVOID)&kInfoHeader, sizeof(BITMAPINFOHEADER), &dwWritten, NULL);
			WriteFile(hFile, (LPCVOID)pRGBData, unDataBytes, &dwWritten, NULL);// 写入位图文件其余内容
			CloseHandle(hFile); 	
		}*/


		int BmpLen = unDataBytes + sizeof(BITMAPINFOHEADER);
		unsigned char *pBibData = new unsigned char [BmpLen];
		if( pBibData )
		{
			memcpy(pBibData, &kInfoHeader, sizeof(BITMAPINFOHEADER) ); 
			memcpy(pBibData + sizeof(BITMAPINFOHEADER), pRGBData, unDataBytes ); 

			hBitmap = DIBToDDB( pBibData );

			delete pBibData;
		}
			

		free(pRGBData);
		QRcode_free(pQRC);

		return hBitmap;
	}
	return NULL;
}

void CSSCapDlg::CopyToQRCode()
{
	int nSelected = m_listSSNodes.GetSelectionMark();
	if( nSelected == -1 ) return;

	CSSNodeInfo *pNode = ( CSSNodeInfo *) m_listSSNodes.GetItemData( nSelected );
	if( !pNode ) return;

	string s = pNode->ToSSlink();
	if( s.empty() ) return;

	HBITMAP hBitmap = QRcode_encodeStrongToBitmap( s.c_str(), 0, QR_ECLEVEL_H, QR_MODE_8, 1 );
	if( hBitmap )
	{

		if (OpenClipboard()) 
		{
			//清空剪贴板
			EmptyClipboard();
			//把屏幕内容粘贴到剪贴板上,
			//hBitmap 为刚才的屏幕位图句柄
			SetClipboardData(CF_BITMAP, hBitmap);
			//关闭剪贴板
			CloseClipboard();
		}
	}
}

/**
{
"configs" :
[
{
"server" : "192.168.1.100",
"server_port" : 52027,
"password" : "123456",
"method" : "aes-256-cfb",
"remarks" : "HK",
"enable" : true
},
{
"server" : "192.168.1.101",
"server_port" : 52027,
"password" : "123456",
"method" : "aes-256-cfb",
"remarks" : "HK1",
"enable" : true
}
]}*/
void CSSCapDlg::CopyToJosn()
{
	int nSelected = m_listSSNodes.GetSelectionMark();
	if( nSelected == -1 ) return;

	CSSNodeInfo *pNode = ( CSSNodeInfo *) m_listSSNodes.GetItemData( nSelected );
	if( !pNode ) return;

	string s = pNode->ToJson();

	if( !s.empty() )
		PutTextToClipboardA( s.c_str(), m_hWnd );
}
/*
* 一个SS LINK格式是:
* ss://YWVzLTI1Ni1jZmI6YXNkMTQ3QGxhbGFhaS1tb3V5dWVtb3VyaS5teWFsYXVkYS5jbjoxMDcwMA==
* 由BASE64编码, 解码后是:
* ss://aes-256-cfb:asd147@lalaai-mouyuemouri.myalauda.cn:10700
* 格式为: ss://加密方式:密码@服务器:端口
*/
void CSSCapDlg::CopyToSSlink()
{
	int nSelected = m_listSSNodes.GetSelectionMark();
	if( nSelected == -1 ) return;

	CSSNodeInfo *pNode = ( CSSNodeInfo *) m_listSSNodes.GetItemData( nSelected );
	if( !pNode ) return;


	string s = pNode->ToSSlink();
	if( !s.empty() )
		PutTextToClipboardA( s.c_str(), m_hWnd );
}

void CSSCapDlg::CopyToPlainNodeInfo()
{
	int nSelected = m_listSSNodes.GetSelectionMark();
	if( nSelected == -1 ) return;

	CSSNodeInfo *pNode = ( CSSNodeInfo *) m_listSSNodes.GetItemData( nSelected );
	if( !pNode ) return;

	string s = pNode->ToPlainNodeInfo();
	if( !s.empty() )
		PutTextToClipboardA( s.c_str(), m_hWnd );
}

void CSSCapDlg::OnBnClickedButtonAddQRCode()
{
	CSreenCaptureDlg capture( CRunConfig::GetAppWorkingDirectory() , this );
	if( capture.DoModal() != IDOK )
		return;

	CString strQRCodeFile = CString ( CRunConfig::GetAppWorkingDirectory() ) +CString(_T("\\temp\\")) + CString(TEMP_QRCODE_FILE);
	//CString strQRCodeFile = CString ( CRunConfig::GetAppWorkingDirectory() ) +CString(_T("\\temp\\tempimage.bmp"));

	if( !IsFileExist( strQRCodeFile.GetBuffer() ) )
		return;

	char *pAnsiFile = lm_u2a_s( strQRCodeFile.GetBuffer());

	string qrcode_result;

	// zxing
	if( ZXingParseQRInfo( string( pAnsiFile), qrcode_result ) && !qrcode_result.empty() )
	// ZBar
	//if( ZBarParseQRCode( string( pAnsiFile), qrcode_result ) && !qrcode_result.empty() )
	{
		CSSConfigInfo *pCfg = GetConfigInfo();
		CSSNodeInfo *pNode = pCfg->AddNodeFromLink( qrcode_result );
		if( pNode ) 
		{
			UILC_AddItem( &m_listSSNodes,pNode );
			SaveShadowsocksServer();
			RefreshSSNodeListCountTip();
		}
	}
}

void CSSCapDlg::OnBnClickedButtonEdit()
{
	int nSelected = m_listSSNodes.GetSelectionMark();
	if( nSelected == -1 )
		return ;

	CSSConfigInfo *pCfg = GetConfigInfo();
	CSSNodeInfo *pNode = ( CSSNodeInfo *) m_listSSNodes.GetItemData( nSelected );
	if( !pNode ) return;

	if( pNode->IsOnlineNode() ) return;

	CNewSSNodeDlg dlg( TRUE , this );

	dlg.SetEditIndex( pNode->id );

	dlg.m_strServer = lm_u82u16_s( pNode->server.c_str() );
	//dlg.m_strPort.Format( _T("%d"), pNode->server_port );
	dlg.m_nPort = pNode->server_port;
	dlg.m_strPassword = lm_u82u16_s( pNode->password.c_str() );
	dlg.m_strMem = lm_u82u16_s( pNode->remarks.c_str() );
	//dlg.strEnc = lm_u82u16_s( pNode->method.c_str() );
	dlg.m_bEnable = pNode->enable?1:0;

	int nCryptionIdx = CEncryptionMgr::GetCryptionIndexByName( pNode->method );
	dlg.m_nCryptionIdx = nCryptionIdx == -1 ? 0 : nCryptionIdx;

	dlg.DoModal();
}

void CSSCapDlg::OnBnClickedButtonDelAll()
{
	if( ::MessageBox( m_hWnd, lm_u82u16_s( _("Are you sure to delete all shadowsocks server nodes?")), CAPPConfig::GetSoftName().c_str(),MB_YESNO) == IDYES )
	{
		m_listSSNodes.DeleteAllItems();

		CSSConfigInfo *pCfg = GetConfigInfo();
		pCfg->DeleteAllNodes( 0 );

		SaveShadowsocksServer();

		RefreshSSNodeListCountTip();
	}
}

void CSSCapDlg::OnBnClickedButtonDel()
{
	int nCount = m_listSSNodes.GetItemCount();

	int nSelected = m_listSSNodes.GetSelectionMark();
	if( nSelected == -1 )
		return ;

	CSSNodeInfo *pNode = ( CSSNodeInfo *) m_listSSNodes.GetItemData( nSelected );
	if( !pNode ) return;

	if( pNode->IsOnlineNode() ) return;

	if( ::MessageBox( m_hWnd, lm_u82u16_s( _("Are you sure to delete this shadowsocks server?")), CAPPConfig::GetSoftName().c_str(),MB_YESNO) == IDYES )
	{
		CSSConfigInfo *pCfg = GetConfigInfo();
		if( pCfg->DeleteNode( pNode->id ) )
		{
			UILC_DeleteItem( &m_listSSNodes, nSelected );

			// 如果删除的是激活的节点, 需要重新分配一个激活节点
			if( pCfg->idInUse == pNode->id )
			{
				if( nCount > 1 )
				{
					int nNextSel = nSelected - 1;
					if( nNextSel < 0 )
						nNextSel = 0;

					CSSNodeInfo *pNodeNext = (CSSNodeInfo  *)m_listSSNodes.GetItemData( nNextSel );
					if( pNodeNext )
					{
						pCfg->ActiveSSNode( pNodeNext->id );
						m_listSSNodes.Invalidate( TRUE );

					}
				}
			}

			SaveShadowsocksServer();

			RefreshSSNodeListCountTip();
		}
	}
}

void CSSCapDlg::OnBnClickedButtonAddBatch()
{
	CAddSSFromJsonDlg json;
	if( json.DoModal() == IDOK )
	{
		RefreshSSNodeListCountTip();
	}
}


void CSSCapDlg::OnBnClickedButtonAddLink()
{
	CAddSSFromLink link;
	if( link.DoModal() == IDOK )
	{
		if( link.m_nAddedNodes > 0 )
			SaveShadowsocksServer();

		RefreshSSNodeListCountTip();
	}
}


void CSSCapDlg::OnBnClickedButtonActive()
{
	int nSelected = m_listSSNodes.GetSelectionMark();
	if( nSelected == -1 )
		return;

	CSSConfigInfo *pCfg = GetConfigInfo();
	CSSNodeInfo *pNode = ( CSSNodeInfo *)m_listSSNodes.GetItemData( nSelected );

	if( !pCfg || !pNode )return;

	if( !pNode->enable )
	{
		char szMsg[1000] = {0};
		sprintf_s( szMsg, 1000, _("You can't active a disabled node."));

		::MessageBox( m_hWnd, lm_u82u16_s( szMsg ),CAPPConfig::GetSoftName().c_str(), MB_OK | MB_ICONERROR );

		return;
	}

	CSSNodeInfo *pOldNode = pCfg->GetActiveNodeInfo();

	// 旧节点存在 , 未启用服务器均衡, 切换节点时自动断开连接
	if( pOldNode && !pCfg->random  && pCfg->auto_disconnect_connection )
	{
		DisconnectAllConnection( pOldNode->id );
	}

	pCfg->ActiveSSNode( pNode->id );

	SaveShadowsocksServer();

	m_listSSNodes.Invalidate( TRUE );

	_UpdateTrayIconMessage();
}
/** @brief 测试所有SS节点
*
* @param bIsUdp 是否测试UDP
*/
void CSSCapDlg::CheckAllSSNode( BOOL bIsUdp )
{
	int nCount = m_listSSNodes.GetItemCount() ;
	if( nCount <= 0 ) return;

	CRect rcClient;
	GetWindowRect( &rcClient );
	int x = rcClient.right;
	int y = rcClient.top;
	int Height = rcClient.Height();
	int Width = 0;

	if( pTestingCurrentProxyDlg )
	{
		// 必须先清除测试列表.否则会将之前测试过的再测一次. 
		pTestingCurrentProxyDlg->ClearSSTestingNodes();

		for( int i = 0 ; i< nCount ; i ++ )
		{
			CSSNodeInfo *pNode = (CSSNodeInfo *)m_listSSNodes.GetItemData( i );
			if( pNode )
				pTestingCurrentProxyDlg->PushSSNodeInfo( pNode );
		}

		CRect rcTestWndSize;
		pTestingCurrentProxyDlg->GetWindowRect( &rcTestWndSize );
		Width = rcTestWndSize.Width();

		pTestingCurrentProxyDlg->SetWindowPos( NULL, x, y, Width,Height, SWP_SHOWWINDOW );
		pTestingCurrentProxyDlg->ShowWindow( SW_SHOW );
	}
	else 
	{
		pTestingCurrentProxyDlg = new CTestingCurrentNodeDlg( this );
		pTestingCurrentProxyDlg->Create(IDD_DIALOG_TESTING_CURRENT_SERVER,this );
		//pTestingCurrentProxyDlg->SetOwner( AfxGetMainWnd() );

		for( int i = 0 ; i< nCount ; i ++ )
		{
			CSSNodeInfo *pNode = (CSSNodeInfo *)m_listSSNodes.GetItemData( i );
			if( pNode )
				pTestingCurrentProxyDlg->PushSSNodeInfo( pNode );
		}

		CRect rcTestWndSize;
		pTestingCurrentProxyDlg->GetWindowRect( &rcTestWndSize );
		Width = rcTestWndSize.Width();
		pTestingCurrentProxyDlg->SetWindowPos( NULL, x, y, Width,Height, SWP_SHOWWINDOW );
		pTestingCurrentProxyDlg->ShowWindow( SW_SHOW );
	}

	// 开始测试当前代理.
	// 如果用户连续点击, 之前的测试还在进行当中, 会自动忽略.
	pTestingCurrentProxyDlg->StartTestCurrentProxy( bIsUdp );
}
/** @brief 测试SS节点
*
* @param bIsUdp 是否测试UDP
*/
void CSSCapDlg::CheckSSNode( BOOL bIsUdp )
{
	int nSelected = m_listSSNodes.GetSelectionMark();
	if( nSelected == -1 )
		return ;

	CSSNodeInfo *pNode = (CSSNodeInfo *)m_listSSNodes.GetItemData( nSelected );
	if( pNode == NULL )
		return ;

	CRect rcClient;
	GetWindowRect( &rcClient );
	int x = rcClient.right;
	int y = rcClient.top;
	int Height = rcClient.Height();
	int Width = 0;

	if( pTestingCurrentProxyDlg )
	{
		// 必须先清除测试列表.否则会将之前测试过的再测一次. 
		pTestingCurrentProxyDlg->ClearSSTestingNodes();

		pTestingCurrentProxyDlg->PushSSNodeInfo( pNode );

		CRect rcTestWndSize;
		pTestingCurrentProxyDlg->GetWindowRect( &rcTestWndSize );
		Width = rcTestWndSize.Width();
		pTestingCurrentProxyDlg->SetWindowPos( NULL, x, y, Width,Height, SWP_SHOWWINDOW);
		pTestingCurrentProxyDlg->ShowWindow( SW_SHOW );
	}
	else 
	{
		pTestingCurrentProxyDlg = new CTestingCurrentNodeDlg( this );
		pTestingCurrentProxyDlg->Create(IDD_DIALOG_TESTING_CURRENT_SERVER,this );
		//pTestingCurrentProxyDlg->SetOwner( AfxGetMainWnd() );
		pTestingCurrentProxyDlg->PushSSNodeInfo( pNode );

		CRect rcTestWndSize;
		pTestingCurrentProxyDlg->GetWindowRect( &rcTestWndSize );
		Width = rcTestWndSize.Width();
		pTestingCurrentProxyDlg->SetWindowPos( NULL, x, y, Width,Height, SWP_SHOWWINDOW );
		pTestingCurrentProxyDlg->ShowWindow( SW_SHOW );
	}

	// 开始测试当前代理.
	// 如果用户连续点击, 之前的测试还在进行当中, 会自动忽略.
	pTestingCurrentProxyDlg->StartTestCurrentProxy( bIsUdp );
}

void CSSCapDlg::PingSSNode( )
{
	int nSelected = m_listSSNodes.GetSelectionMark();
	if( nSelected == -1 )
		return ;

	CSSNodeInfo *pNode = (CSSNodeInfo *)m_listSSNodes.GetItemData( nSelected );
	if( pNode == NULL )
		return ;

	CString strPing;
	strPing.Format( _T("%S"), pNode->server.c_str() );

	ShellExecute(NULL,_T("open"), _T("ping.exe"),strPing.GetBuffer(),NULL,SW_SHOW);
}

void CSSCapDlg::OnBnClickedButtonCheck()
{
	CheckSSNode( FALSE );
}
void CSSCapDlg::OnBnClickedButtonExit()
{
	EndGetIPLocation();

	if( pWebLoaderTread && pWebLoaderTread->m_hThread )
	{
		TerminateThread( pWebLoaderTread->m_hThread , 0 );
		CloseHandle( pWebLoaderTread->m_hThread );
	}

	CSSConfigInfo *pCfg = GetConfigInfo();

#ifndef USE_LIBPRIVOXY
	// 结束privoxy进程.
	EndPrivoxyProcess();
#endif

	if( pCfg->enable )
	{
		// 禁止系统代理.
		DisableSystemProxy();
	}

	// 保存参数
	SaveShadowsocksServer();

	//////////////////////////////////////////////////////////////////////////
	// 恢复启动之前IE的代理设置
	if( bUseAutoConfigUrl || bUseProxyServer )
	{
		SetSystemProxy( bUseProxyServer?lpProxyServer:NULL,bUseAutoConfigUrl?lpAutoConfigUrl:NULL,lpByPass );
	}

	//////////////////////////////////////////////////////////////////////////
	AfxGetMainWnd()->PostMessage(WM_QUIT,0,0);
}


void CSSCapDlg::OnBnClickedButtonSetting()
{
	CSystemConfigureDlg setting;
	setting.DoModal();
}


void CSSCapDlg::OnBnClickedButtonMainpage()
{
	ShellExecute(NULL,_T("open"), CAPPConfig::GetWebsite().c_str() ,NULL,NULL,SW_SHOW);
}


void CSSCapDlg::OnBnClickedButtonAbout()
{
	CAboutDlg dlgAbout;
	dlgAbout.DoModal();
}


void CSSCapDlg::OnBnClickedButtonRefresh()
{
}

LRESULT CSSCapDlg::OnFoundNewVersion(WPARAM wParam ,LPARAM lParam)
{
	CDownloadSelectorDialog download_selector(NewVersion_GetVersionNo(),NewVersion_GetVersionChange(),this );

	int RetMsg = download_selector.DoModal();

	return 1;
}
// 获取ssManager对象
CSSManager *CSSCapDlg::GetSSManagerObj()
{
	return &ssManager;
}
/** @brief 本地socks 5服务是否已经启动
*/
BOOL CSSCapDlg::IsLocalSocks5ServiceStarted( )
{
	return ssManager.IsListenerStarted();
}

void CSSCapDlg::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
	if( !IsShowMainWindow() )
		lpwndpos->flags &= ~SWP_SHOWWINDOW;

	CDialogEx::OnWindowPosChanging(lpwndpos);
}

/** @brief 更新线上PAC文件
*/
BOOL CSSCapDlg::UpdateOnlinePacFile()
{
	GenericHTTPClient client;
	BOOL bRet = FALSE;

	if( client.Request( ONLINE_PAC_URL,GenericHTTPClient::RequestGetMethod ) )
	{
		if( !client.Is200() )
		{
			goto End;
		}
		string strContent = string( client.QueryHTTPResponse() );
		if( strContent.empty() )
			goto End;

		string pBodyDecode = base64::decode( strContent.c_str() );
		if( pBodyDecode.empty() )
			goto End;

		string strDecodeContent = string( pBodyDecode );
		//free( pBodyDecode );

		FILE *f=NULL;
		CSSConfigInfo *pCfg = GetConfigInfo();
		int length = strDecodeContent.length();

		f = _tfsopen( pCfg->localPacFileFullName.c_str(), _T("wb"), _SH_DENYNO );
		if( f )
		{
			fwrite( strDecodeContent.c_str(), 1, length, f );

			fflush( f );
			fclose( f );

			LoadPacFile( TRUE );
			bRet = TRUE;
		}
	}

End:

	if( bRet )
		::MessageBox( m_hWnd, lm_u82u16_s(_("PAC file is updated successfully.")), CAPPConfig::GetSoftName().c_str(), MB_OK );
	else 
		::MessageBox( m_hWnd, lm_u82u16_s(_("PAC file is updated unsuccessfully.")), CAPPConfig::GetSoftName().c_str(), MB_OK );
	return TRUE;
}

void CSSCapDlg::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CDialogEx::OnWindowPosChanged(lpwndpos);

	ChangeControlSize();
}


void CSSCapDlg::OnHotKey(UINT nHotKeyId, UINT nKey1, UINT nKey2)
{
	if( nHotKeyId == WM_HOTKEY_FOR_ADDFROMQRCODE )
	{
		TRACE(_T("OnHotKey\r\n"));
		OnBnClickedButtonAddQRCode();
	}

	CDialogEx::OnHotKey(nHotKeyId, nKey1, nKey2);
}
