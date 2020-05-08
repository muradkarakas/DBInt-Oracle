#include "pch.h"

#include "oracle-interface.h"

#include "..\SodiumShared\SodiumShared.h"

#include <ocilib.h>

ORACLEINTERFACE_API 
char* 
oracleGetPrimaryKeyColumn(
	DBInt_Connection* mkDBConnection, 
	const char* schemaName, 
	const char* tableName, 
	int position
)
{
	char* retval = NULL;
	char positionStr[10];
	mkDBConnection->errText = NULL;
	
	mkItoa(position, positionStr);

	DBInt_Statement	* ociStatement =  oracleCreateStatement(mkDBConnection);

	char* sql = mkStrcat(mkDBConnection->heapHandle, __FILE__, __LINE__,
							"SELECT "
							"	cols.column_name, cols.position "
							"FROM "
							"	all_constraints cons, all_cons_columns cols "
							"WHERE "
							"		LOWER(cols.table_name) = LOWER('", tableName, "') "
							"	AND LOWER(cons.owner) = LOWER('", schemaName, "') "
							"	AND cons.constraint_type = 'P' "
							"	AND cons.constraint_name = cols.constraint_name "
							"	AND cons.owner = cols.owner " 
							"	AND	cols.position = ", positionStr, " ",
							"ORDER BY "
							"	cols.position",
							NULL
		);
	
	oraclePrepare(mkDBConnection, ociStatement, sql);

	oracleExecuteSelectStatement(mkDBConnection, ociStatement, sql);

	const char *tmp = oracleGetColumnValueByColumnName(mkDBConnection, ociStatement, "COLUMN_NAME");

	if (tmp) {
		retval =  mkStrdup(mkDBConnection->heapHandle, tmp, __FILE__, __LINE__);
	}
	return retval;
}

ORACLEINTERFACE_API void oracleExecuteAnonymousBlock(DBInt_Connection * conn, DBInt_Statement *stm, const char *sql) {
	oracleExecuteUpdateInsertStatements(conn, stm, sql);
}

ORACLEINTERFACE_API char * oracleGetDatabaseName(DBInt_Connection * conn) {
	char *retval = NULL;
	if (conn->connection.oracleHandle) {
		retval = (char*) OCI_GetDBName(conn->connection.oracleHandle);
		if (retval) {
			retval = mkStrdup(conn->heapHandle, retval, __FILE__, __LINE__);
		}
	}
	return retval;
}

ORACLEINTERFACE_API void oracleBindLob(DBInt_Connection *mkDBConnection, DBInt_Statement *stm, const char *imageFileName, char *bindVariableName) {
	mkDBConnection->errText = NULL;

	if (imageFileName == NULL) {
		mkDBConnection->errText = "No image file name provided";
		return;
	}

	// Opening binary file
	FILE *file = _fsopen(imageFileName, "rb", _SH_DENYNO);
	
	if (file == NULL) {
		mkDBConnection->errText = "Image file name is not accessible";
		return;
	}

	OCI_Lob *lob = NULL;
	// Getting file size 
	fseek(file, 0L, SEEK_END);
	DWORD fileSize = ftell(file);

	if (fileSize > 0) {
		// Reading file content into memory 
		void *buffer = mkMalloc(mkDBConnection->heapHandle, fileSize, __FILE__, __LINE__);
		fseek(file, 0L, SEEK_SET);
		size_t blocks_read = fread(buffer, fileSize, 1, file);

		if (blocks_read == 1) {
			lob = OCI_LobCreate(mkDBConnection->connection.oracleHandle, OCI_BLOB);

			// Write the file content to the lob 
			unsigned int bytesWritten = fileSize;
			int retVal = OCI_LobWrite2(lob, buffer, &bytesWritten, &bytesWritten);
			if (bytesWritten == fileSize && retVal) {
				BOOL ret = OCI_BindLob(stm->statement.oracle.statement, bindVariableName, lob);
				if (ret == FALSE) {
					OCI_Error *err = OCI_GetLastError();
					mkDBConnection->errText = OCI_ErrorGetString(err);
				}
			}
			else {
				// Error handling 
				//mkDBConnection->errText = ETEXT(ERR_CORE_IMAGE_CANNOT_BE_WRITEN_TO_DB);
			}
		}
		else {
			// Error handling 
			//mkDBConnection->errText = ETEXT(ERR_CORE_IMAGE_CANNOT_BE_READ);
		}
		// Free resources 
		mkFree(mkDBConnection->heapHandle, buffer);
	}

	// Free resources 
	fclose(file);
}

ORACLEINTERFACE_API void oracleDestroyConnection(DBInt_Connection *conn) {
	conn->errText = NULL;
	if (conn) {
		if (conn->connection.oracleHandle) {
			OCI_ConnectionFree(conn->connection.oracleHandle);
			conn->connection.oracleHandle = NULL;
		}
	}
}

ORACLEINTERFACE_API void oracleRegisterString(DBInt_Connection * conn, DBInt_Statement *stm, const char *bindVariableName, int maxLength) {
	conn->errText = NULL;
	OCI_Statement *lStm = stm->statement.oracle.statement;
	BOOL ret = OCI_RegisterString(lStm, MT(bindVariableName), maxLength);
	if (ret == FALSE) {
		OCI_Error *err = OCI_GetLastError();
		conn->errText = OCI_ErrorGetString(err);
	}
}

ORACLEINTERFACE_API void oracleExecuteDeleteStatement(DBInt_Connection * conn, DBInt_Statement *stm, const char *sql) {
	oracleExecuteUpdateInsertStatements(conn, stm, sql);
}

ORACLEINTERFACE_API void oracleExecuteUpdateStatement(DBInt_Connection * conn, DBInt_Statement *stm, const char *sql) {
	oracleExecuteUpdateInsertStatements(conn, stm, sql);
}

ORACLEINTERFACE_API void oracleExecuteDescribe(DBInt_Connection * conn, DBInt_Statement *stm, const char *sql) {
	conn->errText = NULL;
	stm->statement.oracle.eof = TRUE;

	int ret = OCI_Describe(stm->statement.oracle.statement, sql);

	if (ret == FALSE) {
		OCI_Error *err = OCI_GetLastError();
		conn->errText = OCI_ErrorGetString(err);
	}
	else {
		stm->statement.oracle.resultset = NULL;
	}	
}

ORACLEINTERFACE_API void oracleExecuteSelectStatement(DBInt_Connection * conn, DBInt_Statement *stm, const char *sql) {
	unsigned int rowCount = 0;
	conn->errText = NULL;
	OCI_Statement *ociStatement = stm->statement.oracle.statement;

	int ret = OCI_Execute(ociStatement);

	if (ret == FALSE) {
		OCI_Error *err = OCI_GetLastError();
		conn->errText = OCI_ErrorGetString(err);
	}
	else {
		stm->statement.oracle.resultset = OCI_GetResultset(ociStatement);
		if (stm->statement.oracle.resultset) {
			ret = OCI_FetchNext(stm->statement.oracle.resultset);
			if (ret == FALSE) {
				// let check if error occured or if there is no row returned
				OCI_Error *err = OCI_GetLastError();
				if (err) {
					/* do not modify this error checking */
					conn->errText = OCI_ErrorGetString(err);
				} 
			}
			else {
				rowCount = OCI_GetRowCount(stm->statement.oracle.resultset);
			}
			stm->statement.oracle.eof = (stm->statement.oracle.resultset == NULL) || (rowCount == 0) || (ret == FALSE);
		}
	}
}

ORACLEINTERFACE_API BOOL	oracleNext(DBInt_Statement *stm) {
	OCI_Resultset *rs = stm->statement.oracle.resultset;
	int ret = OCI_FetchNext(rs);
	stm->statement.oracle.eof = (ret == FALSE);
	return stm->statement.oracle.eof;
}

ORACLEINTERFACE_API void	oracleFirst(DBInt_Connection * conn, DBInt_Statement *stm) {
	conn->errText = NULL;
	OCI_Resultset *rs = stm->statement.oracle.resultset;
	int ret = OCI_FetchFirst(rs);
	if (ret == FALSE) {
		OCI_Error *err = OCI_GetLastError();
		conn->errText = OCI_ErrorGetString(err);
	}
}

ORACLEINTERFACE_API void	oracleLast(DBInt_Connection * conn, DBInt_Statement *stm) {
	conn->errText = NULL;
	OCI_Resultset *rs = stm->statement.oracle.resultset;
	int ret = OCI_FetchLast(rs);
	if (ret == FALSE) {
		OCI_Error *err = OCI_GetLastError();
		conn->errText = OCI_ErrorGetString(err);
	}
}


ORACLEINTERFACE_API BOOL	oraclePrev(DBInt_Statement *stm) {
	OCI_Resultset *rs = stm->statement.oracle.resultset;
	int ret = OCI_FetchPrev(rs);
	stm->statement.oracle.eof = (ret == FALSE);
	return stm->statement.oracle.eof;
}

ORACLEINTERFACE_API void oraclePrepare(DBInt_Connection * conn, DBInt_Statement *stm, const char *sql) {
	conn->errText = NULL;
	OCI_Statement *ociStatement = stm->statement.oracle.statement;
	OCI_SetFetchMode(ociStatement, OCI_SFM_SCROLLABLE);
	int ret = OCI_Prepare(ociStatement, MT(sql));
	if (ret == FALSE) {
		OCI_Error *err = OCI_GetLastError();
		conn->errText = OCI_ErrorGetString(err);
		int r = OCI_ErrorGetRow(err);
		int s = r * 2;
	}
}



ORACLEINTERFACE_API char *oracleExecuteInsertStatement(DBInt_Connection * conn, DBInt_Statement *stm, const char *sql) {
	conn->errText = NULL;
	char *retval = mkMalloc(conn->heapHandle, 15, __FILE__, __LINE__);

	OCI_Statement *ociStatement = stm->statement.oracle.statement;

	int ret = OCI_Execute(ociStatement);

	if (ret == FALSE) {
		OCI_Error *err = OCI_GetLastError();
		conn->errText = OCI_ErrorGetString(err);
	}
	else {
		stm->statement.oracle.resultset = OCI_GetResultset(ociStatement);
		int ret = OCI_FetchNext(stm->statement.oracle.resultset);
		stm->statement.oracle.eof = (ret == FALSE);
		//retval = OCI_GetString2(stm->statement.oracle.resultset, MT(":new_row_id"));
	}

	int rowCountAffected = OCI_GetAffectedRows(stm->statement.oracle.statement);
	mkItoa(rowCountAffected, retval);

	return retval;
}

ORACLEINTERFACE_API unsigned int oracleGetAffectedRows(DBInt_Connection *conn, DBInt_Statement *stm) {
	conn->errText = NULL;
	OCI_Statement *lStm = stm->statement.oracle.statement;
	unsigned int retval = OCI_GetAffectedRows(lStm);
	return retval;
}

ORACLEINTERFACE_API unsigned int oracleGetColumnCount(DBInt_Connection *conn, DBInt_Statement *stm) {
	conn->errText = NULL;
	OCI_Resultset *rs = stm->statement.oracle.resultset;
	unsigned int retval = OCI_GetColumnCount(rs);
	return retval;
}

ORACLEINTERFACE_API void oracleBindString(DBInt_Connection *mkDBConnection, DBInt_Statement *stm, char *bindVariableName, char *bindVariableValue, size_t valueLength) {
	mkDBConnection->errText = NULL;
	OCI_Statement *lStm = stm->statement.oracle.statement;
	BOOL res = OCI_BindString(lStm, bindVariableName, bindVariableValue, (unsigned int) valueLength);
	if (res == FALSE) {
		OCI_Error *err = OCI_GetLastError();
		mkDBConnection->errText = OCI_ErrorGetString(err);
	}
}

ORACLEINTERFACE_API BOOL oracleRollback(DBInt_Connection *conn) {
	conn->errText = NULL;
	int ret = OCI_Rollback(conn->connection.oracleHandle);
	if (ret == FALSE) {
		OCI_Error *err = OCI_GetLastError();
		conn->errText = OCI_ErrorGetString(err);
	}
	return (ret == TRUE);
}

ORACLEINTERFACE_API BOOL oracleCommit(DBInt_Connection *conn) {
	conn->errText = NULL;
	int ret = OCI_Commit(conn->connection.oracleHandle);
	if (ret == FALSE) {
		OCI_Error *err = OCI_GetLastError();
		conn->errText = OCI_ErrorGetString(err);
	}
	return (ret == TRUE);
}


ORACLEINTERFACE_API const char *oracleGetLastErrorText(DBInt_Connection *conn) {
	OCI_Error *err = OCI_GetLastError();
	conn->errText = OCI_ErrorGetString(err);
	return conn->errText;
}

ORACLEINTERFACE_API int oracleGetLastError(DBInt_Connection *conn) {
	OCI_Error *err = OCI_GetLastError();
	int errNo = OCI_ErrorGetOCICode(err);
	return errNo;
}

ORACLEINTERFACE_API void oracleSeek(DBInt_Connection *conn, DBInt_Statement *stm, int rowNum) {
	conn->errText = NULL;
	if (stm->statement.oracle.resultset) {
		int ret = OCI_FetchSeek(stm->statement.oracle.resultset, OCI_SFD_ABSOLUTE, rowNum);
		if (ret == FALSE) {
			OCI_Error *err = OCI_GetLastError();
			if (err) {
				conn->errText = OCI_ErrorGetString(err);
			}
		}
	}	
}

ORACLEINTERFACE_API DBInt_Statement	*oracleCreateStatement(DBInt_Connection *conn) {
	DBInt_Statement	*retObj = (DBInt_Statement *) mkMalloc(conn->heapHandle, sizeof(DBInt_Statement), __FILE__, __LINE__);
	OCI_Statement *stm = OCI_StatementCreate(conn->connection.oracleHandle);
	retObj->statement.oracle.statement = stm;
	retObj->statement.oracle.resultset = NULL;	
	conn->errText = NULL;
	conn->err = FALSE;
	return retObj;
}


ORACLEINTERFACE_API void	oracleFreeStatement(DBInt_Connection *conn, DBInt_Statement *stm) {
	if (stm) {
		if (stm->statement.oracle.statement) {
			OCI_StatementFree(stm->statement.oracle.statement);
			stm->statement.oracle.statement = NULL;
		}
		if (stm->statement.oracle.resultset) {
			stm->statement.oracle.resultset = NULL;
		}
		mkFree(conn->heapHandle, stm);
	}
}


ORACLEINTERFACE_API int	oracleIsEof(DBInt_Statement *stm) {
	return stm->statement.oracle.eof;
}

ORACLEINTERFACE_API void *oracleGetLob(DBInt_Connection * conn, DBInt_Statement *stm, const char *columnName, DWORD *sizeOfValue) {
	
	conn->errText = NULL;
	void *lobContent = NULL;
	*sizeOfValue = 0;

	OCI_Resultset *rs = stm->statement.oracle.resultset;

	OCI_Column* ociColumn = OCI_GetColumn2(rs, columnName);

	unsigned int colType = OCI_ColumnGetType(ociColumn);
	
	if (colType == OCI_CDT_LOB) {
		
		// Column type is LOB
		// We will encode its content to Base64 and then return 
		OCI_Lob *lob = OCI_GetLob2(rs, columnName);
		if (lob) {
			DWORD len = (DWORD)OCI_LobGetLength(lob);
			if (len > 0) {				
				lobContent = mkMalloc(conn->heapHandle, len, __FILE__, __LINE__);
				DWORD numOfBytes = OCI_LobRead(lob, lobContent, (unsigned int)len);
				if (numOfBytes == len) {
					*sizeOfValue = len;
				}
			}
		}
	}
	else {
		conn->errText = "Column is not a type of LOB";
	}
	return lobContent;
}

ORACLEINTERFACE_API const char *oracleGetColumnNameByIndex(DBInt_Connection *mkDBConnection, DBInt_Statement *stm, unsigned int index) {
	OCI_Column *ociCol = OCI_GetColumn(stm->statement.oracle.resultset, index+1);
	const char *columnName = OCI_GetColumnName(ociCol);
	return columnName;
}

ORACLEINTERFACE_API const char *oracleGetColumnValueByColumnName(DBInt_Connection * conn, DBInt_Statement *stm, const char *columnName) {
	
	const char *retVal = NULL;
	conn->errText = NULL;
	OCI_Resultset *rs = stm->statement.oracle.resultset;

	OCI_Column* ociColumn = OCI_GetColumn2(rs, columnName);
	unsigned int colType = OCI_ColumnGetType(ociColumn);
	if (colType == OCI_CDT_LOB) {
		// Column type is LOB
		conn->errText = "For lob columns, use appropriate function.";
	}
	else {
		// Column type is text based. No conversation/encoding needed. Returning column value. 
		retVal = OCI_GetString2(rs, columnName);
	}

	return retVal;
}

ORACLEINTERFACE_API unsigned int oracleGetColumnSize(DBInt_Connection * conn, DBInt_Statement *stm, const char *columnName) {
	conn->errText = NULL;
	OCI_Resultset *rs = stm->statement.oracle.resultset;
	OCI_Column* ociColumn = OCI_GetColumn2(rs, columnName);
	unsigned int colSize = OCI_ColumnGetSize(ociColumn);
	return colSize;
}

ORACLEINTERFACE_API 
SODIUM_DATABASE_COLUMN_TYPE 
oracleGetColumnType(
	DBInt_Connection * conn, 
	DBInt_Statement * stm, 
	const char * columnName
)
{
	SODIUM_DATABASE_COLUMN_TYPE  retVal = HTSQL_COLUMN_TYPE_NOTSET;
	conn->errText = NULL;
	OCI_Resultset *rs = stm->statement.oracle.resultset;
	OCI_Column* ociColumn = OCI_GetColumn2(rs, columnName);
	unsigned int colType = OCI_ColumnGetType(ociColumn);
	switch (colType) {
		case OCI_CDT_TIMESTAMP:
		case OCI_CDT_DATETIME: {
			retVal = HTSQL_COLUMN_TYPE_DATE;
			break;
		}
		case OCI_CDT_NUMERIC: {
			retVal = HTSQL_COLUMN_TYPE_NUMBER;
			break;
		}
		case OCI_CDT_RAW:
		case OCI_CDT_LONG:
		case OCI_CDT_LOB: {
			retVal = HTSQL_COLUMN_TYPE_LOB;
			break;
		}
		case OCI_CDT_TEXT: {
			retVal = HTSQL_COLUMN_TYPE_TEXT;
			break;
		}
		case OCI_CDT_OBJECT: {
			retVal = HTSQL_COLUMN_TYPE_OBJECT;
			break;
		}
	}
	return retVal;
}

ORACLEINTERFACE_API void oracleInitConnection(DBInt_Connection * conn) {

}

ORACLEINTERFACE_API 
DBInt_Connection * 
oracleCreateConnection(
	HANDLE heapHandle, 
	DBInt_SupportedDatabaseType dbType, 
	const char * dbName, 
	const char * userName, 
	const char * password
)
{	
	DBInt_Connection *conn = (DBInt_Connection *) mkMalloc(heapHandle, sizeof(DBInt_Connection), __FILE__, __LINE__);
	conn->dbType = SODIUM_ORACLE_SUPPORT;
	conn->errText = NULL;
	conn->heapHandle = heapHandle;
	conn->err = FALSE;
	strcpy_s(conn->hostName, HOST_NAME_LENGTH, "");

	if (IsOracleClientDriverLoaded == TRUE) {
		OCI_Connection* ociConnection = OCI_ConnectionCreate(dbName, userName, password, OCI_SESSION_DEFAULT);

		if (ociConnection) {
			conn->connection.oracleHandle = ociConnection;
		}
		else {
			OCI_Error* err = OCI_GetLastError();
			conn->errText = OCI_ErrorGetString(err);
			conn->err = TRUE;
		}

	} 
	else 
	{
		conn->err = TRUE;
		conn->errText = "Oracle client driver not loaded";
	}
	
	return conn;
}

ORACLEINTERFACE_API int oracleIsConnectionOpen(DBInt_Connection * conn)
{
	if (conn == NULL) {
		return FALSE;
	}
	if (conn->connection.oracleHandle == NULL) {
		return FALSE;
	}

	int retVal = OCI_IsConnected(conn->connection.oracleHandle);
	return retVal;
}


void oracleExecuteUpdateInsertStatements(DBInt_Connection * conn, DBInt_Statement *stm, const char *sql) {
	OCI_Statement *ociStatement = stm->statement.oracle.statement;

	int ret = OCI_Execute(ociStatement);

	if (ret == FALSE) {
		OCI_Error *err = OCI_GetLastError();
		conn->errText = OCI_ErrorGetString(err);
	}
	else {
		stm->statement.oracle.resultset = NULL;
		stm->statement.oracle.eof = TRUE;
	}
}