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

#include "OsHelpers.h"

LPWSTR CopyString(LPWSTR s, size_t len) {
	auto d = lstrcpy(new WCHAR[len+1], s);
	d[len] = 0;
	return d;
}

bool DismountVolume(HANDLE h)
{
	DWORD notUsed;
	return DeviceIoControl(h, FSCTL_DISMOUNT_VOLUME, nullptr, 0, nullptr, 0, &notUsed, nullptr)==TRUE;
}

bool LockVolume(HANDLE h)
{
	DWORD notUsed;
	return DeviceIoControl(h, FSCTL_LOCK_VOLUME, nullptr, 0, nullptr, 0, &notUsed, nullptr)==TRUE;
}

bool AllowExtendedIo(HANDLE h)
{
	DWORD notUsed;
	return DeviceIoControl(h, FSCTL_ALLOW_EXTENDED_DASD_IO, nullptr, 0, nullptr, 0, &notUsed, nullptr)==TRUE;
}

bool SetPosition(HANDLE h, UINT64 pos)
{
	LARGE_INTEGER toMove;
	toMove.QuadPart = pos;
	return SetFilePointerEx(h, toMove, nullptr, FILE_BEGIN)==TRUE;
}

UINT64 GetDriveSize(HANDLE h)
{
	GET_LENGTH_INFORMATION info;
	DWORD notUsed;
	if (!DeviceIoControl(h, IOCTL_DISK_GET_LENGTH_INFO, nullptr, 0, &info, sizeof(info), &notUsed, nullptr))
		return 0;
	return info.Length.QuadPart;
}

MEDIA_TYPE GetMediaType(HANDLE h)
{
	DISK_GEOMETRY dg;
	DWORD notUsed;
	if (!DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_GEOMETRY, nullptr, 0, &dg, sizeof(dg), &notUsed, nullptr))
		return Unknown;
	return dg.MediaType;
}
