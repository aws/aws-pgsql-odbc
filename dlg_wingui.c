#ifdef	WIN32
/*-------
 * Module:			dlg_wingui.c
 *
 * Description:		This module contains any specific code for handling
 *					dialog boxes such as driver/datasource options.  Both the
 *					ConfigDSN() and the SQLDriverConnect() functions use
 *					functions in this module.  If you were to add a new option
 *					to any dialog box, you would most likely only have to change
 *					things in here rather than in 2 separate places as before.
 *
 * Classes:			none
 *
 * API functions:	none
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *-------
 */
/* Multibyte support	Eiji Tokuya 2001-03-15 */

#include "resource.h"
#include "dlg_specific.h"
#include "misc.h" // strncpy_null
#include "win_setup.h"

#include "loadlib.h"

#include "pgapifunc.h"
#ifdef _HANDLE_ENLIST_IN_DTC_
#include "xalibname.h"
#include "connexp.h"
#endif /* _HANDLE_ENLIST_IN_DTC_ */

#include <commctrl.h>

extern HINSTANCE s_hModule;
static int	driver_optionsDraw(HWND, const ConnInfo *, int src, BOOL enable);
static int	driver_options_update(HWND hdlg, ConnInfo *ci);
static int  limitless_options_update(HWND hdlg, ConnInfo *ci);
static int	failover_options_update(HWND hdlg, ConnInfo *ci);

static int	ds_options_update(HWND hdlg, ConnInfo *ci);

static int	ds_options3Draw(HWND, const ConnInfo *);
static int	ds_options3_update(HWND hdlg, ConnInfo *ci);

static struct {
	int	ids;
	const char* const	authtypestr;
} authtype[] = {
		  {IDS_AUTHTYPE_DATABASE, DATABASE_MODE}
		, {IDS_AUTHTYPE_IAM, IAM_MODE}
		, {IDS_AUTHTYPE_ADFS, ADFS_MODE}
		, {IDS_AUTHTYPE_OKTA, OKTA_MODE}
		, {IDS_AUTHTYPE_SECRET, SECRET_MODE}
};

static struct {
	int	ids;
    const char *const modestr;
} failoverModes[] = {
		  {IDC_EDIT_FAILOVER_MODE_READER_OR_WRITER, FAILOVER_READER_OR_WRITER}
		, {IDC_EDIT_FAILOVER_MODE_STRICT_READER, FAILOVER_STRICT_READER}
		, {IDC_EDIT_FAILOVER_MODE_STRICT_WRITER, FAILOVER_STRICT_WRITER}
};

static struct {
  int ids;
  const char *const modestr;
} failoverReaderHostSelectorStrategies[] = {
    {IDC_EDIT_READER_STRATEGY_RANDOM, READER_STRATEGY_RANDOM},
    {IDC_EDIT_READER_STRATEGY_ROUND_ROBIN, READER_STRATEGY_ROUND_ROBIN},
    {IDC_EDIT_READER_STRATEGY_HIGHEST_WEIGHT, READER_STRATEGY_HIGHEST_WEIGHT}};

static struct {
	int	ids;
	const char * const	modestr;
} modetab[] = {
		  {IDS_SSLREQUEST_DISABLE, SSLMODE_DISABLE}
		, {IDS_SSLREQUEST_ALLOW, SSLMODE_ALLOW}
		, {IDS_SSLREQUEST_PREFER, SSLMODE_PREFER}
		, {IDS_SSLREQUEST_REQUIRE, SSLMODE_REQUIRE}
		, {IDS_SSLREQUEST_VERIFY_CA, SSLMODE_VERIFY_CA}
		, {IDS_SSLREQUEST_VERIFY_FULL, SSLMODE_VERIFY_FULL}
	};

static struct {
	int	ids;
	const char * const	modestr;
} limitlessmode[] = {
		  {IDC_LIMITLESS_MODE_LAZY, LIMITLESS_LAZY}
		, {IDC_LIMITLESS_MODE_IMMEDIATE, LIMITLESS_IMMEDIATE}
	};

static int	dspcount_bylevel[] = {1, 4, 6};

// window handle of iam host
HWND iamHostDlg;
// window handle of region
HWND regionDlg;
// window handle of user name
HWND userNameDlg;
// window handle of password
HWND passwordDlg;
// window handle of token expiration
HWND tokenExpirationDlg;

// IDP window handles
HWND idpEndpointDlg;
HWND idpPortDlg;
HWND idpUserNameDlg;
HWND idpPasswordDlg;
HWND idpRoleArnDlg;
HWND idpArnDlg;
HWND socketTimeoutDlg;
HWND connTimeoutDlg;
HWND relayingPartyIDDlg;
HWND appIdDlg;

bool isInList(char* check, char *valid[], unsigned int size) {
    for (int i = 0; i < size; i++) {
        if (stricmp(check, valid[i]) == 0) {
            return true;
        }
    }
    return false;
}

// window handle of secret ID
HWND secretIdDlg;

void EnableWindows(int index) {
	// Enable iamHost only when authtype is not Database or Secrets Manager
	EnableWindow(iamHostDlg, stricmp(authtype[index].authtypestr, DATABASE_MODE) != 0 && stricmp(authtype[index].authtypestr, SECRET_MODE) != 0);
	// Enable region and token expiration only when authtype is not Database
	EnableWindow(regionDlg, stricmp(authtype[index].authtypestr, DATABASE_MODE) != 0);
	EnableWindow(tokenExpirationDlg, stricmp(authtype[index].authtypestr, DATABASE_MODE) != 0);

	// Enable user name only when authtype is not Secrets Manager
	EnableWindow(userNameDlg, stricmp(authtype[index].authtypestr, SECRET_MODE) != 0);

	// Enable password only when authtype is DATABASE
	EnableWindow(passwordDlg, stricmp(authtype[index].authtypestr, DATABASE_MODE) == 0);

	// ADFS & Okta
	bool isFederated = isInList(authtype[index].authtypestr, (char*[]){ADFS_MODE, OKTA_MODE}, 2);
	EnableWindow(idpEndpointDlg, isFederated);
	EnableWindow(idpPortDlg, isFederated);
	EnableWindow(idpUserNameDlg, isFederated);
	EnableWindow(idpPasswordDlg, isFederated);
	EnableWindow(idpRoleArnDlg, isFederated);
	EnableWindow(idpArnDlg, isFederated);
	EnableWindow(socketTimeoutDlg, isFederated);
	EnableWindow(connTimeoutDlg, isFederated);

	// ADFS Specific
	EnableWindow(relayingPartyIDDlg, stricmp(authtype[index].authtypestr, ADFS_MODE) == 0);

	// Okta Specfic
	EnableWindow(appIdDlg, stricmp(authtype[index].authtypestr, OKTA_MODE) == 0);

	// Secrets Manager Specific
	EnableWindow(secretIdDlg, stricmp(authtype[index].authtypestr, SECRET_MODE) == 0);
}

// Function to handle notifications from the AuthType drop list
LRESULT CALLBACK ListBoxProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	if (message == WM_COMMAND) {
		if (HIWORD(wParam) == CBN_SELCHANGE) {
			// Get the selected index
			int selidx = SendMessage(hdlg, CB_GETCURSEL, 0, 0);

			MYLOG(MIN_LOG_LEVEL, "selected index is %d\n", selidx);

			EnableWindows(selidx);
		}
	}
	return DefSubclassProc(hdlg, message, wParam, lParam);
}

void
SetDlgStuff(HWND hdlg, const ConnInfo *ci)
{
    char	buff[MEDIUM_REGISTRY_LEN + 1], strbuf[10], titlebuf[80];
	int	i, dsplevel, selidx, dspcount;

    LoadString(s_hModule, IDS_SETUP, strbuf, sizeof(strbuf));
    snprintf(titlebuf, sizeof(titlebuf), "%s %s", ci->drivername, strbuf);
    SetWindowText(hdlg, titlebuf);

	/*
	 * If driver attribute NOT present, then set the datasource name and
	 * description
	 */
	SetDlgItemText(hdlg, IDC_DSNAME, ci->dsn);
	SetDlgItemText(hdlg, IDC_DESC, ci->desc);

	SetDlgItemText(hdlg, IDC_DATABASE, ci->database);
	SetDlgItemText(hdlg, IDC_SERVER, ci->server);
	SetDlgItemText(hdlg, IDC_USER, ci->username);
	SetDlgItemText(hdlg, IDC_PASSWORD, SAFE_NAME(ci->password));
	SetDlgItemText(hdlg, IDC_PORT, ci->port);
	SetDlgItemText(hdlg, IDC_IAM_HOST, ci->iam_host);
	SetDlgItemText(hdlg, IDC_REGION, ci->region);
	SetDlgItemText(hdlg, IDC_TOKEN_EXPIRATION, ci->token_expiration);

	SetDlgItemText(hdlg, IDC_IDP_ENDPOINT, ci->federation_cfg.idp_endpoint);
	SetDlgItemText(hdlg, IDC_IDP_PORT, ci->federation_cfg.idp_port);
	SetDlgItemText(hdlg, IDC_IDP_USER_NAME, ci->federation_cfg.idp_username);
	SetDlgItemText(hdlg, IDC_IDP_PASSWORD, ci->federation_cfg.idp_password);
	SetDlgItemText(hdlg, IDC_ROLE_ARN, ci->federation_cfg.iam_role_arn);
	SetDlgItemText(hdlg, IDC_IDP_ARN, ci->federation_cfg.iam_idp_arn);
	SetDlgItemText(hdlg, IDC_SOCKET_TIMEOUT, ci->federation_cfg.http_client_socket_timeout);
	SetDlgItemText(hdlg, IDC_CONN_TIMEOUT, ci->federation_cfg.http_client_connect_timeout);
	SetDlgItemText(hdlg, IDC_RELAYING_PARTY_ID, ci->federation_cfg.relaying_party_id);
	SetDlgItemText(hdlg, IDC_APP_ID, ci->federation_cfg.app_id);

	SetDlgItemText(hdlg, IDC_SECRET_ID, ci->secret_id);

	dsplevel = 0;

	/*
	 * XXX: We used to hide or show this depending whether libpq was loaded,
	 * but we depend on libpq directly nowadays, so it's always loaded.
	 */
	ShowWindow(GetDlgItem(hdlg, IDC_NOTICE_USER), SW_HIDE);
	dsplevel = 2;

	selidx = -1;
	for (i = 0; i < sizeof(modetab) / sizeof(modetab[0]); i++)
	{
		if (!stricmp(ci->sslmode, modetab[i].modestr))
		{
			selidx = i;
			break;
		}
	}
	for (i = dsplevel; i < sizeof(dspcount_bylevel) / sizeof(int); i++)
	{
		if (selidx < dspcount_bylevel[i])
			break;
		dsplevel++;
	}

	dspcount = dspcount_bylevel[dsplevel];
	for (i = 0; i < dspcount; i++)
	{
		LoadString(GetWindowInstance(hdlg), modetab[i].ids, buff, MEDIUM_REGISTRY_LEN);
		SendDlgItemMessage(hdlg, IDC_SSLMODE, CB_ADDSTRING, 0, (WPARAM) buff);
	}

	SendDlgItemMessage(hdlg, IDC_SSLMODE, CB_SETCURSEL, selidx, (WPARAM) 0);

	selidx = 0;
	for (i = 0; i < sizeof(authtype) / sizeof(authtype[0]); i++)
	{
		if (!stricmp(ci->authtype, authtype[i].authtypestr))
		{
			selidx = i;
			break;
		}
	}
	// for authentication type
	for (i = 0; i < sizeof(authtype) / sizeof(authtype[0]); i++)
	{
		LoadString(GetWindowInstance(hdlg), authtype[i].ids, buff, MEDIUM_REGISTRY_LEN);
		SendDlgItemMessage(hdlg, IDC_AUTHTYPE, CB_ADDSTRING, 0, (WPARAM)buff);
	}
	SendDlgItemMessage(hdlg, IDC_AUTHTYPE, CB_SETCURSEL, selidx, (WPARAM)0);

	// Set subclass procedure for the authtype drop list to handle notifications
	SetWindowSubclass(GetDlgItem(hdlg, IDC_AUTHTYPE), ListBoxProc, 0, 0);

    iamHostDlg = GetDlgItem(hdlg, IDC_IAM_HOST);
    regionDlg = GetDlgItem(hdlg, IDC_REGION);
	tokenExpirationDlg = GetDlgItem(hdlg, IDC_TOKEN_EXPIRATION);
	userNameDlg = GetDlgItem(hdlg, IDC_USER);
	passwordDlg = GetDlgItem(hdlg, IDC_PASSWORD);

	idpEndpointDlg = GetDlgItem(hdlg, IDC_IDP_ENDPOINT);
	idpPortDlg = GetDlgItem(hdlg, IDC_IDP_PORT);
	idpUserNameDlg = GetDlgItem(hdlg, IDC_IDP_USER_NAME);
	idpPasswordDlg = GetDlgItem(hdlg, IDC_IDP_PASSWORD);
	idpRoleArnDlg = GetDlgItem(hdlg, IDC_ROLE_ARN);
	idpArnDlg = GetDlgItem(hdlg, IDC_IDP_ARN);

	socketTimeoutDlg = GetDlgItem(hdlg, IDC_SOCKET_TIMEOUT);
	connTimeoutDlg = GetDlgItem(hdlg, IDC_CONN_TIMEOUT);
	relayingPartyIDDlg = GetDlgItem(hdlg, IDC_RELAYING_PARTY_ID);
	appIdDlg = GetDlgItem(hdlg, IDC_APP_ID);

    secretIdDlg = GetDlgItem(hdlg, IDC_SECRET_ID);

	EnableWindows(selidx);
}

void
GetDlgStuff(HWND hdlg, ConnInfo *ci)
{
	int	sslposition, authtypeposition;
	char	medium_buf[MEDIUM_REGISTRY_LEN];

	GetDlgItemText(hdlg, IDC_DESC, ci->desc, sizeof(ci->desc));

	GetDlgItemText(hdlg, IDC_DATABASE, ci->database, sizeof(ci->database));
	GetDlgItemText(hdlg, IDC_SERVER, ci->server, sizeof(ci->server));
	GetDlgItemText(hdlg, IDC_USER, ci->username, sizeof(ci->username));
	GetDlgItemText(hdlg, IDC_PASSWORD, medium_buf, sizeof(medium_buf));
	STR_TO_NAME(ci->password, medium_buf);
	GetDlgItemText(hdlg, IDC_IAM_HOST, ci->iam_host, sizeof(ci->iam_host));
	GetDlgItemText(hdlg, IDC_REGION, ci->region, sizeof(ci->region));
	GetDlgItemText(hdlg, IDC_PORT, ci->port, sizeof(ci->port));
	sslposition = (int)(DWORD)SendMessage(GetDlgItem(hdlg, IDC_SSLMODE), CB_GETCURSEL, 0L, 0L);
	STRCPY_FIXED(ci->sslmode, modetab[sslposition].modestr);
	authtypeposition = (int)(DWORD)SendMessage(GetDlgItem(hdlg, IDC_AUTHTYPE), CB_GETCURSEL, 0L, 0L);
	STRCPY_FIXED(ci->authtype, authtype[authtypeposition].authtypestr);
	GetDlgItemText(hdlg, IDC_TOKEN_EXPIRATION, ci->token_expiration, sizeof(ci->token_expiration));

	GetDlgItemText(hdlg, IDC_IDP_ENDPOINT, ci->federation_cfg.idp_endpoint, sizeof(ci->federation_cfg.idp_endpoint));
	GetDlgItemText(hdlg, IDC_IDP_PORT, ci->federation_cfg.idp_port, sizeof(ci->federation_cfg.idp_port));
	GetDlgItemText(hdlg, IDC_IDP_USER_NAME, ci->federation_cfg.idp_username, sizeof(ci->federation_cfg.idp_username));
	GetDlgItemText(hdlg, IDC_IDP_PASSWORD, ci->federation_cfg.idp_password, sizeof(medium_buf));
	GetDlgItemText(hdlg, IDC_ROLE_ARN, ci->federation_cfg.iam_role_arn, sizeof(ci->federation_cfg.iam_role_arn));
	GetDlgItemText(hdlg, IDC_IDP_ARN, ci->federation_cfg.iam_idp_arn, sizeof(ci->federation_cfg.iam_idp_arn));

	GetDlgItemText(hdlg, IDC_SOCKET_TIMEOUT, ci->federation_cfg.http_client_socket_timeout, sizeof(ci->federation_cfg.http_client_socket_timeout));
	GetDlgItemText(hdlg, IDC_CONN_TIMEOUT, ci->federation_cfg.http_client_connect_timeout, sizeof(ci->federation_cfg.http_client_connect_timeout));
	GetDlgItemText(hdlg, IDC_RELAYING_PARTY_ID, ci->federation_cfg.relaying_party_id, sizeof(ci->federation_cfg.relaying_party_id));

	GetDlgItemText(hdlg, IDC_APP_ID, ci->federation_cfg.app_id, sizeof(ci->federation_cfg.app_id));

	GetDlgItemText(hdlg, IDC_SECRET_ID, ci->secret_id, sizeof(ci->secret_id));
}

static void
getDriversDefaultsOfCi(const ConnInfo *ci, GLOBAL_VALUES *glbv)
{
	const char *drivername = NULL;

	if (ci->drivername[0])
		drivername = ci->drivername;
	else if (NAME_IS_VALID(ci->drivers.drivername))
		drivername = SAFE_NAME(ci->drivers.drivername);
	if (drivername && drivername[0])
		getDriversDefaults(drivername, glbv);
	else
		getDriversDefaults(INVALID_DRIVER, glbv);
}

static void limitless_optionsDlgEnable(HWND hdlg, const ConnInfo *ci) {
	EnableWindow(GetDlgItem(hdlg, IDC_LIMITLESS_MODE), ci->limitless_enabled > 0);
	EnableWindow(GetDlgItem(hdlg, IDC_LIMITLESS_MONITOR_INTERVAL_MS), ci->limitless_enabled > 0);
	EnableWindow(GetDlgItem(hdlg, IDC_LIMITLESS_SERVICE_ID), ci->limitless_enabled > 0);
}

static int
driver_optionsDraw(HWND hdlg, const ConnInfo *ci, int src, BOOL enable)
{
	const GLOBAL_VALUES *comval = NULL;
	const char * drivername = NULL;
	GLOBAL_VALUES defval;

MYLOG(MIN_LOG_LEVEL, "entering src=%d\n", src);
	init_globals(&defval);
	switch (src)
	{
		case 0:			/* default */
			getDriversDefaultsOfCi(ci, &defval);
			defval.debug = DEFAULT_DEBUG;
			defval.commlog = DEFAULT_COMMLOG;
			comval = &defval;
			break;
		case 1:			/* dsn specific */
			comval = &(ci->drivers);
			break;
		default:
			return 0;
	}

	ShowWindow(GetDlgItem(hdlg, DRV_MSG_LABEL2), enable ? SW_SHOW : SW_HIDE);

	CheckDlgButton(hdlg, DRV_COMMLOG, comval->commlog > 0);
	SetDlgItemInt(hdlg, DS_COMMLOG, comval->commlog, FALSE);
	ShowWindow(GetDlgItem(hdlg, DS_COMMLOG), comval->commlog > 0 ? SW_SHOW : SW_HIDE);

	CheckDlgButton(hdlg, DRV_UNIQUEINDEX, comval->unique_index);
	/* EnableWindow(GetDlgItem(hdlg, DRV_UNIQUEINDEX), enable); */
	EnableWindow(GetDlgItem(hdlg, DRV_READONLY), FALSE);
	CheckDlgButton(hdlg, DRV_USEDECLAREFETCH, comval->use_declarefetch);

	/* Unknown Sizes clear */
	CheckDlgButton(hdlg, DRV_UNKNOWN_DONTKNOW, 0);
	CheckDlgButton(hdlg, DRV_UNKNOWN_LONGEST, 0);
	CheckDlgButton(hdlg, DRV_UNKNOWN_MAX, 0);
	/* Unknown (Default) Data Type sizes */
	switch (comval->unknown_sizes)
	{
		case UNKNOWNS_AS_DONTKNOW:
			CheckDlgButton(hdlg, DRV_UNKNOWN_DONTKNOW, 1);
			break;
		case UNKNOWNS_AS_LONGEST:
			CheckDlgButton(hdlg, DRV_UNKNOWN_LONGEST, 1);
			break;
		case UNKNOWNS_AS_MAX:
		default:
			CheckDlgButton(hdlg, DRV_UNKNOWN_MAX, 1);
			break;
	}

	CheckDlgButton(hdlg, DRV_TEXT_LONGVARCHAR, comval->text_as_longvarchar);
	CheckDlgButton(hdlg, DRV_UNKNOWNS_LONGVARCHAR, comval->unknowns_as_longvarchar);
	CheckDlgButton(hdlg, DRV_BOOLS_CHAR, comval->bools_as_char);
	CheckDlgButton(hdlg, DRV_PARSE, comval->parse);

	CheckDlgButton(hdlg, DRV_DEBUG, comval->debug > 0);
	SetDlgItemInt(hdlg, DS_DEBUG, comval->debug, FALSE);
	ShowWindow(GetDlgItem(hdlg, DS_DEBUG), comval->debug > 0 ? SW_SHOW : SW_HIDE);

	SetDlgItemInt(hdlg, DRV_CACHE_SIZE, comval->fetch_max, FALSE);
	SetDlgItemInt(hdlg, DRV_VARCHAR_SIZE, comval->max_varchar_size, FALSE);
	SetDlgItemInt(hdlg, DRV_LONGVARCHAR_SIZE, comval->max_longvarchar_size, TRUE);
	SetDlgItemText(hdlg, DRV_EXTRASYSTABLEPREFIXES, comval->extra_systable_prefixes);
	switch (src)
	{
		case 1:
			ShowWindow(GetDlgItem(hdlg, DS_BATCH_SIZE), SW_SHOW);
			ShowWindow(GetDlgItem(hdlg, DS_IGNORETIMEOUT), SW_SHOW);
			SetDlgItemInt(hdlg, DS_BATCH_SIZE, ci->batch_size, FALSE);
			CheckDlgButton(hdlg, DS_IGNORETIMEOUT, ci->ignore_timeout);
			break;
		default:
			ShowWindow(GetDlgItem(hdlg, DS_BATCH_SIZE), SW_HIDE);
			ShowWindow(GetDlgItem(hdlg, DS_IGNORETIMEOUT), SW_HIDE);
			break;
	}

	CheckDlgButton(hdlg, DRV_RDS_LOGGING_ENABLED, ci->rds_logging_enabled > 0);
	SetDlgItemInt(hdlg, DRV_RDS_LOG_THRESHOLD, ci->rds_log_threshold, FALSE);

	/* Driver Connection Settings */
	EnableWindow(GetDlgItem(hdlg, DRV_CONNSETTINGS), FALSE);
	ShowWindow(GetDlgItem(hdlg, ID2NDPAGE), enable ? SW_HIDE : SW_SHOW);
	ShowWindow(GetDlgItem(hdlg, ID3RDPAGE), enable ? SW_HIDE : SW_SHOW);

	finalize_globals(&defval);
	return 0;
}

static int
limitless_optionsDraw(HWND hdlg, const ConnInfo *ci, int src, BOOL enable)
{
	const GLOBAL_VALUES *comval = NULL;
	GLOBAL_VALUES defval;
	char	buff[MEDIUM_REGISTRY_LEN + 1];

	MYLOG(MIN_LOG_LEVEL, "entering src=%d\n", src);
	init_globals(&defval);
	switch (src)
	{
		case 0:			/* default */
			getDriversDefaultsOfCi(ci, &defval);
			defval.debug = DEFAULT_DEBUG;
			defval.commlog = DEFAULT_COMMLOG;
			comval = &defval;
			break;
		case 1:			/* dsn specific */
			comval = &(ci->drivers);
			break;
		default:
			return 0;
	}

	CheckDlgButton(hdlg, IDC_ENABLE_LIMITLESS, ci->limitless_enabled > 0);
	SetDlgItemInt(hdlg, IDC_LIMITLESS_MONITOR_INTERVAL_MS, ci->limitless_monitor_interval_ms, FALSE);
	SetDlgItemText(hdlg, IDC_LIMITLESS_SERVICE_ID, ci->limitless_service_id);

	int i, selidx = 0;
	for (i = 0; i < sizeof(limitlessmode) / sizeof(limitlessmode[0]); i++) {
		if (!stricmp(ci->limitless_mode, limitlessmode[i].modestr)) {
			selidx = i;
			break;
		}
	}
	for (i = 0; i < sizeof(limitlessmode) / sizeof(limitlessmode[0]); i++) {
		LoadString(GetWindowInstance(hdlg), limitlessmode[i].ids, buff, MEDIUM_REGISTRY_LEN);
		SendDlgItemMessage(hdlg, IDC_LIMITLESS_MODE, CB_ADDSTRING, 0, (WPARAM)buff);
	}
	SendDlgItemMessage(hdlg, IDC_LIMITLESS_MODE, CB_SETCURSEL, selidx, (WPARAM)0);
	SetWindowSubclass(GetDlgItem(hdlg, IDC_LIMITLESS_MODE), DefSubclassProc, 0, 0);

	limitless_optionsDlgEnable(hdlg, ci);

	ShowWindow(GetDlgItem(hdlg, DRV_MSG_LABEL2), enable ? SW_SHOW : SW_HIDE);

	finalize_globals(&defval);
	return 0;
}

static void failover_optionsDlgEnable(HWND hdlg, const ConnInfo *ci) {
	// Disable configuration if failover is not enabled
	EnableWindow(GetDlgItem(hdlg, IDC_EDIT_FAILOVER_MODE), ci->enable_failover > 0);
	EnableWindow(GetDlgItem(hdlg, IDC_EDIT_FAILOVER_READER_HOST_SELECTOR_STRATEGY), ci->enable_failover > 0);
	EnableWindow(GetDlgItem(hdlg, IDC_EDIT_HOST_PATTERN), ci->enable_failover > 0);
	EnableWindow(GetDlgItem(hdlg, IDC_EDIT_CLUSTER_ID), ci->enable_failover > 0);
	EnableWindow(GetDlgItem(hdlg, IDC_EDIT_TOPOLOGY_REFRESH_RATE), ci->enable_failover > 0);
	EnableWindow(GetDlgItem(hdlg, IDC_EDIT_TOPOLOGY_HIGH_REFRESH_RATE), ci->enable_failover > 0);
	EnableWindow(GetDlgItem(hdlg, IDC_EDIT_IGNORE_TOPOLOGY_REFRESH_RATE), ci->enable_failover > 0);
	EnableWindow(GetDlgItem(hdlg, IDC_EDIT_FAILOVER_TIMEOUT), ci->enable_failover > 0);
}

// Failover - Draw out GUI
// Sets values based on current connection Info
static int
failover_optionsDraw(HWND hdlg, const ConnInfo *ci, int src, BOOL enable)
{
	const GLOBAL_VALUES *comval = NULL;
	GLOBAL_VALUES defval;
	char buff[MEDIUM_REGISTRY_LEN + 1];

	MYLOG(0, "entering src=%d\n", src);
	init_globals(&defval);
	switch (src)
	{
		case 0: /* default */
			getDriversDefaultsOfCi(ci, &defval);
			defval.debug = DEFAULT_DEBUG;
			defval.commlog = DEFAULT_COMMLOG;
			comval = &defval;
			break;
		case 1: /* dsn specific */
			comval = &(ci->drivers);
			break;
		default:
			return 0;
	}

	// Bool
	CheckDlgButton(hdlg, IDC_CHECK_ENABLE_CLUSTER_FAILOVER, ci->enable_failover > 0);
	
	// String (handle, resource id, source)
	SetDlgItemText(hdlg, IDC_EDIT_FAILOVER_MODE, ci->failover_mode);
	SetDlgItemText(hdlg, IDC_EDIT_FAILOVER_READER_HOST_SELECTOR_STRATEGY, ci->reader_host_selector_strategy);
	SetDlgItemText(hdlg, IDC_EDIT_HOST_PATTERN, ci->host_pattern);
	SetDlgItemText(hdlg, IDC_EDIT_CLUSTER_ID, ci->cluster_id);

	// Ints (handle, resource id, source, is signed)
	SetDlgItemInt(hdlg, IDC_EDIT_TOPOLOGY_REFRESH_RATE, ci->topology_refresh, FALSE);
	SetDlgItemInt(hdlg, IDC_EDIT_TOPOLOGY_HIGH_REFRESH_RATE, ci->topology_high_refresh, FALSE);
	SetDlgItemInt(hdlg, IDC_EDIT_IGNORE_TOPOLOGY_REFRESH_RATE, ci->ignore_topology_refresh, FALSE);
	SetDlgItemInt(hdlg, IDC_EDIT_FAILOVER_TIMEOUT, ci->failover_timeout, FALSE);

	// Dropdown Menu for Failover Type
	int failoverModeIdx = -1;
	for (int i = 0; i < sizeof(failoverModes) / sizeof(failoverModes[0]); i++) {
		if (!stricmp(ci->failover_mode, failoverModes[i].modestr)) {
			failoverModeIdx = i;
			break;
		}
	}
	for (int i = 0; i < sizeof(failoverModes) / sizeof(failoverModes[0]); i++) {
		LoadString(GetWindowInstance(hdlg), failoverModes[i].ids, buff, MEDIUM_REGISTRY_LEN);
		SendDlgItemMessage(hdlg, IDC_EDIT_FAILOVER_MODE, CB_ADDSTRING, 0, (WPARAM)buff);
	}
	SendDlgItemMessage(hdlg, IDC_EDIT_FAILOVER_MODE, CB_SETCURSEL, failoverModeIdx, (WPARAM)0);
	SetWindowSubclass(GetDlgItem(hdlg, IDC_EDIT_FAILOVER_MODE), DefSubclassProc, 0, 0);

	// Dropdown Menu for Failover Reader Host Selector Strategies
	int strategiesSize = sizeof(failoverReaderHostSelectorStrategies) / sizeof(failoverReaderHostSelectorStrategies[0]);
	int strategyIdx = -1;
	for (int i = 0; i < strategiesSize; i++) {
		if (!stricmp(ci->reader_host_selector_strategy, failoverReaderHostSelectorStrategies[i].modestr)) {
			strategyIdx = i;
			break;
		}
	}
	for (int i = 0; i < strategiesSize; i++) {
		LoadString(GetWindowInstance(hdlg), failoverReaderHostSelectorStrategies[i].ids, buff, MEDIUM_REGISTRY_LEN);
		SendDlgItemMessage(hdlg, IDC_EDIT_FAILOVER_READER_HOST_SELECTOR_STRATEGY, CB_ADDSTRING, 0, (WPARAM)buff);
	}
	SendDlgItemMessage(hdlg, IDC_EDIT_FAILOVER_READER_HOST_SELECTOR_STRATEGY, CB_SETCURSEL, strategyIdx, (WPARAM)0);
	SetWindowSubclass(GetDlgItem(hdlg, IDC_EDIT_FAILOVER_READER_HOST_SELECTOR_STRATEGY), DefSubclassProc, 0, 0);

	failover_optionsDlgEnable(hdlg, ci);

	ShowWindow(GetDlgItem(hdlg, DRV_MSG_LABEL2), enable ? SW_SHOW : SW_HIDE);

	finalize_globals(&defval);
	return 0;
}

#define	INIT_DISP_LOGVAL	2

static int
driver_options_update(HWND hdlg, ConnInfo *ci)
{
	GLOBAL_VALUES *comval;
	BOOL	bTranslated;

MYLOG(3, "entering\n");
	comval = &(ci->drivers);

	(comval->commlog = GetDlgItemInt(hdlg, DS_COMMLOG, &bTranslated, FALSE)) || bTranslated || (comval->commlog = INIT_DISP_LOGVAL);
	comval->unique_index = IsDlgButtonChecked(hdlg, DRV_UNIQUEINDEX);
	comval->use_declarefetch = IsDlgButtonChecked(hdlg, DRV_USEDECLAREFETCH);

	/* Unknown (Default) Data Type sizes */
	if (IsDlgButtonChecked(hdlg, DRV_UNKNOWN_MAX))
		comval->unknown_sizes = UNKNOWNS_AS_MAX;
	else if (IsDlgButtonChecked(hdlg, DRV_UNKNOWN_DONTKNOW))
		comval->unknown_sizes = UNKNOWNS_AS_DONTKNOW;
	else if (IsDlgButtonChecked(hdlg, DRV_UNKNOWN_LONGEST))
		comval->unknown_sizes = UNKNOWNS_AS_LONGEST;
	else
		comval->unknown_sizes = UNKNOWNS_AS_MAX;

	comval->text_as_longvarchar = IsDlgButtonChecked(hdlg, DRV_TEXT_LONGVARCHAR);
	comval->unknowns_as_longvarchar = IsDlgButtonChecked(hdlg, DRV_UNKNOWNS_LONGVARCHAR);
	comval->bools_as_char = IsDlgButtonChecked(hdlg, DRV_BOOLS_CHAR);

	comval->parse = IsDlgButtonChecked(hdlg, DRV_PARSE);

	(comval->debug = GetDlgItemInt(hdlg, DS_DEBUG, &bTranslated, FALSE)) || bTranslated || (comval->debug = INIT_DISP_LOGVAL);

	comval->fetch_max = GetDlgItemInt(hdlg, DRV_CACHE_SIZE, NULL, FALSE);
	comval->max_varchar_size = GetDlgItemInt(hdlg, DRV_VARCHAR_SIZE, NULL, FALSE);
	comval->max_longvarchar_size = GetDlgItemInt(hdlg, DRV_LONGVARCHAR_SIZE, NULL, TRUE);		/* allows for
																								 * SQL_NO_TOTAL */

	GetDlgItemText(hdlg, DRV_EXTRASYSTABLEPREFIXES, comval->extra_systable_prefixes, sizeof(comval->extra_systable_prefixes));
	ci->batch_size = GetDlgItemInt(hdlg, DS_BATCH_SIZE, NULL, FALSE);
	ci->ignore_timeout = IsDlgButtonChecked(hdlg, DS_IGNORETIMEOUT);

	ci->rds_logging_enabled = IsDlgButtonChecked(hdlg, DRV_RDS_LOGGING_ENABLED);
	ci->rds_log_threshold = GetDlgItemInt(hdlg, DRV_RDS_LOG_THRESHOLD, NULL, FALSE);
	if (ci->rds_log_threshold < 0) ci->rds_log_threshold = 0;
	if (ci->rds_log_threshold > 4) ci->rds_log_threshold = 4;

	/* fall through */
	return 0;
}

static int
limitless_options_update(HWND hdlg, ConnInfo *ci)
{
	ci->limitless_enabled = IsDlgButtonChecked(hdlg, IDC_ENABLE_LIMITLESS);
	GetDlgItemText(hdlg, IDC_LIMITLESS_MODE, ci->limitless_mode, sizeof(ci->limitless_mode));
	ci->limitless_monitor_interval_ms = GetDlgItemInt(hdlg, IDC_LIMITLESS_MONITOR_INTERVAL_MS, NULL, FALSE);
	GetDlgItemText(hdlg, IDC_LIMITLESS_SERVICE_ID, ci->limitless_service_id, sizeof(ci->limitless_service_id));

	limitless_optionsDlgEnable(hdlg, ci);
	return 0;
}
// Failover - Saves inputs from GUI into Connection Info
static int
failover_options_update(HWND hdlg, ConnInfo *ci)
{
	// Bool
	ci->enable_failover = IsDlgButtonChecked(hdlg, IDC_CHECK_ENABLE_CLUSTER_FAILOVER);
	// String
	GetDlgItemText(hdlg, IDC_EDIT_FAILOVER_MODE, ci->failover_mode, sizeof(ci->failover_mode));
	GetDlgItemText(hdlg, IDC_EDIT_FAILOVER_READER_HOST_SELECTOR_STRATEGY, ci->reader_host_selector_strategy, sizeof(ci->reader_host_selector_strategy));
	GetDlgItemText(hdlg, IDC_EDIT_HOST_PATTERN, ci->host_pattern, sizeof(ci->host_pattern));
	GetDlgItemText(hdlg, IDC_EDIT_CLUSTER_ID, ci->cluster_id, sizeof(ci->cluster_id));
	// Ints
	ci->topology_refresh = GetDlgItemInt(hdlg, IDC_EDIT_TOPOLOGY_REFRESH_RATE, NULL, FALSE);
	ci->topology_high_refresh = GetDlgItemInt(hdlg, IDC_EDIT_TOPOLOGY_HIGH_REFRESH_RATE, NULL, FALSE);
	ci->ignore_topology_refresh = GetDlgItemInt(hdlg, IDC_EDIT_IGNORE_TOPOLOGY_REFRESH_RATE, NULL, FALSE);
	ci->failover_timeout = GetDlgItemInt(hdlg, IDC_EDIT_FAILOVER_TIMEOUT, NULL, FALSE);

	failover_optionsDlgEnable(hdlg, ci);
	return 0;
}

#ifdef _HANDLE_ENLIST_IN_DTC_
static
HMODULE DtcProc(const char *procname, FARPROC *proc)
{
	HMODULE	hmodule;

	*proc = NULL;
	if (hmodule = LoadLibraryEx(GetXaLibPath(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH), NULL != hmodule)
	{
MYLOG(MIN_LOG_LEVEL, "GetProcAddress for %s\n", procname);
		*proc = GetProcAddress(hmodule, procname);
	}

	return hmodule;
}
#endif /* _HANDLE_ENLIST_IN_DTC_ */

#include <sys/stat.h>
static const char *IsAnExistingDirectory(const char *path)
{

	struct stat st;

	if (stat(path, &st) < 0)
	{
		CSTR errmsg_doesnt_exist = "doesn't exist";

		return errmsg_doesnt_exist;
	}
	if ((st.st_mode & S_IFDIR) == 0)
	{
		CSTR errmsg_isnt_a_dir = "isn't a directory";

		return errmsg_isnt_a_dir;
	}
	return NULL;
}

LRESULT			CALLBACK
global_optionsProc(HWND hdlg,
				   UINT wMsg,
				   WPARAM wParam,
				   LPARAM lParam)
{
#ifdef _HANDLE_ENLIST_IN_DTC_
	HMODULE	hmodule;
	FARPROC	proc;
#endif /* _HANDLE_ENLIST_IN_DTC_ */
	ConnInfo	*ci;
	char logdir[PATH_MAX];
	const char *logmsg;
	GLOBAL_VALUES	defval;

// if (WM_INITDIALOG == wMsg || WM_COMMAND == wMsg)
// MYLOG(MIN_LOG_LEVEL, "entering wMsg=%d\n", wMsg);
	init_globals(&defval);
	switch (wMsg)
	{
		case WM_INITDIALOG:
			SetWindowLongPtr(hdlg, DWLP_USER, lParam); /* save for test etc */
			ci = (ConnInfo *) lParam;
			getDriversDefaultsOfCi(ci, &defval);
			CheckDlgButton(hdlg, DRV_COMMLOG, getGlobalCommlog());
			CheckDlgButton(hdlg, DRV_DEBUG, getGlobalDebug());
			getLogDir(logdir, sizeof(logdir));
			SetDlgItemText(hdlg, DS_LOGDIR, logdir);
#ifdef _HANDLE_ENLIST_IN_DTC_
			hmodule = DtcProc("GetMsdtclog", &proc);
			if (proc)
			{
				INT_PTR res = (*proc)();
				CheckDlgButton(hdlg, DRV_DTCLOG, 0 != res);
			}
			else
				EnableWindow(GetDlgItem(hdlg, DRV_DTCLOG), FALSE);
			if (hmodule)
				FreeLibrary(hmodule);
#else
			ShowWindow(GetDlgItem(hdlg, DRV_DTCLOG), SW_HIDE);
#endif /* _HANDLE_ENLIST_IN_DTC_ */
			break;

		case WM_COMMAND:
			ci = (ConnInfo *) GetWindowLongPtr(hdlg, DWLP_USER);
			switch (GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK:
					getDriversDefaultsOfCi(ci, &defval);
					GetDlgItemText(hdlg, DS_LOGDIR, logdir, sizeof(logdir));
					if (logdir[0] && (logmsg = IsAnExistingDirectory(logdir)) != NULL)
					{
						MessageBox(hdlg, "Folder for logging error", logmsg, MB_ICONEXCLAMATION | MB_OK);
						break;
					}
					setGlobalCommlog(IsDlgButtonChecked(hdlg, DRV_COMMLOG));
					setGlobalDebug(IsDlgButtonChecked(hdlg, DRV_DEBUG));
					writeGlobalLogs();
					if (writeDriversDefaults(ci->drivername, &defval) < 0)
						MessageBox(hdlg, "Sorry, impossible to update the values\nWrite permission seems to be needed", "Update Error", MB_ICONEXCLAMATION | MB_OK);
					setLogDir(logdir[0] ? logdir : NULL);
#ifdef _HANDLE_ENLIST_IN_DTC_
					hmodule = DtcProc("SetMsdtclog", &proc);
					if (proc)
						(*proc)(IsDlgButtonChecked(hdlg, DRV_DTCLOG));
					if (hmodule)
						FreeLibrary(hmodule);
#endif /* _HANDLE_ENLIST_IN_DTC_ */

				case IDCANCEL:
					EndDialog(hdlg, GET_WM_COMMAND_ID(wParam, lParam) == IDOK);
					return TRUE;
			}
	}

	finalize_globals(&defval);
	return FALSE;
}

LRESULT	CALLBACK
limitless_optionsProc(HWND hdlg,
				   UINT wMsg,
				   WPARAM wParam,
				   LPARAM lParam)
{
	ConnInfo   *ci;
	char	strbuf[128];
	GLOBAL_VALUES	defval;
	init_globals(&defval);
	switch (wMsg) {
		// Initial Open
		case WM_INITDIALOG:
			SetWindowLongPtr(hdlg, DWLP_USER, lParam);		/* save for OK etc */
			ci = (ConnInfo *) lParam;
			// Window Name
			char	fbuf[64];
			STRCPY_FIXED(fbuf, "Limitless Settings (%s)");
			SPRINTF_FIXED(strbuf, fbuf, ci->dsn);
			SetWindowText(hdlg, strbuf);
			// Draw Window Options
			limitless_optionsDraw(hdlg, ci, 1, FALSE);
			break;
		// Button Presses
		case WM_COMMAND:
			ci = (ConnInfo *) GetWindowLongPtr(hdlg, DWLP_USER);
            int commandId = GET_WM_COMMAND_ID(wParam, lParam);
			switch (commandId)
			{
				case IDOK:
					limitless_options_update(hdlg, ci);

				case IDCANCEL:
					EndDialog(hdlg, GET_WM_COMMAND_ID(wParam, lParam) == IDOK);
					return TRUE;

				case IDAPPLY:
					limitless_options_update(hdlg, ci);
					SendMessage(GetWindow(hdlg, GW_OWNER), WM_COMMAND, wParam, lParam);
					break;

				case IDDEFAULTS:
					limitless_optionsDraw(hdlg, ci, 0, FALSE);
					break;

                default:
                    {
                        int controlId = commandId;
                        int notificationCode = HIWORD(wParam);

                        if (notificationCode == BN_CLICKED) {
                            switch (controlId) {
                            case IDC_ENABLE_LIMITLESS:
                                limitless_options_update(hdlg, ci);
                                break;
                            }
                        }
                    }
                    break;
			}
	}

	return FALSE;
}

// Failover - GUI Display Controls
LRESULT			CALLBACK
failover_optionsProc(HWND hdlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
	ConnInfo   *ci;
	char	strbuf[128];
	GLOBAL_VALUES	defval;
	init_globals(&defval);
	switch (wMsg) {
		// Initial Open
		case WM_INITDIALOG:
			SetWindowLongPtr(hdlg, DWLP_USER, lParam); /* save for OK etc */
			ci = (ConnInfo *) lParam;
			// Window Name
			char	fbuf[64];
			STRCPY_FIXED(fbuf, "Failover Settings (%s)");
			SPRINTF_FIXED(strbuf, fbuf, ci->dsn);
			SetWindowText(hdlg, strbuf);
			// Draw Window Options
			failover_optionsDraw(hdlg, ci, 1, FALSE);
			break;
		// Button Presses
		case WM_COMMAND:
			ci = (ConnInfo *) GetWindowLongPtr(hdlg, DWLP_USER);
			int command_id = GET_WM_COMMAND_ID(wParam, lParam);
			switch (command_id)
			{
				case IDOK:
					failover_options_update(hdlg, ci);

				case IDCANCEL:
					EndDialog(hdlg, command_id == IDOK);
					return TRUE;

				case IDAPPLY:
					failover_options_update(hdlg, ci);
					SendMessage(GetWindow(hdlg, GW_OWNER), WM_COMMAND, wParam, lParam);
					break;

				case IDDEFAULTS:
					failover_optionsDraw(hdlg, ci, 0, FALSE);
					break;

				default:
					{
						int notificationCode = HIWORD(wParam);
						if (notificationCode == BN_CLICKED 
							&& command_id == IDC_CHECK_ENABLE_CLUSTER_FAILOVER) {
							failover_options_update(hdlg, ci);
						}
					}
					break;
			}
	}

	return FALSE;
}

static void
CtrlCheckButton(HWND hdlg, int nIDcheck, int nIDint)
{
	BOOL	bTranslated;

	switch (Button_GetCheck(GetDlgItem(hdlg, nIDcheck)))
	{
		case BST_CHECKED:
			if (!GetDlgItemInt(hdlg, nIDint, &bTranslated, FALSE))
			{
				ShowWindow(GetDlgItem(hdlg, nIDint), SW_SHOW);
				if (bTranslated)
					SetDlgItemInt(hdlg, nIDint, INIT_DISP_LOGVAL, FALSE);
			}
			break;
		case BST_UNCHECKED:
			if (GetDlgItemInt(hdlg, nIDint, &bTranslated, FALSE))
			{
				ShowWindow(GetDlgItem(hdlg, nIDint), SW_HIDE);
				SetDlgItemInt(hdlg, nIDint, 0, FALSE);
			}
			break;
	}
}

LRESULT			CALLBACK
ds_options1Proc(HWND hdlg,
				   UINT wMsg,
				   WPARAM wParam,
				   LPARAM lParam)
{
	ConnInfo   *ci;
	char	strbuf[128];

// if (WM_INITDIALOG == wMsg || WM_COMMAND == wMsg)
// MYLOG(MIN_LOG_LEVEL, "entering wMsg=%d in\n", wMsg);
	switch (wMsg)
	{
		case WM_INITDIALOG:
			SetWindowLongPtr(hdlg, DWLP_USER, lParam);		/* save for OK etc */
			ci = (ConnInfo *) lParam;
			if (ci && ci->dsn && ci->dsn[0])
			{
				DWORD	cmd;
				char	fbuf[64];

				cmd = LoadString(s_hModule,
						IDS_ADVANCE_OPTION_DSN1,
						fbuf,
						sizeof(fbuf));
				if (cmd <= 0)
					STRCPY_FIXED(fbuf, "Advanced Options (%s) 1/3");
				SPRINTF_FIXED(strbuf, fbuf, ci->dsn);
				SetWindowText(hdlg, strbuf);
			}
			else
			{
				LoadString(s_hModule, IDS_ADVANCE_OPTION_CON1, strbuf, sizeof(strbuf));
				SetWindowText(hdlg, strbuf);
				ShowWindow(GetDlgItem(hdlg, IDAPPLY), SW_HIDE);
			}
			driver_optionsDraw(hdlg, ci, 1, FALSE);
			break;

		case WM_COMMAND:
			ci = (ConnInfo *) GetWindowLongPtr(hdlg, DWLP_USER);
			switch (GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK:
					driver_options_update(hdlg, ci);

				case IDCANCEL:
					EndDialog(hdlg, GET_WM_COMMAND_ID(wParam, lParam) == IDOK);
					return TRUE;

				case IDAPPLY:
					driver_options_update(hdlg, ci);
					SendMessage(GetWindow(hdlg, GW_OWNER), WM_COMMAND, wParam, lParam);
					break;

				case IDDEFAULTS:
					driver_optionsDraw(hdlg, ci, 0, FALSE);
					break;

				case ID2NDPAGE:
					driver_options_update(hdlg, ci);
					EndDialog(hdlg, FALSE);
					DialogBoxParam(s_hModule,
								   MAKEINTRESOURCE(DLG_OPTIONS_DS),
								   hdlg, ds_options2Proc, (LPARAM) ci);
					break;
				case ID3RDPAGE:
					driver_options_update(hdlg, ci);
					EndDialog(hdlg, FALSE);
					DialogBoxParam(s_hModule,
								   MAKEINTRESOURCE(DLG_OPTIONS_DS3),
								   hdlg, ds_options3Proc, (LPARAM) ci);
					break;
				case DRV_COMMLOG:
				case DS_COMMLOG:
					CtrlCheckButton(hdlg, DRV_COMMLOG, DS_COMMLOG);
					break;
				case DRV_DEBUG:
				case DS_DEBUG:
					CtrlCheckButton(hdlg, DRV_DEBUG, DS_DEBUG);
					break;
			}
	}

	return FALSE;
}

static int
ds_options_update(HWND hdlg, ConnInfo *ci)
{
	char		buf[128];

	MYLOG(MIN_LOG_LEVEL, "entering got ci=%p\n", ci);

	/* ExtraOptions */
	GetDlgItemText(hdlg, DS_EXTRA_OPTIONS, buf, sizeof(buf));
	if (!setExtraOptions(ci, buf, NULL))
	{
		MessageBox(hdlg, "ExtraOptions must be hex, decimal or octal", "Format error", MB_ICONEXCLAMATION | MB_OK);
		return 1;
	}
	/* Readonly */
	ITOA_FIXED(ci->onlyread, IsDlgButtonChecked(hdlg, DS_READONLY));

	/* Issue rollback command on error */
	if (IsDlgButtonChecked(hdlg, DS_NO_ROLLBACK))
		ci->rollback_on_error = 0;
	else if (IsDlgButtonChecked(hdlg, DS_TRANSACTION_ROLLBACK))
		ci->rollback_on_error = 1;
	else if (IsDlgButtonChecked(hdlg, DS_STATEMENT_ROLLBACK))
		ci->rollback_on_error = 2;
	else
		/* no button is checked */
		ci->rollback_on_error = -1;

	/* Int8 As */
	if (IsDlgButtonChecked(hdlg, DS_INT8_AS_DEFAULT))
		ci->int8_as = 0;
	else if (IsDlgButtonChecked(hdlg, DS_INT8_AS_BIGINT))
		ci->int8_as = SQL_BIGINT;
	else if (IsDlgButtonChecked(hdlg, DS_INT8_AS_NUMERIC))
		ci->int8_as = SQL_NUMERIC;
	else if (IsDlgButtonChecked(hdlg, DS_INT8_AS_DOUBLE))
		ci->int8_as = SQL_DOUBLE;
	else if (IsDlgButtonChecked(hdlg, DS_INT8_AS_INT4))
		ci->int8_as = SQL_INTEGER;
	else
		ci->int8_as = SQL_VARCHAR;

	/* Numeric without precision As */
	if (IsDlgButtonChecked(hdlg, DS_NUMERIC_AS_NUMERIC))
		ci->numeric_as = SQL_NUMERIC;
	else if (IsDlgButtonChecked(hdlg, DS_NUMERIC_AS_DOUBLE))
		ci->numeric_as = SQL_DOUBLE;
	else if (IsDlgButtonChecked(hdlg, DS_NUMERIC_AS_VARCHAR))
		ci->numeric_as = SQL_VARCHAR;
	else if (IsDlgButtonChecked(hdlg, DS_NUMERIC_AS_LONGVARCHAR))
		ci->numeric_as = SQL_LONGVARCHAR;
	else
		ci->numeric_as = DEFAULT_NUMERIC_AS;

	ITOA_FIXED(ci->show_system_tables, IsDlgButtonChecked(hdlg, DS_SHOWSYSTEMTABLES));

	ITOA_FIXED(ci->row_versioning, IsDlgButtonChecked(hdlg, DS_ROWVERSIONING));
	ci->lf_conversion = IsDlgButtonChecked(hdlg, DS_LFCONVERSION);
	ci->optional_errors = IsDlgButtonChecked(hdlg, DS_OPTIONALERRORS);
	ci->true_is_minus1 = IsDlgButtonChecked(hdlg, DS_TRUEISMINUS1);
	ci->allow_keyset = IsDlgButtonChecked(hdlg, DS_UPDATABLECURSORS);
	ci->use_server_side_prepare = IsDlgButtonChecked(hdlg, DS_SERVERSIDEPREPARE);
	ci->bytea_as_longvarbinary = IsDlgButtonChecked(hdlg, DS_BYTEAASLONGVARBINARY);
	ci->fetch_refcursors = IsDlgButtonChecked(hdlg, DS_FETCH_REFCURSORS);
	/*ci->lower_case_identifier = IsDlgButtonChecked(hdlg, DS_LOWERCASEIDENTIFIER);*/

	/* OID Options */
	ITOA_FIXED(ci->fake_oid_index, IsDlgButtonChecked(hdlg, DS_FAKEOIDINDEX));
	ITOA_FIXED(ci->show_oid_column, IsDlgButtonChecked(hdlg, DS_SHOWOIDCOLUMN));

	/* Datasource Connection Settings */
	{
		char conn_settings[LARGE_REGISTRY_LEN];
		GetDlgItemText(hdlg, DS_CONNSETTINGS, conn_settings, sizeof(conn_settings));
		if ('\0' != conn_settings[0])
			STR_TO_NAME(ci->conn_settings, conn_settings);
	}

	/* TCP KEEPALIVE */
	ci->disable_keepalive = IsDlgButtonChecked(hdlg, DS_DISABLE_KEEPALIVE);
	ci->extra_opts = getExtraOptions(ci);
	if (ci->disable_keepalive)
	{
		ci->keepalive_idle = -1;
		ci->keepalive_interval = -1;
	}
	else
	{
		char	temp[64];
		int	val;

		GetDlgItemText(hdlg, DS_KEEPALIVETIME, temp, sizeof(temp));
		if  (val = pg_atoi(temp), 0 == val)
			val = -1;
		ci->keepalive_idle = val;
		GetDlgItemText(hdlg, DS_KEEPALIVEINTERVAL, temp, sizeof(temp));
		if  (val = pg_atoi(temp), 0 == val)
			val = -1;
		ci->keepalive_interval = val;
	}
	return 0;
}

LRESULT			CALLBACK
ds_options2Proc(HWND hdlg,
			   UINT wMsg,
			   WPARAM wParam,
			   LPARAM lParam)
{
	ConnInfo   *ci;
	char		buf[128];
	DWORD		cmd;
	BOOL		enable;

// if (WM_INITDIALOG == wMsg || WM_COMMAND == wMsg)
// MYLOG(MIN_LOG_LEVEL, "entering wMsg=%d in\n", wMsg);
	switch (wMsg)
	{
		case WM_INITDIALOG:
			ci = (ConnInfo *) lParam;
			SetWindowLongPtr(hdlg, DWLP_USER, lParam);		/* save for OK */

			/* Change window caption */
			if (ci && ci->dsn && ci->dsn[0])
			{
				char	fbuf[64];

				cmd = LoadString(s_hModule,
						IDS_ADVANCE_OPTION_DSN2,
						fbuf,
						sizeof(fbuf));
				if (cmd <= 0)
					STRCPY_FIXED(fbuf, "Advanced Options (%s) 2/3");
				SPRINTF_FIXED(buf, fbuf, ci->dsn);
				SetWindowText(hdlg, buf);
			}
			else
			{
				LoadString(s_hModule, IDS_ADVANCE_OPTION_CON2, buf, sizeof(buf));
				SetWindowText(hdlg, buf);
				ShowWindow(GetDlgItem(hdlg, IDAPPLY), SW_HIDE);				}

			/* Readonly */
			CheckDlgButton(hdlg, DS_READONLY, pg_atoi(ci->onlyread));

			/* Protocol */
			enable = (ci->sslmode[0] == SSLLBYTE_DISABLE || ci->username[0] == '\0');

			/* How to issue Rollback */
			switch (ci->rollback_on_error)
			{
				case 0:
					CheckDlgButton(hdlg, DS_NO_ROLLBACK, 1);
					break;
				case 1:
					CheckDlgButton(hdlg, DS_TRANSACTION_ROLLBACK, 1);
					break;
				case 2:
					CheckDlgButton(hdlg, DS_STATEMENT_ROLLBACK, 1);
					break;
			}

			/* Int8 As */
			switch (ci->int8_as)
			{
				case SQL_BIGINT:
					CheckDlgButton(hdlg, DS_INT8_AS_BIGINT, 1);
					break;
				case SQL_NUMERIC:
					CheckDlgButton(hdlg, DS_INT8_AS_NUMERIC, 1);
					break;
				case SQL_VARCHAR:
					CheckDlgButton(hdlg, DS_INT8_AS_VARCHAR, 1);
					break;
				case SQL_DOUBLE:
					CheckDlgButton(hdlg, DS_INT8_AS_DOUBLE, 1);
					break;
				case SQL_INTEGER:
					CheckDlgButton(hdlg, DS_INT8_AS_INT4, 1);
					break;
				default:
					CheckDlgButton(hdlg, DS_INT8_AS_DEFAULT, 1);
			}
			/* Numeric As */
			switch (ci->numeric_as)
			{
				case SQL_NUMERIC:
					CheckDlgButton(hdlg, DS_NUMERIC_AS_NUMERIC, 1);
					break;
				case SQL_VARCHAR:
					CheckDlgButton(hdlg, DS_NUMERIC_AS_VARCHAR, 1);
					break;
				case SQL_DOUBLE:
					CheckDlgButton(hdlg, DS_NUMERIC_AS_DOUBLE, 1);
					break;
				case SQL_LONGVARCHAR:
					CheckDlgButton(hdlg, DS_NUMERIC_AS_LONGVARCHAR, 1);
					break;
				default:
					CheckDlgButton(hdlg, DS_NUMERIC_AS_DEFAULT, 1);
			}
			SPRINTF_FIXED(buf, "0x%x", getExtraOptions(ci));
			SetDlgItemText(hdlg, DS_EXTRA_OPTIONS, buf);

			CheckDlgButton(hdlg, DS_SHOWOIDCOLUMN, pg_atoi(ci->show_oid_column));
			CheckDlgButton(hdlg, DS_FAKEOIDINDEX, pg_atoi(ci->fake_oid_index));
			CheckDlgButton(hdlg, DS_ROWVERSIONING, pg_atoi(ci->row_versioning));
			CheckDlgButton(hdlg, DS_SHOWSYSTEMTABLES, pg_atoi(ci->show_system_tables));
			CheckDlgButton(hdlg, DS_LFCONVERSION, ci->lf_conversion);
			CheckDlgButton(hdlg, DS_OPTIONALERRORS, ci->optional_errors);
			CheckDlgButton(hdlg, DS_TRUEISMINUS1, ci->true_is_minus1);
			CheckDlgButton(hdlg, DS_UPDATABLECURSORS, ci->allow_keyset);
			CheckDlgButton(hdlg, DS_SERVERSIDEPREPARE, ci->use_server_side_prepare);
			CheckDlgButton(hdlg, DS_BYTEAASLONGVARBINARY, ci->bytea_as_longvarbinary);
			CheckDlgButton(hdlg, DS_FETCH_REFCURSORS, ci->fetch_refcursors);
			/*CheckDlgButton(hdlg, DS_LOWERCASEIDENTIFIER, ci->lower_case_identifier);*/

			EnableWindow(GetDlgItem(hdlg, DS_FAKEOIDINDEX), pg_atoi(ci->show_oid_column));

			/* Datasource Connection Settings */
			SetDlgItemText(hdlg, DS_CONNSETTINGS, SAFE_NAME(ci->conn_settings));
			/* KEEPALIVE */
			enable = (0 == (ci->extra_opts & BIT_DISABLE_KEEPALIVE));
			CheckDlgButton(hdlg, DS_DISABLE_KEEPALIVE, !enable);
			if (enable)
			{
				if (ci->keepalive_idle > 0)
				{
					ITOA_FIXED(buf, ci->keepalive_idle);
					SetDlgItemText(hdlg, DS_KEEPALIVETIME, buf);
				}
				if (ci->keepalive_interval > 0)
				{
					ITOA_FIXED(buf, ci->keepalive_interval);
					SetDlgItemText(hdlg, DS_KEEPALIVEINTERVAL, buf);
				}
			}
			break;

		case WM_COMMAND:
			ci = (ConnInfo *) GetWindowLongPtr(hdlg, DWLP_USER);
			switch (cmd = GET_WM_COMMAND_ID(wParam, lParam))
			{
				case DS_SHOWOIDCOLUMN:
					MYLOG(MIN_LOG_LEVEL, "WM_COMMAND: DS_SHOWOIDCOLUMN\n");
					EnableWindow(GetDlgItem(hdlg, DS_FAKEOIDINDEX), IsDlgButtonChecked(hdlg, DS_SHOWOIDCOLUMN));
					return TRUE;
				case DS_DISABLE_KEEPALIVE:
					MYLOG(MIN_LOG_LEVEL, "WM_COMMAND: DS_SHOWOIDCOLUMN\n");
					EnableWindow(GetDlgItem(hdlg, DS_KEEPALIVETIME), !IsDlgButtonChecked(hdlg, cmd));
					EnableWindow(GetDlgItem(hdlg, DS_KEEPALIVEINTERVAL), !IsDlgButtonChecked(hdlg, cmd));
					return TRUE;

				case IDOK:
					ds_options_update(hdlg, ci);
				case IDCANCEL:
					EndDialog(hdlg, IDOK == cmd);
					return TRUE;
				case IDAPPLY:
					ds_options_update(hdlg, ci);
					SendMessage(GetWindow(hdlg, GW_OWNER), WM_COMMAND, wParam, lParam);
					break;
				case ID1STPAGE:
					ds_options_update(hdlg, ci);
					EndDialog(hdlg, cmd == IDOK);
					DialogBoxParam(s_hModule,
						MAKEINTRESOURCE(DLG_OPTIONS_DRV),
						   hdlg, ds_options1Proc, (LPARAM) ci);
					break;
				case ID3RDPAGE:
					ds_options_update(hdlg, ci);
					EndDialog(hdlg, cmd == IDOK);
					DialogBoxParam(s_hModule,
						MAKEINTRESOURCE(DLG_OPTIONS_DS3),
						   hdlg, ds_options3Proc, (LPARAM) ci);
					break;
			}
	}

	return FALSE;
}

static int
ds_options3Draw(HWND hdlg, const ConnInfo *ci)
{
	BOOL	enable = TRUE;
	static BOOL defset = FALSE;

MYLOG(MIN_LOG_LEVEL, "entering\n");
#ifdef	_HANDLE_ENLIST_IN_DTC_
	switch (ci->xa_opt)
	{
		case 0:
			enable = FALSE;
			break;
		case DTC_CHECK_LINK_ONLY:
			CheckDlgButton(hdlg, DS_DTC_LINK_ONLY, 1);
			break;
		case DTC_CHECK_BEFORE_LINK:
			CheckDlgButton(hdlg, DS_DTC_SIMPLE_PRECHECK, 1);
			break;
		case DTC_CHECK_RM_CONNECTION:
			CheckDlgButton(hdlg, DS_DTC_CONFIRM_RM_CONNECTION, 1);
			break;
	}
#else
	enable = FALSE;
#endif /* _HANDLE_ENLIST_IN_DTC_ */
	if (!enable)
	{
		EnableWindow(GetDlgItem(hdlg, DS_DTC_LINK_ONLY), enable);
		EnableWindow(GetDlgItem(hdlg, DS_DTC_SIMPLE_PRECHECK), enable);
		EnableWindow(GetDlgItem(hdlg, DS_DTC_CONFIRM_RM_CONNECTION), enable);
	}
	/* Datasource libpq parameters */
	SetDlgItemText(hdlg, DS_LIBPQOPT, SAFE_NAME(ci->pqopt));

	return 0;
}

void *PQconninfoParse(const char *, char **);
void PQconninfoFree(void *);
void PQfreemem(void *ptr);
typedef void *(*PQCONNINFOPARSEPROC)(const char *, char **);
typedef void (*PQCONNINFOFREEPROC)(void *); 
typedef void (*PQFREEMEMPROC)(void *);
static int
ds_options3_update(HWND hdlg, ConnInfo *ci)
{
	void	*connOpt = NULL;
	char	*ermsg = NULL;
	char	pqopt[LARGE_REGISTRY_LEN];
	HMODULE	hmodule;
	PQCONNINFOPARSEPROC	pproc = NULL;
	PQCONNINFOFREEPROC	fproc = NULL;	
	PQFREEMEMPROC		fmproc = NULL;

	MYLOG(MIN_LOG_LEVEL, "entering got ci=%p\n", ci);

	/* Datasource libpq parameters */
	GetDlgItemText(hdlg, DS_LIBPQOPT, pqopt, sizeof(pqopt));
	if (hmodule = LoadLibraryEx("libpq.dll", NULL, LOAD_WITH_ALTERED_SEARCH_PATH), NULL != hmodule)
	{
		pproc = (PQCONNINFOPARSEPROC) GetProcAddress(hmodule, "PQconninfoParse");
		if (NULL != pproc && NULL == (connOpt= (*pproc)(pqopt, &ermsg)))
		{
			const char *logmsg = "libpq parameter error";

			MessageBox(hdlg, ermsg ? ermsg : "memory over?", logmsg, MB_ICONEXCLAMATION | MB_OK);
			if (NULL != ermsg) {
				fmproc = (PQFREEMEMPROC)GetProcAddress(hmodule, "PQfreemem");
				(*fmproc)(ermsg);
			}
			FreeLibrary(hmodule);

			return 1;
		}
		fproc = (PQCONNINFOFREEPROC) GetProcAddress(hmodule, "PQconninfoFree");
		if (fproc)
			(*fproc)(connOpt);
		FreeLibrary(hmodule);
	}
	STR_TO_NAME(ci->pqopt, pqopt);

#ifdef	_HANDLE_ENLIST_IN_DTC_
	if (IsDlgButtonChecked(hdlg, DS_DTC_LINK_ONLY))
		ci->xa_opt = DTC_CHECK_LINK_ONLY;
	else if (IsDlgButtonChecked(hdlg, DS_DTC_SIMPLE_PRECHECK))
		ci->xa_opt = DTC_CHECK_BEFORE_LINK;
	else if (IsDlgButtonChecked(hdlg, DS_DTC_CONFIRM_RM_CONNECTION))
		ci->xa_opt = DTC_CHECK_RM_CONNECTION;
	else
		ci->xa_opt = 0;
#endif /* _HANDLE_ENLIST_IN_DTC_ */

	return 0;
}

LRESULT			CALLBACK
ds_options3Proc(HWND hdlg,
			   UINT wMsg,
			   WPARAM wParam,
			   LPARAM lParam)
{
	ConnInfo   *ci, tmpInfo;
	char		buf[128];
	DWORD		cmd;

if (WM_INITDIALOG == wMsg || WM_COMMAND == wMsg)
MYLOG(MIN_LOG_LEVEL, "entering wMsg=%d\n", wMsg);
	switch (wMsg)
	{
		case WM_INITDIALOG:
			ci = (ConnInfo *) lParam;
			SetWindowLongPtr(hdlg, DWLP_USER, lParam);		/* save for OK */

			/* Change window caption */
			if (ci && ci->dsn && ci->dsn[0])
			{
				char	fbuf[64];

				cmd = LoadString(s_hModule,
						IDS_ADVANCE_OPTION_DSN3,
						fbuf,
						sizeof(fbuf));
				if (cmd <= 0)
					STRCPY_FIXED(fbuf, "Advanced Options (%s) 3/3");
				SPRINTF_FIXED(buf, fbuf, ci->dsn);
				SetWindowText(hdlg, buf);
			}
			else
			{
				LoadString(s_hModule, IDS_ADVANCE_OPTION_CON3, buf, sizeof(buf));
				SetWindowText(hdlg, buf);
				ShowWindow(GetDlgItem(hdlg, IDAPPLY), SW_HIDE);				}

			ds_options3Draw(hdlg, ci);
			break;

		case WM_COMMAND:
			ci = (ConnInfo *) GetWindowLongPtr(hdlg, DWLP_USER);
			switch (cmd = GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK:
					ds_options3_update(hdlg, ci);
				case IDCANCEL:
					EndDialog(hdlg, IDOK == cmd);
					return TRUE;
				case IDAPPLY:
					ds_options3_update(hdlg, ci);
					SendMessage(GetWindow(hdlg, GW_OWNER), WM_COMMAND, wParam, lParam);
					break;
				case IDC_TEST:
					CC_copy_conninfo(&tmpInfo, ci);
					ds_options3_update(hdlg, &tmpInfo);
					test_connection(hdlg, &tmpInfo, TRUE);
					CC_conninfo_release(&tmpInfo);
					break;
				case ID1STPAGE:
					ds_options3_update(hdlg, ci);
					EndDialog(hdlg, cmd == IDOK);
					DialogBoxParam(s_hModule,
						MAKEINTRESOURCE(DLG_OPTIONS_DRV),
						   hdlg, ds_options1Proc, (LPARAM) ci);
					break;
				case ID2NDPAGE:
					ds_options3_update(hdlg, ci);
					EndDialog(hdlg, cmd == IDOK);
					DialogBoxParam(s_hModule,
						MAKEINTRESOURCE(DLG_OPTIONS_DS),
						   hdlg, ds_options2Proc, (LPARAM) ci);
					break;
			}
	}

	return FALSE;
}


typedef	SQLRETURN (SQL_API *SQLAPIPROC)();
static int
makeDriversList(HWND lwnd, const ConnInfo *ci)
{
	HMODULE	hmodule;
	SQLHENV	henv;
	int	lcount = 0;
	LRESULT iidx;
	char	drvname[64], drvatt[128];
	SQLUSMALLINT	direction = SQL_FETCH_FIRST;
	SQLSMALLINT	drvncount, drvacount;
	SQLRETURN	ret;
	SQLAPIPROC	addr;

	hmodule = GetModuleHandle("ODBC32");
	if (!hmodule)	return lcount;
	addr = (SQLAPIPROC) GetProcAddress(hmodule, "SQLAllocEnv");
	if (!addr)	return lcount;
	ret = (*addr)(&henv);
	if (SQL_SUCCESS != ret)	return lcount;
	do
	{
		ret = SQLDrivers(henv, direction,
						 drvname, sizeof(drvname), &drvncount,
						 drvatt, sizeof(drvatt), &drvacount);
		if (SQL_SUCCESS != ret && SQL_SUCCESS_WITH_INFO != ret)
			break;
		// Load all AWS Drivers that are for PostgreSQL
		if (strnicmp(drvname, "aws", 3) == 0 && stristr(drvname, "postgresql"))
		{
			iidx = SendMessage(lwnd, LB_ADDSTRING, 0, (LPARAM) drvname);
			if (LB_ERR != iidx && stricmp(drvname, ci->drivername) == 0)
			{
				SendMessage(lwnd, LB_SETCURSEL, (WPARAM) iidx, (LPARAM) 0);
			}
			lcount++;
		}
		direction = SQL_FETCH_NEXT;
	} while (1);
	addr = (SQLAPIPROC) GetProcAddress(hmodule, "SQLFreeEnv");
	if (addr)
		(*addr)(henv);

	return lcount;
}

LRESULT		CALLBACK
manage_dsnProc(HWND hdlg, UINT wMsg,
			   WPARAM wParam, LPARAM lParam)
{
	LPSETUPDLG	lpsetupdlg;
	ConnInfo	*ci;
	HWND		lwnd;
	LRESULT		sidx;
	char		drvname[64];

	switch (wMsg)
	{
		case WM_INITDIALOG:
			SetWindowLongPtr(hdlg, DWLP_USER, lParam);
			lpsetupdlg = (LPSETUPDLG) lParam;
			ci = &lpsetupdlg->ci;
			lwnd = GetDlgItem(hdlg, IDC_DRIVER_LIST);
			makeDriversList(lwnd, ci);
			break;

		case WM_COMMAND:
			switch (GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK:
					lpsetupdlg = (LPSETUPDLG) GetWindowLongPtr(hdlg, DWLP_USER);
					lwnd = GetDlgItem(hdlg, IDC_DRIVER_LIST);
					sidx = SendMessage(lwnd, LB_GETCURSEL,
									   (WPARAM) 0, (LPARAM) 0);
					if (LB_ERR == sidx)
						return FALSE;
					sidx = SendMessage(lwnd, LB_GETTEXT,
									   (WPARAM) sidx, (LPARAM) drvname);
					if (LB_ERR == sidx)
						return FALSE;
					ChangeDriverName(hdlg, lpsetupdlg, drvname);

				case IDCANCEL:
					EndDialog(hdlg, GET_WM_COMMAND_ID(wParam, lParam) == IDOK);
					return TRUE;
			}
	}

	return FALSE;
}

#endif /* WIN32 */
