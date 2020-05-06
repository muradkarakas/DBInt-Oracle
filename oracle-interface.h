#pragma once

#include "..\DBInt\db-interface.h"


#define ORACLEINTERFACE_API __declspec(dllexport)


/* DDL's PRIVATE FUNCTIONS  */
void									oracleExecuteUpdateInsertStatements(DBInt_Connection *mkConnection, DBInt_Statement *stm, const char *sql);

/* DDL's PUBLIC FUNCTIONS  */
ORACLEINTERFACE_API void				oracleInitConnection(DBInt_Connection * conn);
ORACLEINTERFACE_API DBInt_Connection  * oracleCreateConnection(HANDLE heapHandle, DBInt_SupportedDatabaseType dbType, const char *dbName, const char* userName, const char *password);
ORACLEINTERFACE_API void				oracleDestroyConnection(DBInt_Connection *mkConnection);
ORACLEINTERFACE_API int					oracleIsConnectionOpen(DBInt_Connection *mkConnection);
ORACLEINTERFACE_API int					oracleIsEof(DBInt_Statement *stm);
ORACLEINTERFACE_API void				oracleFirst(DBInt_Connection * mkConnection, DBInt_Statement *stm);
ORACLEINTERFACE_API void				oracleLast(DBInt_Connection * mkConnection, DBInt_Statement *stm);
ORACLEINTERFACE_API BOOL				oracleNext(DBInt_Statement *stm);
ORACLEINTERFACE_API BOOL				oraclePrev(DBInt_Statement *stm);
ORACLEINTERFACE_API DBInt_Statement		*oracleCreateStatement(DBInt_Connection *mkConnection);
ORACLEINTERFACE_API void				oracleFreeStatement(DBInt_Connection *mkConnection, DBInt_Statement *stm);
ORACLEINTERFACE_API void				oracleSeek(DBInt_Connection *mkConnection, DBInt_Statement *stm, int rowNum);
ORACLEINTERFACE_API int					oracleGetLastError(DBInt_Connection *mkConnection);
ORACLEINTERFACE_API const char			*oracleGetLastErrorText(DBInt_Connection *mkConnection);
ORACLEINTERFACE_API BOOL				oracleCommit(DBInt_Connection *mkConnection);
/*	CALLER MUST RELEASE RETURN VALUE  */
ORACLEINTERFACE_API char *				oracleGetPrimaryKeyColumn(DBInt_Connection* mkDBConnection, const char* schemaName, const char* tableName, int position);
ORACLEINTERFACE_API BOOL				oracleRollback(DBInt_Connection *mkConnection);
ORACLEINTERFACE_API void				oracleRegisterString(DBInt_Connection * mkConnection, DBInt_Statement *stm, const char *bindVariableName, int maxLength);
ORACLEINTERFACE_API unsigned int		oracleGetAffectedRows(DBInt_Connection *mkConnection, DBInt_Statement *stm);
ORACLEINTERFACE_API void				oracleBindString(DBInt_Connection *mkDBConnection, DBInt_Statement *stm, char *bindVariableName, char *bindVariableValue, size_t valueLength);
ORACLEINTERFACE_API void				oracleBindLob(DBInt_Connection *mkDBConnection, DBInt_Statement *stm, const char *imageFileName, char *bindVariableName);
ORACLEINTERFACE_API void				oracleExecuteSelectStatement(DBInt_Connection * mkConnection, DBInt_Statement *stm, const char *sql);
ORACLEINTERFACE_API void				oracleExecuteDescribe(DBInt_Connection * mkConnection, DBInt_Statement *stm, const char *sql);
/*  CALLER MUST RELEASE RETURN VALUE */
ORACLEINTERFACE_API char				*oracleExecuteInsertStatement(DBInt_Connection * mkConnection, DBInt_Statement *stm, const char *sql);
ORACLEINTERFACE_API void				oracleExecuteDeleteStatement(DBInt_Connection * mkConnection, DBInt_Statement *stm, const char *sql);
ORACLEINTERFACE_API void				oracleExecuteUpdateStatement(DBInt_Connection * mkConnection, DBInt_Statement *stm, const char *sql);
ORACLEINTERFACE_API void				oracleExecuteAnonymousBlock(DBInt_Connection * mkConnection, DBInt_Statement *stm, const char *sql);
ORACLEINTERFACE_API void				oraclePrepare(DBInt_Connection * mkConnection, DBInt_Statement *stm, const char *sql);
ORACLEINTERFACE_API unsigned int		oracleGetColumnCount(DBInt_Connection *mkConnection, DBInt_Statement *stm);
ORACLEINTERFACE_API const char			*oracleGetColumnValueByColumnName(DBInt_Connection * mkConnection, DBInt_Statement *stm, const char *columnName);
ORACLEINTERFACE_API void				*oracleGetLob(DBInt_Connection * mkConnection, DBInt_Statement *stm, const char *columnName, DWORD *sizeOfValue);
ORACLEINTERFACE_API HTSQL_COLUMN_TYPE	oracleGetColumnType(DBInt_Connection * mkConnection, DBInt_Statement *stm, const char *columnName);
ORACLEINTERFACE_API unsigned int		oracleGetColumnSize(DBInt_Connection * mkConnection, DBInt_Statement *stm, const char *columnName);
ORACLEINTERFACE_API const char			*oracleGetColumnNameByIndex(DBInt_Connection *mkDBConnection, DBInt_Statement *stm, unsigned int index);
/* Caller must release return value */
ORACLEINTERFACE_API char				* oracleGetDatabaseName(DBInt_Connection * conn);


