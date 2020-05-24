/**
 * This file is part of Sodium Language project
 *
 * Copyright © 2020 Murad Karakaþ <muradkarakas@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License v3.0
 * as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 *	https://choosealicense.com/licenses/gpl-3.0/
 */

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