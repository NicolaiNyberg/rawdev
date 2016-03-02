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

#include <Windows.h>
#include "Args.h"
#include "Drive.h"
#include "Finders.h"
#include "Partition.h"
#include "Volume.h"

DriveList g_drives;
PartitionList g_partitions;
VolumeList g_volumes;
Args g_args;

LPCWSTR usageText = L"rawdev <-h|-lv|-lp|-cp from to>";

int Usage(HRESULT hr = 0, LPCWSTR reason = nullptr)
{
	if (reason==nullptr && hr==0)
	{
		wprintf(L"%s\n", usageText);
		wprintf(L"-lv : list [-all|-a] volumes\n");
		wprintf(L"-lp : list physical disks and partitions\n");
		wprintf(L"-cp : copy from/to disk, volume, partition, file\n");
		wprintf(L"-cp [-l length] [-so sourceOffset] [-do destOffset]\n");
		wprintf(L"      Examples of valid from/to names\n");
		wprintf(L"      \\\\?\\Volume{884d6af9-a72a-11e5-8080-005056c00008}\\\n");
		wprintf(L"      \\Device\\HarddiskVolume2\n");
		wprintf(L"      H:\\\n");
		wprintf(L"      \\\\.\\PhysicalDrive0\n");
		wprintf(L"      \\\\.\\PhysicalDrive0\\Partition1\n");
		wprintf(L"      c:\\temp\\drive0.part1.bin\n");
		wprintf(L"Example: read Master Boot Record\n");
		wprintf(L"-cp \\\\.\\PhysicalDrive0 c:\\temp\\mbr.bin -l 512\n");
		wprintf(L"Example: backup partition\n");
		wprintf(L"-cp \\\\.\\PhysicalDrive0\\Partition1 c:\\temp\\drive0.part1.bin\n");
		wprintf(L"Example: restore partition\n");
		wprintf(L"-cp c:\\temp\\drive0.part1.bin \\\\.\\PhysicalDrive0\\Partition1\n");
		wprintf(L"Example: hide data between MBR and first partition, which happens to start at offset=1048576 so length is forced to be 1048064\n");
		wprintf(L"-cp c:\\temp\\tamtam.bin \\\\.\\PhysicalDrive1 -do 512 -l 1048064\n");
	}
	else
	{
		if (hr==0)
			wprintf(L"%s\nError: %s\n", usageText, reason);
		else
			wprintf(L"%s\nHRESULT=%d Error: %s\n", usageText, hr, reason);
	}
	return 1;
}

void EnumerateDrivesAndPartitions()
{
	WCHAR name[MAX_PATH+1];
	for (DWORD i=0; i<1024; i++)
	{
		wsprintf(name, L"\\\\.\\PhysicalDrive%d", i);
		auto h = CreateFile(name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
		if (h==INVALID_HANDLE_VALUE)
			continue;
		auto d = new Drive();
		d->Init(h, i, name);
		g_drives.push_back(shared_ptr<Drive>(d));
		CloseHandle(h);
	}
}

int EnumerateVolumes()
{
	HRESULT error;
    WCHAR volname[MAX_PATH+1];
	auto hFind = FindFirstVolume(volname, ARRAYSIZE(volname));
	if (hFind==INVALID_HANDLE_VALUE)
		return Usage(GetLastError(), L"FindFirstVolume() failed");
	while(true)
	{
		auto len = wcslen(volname);
		volname[len] = 0;
		auto v = new Volume();
		v->Init(volname, len);
		g_volumes.push_back(shared_ptr<Volume>(v));
		auto hasNext = FindNextVolume(hFind, volname, ARRAYSIZE(volname));
		if (hasNext)
			continue;
		error = GetLastError();
		if (error==ERROR_NO_MORE_FILES)
			error = 0;
		break;
	}
	FindVolumeClose(hFind);
	if (error!=0)
		return Usage(error, L"ProcessLv failed");
	return 0;
}

void ListVolumes()
{
	for (auto &v : g_volumes)
	{
		if (v->size==0 && !g_args.allVolumes)
			continue;
		wprintf(L"%s\n", v->deviceName);
		wprintf(L"%s\n", v->name);
		auto size = v->size;
		wprintf(L"Volume size: %llu GB, %llu MB, %I64u bytes\n", size/1024/1024/1024, size/1024/1024, size);
		if (v->dosName) wprintf(L"Path: %s\n", v->dosName);
		if (v->label) wprintf(L"Label: %s\n", v->label);
		if (v->fileSystem) wprintf(L"FileSystem: %s\n", v->fileSystem);
		if (v->extents.size()>0)
		{
			for (auto &e : v->extents)
			{
				auto disk = e->DiskNumber;
				UINT64 offset = e->StartingOffset.QuadPart;
				auto p = FindPartition(disk, offset);
				auto aligned = p== nullptr
					? L"not aligned with partition"
					: L"aligned with partition";
				wprintf(L"\\\\.\\PhysicalDrive%d  offset=%I64u  size=%I64u  (%s)\n", disk, offset, e->ExtentLength.QuadPart, aligned); 
			}
		}
		wprintf(L"\n");
	}
}

void ListPartitions()
{
	for (auto &d : g_drives)
	{
		auto media = d->isFixed
			? L"Fixed    "
			: d->isRemovable
			? L"Removable"
			: L"Unknown  ";
		auto partStyle = d->partitionStyle==0
			? L"Mbr"
			: d->partitionStyle==1
			? L"Gpt"
			: L"Raw";
		wprintf(L"%-35s%20I64u bytes   %s  %s\n", d->name, d->size, media, partStyle);
		for (auto &p : d->partitions)
		{
			wprintf(L"%-35s%20I64u bytes   offset=%I64u\n", p->name, p->size, p->offset);
		}
	}
}

HRESULT OpenDiskOrVolumeOrFile(HANDLE & h, LPWSTR name, bool isRead, UINT64 * psize = nullptr)
{
	DWORD desiredAccess = isRead ? GENERIC_READ : GENERIC_READ|GENERIC_WRITE;
	DWORD devFlags = isRead ? FILE_FLAG_NO_BUFFERING : FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH;
	DWORD fileCreation = isRead ? OPEN_EXISTING : CREATE_ALWAYS;
	auto v = FindVolume(name);
	if (v) 
	{
		auto hr = v->Open(desiredAccess, devFlags, h);
		if (hr) return hr;
		if (psize) *psize = v->size;
		return 0;
	}
	auto d = FindDrive(name);
	if (d)
	{
		auto hr = d->Open(isRead, desiredAccess, devFlags, h);
		if (hr) return hr;
		if (psize) *psize = d->size;
		return 0;
	}
	auto p = FindPartition(name);
	if (p)
	{
		auto hr = p->open(isRead, desiredAccess, devFlags, h);
		if (hr) return hr;
		if (psize) *psize = p->size;
		return 0;
	}
	h = CreateFile(
				name,
				desiredAccess,
				FILE_SHARE_READ,
				nullptr,
				fileCreation,
				FILE_FLAG_SEQUENTIAL_SCAN,
				nullptr);
	if (h==INVALID_HANDLE_VALUE)
		return GetLastError();
	if (psize)
	{
		LARGE_INTEGER large;
		if (!GetFileSizeEx(h, &large))
			return GetLastError();
		*psize = large.QuadPart;
	}
	return 0;
}

HRESULT OpenSource(HANDLE & h, UINT64 & size)
{
	return OpenDiskOrVolumeOrFile(h, g_args.cpSource, true, &size);
}

HRESULT OpenDest(HANDLE & h)
{
	return OpenDiskOrVolumeOrFile(h, g_args.cpDest, false);
}

HRESULT AdjustSource(HANDLE h, UINT64 & size)
{
	if (g_args.length!=0)
	{
		wprintf(L"Forcing size=%I64u\n", g_args.length);
		size = g_args.length;
	}
	if (g_args.offsetSource!=0)
	{
		wprintf(L"Forcing source offset=%I64u\n", g_args.offsetSource);
		if (!SetPosition(h, g_args.offsetSource))
			return GetLastError();
	}
	return 0;
}

HRESULT AdjustDest(HANDLE h)
{
	if (g_args.offsetDest!=0)
	{
		wprintf(L"Forcing destination offset=%I64u\n", g_args.offsetDest);
		if (!SetPosition(h, g_args.offsetDest))
			return GetLastError();
	}
	return 0;
}

byte copyBuf[1024*1024];
int Copy(HANDLE hsrc, HANDLE hdst, UINT64 size)
{
	UINT64 total = 0;
	UINT64 prevGb = 0;
	while(true)
	{
		DWORD len;
		auto toRead = size - total;
		if (toRead > ARRAYSIZE(copyBuf))
			toRead = ARRAYSIZE(copyBuf);
		if (!ReadFile(hsrc, copyBuf, (DWORD)toRead, &len, nullptr))
			return GetLastError();
		if (!WriteFile(hdst, copyBuf, len, &len, nullptr))
			return GetLastError();
		total += len;
		if (total==size)
			return 0;
		auto gb = total / 1024 / 1024 / 1024;
		if (gb<=prevGb)
			continue;
		wprintf(L"Copied %I64u GB\n", gb);
		prevGb = gb;
	}
}

int Copy()
{
	auto hsrc = INVALID_HANDLE_VALUE;
	auto hdst = INVALID_HANDLE_VALUE;
	LPWSTR reason = L"OpenSource";
	UINT64 size;
	auto hr = OpenSource(hsrc, size);
	if (!hr)
	{
		hr = AdjustSource(hsrc, size);
		if (hr) return hr;
		reason = L"OpenDestination";
		hr = OpenDest(hdst);
		if (!hr)
		{
			hr = AdjustDest(hdst);
			if (hr) return hr;
			reason = L"Copy";
			hr = Copy(hsrc, hdst, size);
		}
	}
	if (hsrc!=INVALID_HANDLE_VALUE) CloseHandle(hsrc);
	if (hdst!=INVALID_HANDLE_VALUE) CloseHandle(hdst);
	if (hr)
		return Usage(hr, reason);
	return 0;
}

int wmain(int argc, LPWSTR argv[])
{
	if (argc==1) return Usage(0, L"No arguments");
	if (!g_args.Parse(argc, argv))
		return Usage();
	if (g_args.hasHelp) return Usage();
	EnumerateDrivesAndPartitions();
	auto hr = EnumerateVolumes();
	if (hr) return Usage(hr, L"EnumerateVolumes");
	if (g_args.hasLv) ListVolumes();
	else if (g_args.hasLp) ListPartitions();
	else if (g_args.hasCp) return Copy();
	else return Usage(0, L"Incorrect arguments");
	return 0;
}
