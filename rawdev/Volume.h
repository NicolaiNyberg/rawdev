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

#ifndef VOLUME_H_
#define VOLUME_H_

#include <memory>
#include <list>
#include <Windows.h>
#include "OsHelpers.h"

typedef list<shared_ptr<DISK_EXTENT>> DiskExtentList;
struct Volume
{
	LPWSTR name;
	LPWSTR deviceName;
	UINT64 size;
	LPWSTR dosName;
	LPWSTR label;
	LPWSTR fileSystem;
	DiskExtentList extents;

	Volume()
	{
		name = nullptr;
		deviceName = nullptr;
		dosName = nullptr;
		size = 0;
		label = nullptr;
		fileSystem = nullptr;
	}

	~Volume()
	{
		if (name) delete name;
		if (deviceName) delete deviceName;
		if (dosName) delete dosName;
		if (label) delete label;
		if (fileSystem) delete fileSystem;
	}

	void Init(LPWSTR volname, size_t len)
	{
		name = CopyString(volname, len);
		GetDeviceName();
		GetDosName();
		GetDriveInformation();
	}

	void GetDeviceName()
	{
		auto len = wcslen(name);
		name[len-1] = 0;
		WCHAR devname[MAX_PATH+1];
		auto devlen = QueryDosDevice(&(name[4]), devname, ARRAYSIZE(devname));
		if (devlen>0)
		{
			devname[devlen] = 0;
			deviceName = CopyString(devname, devlen);
		}
		name[len-1] = L'\\';
	}

	void GetDosName()
	{
		WCHAR path[MAX_PATH+1];
		DWORD len;
		if (!GetVolumePathNamesForVolumeName(name, path, ARRAYSIZE(path), &len))
			return;
		path[len] = 0;
		dosName = CopyString(path, len);
	}

	void GetDriveInformation()
	{
		auto len = wcslen(name);
		name[len-1] = 0;
		auto h = CreateFile(
						name,
						GENERIC_READ,
						FILE_SHARE_WRITE | FILE_SHARE_READ,
						nullptr,
						OPEN_EXISTING,
						0,
						nullptr);
		if (h!=INVALID_HANDLE_VALUE)
		{
			size = GetDriveSize(h);
			GetLabelAndFileSystem(h);
			GetExtents(h);
			CloseHandle(h);
		}
		name[len-1] = L'\\';
	}

	void GetExtents(HANDLE h)
	{
		byte pvebuf[1024];
		auto pve = (VOLUME_DISK_EXTENTS *) pvebuf;
		DWORD reqSize = ARRAYSIZE(pvebuf);
		if (!DeviceIoControl(h, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, pve, reqSize, &reqSize, nullptr))
			return;
		for (DWORD i=0; i<pve->NumberOfDiskExtents; i++)
		{
			auto de = new DISK_EXTENT();
			de->DiskNumber = pve->Extents[i].DiskNumber;
			de->ExtentLength = pve->Extents[i].ExtentLength;
			de->StartingOffset = pve->Extents[i].StartingOffset;
			extents.push_back(shared_ptr<DISK_EXTENT>(de));
		}
	}

	void GetLabelAndFileSystem(HANDLE h)
	{
		WCHAR tmpLabel[MAX_PATH+1];
		WCHAR tmpFs[MAX_PATH+1];
		if (!GetVolumeInformationByHandleW(h, tmpLabel, ARRAYSIZE(tmpLabel), nullptr, nullptr, nullptr, tmpFs, ARRAYSIZE(tmpFs)))
			return;
		label = CopyString(tmpLabel, wcslen(tmpLabel));
		fileSystem = CopyString(tmpFs, wcslen(tmpFs));
	}

	bool IsRawFileSystem() const { return lstrcmp(L"RAW", fileSystem)==0; }
	bool IsOndisk(DWORD disk)
	{
		for (auto &de : extents)
			if (de->DiskNumber==disk)
				return true;
		return false;
	}
	HRESULT Open(DWORD desiredAccess, DWORD devFlags, HANDLE & h) const
	{
		auto len = wcslen(name);
		name[len-1] = 0;
		h = CreateFile(
					name,
					desiredAccess,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					nullptr,
					OPEN_EXISTING,
					devFlags,
					nullptr);
		if (h==INVALID_HANDLE_VALUE)
			return GetLastError();
		if (!LockVolume(h))
			return GetLastError();
		if (!IsRawFileSystem() && !AllowExtendedIo(h))
			return GetLastError();
		return 0;
	}
};
typedef list<shared_ptr<Volume>> VolumeList;

#endif//VOLUME_H_
