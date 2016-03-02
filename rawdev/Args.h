/*
 Copyright (c) 2016, Nicolai R. Nyberg
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ARGS_H_
#define ARGS_H_

#include <Windows.h>
#include "OsHelpers.h"
#include <cwchar>

struct Args
{
	bool hasLv;
	bool allVolumes;
	bool hasLp;
	bool hasHelp;
	bool hasCp;
	bool hasRead;
	LPWSTR cpSource;
	LPWSTR cpDest;
	UINT64 offsetSource;
	UINT64 offsetDest;
	UINT64 length;
	
	Args() { memset(this, 0, sizeof(Args)); }
	bool Parse(int argc, LPWSTR argv[])
	{
		for(auto i=1; i<argc; i++)
		{
			if (lstrcmp(argv[i], L"-lv")==0)
				hasLv = true;
			else if (lstrcmp(argv[i], L"-lp")==0)
				hasLp = true;
			else if (lstrcmp(argv[i], L"-h")==0)
				hasHelp = true;
			else if (lstrcmp(argv[i], L"-cp")==0 && (i+2)<argc)
			{
				hasCp = true;
				cpSource = CopyString(argv[i+1], wcslen(argv[i+1]));
				cpDest = CopyString(argv[i+2], wcslen(argv[i+2]));
				i += 2;
			}
			else if (lstrcmp(argv[i], L"-so")==0 && (i+1)<argc)
			{
				offsetSource = _wtoi64(argv[i+1]);
				i += 1;
			}
			else if (lstrcmp(argv[i], L"-do")==0 && (i+1)<argc)
			{
				offsetDest = _wtoi64(argv[i+1]);
				i += 1;
			}
			else if (lstrcmp(argv[i], L"-l")==0 && (i+1)<argc)
			{
				length = _wtoi64(argv[i+1]);
				i += 1;
			}
			else if (lstrcmp(argv[i], L"-all")==0 || lstrcmp(argv[i], L"-a")==0)
				allVolumes = true;
			else
			{
				wprintf(L"Unknown arg: %s\n", argv[i]);
				return false;
			}

		}
		return true;
	}
};

#endif//ARGS_H_
