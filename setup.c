/*-------
 * Module:			setup.c
 *
 * Description:		This module contains the setup functions for
 *					adding/modifying a Data Source in the ODBC.INI portion
 *					of the registry.
 *
 * Classes:			n/a
 *
 * API functions:	ConfigDSN, ConfigDriver
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *-------
 */

#include  "psqlodbc.h"
#include  "pgenlist.h"
#include  "loadlib.h"
#include  "misc.h" // strncpy_null

//#include  "environ.h"
#include  <windowsx.h>
#include  <string.h>
#include  <stdlib.h>
#include  "resource.h"
#include  "pgapifunc.h"
#include  "dlg_specific.h"
#include  "win_setup.h"


#define INTFUNC  __stdcall

extern HINSTANCE s_hModule;	/* Saved module handle. */

/* Constants */
#define MIN(x,y)	  ((x) < (y) ? (x) : (y))

#define MAXKEYLEN		(32+1)	/* Max keyword length */
#define MAXDESC			(255+1) /* Max description length */
#define MAXDSNAME		(32+1)	/* Max data source name length */

static void ParseAttributes(LPCSTR lpszAttributes, LPSETUPDLG lpsetupdlg);
static BOOL SetDSNAttributes(HWND hwndParent, LPSETUPDLG lpsetupdlg, DWORD *errcode);
static BOOL SetDriverAttributes(LPCSTR lpszDriver, DWORD *pErrorCode, LPSTR pErrorMessage, WORD cbMessage);
static void CenterDialog(HWND hdlg);

/*--------
 *	ConfigDSN
 *
 *	Description:	ODBC Setup entry point
 *				This entry point is called by the ODBC Installer
 *				(see file header for more details)
 *	Input	 :	hwnd ----------- Parent window handle
 *				fRequest ------- Request type (i.e., add, config, or remove)
 *				lpszDriver ----- Driver name
 *				lpszAttributes - data source attribute string
 *	Output	 :	TRUE success, FALSE otherwise
 *--------
 */
BOOL		CALLBACK
ConfigDSN(HWND hwnd,
		  WORD fRequest,
		  LPCSTR lpszDriver,
		  LPCSTR lpszAttributes)
{
	BOOL		fSuccess;		/* Success/fail flag */
	GLOBALHANDLE hglbAttr;
	LPSETUPDLG	lpsetupdlg;


	/* Allocate attribute array */
	hglbAttr = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(SETUPDLG));
	if (!hglbAttr)
		return FALSE;
	lpsetupdlg = (LPSETUPDLG) GlobalLock(hglbAttr);

	/* First of all, parse attribute string only for DSN entry */
	CC_conninfo_init(&(lpsetupdlg->ci), INIT_GLOBALS);
	if (lpszAttributes)
		ParseAttributes(lpszAttributes, lpsetupdlg);

	/* Save original data source name */
	if (lpsetupdlg->ci.dsn[0])
		STRCPY_FIXED(lpsetupdlg->szDSN, lpsetupdlg->ci.dsn);
	else
		lpsetupdlg->szDSN[0] = '\0';

	/* Remove data source */
	if (ODBC_REMOVE_DSN == fRequest)
	{
		/* Fail if no data source name was supplied */
		if (!lpsetupdlg->ci.dsn[0])
			fSuccess = FALSE;

		/* Otherwise remove data source from ODBC.INI */
		else
			fSuccess = SQLRemoveDSNFromIni(lpsetupdlg->ci.dsn);
	}
	/* Add or Configure data source */
	else
	{
		/* Save passed variables for global access (e.g., dialog access) */
		lpsetupdlg->hwndParent = hwnd;
		lpsetupdlg->lpszDrvr = lpszDriver;
		lpsetupdlg->fNewDSN = (ODBC_ADD_DSN == fRequest);
		lpsetupdlg->fDefault = !lstrcmpi(lpsetupdlg->ci.dsn, INI_DSN);

		/* Cleanup conninfo and restore data source name */
		CC_conninfo_init(&(lpsetupdlg->ci), CLEANUP_FOR_REUSE | INIT_GLOBALS);
		STRCPY_FIXED(lpsetupdlg->ci.dsn, lpsetupdlg->szDSN);
		/* Get common attributes of Data Source */
		getDSNinfo(&(lpsetupdlg->ci), lpsetupdlg->lpszDrvr);
		/*
		 * Parse attribute string again
		 *
		 * NOTE: Values supplied in the attribute string will always
		 *	 override settings in ODBC.INI
		 */
		if (lpszAttributes)
			ParseAttributes(lpszAttributes, lpsetupdlg);

		/*
		 * Display the appropriate dialog (if parent window handle
		 * supplied)
		 */
		if (hwnd)
		{
			/* Display dialog(s) */
			fSuccess = (IDOK == DialogBoxParam(s_hModule,
				MAKEINTRESOURCE(DLG_CONFIG),
				hwnd,
				ConfigDlgProc,
				(LPARAM) lpsetupdlg));
		}
		else if (lpsetupdlg->ci.dsn[0])
		{
			MYLOG(MIN_LOG_LEVEL, "SetDSNAttributes\n");
			fSuccess = SetDSNAttributes(hwnd, lpsetupdlg, NULL);
		}
		else
			fSuccess = FALSE;
	}

	CC_conninfo_release(&(lpsetupdlg->ci));
	GlobalUnlock(hglbAttr);
	GlobalFree(hglbAttr);

	return fSuccess;
}

/*--------
 *	ConfigDriver
 *
 *	Description:	ODBC Setup entry point
 *			This entry point is called by the ODBC Installer
 *			(see file header for more details)
 *	Arguments :	hwnd ----------- Parent window handle
 *			fRequest ------- Request type (i.e., add, config, or remove)
 *			lpszDriver ----- Driver name
 *			lpszArgs ------- A null-terminated string containing
				arguments for a driver specific fRequest
 *			lpszMsg -------- A null-terimated string containing
				an output message from the driver setup
 *			cnMsgMax ------- Length of lpszMSg
 *			pcbMsgOut ------ Total number of bytes available to
				return in lpszMsg
 *	Returns :	TRUE success, FALSE otherwise
 *--------
 */
BOOL		CALLBACK
ConfigDriver(HWND hwnd,
		WORD fRequest,
		LPCSTR lpszDriver,
		LPCSTR lpszArgs,
		LPSTR lpszMsg,
		WORD cbMsgMax,
		WORD *pcbMsgOut)
{
	DWORD	errorCode = 0;
	BOOL	fSuccess = TRUE;	/* Success/fail flag */

	if (cbMsgMax > 0 && NULL != lpszMsg)
		*lpszMsg = '\0';
	if (NULL != pcbMsgOut)
		*pcbMsgOut = 0;

	/* Add the driver */
	switch (fRequest)
	{
		case ODBC_INSTALL_DRIVER:
			fSuccess = SetDriverAttributes(lpszDriver, &errorCode, lpszMsg, cbMsgMax);
			if (cbMsgMax > 0 && NULL != lpszMsg)
				*pcbMsgOut = (WORD) strlen(lpszMsg);
			break;
		case ODBC_REMOVE_DRIVER:
			break;
		default:
			errorCode = ODBC_ERROR_INVALID_REQUEST_TYPE;
			fSuccess = FALSE;
	}

	if (!fSuccess)
		SQLPostInstallerError(errorCode, lpszMsg);
	return fSuccess;
}


/*-------
 * CenterDialog
 *
 *		Description:  Center the dialog over the frame window
 *		Input	   :  hdlg -- Dialog window handle
 *		Output	   :  None
 *-------
 */
static void
CenterDialog(HWND hdlg)
{
	HWND		hwndFrame;
	RECT		rcDlg,
				rcScr,
				rcFrame;
	int			cx,
				cy;

	hwndFrame = GetParent(hdlg);

	GetWindowRect(hdlg, &rcDlg);
	cx = rcDlg.right - rcDlg.left;
	cy = rcDlg.bottom - rcDlg.top;

	GetClientRect(hwndFrame, &rcFrame);
	ClientToScreen(hwndFrame, (LPPOINT) (&rcFrame.left));
	ClientToScreen(hwndFrame, (LPPOINT) (&rcFrame.right));
	rcDlg.top = rcFrame.top + (((rcFrame.bottom - rcFrame.top) - cy) >> 1);
	rcDlg.left = rcFrame.left + (((rcFrame.right - rcFrame.left) - cx) >> 1);
	rcDlg.bottom = rcDlg.top + cy;
	rcDlg.right = rcDlg.left + cx;

	GetWindowRect(GetDesktopWindow(), &rcScr);
	if (rcDlg.bottom > rcScr.bottom)
	{
		rcDlg.bottom = rcScr.bottom;
		rcDlg.top = rcDlg.bottom - cy;
	}
	if (rcDlg.right > rcScr.right)
	{
		rcDlg.right = rcScr.right;
		rcDlg.left = rcDlg.right - cx;
	}

	if (rcDlg.left < 0)
		rcDlg.left = 0;
	if (rcDlg.top < 0)
		rcDlg.top = 0;

	MoveWindow(hdlg, rcDlg.left, rcDlg.top, cx, cy, TRUE);
	return;
}

/*-------
 * ConfigDlgProc
 *	Description:	Manage add data source name dialog
 *	Input	 :	hdlg --- Dialog window handle
 *				wMsg --- Message
 *				wParam - Message parameter
 *				lParam - Message parameter
 *	Output	 :	TRUE if message processed, FALSE otherwise
 *-------
 */
LRESULT			CALLBACK
ConfigDlgProc(HWND hdlg,
			  UINT wMsg,
			  WPARAM wParam,
			  LPARAM lParam)
{
	LPSETUPDLG	lpsetupdlg;
	ConnInfo   *ci;
	DWORD		cmd;
	char		strbuf[64];

	switch (wMsg)
	{
			/* Initialize the dialog */
		case WM_INITDIALOG:
			lpsetupdlg = (LPSETUPDLG) lParam;
			ci = &lpsetupdlg->ci;

			/* Hide the driver connect message */
			ShowWindow(GetDlgItem(hdlg, DRV_MSG_LABEL), SW_HIDE);
			LoadString(s_hModule, IDS_ADVANCE_SAVE, strbuf, sizeof(strbuf));
			SetWindowText(GetDlgItem(hdlg, IDOK), strbuf);

			SetWindowLongPtr(hdlg, DWLP_USER, lParam);
			CenterDialog(hdlg); /* Center dialog */

			/* Save drivername */
			if (!(lpsetupdlg->ci.drivername[0]))
				STRCPY_FIXED(lpsetupdlg->ci.drivername, lpsetupdlg->lpszDrvr);

			/* Initialize dialog fields */
			SetDlgStuff(hdlg, ci);

			if (lpsetupdlg->fNewDSN || !ci->dsn[0])
				ShowWindow(GetDlgItem(hdlg, IDC_MANAGEDSN), SW_HIDE);
			if (lpsetupdlg->fDefault)
			{
				EnableWindow(GetDlgItem(hdlg, IDC_DSNAME), FALSE);
				EnableWindow(GetDlgItem(hdlg, IDC_DSNAMETEXT), FALSE);
			}
			else
				SendDlgItemMessage(hdlg, IDC_DSNAME,
							 EM_LIMITTEXT, (WPARAM) (MAXDSNAME - 1), 0L);

			SendDlgItemMessage(hdlg, IDC_DESC,
							   EM_LIMITTEXT, (WPARAM) (MAXDESC - 1), 0L);
			return TRUE;		/* Focus was not set */

			/* Process buttons */
		case WM_COMMAND:
			lpsetupdlg = (LPSETUPDLG) GetWindowLongPtr(hdlg, DWLP_USER);
			switch (cmd = GET_WM_COMMAND_ID(wParam, lParam))
			{
					/*
					 * Ensure the OK button is enabled only when a data
					 * source name
					 */
					/* is entered */
				case IDC_DSNAME:
					if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
					{
						char		szItem[MAXDSNAME];	/* Edit control text */

						/* Enable/disable the OK button */
						EnableWindow(GetDlgItem(hdlg, IDOK),
									 GetDlgItemText(hdlg, IDC_DSNAME,
												szItem, sizeof(szItem)));
						return TRUE;
					}
					break;

					/* Accept results */
				case IDOK:
				case IDAPPLY:
					/* Retrieve dialog values */
					if (!lpsetupdlg->fDefault)
						GetDlgItemText(hdlg, IDC_DSNAME,
									   lpsetupdlg->ci.dsn,
									   sizeof(lpsetupdlg->ci.dsn));

					/* Get Dialog Values */
					GetDlgStuff(hdlg, &lpsetupdlg->ci);
					/* Update ODBC.INI */
					SetDSNAttributes(hdlg, lpsetupdlg, NULL);
					if (IDAPPLY == cmd)
						break;
					/* Return to caller */
				case IDCANCEL:
					EndDialog(hdlg, wParam);
					return TRUE;

				case IDC_TEST:
				{
					if (NULL != lpsetupdlg)
					{
						/* Get Dialog Values */
						GetDlgStuff(hdlg, &lpsetupdlg->ci);
						test_connection(lpsetupdlg->hwndParent, &lpsetupdlg->ci, FALSE);
						return TRUE;
					}
					break;
				}
				case IDC_DATASOURCE:
					/* Get Dialog Values */
					GetDlgStuff(hdlg, &lpsetupdlg->ci);
					DialogBoxParam(s_hModule, MAKEINTRESOURCE(DLG_OPTIONS_DRV),
					 hdlg, ds_options1Proc, (LPARAM) &lpsetupdlg->ci);
					return TRUE;

				case IDC_DRIVER:
					/* Get Dialog Values */
					GetDlgStuff(hdlg, &lpsetupdlg->ci);
					DialogBoxParam(s_hModule, MAKEINTRESOURCE(DLG_OPTIONS_GLOBAL),
						 hdlg, global_optionsProc, (LPARAM) &lpsetupdlg->ci);

					return TRUE;
				case IDC_MANAGEDSN:
					if (DialogBoxParam(s_hModule, MAKEINTRESOURCE(DLG_DRIVER_CHANGE),
						hdlg, manage_dsnProc,
						(LPARAM) lpsetupdlg) > 0)
						EndDialog(hdlg, 0);

					return TRUE;

				case IDC_LIMITLESS_SETTINGS:
					/* Get Dialog Values */
					GetDlgStuff(hdlg, &lpsetupdlg->ci);
					DialogBoxParam(s_hModule, MAKEINTRESOURCE(DLG_OPTIONS_LIMITLESS),
						hdlg, limitless_optionsProc, (LPARAM) &lpsetupdlg->ci);
					return TRUE;
				// Failover - Action handler for Failover Settings Button
				case IDC_FAILOVER_SETTINGS:
				{
					/* Get Dialog Values */
					GetDlgStuff(hdlg, &lpsetupdlg->ci);
					DialogBoxParam(s_hModule, MAKEINTRESOURCE(DLG_OPTIONS_FAILOVER),
						hdlg, failover_optionsProc, (LPARAM) &lpsetupdlg->ci);
					return TRUE;
				}
			}
			break;
		case WM_CTLCOLORSTATIC:
			if (lParam == (LPARAM)GetDlgItem(hdlg, IDC_NOTICE_USER))
			{
				HBRUSH hBrush = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
				SetTextColor((HDC)wParam, RGB(255, 0, 0));
				return (LRESULT)hBrush;
			}
			break;
	}

	/* Message not processed */
	return FALSE;
}

#ifdef	USE_PROC_ADDRESS
#define	SQLALLOCHANDLEFUNC	sqlallochandle
#define	SQLSETENVATTRFUNC	sqlsetenvattr
#define	SQLDISCONNECTFUNC	sqldisconnect
#define	SQLFREEHANDLEFUNC	sqlfreehandle
#ifdef	UNICODE_SUPPORT
#define	SQLGETDIAGRECFUNC	sqlgetdiagrecw
#define	SQLDRIVERCONNECTFUNC	sqldriverconnectw
#define	SQLSETCONNECTATTRFUNC	sqlsetconnectattrw
#else
#define	SQLGETDIAGRECFUNC	sqlgetdiagrec
#define	SQLDRIVERCONNECTFUNC	sqldriverconnect
#define	SQLSETCONNECTATTRFUNC	sqlsetconnectAttr
#endif	/* UNICODE_SUPPORT */
#else
#define	SQLALLOCHANDLEFUNC	SQLAllocHandle
#define	SQLSETENVATTRFUNC	SQLSetEnvAttr
#define	SQLDISCONNECTFUNC	SQLDisconnect
#define	SQLFREEHANDLEFUNC	SQLFreeHandle
#ifdef	UNICODE_SUPPORT
#define	SQLGETDIAGRECFUNC	SQLGetDiagRecW
#define	SQLDRIVERCONNECTFUNC	SQLDriverConnectW
#define	SQLSETCONNECTATTRFUNC	SQLSetConnectAttrW
#else
#define	SQLGETDIAGRECFUNC	SQLGetDiagRec
#define	SQLDRIVERCONNECTFUNC	SQLDriverConnect
#define	SQLSETCONNECTATTRFUNC	SQLSetConnectAttr
#endif	/* UNICODE_SUPPORT */
#endif	/* USE_PROC_ADDRESS */

#define	MAX_CONNECT_STRING_LEN	2048
#ifdef	UNICODE_SUPPORT
#define	MESSAGEBOXFUNC		MessageBoxW
#define	_T(str)			L ## str
#define	SNTPRINTF		_snwprintf
#else
#define	MESSAGEBOXFUNC		MessageBoxA
#define	_T(str)			str
#define	SNTPRINTF		snprintf
#endif	/* UNICODE_SUPPORT */

// Case insensitive `strstr`
bool stristr(const char* str, const char* substr) {
    if (!str || !substr) {
        return false;
    }

	// Duplicate and modify to lower
    char* str_cp = strdup(str);
    char* sub_cp = strdup(substr);
    for (char* p = str_cp; *p; p++) *p = tolower(*p);
    for (char* p = sub_cp; *p; p++) *p = tolower(*p);

    bool is_in_str = strstr(str_cp, sub_cp) ? true : false;

    free(str_cp);
    free(sub_cp);

    return is_in_str;
}

void
test_connection(HANDLE hwnd, ConnInfo *ci, BOOL withDTC)
{
	int		errnum;
	char		out_conn[MAX_CONNECT_STRING_LEN];
	SQLRETURN	ret;
	SQLHENV		env = SQL_NULL_HANDLE;
	SQLHDBC		conn = SQL_NULL_HANDLE;
	SQLSMALLINT	str_len;
	char		dsn_1st;
	BOOL		connected = FALSE;
    UINT        connection_test_icon = MB_ICONINFORMATION;
#ifdef	UNICODE_SUPPORT
	SQLWCHAR	wout_conn[MAX_CONNECT_STRING_LEN];
	SQLWCHAR	szMsg[SQL_MAX_MESSAGE_LENGTH];
	const SQLWCHAR	*ermsg = NULL;
	SQLWCHAR	*conn_str;
#else
	SQLCHAR		szMsg[SQL_MAX_MESSAGE_LENGTH];
	const SQLCHAR	*ermsg = NULL;
	SQLCHAR		*conn_str;
#endif	/* UNICODE_SUPPORT */

	dsn_1st = ci->dsn[0];
	ci->dsn[0] = '\0';
	if (NAME_IS_VALID(ci->conn_settings))
		ci->conn_settings_in_str = TRUE;
	if (NAME_IS_VALID(ci->pqopt))
		ci->pqopt_in_str = TRUE;
	makeConnectString(out_conn, ci, sizeof(out_conn));
#ifdef	FORCE_PASSWORD_DISPLAY
MYLOG(MIN_LOG_LEVEL, "conn_string=%s\n", out_conn);
#else
	char* hide_str = hide_password(out_conn, ';');
	MYLOG(MIN_LOG_LEVEL, "conn_string=%s\n", hide_str);
	if (hide_str)
		free(hide_str);
#endif /* FORCE_PASSWORD_DISPLAY */

#ifdef	UNICODE_SUPPORT
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, out_conn, -1, wout_conn, sizeof(wout_conn) / sizeof(wout_conn[0]));
	conn_str = wout_conn;
#else
	conn_str = out_conn;
#endif /* UNICODE_SUPPORT */
	ci->dsn[0] = dsn_1st;
	if (!SQL_SUCCEEDED(ret = SQLALLOCHANDLEFUNC(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env)))
	{
		ermsg = _T("SQLAllocHandle for env error");
		goto cleanup;
	}
	if (!SQL_SUCCEEDED(ret = SQLSETENVATTRFUNC(env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3, 0)))
	{
		SNTPRINTF(szMsg, _countof(szMsg), _T("SQLAllocHandle for env error=%d"), ret);
		goto cleanup;
	}
	if (!SQL_SUCCEEDED(ret = SQLALLOCHANDLEFUNC(SQL_HANDLE_DBC, env, &conn)))
	{
		SQLGETDIAGRECFUNC(SQL_HANDLE_ENV, env, 1, NULL, &errnum, szMsg, _countof(szMsg), &str_len);
		ermsg = szMsg;
		goto cleanup;
	}
	if (!SQL_SUCCEEDED(ret = SQLDRIVERCONNECTFUNC(conn, hwnd, conn_str, SQL_NTS, NULL, MAX_CONNECT_STRING_LEN, &str_len, SQL_DRIVER_NOPROMPT)))
	{
		SQLGETDIAGRECFUNC(SQL_HANDLE_DBC, conn, 1, NULL, &errnum, szMsg, _countof(szMsg), &str_len);
		ermsg = szMsg;
		goto cleanup;
	}
	connected = TRUE;
	ermsg = _T("Connection successful");

	if (withDTC)
	{
#ifdef	_HANDLE_ENLIST_IN_DTC_
		HRESULT	res;
		void *pObj = NULL;

		pObj = CALL_GetTransactionObject(&res);
		if (NULL != pObj)
		{
			SQLRETURN ret = SQLSETCONNECTATTRFUNC(conn, SQL_ATTR_ENLIST_IN_DTC, (SQLPOINTER) pObj, 0);
			if (SQL_SUCCEEDED(ret))
			{
				SQLSETCONNECTATTRFUNC(conn, SQL_ATTR_ENLIST_IN_DTC, SQL_DTC_DONE, 0);
				SNTPRINTF(szMsg, _countof(szMsg), _T("%s\nenlistment was successful\n"), ermsg);
				ermsg = szMsg;
			}
			else
			{
				int	strl;

				SNTPRINTF(szMsg, _countof(szMsg), _T("%s\nMSDTC error:"), ermsg);
				for (strl = 0; strl < SQL_MAX_MESSAGE_LENGTH; strl++)
				{
					if (!szMsg[strl])
						break;
				}
				SQLGETDIAGRECFUNC(SQL_HANDLE_DBC, conn, 1, NULL, &errnum, szMsg + strl, _countof(szMsg) - strl, &str_len);
				ermsg = szMsg;
			}
			CALL_ReleaseTransactionObject(pObj);
		}
		else if (FAILED(res))
		{
			SNTPRINTF(szMsg, _countof(szMsg), _T("%s\nDistibuted Transaction enlistment error %x"), ermsg, res);
			ermsg = szMsg;
		}
#else	/* _HANDLE_ENLIST_IN_DTC_ */
		SNTPRINTF(szMsg, _countof(szMsg), _T("%s\nDistibuted Transaction enlistment not supported by this driver"), ermsg);
		ermsg = szMsg;
#endif
	}

cleanup:
	if (NULL != ermsg && NULL != hwnd)
	{
        if (!connected)
        {
            connection_test_icon = MB_ICONEXCLAMATION;
        }
        MESSAGEBOXFUNC(hwnd, ermsg, _T("Connection Test"), connection_test_icon | MB_OK);
	}

#undef _T

	if (NULL != conn)
	{
		if (connected)
			SQLDISCONNECTFUNC(conn);
		SQLFREEHANDLEFUNC(SQL_HANDLE_DBC, conn);
	}
	if (env)
		SQLFREEHANDLEFUNC(SQL_HANDLE_ENV, env);

	return;
}

/*-------
 * ParseAttributes
 *
 *	Description:	Parse attribute string moving values into the aAttr array
 *	Input	 :	lpszAttributes - Pointer to attribute string
 *	Output	 :	None (global aAttr normally updated)
 *-------
 */
static void
ParseAttributes(LPCSTR lpszAttributes, LPSETUPDLG lpsetupdlg)
{
	LPCSTR		lpsz;
	LPCSTR		lpszStart;
	char		aszKey[MAXKEYLEN];
	size_t		cbKey;
	char		value[MAXPGPATH];

	for (lpsz = lpszAttributes; *lpsz; lpsz++)
	{
		/*
		 * Extract key name (e.g., DSN), it must be terminated by an
		 * equals
		 */
		lpszStart = lpsz;
		for (;; lpsz++)
		{
			if (!*lpsz)
				return;			/* No key was found */
			else if (*lpsz == '=')
				break;			/* Valid key found */
		}
		/* Determine the key's index in the key table (-1 if not found) */
		cbKey = lpsz - lpszStart;
		if (cbKey < sizeof(aszKey))
		{
			memcpy(aszKey, lpszStart, cbKey);
			aszKey[cbKey] = '\0';
		}

		/* Locate end of key value */
		lpszStart = ++lpsz;
		for (; *lpsz; lpsz++)
			;

		/* lpsetupdlg->aAttr[iElement].fSupplied = TRUE; */
		memcpy(value, lpszStart, MIN(lpsz - lpszStart + 1, MAXPGPATH));

		MYLOG(MIN_LOG_LEVEL, "aszKey='%s', value='%s'\n", aszKey, value);

		/* Copy the appropriate value to the conninfo  */
		copyConnAttributes(&lpsetupdlg->ci, aszKey, value);
	}
	return;
}


/*--------
 * SetDSNAttributes
 *
 *	Description:	Write data source attributes to ODBC.INI
 *	Input	 :	hwnd - Parent window handle (plus globals)
 *	Output	 :	TRUE if successful, FALSE otherwise
 *--------
 */
static BOOL
SetDSNAttributes(HWND hwndParent, LPSETUPDLG lpsetupdlg, DWORD *errcode)
{
	LPCSTR		lpszDSN;		/* Pointer to data source name */

	lpszDSN = lpsetupdlg->ci.dsn;

	if (errcode)
		*errcode = 0;
	/* Validate arguments */
	if (lpsetupdlg->fNewDSN && !*lpsetupdlg->ci.dsn)
		return FALSE;

	/* Write the data source name */
	if (!SQLWriteDSNToIni(lpszDSN, lpsetupdlg->lpszDrvr))
	{
		RETCODE	ret = SQL_ERROR;
		DWORD	err = SQL_ERROR;
		char    szMsg[SQL_MAX_MESSAGE_LENGTH];

		ret = SQLInstallerError(1, &err, szMsg, sizeof(szMsg), NULL);
		if (hwndParent)
		{
			char		szBuf[MAXPGPATH];

			if (SQL_SUCCESS != ret)
			{
				LoadString(s_hModule, IDS_BADDSN, szBuf, sizeof(szBuf));
				SPRINTF_FIXED(szMsg, szBuf, lpszDSN);
			}
			LoadString(s_hModule, IDS_MSGTITLE, szBuf, sizeof(szBuf));
			MessageBox(hwndParent, szMsg, szBuf, MB_ICONEXCLAMATION | MB_OK);
		}
		if (errcode)
			*errcode = err;
		return FALSE;
	}

	/* Update ODBC.INI */
	write_Ci_Drivers(ODBC_INI, lpsetupdlg->ci.dsn, &(lpsetupdlg->ci.drivers));
	writeDSNinfo(&lpsetupdlg->ci);

	/* If the data source name has changed, remove the old name */
	if (lstrcmpi(lpsetupdlg->szDSN, lpsetupdlg->ci.dsn))
		SQLRemoveDSNFromIni(lpsetupdlg->szDSN);
	return TRUE;
}

/*--------
 * SetDriverAttributes
 *
 *	Description:	Write driver information attributes to ODBCINST.INI
 *	Input	 :	lpszDriver - The driver name
 *	Output	 :	TRUE if successful, FALSE otherwise
 *--------
 */
static BOOL
SetDriverAttributes(LPCSTR lpszDriver, DWORD *pErrorCode, LPSTR message, WORD cbMessage)
{
	BOOL	ret = FALSE;
	char ver_string[8];

	/* Validate arguments */
	if (!lpszDriver || !lpszDriver[0])
	{
		if (pErrorCode)
			*pErrorCode = ODBC_ERROR_INVALID_NAME;
		strncpy_null(message, "Driver name not specified", cbMessage);
		return FALSE;
	}

	if (!SQLWritePrivateProfileString(lpszDriver, "APILevel", "1", ODBCINST_INI))
		goto cleanup;
	if (!SQLWritePrivateProfileString(lpszDriver, "ConnectFunctions", "YYN", ODBCINST_INI))
		goto cleanup;
	SPRINTF_FIXED(ver_string, "%02x.%02x",
				 ODBCVER / 256,
				 ODBCVER % 256);
	if (!SQLWritePrivateProfileString(lpszDriver, "DriverODBCVer",
		ver_string,
		ODBCINST_INI))
		goto cleanup;
	if (!SQLWritePrivateProfileString(lpszDriver, "FileUsage", "0", ODBCINST_INI))
		goto cleanup;
	if (!SQLWritePrivateProfileString(lpszDriver, "SQLLevel", "1", ODBCINST_INI))
		goto cleanup;

	ret = TRUE;
cleanup:
	if (!ret)
	{
		if (pErrorCode)
			*pErrorCode = ODBC_ERROR_REQUEST_FAILED;
		strncpy_null(message, "Failed to WritePrivateProfileString", cbMessage);
	}
	return ret;
}


#ifdef	WIN32

BOOL	INTFUNC
ChangeDriverName(HWND hwndParent, LPSETUPDLG lpsetupdlg, LPCSTR driver_name)
{
	DWORD   err = 0;
	ConnInfo	*ci = &lpsetupdlg->ci;

	if (!ci->dsn[0])
	{
		err = IDS_BADDSN;
	}
	else if (!driver_name || strnicmp(driver_name, "aws", 3) != 0 || !stristr(driver_name, "postgresql"))
	{
		err = IDS_BADDSN;
	}
	else
	{
		LPCSTR	lpszDrvr = lpsetupdlg->lpszDrvr;

		lpsetupdlg->lpszDrvr = driver_name;
		if (!SetDSNAttributes(hwndParent, lpsetupdlg, &err))
		{
			if (!err)
				err = IDS_BADDSN;
			lpsetupdlg->lpszDrvr = lpszDrvr;
		}
	}
	return (err == 0);
}

#endif /* WIN32 */
