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

#include "Drive.h"
#include "Globals.h"
#include "Volume.h"

void Drive::Init(HANDLE h, DWORD i, LPWSTR dname)
{
	auto len = wcslen(dname);
	number = i;
	name = CopyString(dname, len);
	size = GetDriveSize(h);
	DetermineMediaType(h);
	DeterminePartitions(h);
}

HRESULT Drive::Open(bool isRead, DWORD desiredAccess, DWORD devFlags, HANDLE & h) const
{
	if (!isRead)
	{
		// we want to write to a disk, make sure all volumes on that disk are locked and if necessary has extended io
		for (auto &v : g_volumes)
		{
			if (!v->IsOndisk(number))
				continue;
			HANDLE hvol;
			auto hr = v->Open(desiredAccess, devFlags, hvol);
			if (hr)
				return hr;
		}
	}
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
	if (!DismountVolume(h))
		return GetLastError();
	if (!LockVolume(h))
		return GetLastError();
	return 0;
}

void Drive::DetermineMediaType(HANDLE h)
{
	auto mt = GetMediaType(h);
	isFixed = mt==FixedMedia;
	isRemovable = mt==RemovableMedia;
}

void Drive::DeterminePartitions(HANDLE h)
{
	byte buf[1024];
	auto layout = (DRIVE_LAYOUT_INFORMATION_EX *) buf;
	DWORD notUsed;
	if (!DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, nullptr, 0, layout, ARRAYSIZE(buf), &notUsed, nullptr))
		return;
	partitionStyle = layout->PartitionStyle;
	if (layout->PartitionCount==0)
		return;
	for (DWORD i=0; i<layout->PartitionCount; i++)
	{
		const auto& pe = layout->PartitionEntry[i];
		if (pe.PartitionNumber==0)
			continue;
		auto p = new Partition();
		p->disk = number;
		p->number = pe.PartitionNumber;
		p->offset = pe.StartingOffset.QuadPart;
		p->size = pe.PartitionLength.QuadPart;
		WCHAR name[MAX_PATH];
		size_t len = wsprintf(name, L"\\\\.\\PhysicalDrive%d\\Partition%d", p->disk, p->number);
		p->name = CopyString(name, len);
		shared_ptr<Partition> sp(p);
		partitions.push_back(sp);
		g_partitions.push_back(sp);
	}
}
