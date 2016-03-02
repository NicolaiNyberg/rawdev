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

#include "Finders.h"
#include "Globals.h"

Volume * FindVolume(LPWSTR s)
{
	for (auto &v : g_volumes)
	{
		if (lstrcmp(s, v->dosName)==0) return v.get();
		else if (lstrcmp(s, v->deviceName)==0) return v.get();
		else if (lstrcmp(s, v->name)==0) return v.get();
	}
	return nullptr;
}

Drive * FindDrive(LPWSTR s)
{
	for (auto &d : g_drives)
	{
		if (lstrcmp(s, d->name)==0)
			return d.get();
	}
	return nullptr;
}

Drive * FindDrive(DWORD number)
{
	for (auto &d : g_drives)
	{
		if (d->number==number)
			return d.get();
	}
	return nullptr;
}

Partition * FindPartition(DWORD disk, UINT64 offset)
{
	for (auto &p : g_partitions)
	{
		if (p->disk==disk && p->offset==offset)
			return p.get();
	}
	return nullptr;
}

Partition * FindPartition(LPWSTR s)
{
	for (auto &p : g_partitions)
	{
		if (lstrcmp(s, p->name)==0)
			return p.get();
	}
	return nullptr;
}
