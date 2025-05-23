/*-------
 * Module:			odbcapi30w.c
 *
 * Description:		This module contains UNICODE routines
 *
 * Classes:			n/a
 *
 * API functions:	SQLColAttributeW, SQLGetStmtAttrW, SQLSetStmtAttrW,
			SQLSetConnectAttrW, SQLGetConnectAttrW,
			SQLGetDescFieldW, SQLGetDescRecW, SQLGetDiagFieldW,
			SQLGetDiagRecW,
 *-------
 */

#include "psqlodbc.h"
#include "unicode_support.h"
#include <stdio.h>
#include <string.h>

#include "pgapifunc.h"
#include "connection.h"
#include "statement.h"
#include "misc.h"


RETCODE	SQL_API
SQLGetStmtAttrW(SQLHSTMT hstmt,
				SQLINTEGER	fAttribute,
				PTR		rgbValue,
				SQLINTEGER	cbValueMax,
				SQLINTEGER	*pcbValue)
{
	RETCODE	ret;
	StatementClass	*stmt = (StatementClass *) hstmt;

	MYLOG(MIN_LOG_LEVEL, "Entering\n");
	ENTER_STMT_CS((StatementClass *) hstmt);
	SC_clear_error((StatementClass *) hstmt);
	StartRollbackState(stmt);
	ret = PGAPI_GetStmtAttr(hstmt, fAttribute, rgbValue,
		cbValueMax, pcbValue);
	ret = DiscardStatementSvp(stmt, ret, FALSE);
	LEAVE_STMT_CS((StatementClass *) hstmt);
	return ret;
}

RETCODE SQL_API
SQLSetStmtAttrW(SQLHSTMT hstmt,
				SQLINTEGER	fAttribute,
				PTR		rgbValue,
				SQLINTEGER	cbValueMax)
{
	RETCODE	ret;
	StatementClass	*stmt = (StatementClass *) hstmt;

	MYLOG(MIN_LOG_LEVEL, "Entering\n");
	ENTER_STMT_CS(stmt);
	SC_clear_error(stmt);
	StartRollbackState(stmt);
	ret = PGAPI_SetStmtAttr(hstmt, fAttribute, rgbValue,
		cbValueMax);
	ret = DiscardStatementSvp(stmt, ret, FALSE);
	LEAVE_STMT_CS(stmt);
	return ret;
}

RETCODE SQL_API
SQLGetConnectAttrW(HDBC hdbc,
				   SQLINTEGER	fAttribute,
				   PTR		rgbValue,
				   SQLINTEGER	cbValueMax,
				   SQLINTEGER	*pcbValue)
{
	RETCODE	ret;

	MYLOG(MIN_LOG_LEVEL, "Entering\n");
	CC_examine_global_transaction((ConnectionClass *) hdbc);
	ENTER_CONN_CS((ConnectionClass *) hdbc);
	CC_clear_error((ConnectionClass *) hdbc);
	ret = PGAPI_GetConnectAttr(hdbc, fAttribute, rgbValue,
		cbValueMax, pcbValue);
	LEAVE_CONN_CS((ConnectionClass *) hdbc);
	return ret;
}

RETCODE SQL_API
SQLSetConnectAttrW(HDBC hdbc,
				   SQLINTEGER	fAttribute,
				   PTR		rgbValue,
				   SQLINTEGER	cbValue)
{
	RETCODE	ret;
	ConnectionClass *conn = (ConnectionClass *) hdbc;

	MYLOG(MIN_LOG_LEVEL, "Entering\n");
	CC_examine_global_transaction(conn);
	ENTER_CONN_CS(conn);
	CC_clear_error(conn);
	CC_set_in_unicode_driver(conn);
	ret = PGAPI_SetConnectAttr(hdbc, fAttribute, rgbValue,
		cbValue);
	LEAVE_CONN_CS(conn);
	return ret;
}

/*      new function */
RETCODE  SQL_API
SQLSetDescFieldW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber,
				 SQLSMALLINT FieldIdentifier, PTR Value,
				 SQLINTEGER BufferLength)
{
	RETCODE	ret;
	SQLLEN	vallen;
        char    *uval = NULL;
	BOOL	val_alloced = FALSE;

	MYLOG(MIN_LOG_LEVEL, "Entering\n");
	if (BufferLength > 0 || SQL_NTS == BufferLength)
	{
		switch (FieldIdentifier)
		{
			case SQL_DESC_BASE_COLUMN_NAME:
			case SQL_DESC_BASE_TABLE_NAME:
			case SQL_DESC_CATALOG_NAME:
			case SQL_DESC_LABEL:
			case SQL_DESC_LITERAL_PREFIX:
			case SQL_DESC_LITERAL_SUFFIX:
			case SQL_DESC_LOCAL_TYPE_NAME:
			case SQL_DESC_NAME:
			case SQL_DESC_SCHEMA_NAME:
			case SQL_DESC_TABLE_NAME:
			case SQL_DESC_TYPE_NAME:
				uval = ucs2_to_utf8(Value, BufferLength > 0 ? BufferLength / WCLEN : BufferLength, &vallen, FALSE);
				val_alloced = TRUE;
			break;
		}
	}
	if (!val_alloced)
	{
		uval = Value;
		vallen = BufferLength;
	}
	ret = PGAPI_SetDescField(DescriptorHandle, RecNumber, FieldIdentifier,
				uval, (SQLINTEGER) vallen);
	if (val_alloced)
		free(uval);
	return ret;
}

RETCODE SQL_API
SQLGetDescFieldW(SQLHDESC hdesc, SQLSMALLINT iRecord, SQLSMALLINT iField,
				 PTR rgbValue, SQLINTEGER cbValueMax,
				 SQLINTEGER *pcbValue)
{
	RETCODE	ret;
	SQLINTEGER		blen = 0, bMax,	*pcbV;
        char    *rgbV = NULL, *rgbVt;

	MYLOG(MIN_LOG_LEVEL, "Entering\n");
	switch (iField)
	{
		case SQL_DESC_BASE_COLUMN_NAME:
		case SQL_DESC_BASE_TABLE_NAME:
		case SQL_DESC_CATALOG_NAME:
		case SQL_DESC_LABEL:
		case SQL_DESC_LITERAL_PREFIX:
		case SQL_DESC_LITERAL_SUFFIX:
		case SQL_DESC_LOCAL_TYPE_NAME:
		case SQL_DESC_NAME:
		case SQL_DESC_SCHEMA_NAME:
		case SQL_DESC_TABLE_NAME:
		case SQL_DESC_TYPE_NAME:
			bMax = cbValueMax * 3 / WCLEN;
			rgbV = malloc(bMax + 1);
			pcbV = &blen;
			for (rgbVt = rgbV;; bMax = blen + 1, rgbVt = realloc(rgbV, bMax))
			{
				if (!rgbVt)
				{
					ret = SQL_ERROR;
					break;
				}
				rgbV = rgbVt;
				ret = PGAPI_GetDescField(hdesc, iRecord, iField, rgbV, bMax, pcbV);
				if (SQL_SUCCESS_WITH_INFO != ret || blen < bMax)
					break;
			}
			if (SQL_SUCCEEDED(ret))
			{
				blen = (SQLINTEGER) utf8_to_ucs2(rgbV, blen, (SQLWCHAR *) rgbValue, cbValueMax / WCLEN);
				if (SQL_SUCCESS == ret && blen * WCLEN >= cbValueMax)
				{
					ret = SQL_SUCCESS_WITH_INFO;
					DC_set_error(hdesc, STMT_TRUNCATED, "The buffer was too small for the rgbDesc.");
				}
				if (pcbValue)
					*pcbValue = blen * WCLEN;
			}
			if (rgbV)
				free(rgbV);
			break;
		default:
			rgbV = rgbValue;
			bMax = cbValueMax;
			pcbV = pcbValue;
			ret = PGAPI_GetDescField(hdesc, iRecord, iField, rgbV, bMax, pcbV);
			break;
	}

	return ret;
}

RETCODE SQL_API
SQLGetDiagRecW(SQLSMALLINT fHandleType,
			   SQLHANDLE	handle,
			   SQLSMALLINT	iRecord,
			   SQLWCHAR	*szSqlState,
			   SQLINTEGER	*pfNativeError,
			   SQLWCHAR	*szErrorMsg,
			   SQLSMALLINT	cbErrorMsgMax,
			   SQLSMALLINT	*pcbErrorMsg)
{
	RETCODE	ret;
        SQLSMALLINT	buflen, tlen;
        char    qstr_ansi[8], *mtxt = NULL;

	MYLOG(MIN_LOG_LEVEL, "Entering\n");
	buflen = 0;
        if (szErrorMsg && cbErrorMsgMax > 0)
	{
		buflen = cbErrorMsgMax;
                mtxt = malloc(buflen);
	}
	ret = PGAPI_GetDiagRec(fHandleType, handle, iRecord, (SQLCHAR *) qstr_ansi,
						   pfNativeError, (SQLCHAR *) mtxt, buflen, &tlen);
	if (SQL_SUCCEEDED(ret))
	{
		if (szSqlState)
			utf8_to_ucs2(qstr_ansi, -1, szSqlState, 6);
		if (mtxt && tlen <= cbErrorMsgMax)
		{
			SQLULEN ulen = utf8_to_ucs2_lf(mtxt, tlen, FALSE, szErrorMsg, cbErrorMsgMax, TRUE);
			if (ulen == (SQLULEN) -1)
				tlen = (SQLSMALLINT) locale_to_sqlwchar((SQLWCHAR *) szErrorMsg, mtxt, cbErrorMsgMax, FALSE);
			else
				tlen = (SQLSMALLINT) ulen;
			if (tlen >= cbErrorMsgMax)
				ret = SQL_SUCCESS_WITH_INFO;
			else if (tlen < 0)
			{
				char errc[32];

				SPRINTF_FIXED(errc, "Error: SqlState=%s", qstr_ansi);
				tlen = utf8_to_ucs2(errc, -1, szErrorMsg, cbErrorMsgMax);
			}
		}
		if (pcbErrorMsg)
			*pcbErrorMsg = tlen;
	}
	if (mtxt)
		free(mtxt);
	return ret;
}

SQLRETURN SQL_API
SQLColAttributeW(SQLHSTMT	hstmt,
				 SQLUSMALLINT	iCol,
				 SQLUSMALLINT	iField,
				 SQLPOINTER	pCharAttr,
				 SQLSMALLINT	cbCharAttrMax,
				 SQLSMALLINT	*pcbCharAttr,
#if defined(_WIN64) || defined(SQLCOLATTRIBUTE_SQLLEN)
				 SQLLEN		*pNumAttr
#else
				 SQLPOINTER	pNumAttr
#endif
	)
{
	CSTR func = "SQLColAttributeW";
	RETCODE	ret;
	StatementClass	*stmt = (StatementClass *) hstmt;
	SQLSMALLINT	*rgbL, blen = 0, bMax;
        char    *rgbD = NULL, *rgbDt;

	MYLOG(MIN_LOG_LEVEL, "Entering\n");
	if (SC_connection_lost_check(stmt, __FUNCTION__))
		return SQL_ERROR;

	ENTER_STMT_CS(stmt);
	SC_clear_error(stmt);
	StartRollbackState(stmt);
	switch (iField)
	{
		case SQL_DESC_BASE_COLUMN_NAME:
		case SQL_DESC_BASE_TABLE_NAME:
		case SQL_DESC_CATALOG_NAME:
		case SQL_DESC_LABEL:
		case SQL_DESC_LITERAL_PREFIX:
		case SQL_DESC_LITERAL_SUFFIX:
		case SQL_DESC_LOCAL_TYPE_NAME:
		case SQL_DESC_NAME:
		case SQL_DESC_SCHEMA_NAME:
		case SQL_DESC_TABLE_NAME:
		case SQL_DESC_TYPE_NAME:
		case SQL_COLUMN_NAME:
			bMax = cbCharAttrMax * 3 / WCLEN;
			rgbD = malloc(bMax);
			rgbL = &blen;
			for (rgbDt = rgbD;; bMax = blen + 1, rgbDt = realloc(rgbD, bMax))
			{
				if (!rgbDt)
				{
					ret = SQL_ERROR;
					break;
				}
				rgbD = rgbDt;
				ret = PGAPI_ColAttributes(hstmt, iCol, iField, rgbD,
					bMax, rgbL, pNumAttr);
				if (SQL_SUCCESS_WITH_INFO != ret || blen < bMax)
					break;
			}
			if (SQL_SUCCEEDED(ret))
			{
				blen = (SQLSMALLINT) utf8_to_ucs2(rgbD, blen, (SQLWCHAR *) pCharAttr, cbCharAttrMax / WCLEN);
				if (SQL_SUCCESS == ret && blen * WCLEN >= cbCharAttrMax)
				{
					ret = SQL_SUCCESS_WITH_INFO;
					SC_set_error(stmt, STMT_TRUNCATED, "The buffer was too small for the pCharAttr.", func);
				}
				if (pcbCharAttr)
					*pcbCharAttr = blen * WCLEN;
			}
			if (rgbD)
				free(rgbD);
			break;
		default:
			rgbD = pCharAttr;
			bMax = cbCharAttrMax;
			rgbL = pcbCharAttr;
			ret = PGAPI_ColAttributes(hstmt, iCol, iField, rgbD,
					bMax, rgbL, pNumAttr);
			break;
	}
	ret = DiscardStatementSvp(stmt, ret, FALSE);
	LEAVE_STMT_CS(stmt);

	return ret;
}

RETCODE SQL_API
SQLGetDiagFieldW(SQLSMALLINT	fHandleType,
				 SQLHANDLE		handle,
				 SQLSMALLINT	iRecord,
				 SQLSMALLINT	fDiagField,
				 SQLPOINTER		rgbDiagInfo,
				 SQLSMALLINT	cbDiagInfoMax,
				 SQLSMALLINT   *pcbDiagInfo)
{
	RETCODE		ret;
	SQLSMALLINT	*rgbL, blen = 0, bMax;
	char	   *rgbD = NULL, *rgbDt;

	MYLOG(MIN_LOG_LEVEL, "Entering Handle=(%u,%p) Rec=%d Id=%d info=(%p,%d)\n", fHandleType,
			handle, iRecord, fDiagField, rgbDiagInfo, cbDiagInfoMax);
	switch (fDiagField)
	{
		case SQL_DIAG_DYNAMIC_FUNCTION:
		case SQL_DIAG_CLASS_ORIGIN:
		case SQL_DIAG_CONNECTION_NAME:
		case SQL_DIAG_MESSAGE_TEXT:
		case SQL_DIAG_SERVER_NAME:
		case SQL_DIAG_SQLSTATE:
		case SQL_DIAG_SUBCLASS_ORIGIN:
			bMax = cbDiagInfoMax * 3 / WCLEN + 1;
			if (rgbD = malloc(bMax), !rgbD)
				return SQL_ERROR;
			rgbL = &blen;
			for (rgbDt = rgbD;; bMax = blen + 1, rgbDt = realloc(rgbD, bMax))
			{
				if (!rgbDt)
				{
					free(rgbD);
					return SQL_ERROR;
				}
				rgbD = rgbDt;
				ret = PGAPI_GetDiagField(fHandleType, handle, iRecord, fDiagField, rgbD,
					bMax, rgbL);
				if (SQL_SUCCESS_WITH_INFO != ret || blen < bMax)
					break;
			}
			if (SQL_SUCCEEDED(ret))
			{
				SQLULEN ulen = (SQLSMALLINT) utf8_to_ucs2_lf(rgbD, blen, FALSE, (SQLWCHAR *) rgbDiagInfo, cbDiagInfoMax / WCLEN, TRUE);
				if (ulen == (SQLULEN) -1)
					blen = (SQLSMALLINT) locale_to_sqlwchar((SQLWCHAR *) rgbDiagInfo, rgbD, cbDiagInfoMax / WCLEN, FALSE);
				else
					blen = (SQLSMALLINT) ulen;
				if (SQL_SUCCESS == ret && blen * WCLEN >= cbDiagInfoMax)
					ret = SQL_SUCCESS_WITH_INFO;
				if (pcbDiagInfo)
				{
					*pcbDiagInfo = blen * WCLEN;
				}
			}
			if (rgbD)
				free(rgbD);
			break;
		default:
			rgbD = rgbDiagInfo;
			bMax = cbDiagInfoMax;
			rgbL = pcbDiagInfo;
			ret = PGAPI_GetDiagField(fHandleType, handle, iRecord, fDiagField, rgbD,
				bMax, rgbL);
			break;
	}

	return ret;
}

/*	new function */
RETCODE		SQL_API
SQLGetDescRecW(SQLHDESC DescriptorHandle,
			  SQLSMALLINT RecNumber, SQLWCHAR *Name,
			  SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
			  SQLSMALLINT *Type, SQLSMALLINT *SubType,
			  SQLLEN *Length, SQLSMALLINT *Precision,
			  SQLSMALLINT *Scale, SQLSMALLINT *Nullable)
{
	RETCODE		ret;
	char		*NameA = NULL;
	SQLSMALLINT	nlen;

	MYLOG(MIN_LOG_LEVEL, "Entering h=%p rec=%d name=%p blen=%d\n", DescriptorHandle, RecNumber, Name, BufferLength);
	MYLOG(MIN_LOG_LEVEL, "str=%p type=%p sub=%p len=%p prec=%p scale=%p null=%p\n", StringLength, Type, SubType, Length, Precision, Scale, Nullable);

	if (BufferLength > 0)
		NameA = malloc(BufferLength);

	ret = PGAPI_GetDescRec(DescriptorHandle, RecNumber, (SQLCHAR *) NameA, BufferLength,
			&nlen, Type, SubType, Length, Precision,
			Scale, Nullable);
	if (SQL_SUCCEEDED(ret))
	{
		if (NameA && nlen <= BufferLength)
		{
			SQLULEN ulen = utf8_to_ucs2_lf(NameA, nlen, FALSE, Name, BufferLength, TRUE);
			if (ulen == (SQLULEN) -1)
				nlen = (SQLSMALLINT) locale_to_sqlwchar((SQLWCHAR *) Name, NameA, BufferLength, FALSE);
			else
				nlen = (SQLSMALLINT) ulen;
			if (nlen >= BufferLength)
				ret = SQL_SUCCESS_WITH_INFO;
		}
		if (StringLength)
			*StringLength = nlen;
	}
	if (NameA)
		free(NameA);
	return ret;
}

/*	new function */
RETCODE		SQL_API
SQLSetDescRecW(SQLHDESC DescriptorHandle,
			  SQLSMALLINT RecNumber, SQLSMALLINT Type,
			  SQLSMALLINT SubType, SQLLEN Length,
			  SQLSMALLINT Precision, SQLSMALLINT Scale,
			  PTR Data, SQLLEN *StringLength,
			  SQLLEN *Indicator)
{
	RETCODE		ret;
	SQLLEN	vallen;
	char    *uval = NULL;
	BOOL	val_alloced = FALSE;

	MYLOG(MIN_LOG_LEVEL, "Entering h=%p rec=%d type=%d sub=%d len=" FORMAT_LEN " prec=%d scale=%d data=%p\n", DescriptorHandle, RecNumber, Type, SubType, Length, Precision, Scale, Data);
	MYLOG(MIN_LOG_LEVEL, "str=%p ind=%p\n", StringLength, Indicator);
	
	if (Length > 0 || SQL_NTS == Length)
	{
		uval = ucs2_to_utf8(Data, Length > 0 ? Length / WCLEN : Length, &vallen, FALSE);
		val_alloced = TRUE;
	}
	if (!val_alloced)
	{
		uval = Data;
		vallen = Length;
	}
	ret = PGAPI_SetDescRec(DescriptorHandle, RecNumber, Type,
			SubType, Length, Precision, Scale, uval,
			&vallen, Indicator);
	if (val_alloced)
		free(uval);
	return ret;
}
