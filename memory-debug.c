#include "pch.h"

#include "memory-debug.h"


#ifdef HTSQL_MEMORY_LEAK_DEBUG

OCI_Connection *cn = NULL;

char* myItoa(size_t value, char* result) {

	int base = 10;

	// check that the base if valid
	if (base < 2 || base > 36) { *result = '\0'; return result; }

	char* ptr = result, *ptr1 = result, tmp_char;
	size_t tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
	} while (value);

	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while (ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

__declspec(dllexport) void htsqlDebug_AddErrorEntry(const char *errtext, const char *sourceFile, int sourceLine) {

	char *sql = malloc(750);
	char *sizestr = malloc(50);
	char *linenostr = malloc(50);

	OCI_Statement  *st;

	if (cn == NULL) {
		cn = OCI_ConnectionCreate("XE", "htsql", "htsql", OCI_SESSION_DEFAULT);
	}
	st = OCI_StatementCreate(cn);

	strcpy_s(sql, 750, "insert into ERROR(HATA, FILENAME, LINENO) values ('");
	strcat_s(sql, 750, errtext);
	strcat_s(sql, 750, "',");
	strcat_s(sql, 750, sourceFile);
	strcat_s(sql, 750, "',");
	myItoa(sourceLine, linenostr);
	strcat_s(sql, 750, linenostr);
	strcat_s(sql, 750, ")");

	OCI_Prepare(st, sql);

	boolean ret = OCI_Execute(st);
	if (ret == FALSE) {

	}
	int rowcount = OCI_GetAffectedRows(st);
	if (rowcount != 1) {

	}
	OCI_Commit(cn);

	OCI_StatementFree(st);

	free(sql);
	free(sizestr);
	free(linenostr);
}

__declspec(dllexport) void htsqlDebug_AddMallocEntry(HANDLE pHeapHandle, void *memptr, size_t memsize, const char *sourceFile, int sourceLine) {
	char *sql = malloc(750);
	char *address = malloc(50);
	char *sizestr = malloc(50);
	char *linenostr = malloc(50);

	OCI_Statement  *st;

	if (cn == NULL) {
		cn = OCI_ConnectionCreate("XE", "htsql", "htsql", OCI_SESSION_DEFAULT);
	}
	st = OCI_StatementCreate(cn);

	sprintf_s(address, 50, "%p", memptr);
	strcpy_s(sql, 750, "insert into MALLOC(ADDRESS, MEMSIZE, FILENAME, LINENO) values ('");
	strcat_s(sql, 750, address);
	strcat_s(sql, 750, "',");
	myItoa(memsize, sizestr);
	strcat_s(sql, 750, sizestr);
	strcat_s(sql, 750, ",'");
	strcat_s(sql, 750, sourceFile);
	strcat_s(sql, 750, "',");
	myItoa(sourceLine, linenostr);
	strcat_s(sql, 750, linenostr);
	strcat_s(sql, 750, ")");

	OCI_Prepare(st, sql);

	boolean ret = OCI_Execute(st);
	if (ret == FALSE) {
		htsqlDebug_AddErrorEntry("addMallocEntry", sourceFile, sourceLine);
	}
	int rowcount = OCI_GetAffectedRows(st);
	if (rowcount != 1) {
		htsqlDebug_AddErrorEntry("addMallocEntry satir sayisi 1'den farklı", sourceFile, sourceLine);
	}
	OCI_Commit(cn);

	OCI_StatementFree(st);

	free(sql);
	free(address);
	free(sizestr);
	free(linenostr);
}

__declspec(dllexport) void htsqlDebug_DeleteMallocEntry(HANDLE pHeapHandle, void *memptr) {
	char *sql = malloc(250);
	char *address = malloc(50);
	char *sizestr = malloc(50);

	OCI_Statement  *st;

	st = OCI_StatementCreate(cn);

	sprintf_s(address, 50, "%p", memptr);
	strcpy_s(sql, 250, "delete MALLOC where ADDRESS = '");
	strcat_s(sql, 250, address);
	strcat_s(sql, 250, "'");

	OCI_Prepare(st, sql);

	boolean ret = OCI_Execute(st);
	if (ret == FALSE) {
		htsqlDebug_AddErrorEntry("deleteMallocEntry", "", 0);
	}
	int rowcount = OCI_GetAffectedRows(st);
	if (rowcount != 1) {
		htsqlDebug_AddErrorEntry("deleteMallocEntry satir sayisi 1'den farklı", "", 0);
	}
	OCI_Commit(cn);

	OCI_StatementFree(st);

	free(sql);
	free(address);
	free(sizestr);
}

__declspec(dllexport) void htsqlDebug_UpdateMallocEntry(HANDLE pHeapHandle, void *oldptr, void *newptr, size_t memsize) {
	if (oldptr == newptr) {
		return;
	}

	char *sql = malloc(250);
	char *oldAddress = malloc(50);
	char *newAddress = malloc(50);
	char *sizestr = malloc(50);

	OCI_Statement  *st;

	st = OCI_StatementCreate(cn);

	sprintf_s(newAddress, 50, "%p", newptr);
	strcpy_s(sql, 250, "update MALLOC set ADDRESS = '");
	strcat_s(sql, 250, newAddress);
	strcat_s(sql, 250, "' where address = '");
	sprintf_s(oldAddress, 50, "%p", oldptr);
	strcat_s(sql, 250, oldAddress);
	strcat_s(sql, 250, "'");

	OCI_Prepare(st, sql);

	boolean ret = OCI_Execute(st);
	if (ret == FALSE) {
		htsqlDebug_AddErrorEntry("updateMallocEntry", "", 0);
	}
	int rowcount = OCI_GetAffectedRows(st);
	if (rowcount != 1) {
		htsqlDebug_AddErrorEntry("updateMallocEntry satir sayisi 1'den farklı", "", 0);
	}
	OCI_Commit(cn);

	OCI_StatementFree(st);

	free(sql);
	free(oldAddress);
	free(newAddress);
	free(sizestr);
}

__declspec(dllexport) void htsqlDebug_AddStatementEntry(void *memptr, const char* sourceFile, int sourceLine) {
	char *sql = malloc(750);
	int memsize = 0;
	char *address = malloc(50);
	char *sizestr = malloc(50);
	char *linenostr = malloc(50);

	OCI_Statement  *st;

	if (cn == NULL) {
		cn = OCI_ConnectionCreate("XE", "htsql", "htsql", OCI_SESSION_DEFAULT);
	}
	st = OCI_StatementCreate(cn);

	sprintf_s(address, 50, "%p", memptr);
	strcpy_s(sql, 750, "insert into MALLOC(ADDRESS, MEMSIZE, FILENAME, LINENO) values ('");
	strcat_s(sql, 750, address);
	strcat_s(sql, 750, "',");
	myItoa(memsize, sizestr);
	strcat_s(sql, 750, sizestr);
	strcat_s(sql, 750, ",'");
	strcat_s(sql, 750, sourceFile);
	strcat_s(sql, 750, "',");
	myItoa(sourceLine, linenostr);
	strcat_s(sql, 750, linenostr);
	strcat_s(sql, 750, ")");

	OCI_Prepare(st, sql);

	boolean ret = OCI_Execute(st);
	if (ret == FALSE) {
		htsqlDebug_AddErrorEntry("addStatementEntry", sourceFile, sourceLine);
	}
	int rowcount = OCI_GetAffectedRows(st);
	if (rowcount != 1) {
		htsqlDebug_AddErrorEntry("addStatementEntry satir sayisi 1'den farklı", sourceFile, sourceLine);
	}
	OCI_Commit(cn);

	OCI_StatementFree(st);

	free(sql);
	free(address);
	free(sizestr);
	free(linenostr);
}

__declspec(dllexport) void htsqlDebug_DeleteStatementEntry(void *memptr, const char *sourceFile, int sourceLine) {
	char *sql = malloc(250);
	char *address = malloc(50);
	char *sizestr = malloc(50);

	OCI_Statement  *st;

	st = OCI_StatementCreate(cn);

	sprintf_s(address, 50, "%p", memptr);
	strcpy_s(sql, 250, "delete MALLOC where ADDRESS = '");
	strcat_s(sql, 250, address);
	strcat_s(sql, 250, "' and ");
	strcat_s(sql, 250, "filename = '");
	strcat_s(sql, 250, sourceFile);
	strcat_s(sql, 250, "'");

	OCI_Prepare(st, sql);

	boolean ret = OCI_Execute(st);
	if (ret == FALSE) {
		OCI_Error *err = OCI_GetLastError();
		const char *errstr = OCI_ErrorGetString(err);
	}
	int rowcount = OCI_GetAffectedRows(st);
	if (rowcount != 1) {
		char *a = malloc(10);
		free(a);
	}
	OCI_Commit(cn);

	OCI_StatementFree(st);

	free(sql);
	free(address);
	free(sizestr);
}
#endif // HTSQL_MEMORY_LEAK_DEBUG


