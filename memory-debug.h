#pragma once

#include "oclib\include\ocilib.h"

#ifdef __cplusplus
extern "C" {
#endif

	__declspec(dllexport)		void	htsqlDebug_AddErrorEntry(const char *errtext, const char *sourceFile, int sourceLine);
	__declspec(dllexport)		void	htsqlDebug_DeleteStatementEntry(void *memptr, const char *sourceFile, int sourceLine);
	__declspec(dllexport)		void	htsqlDebug_AddStatementEntry(void *memptr, const char* sourceFile, int sourceLine);
	__declspec(dllexport)		void	htsqlDebug_AddMallocEntry(HANDLE pHeapHandle, void *memptr, size_t memsize, const char *sourceFile, int sourceLine);
	__declspec(dllexport)		void	htsqlDebug_DeleteMallocEntry(HANDLE pHeapHandle, void *memptr);
	__declspec(dllexport)		void	htsqlDebug_UpdateMallocEntry(HANDLE pHeapHandle, void *oldptr, void *newptr, size_t memsize);

	char*	myItoa(size_t value, char* result);

#ifdef __cplusplus
}
#endif