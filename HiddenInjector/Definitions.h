#pragma once

// Driver attributes.

#define SERVICE_NAME L"TrueSight"
#define SYMBOLIC_LINK L"\\\\.\\TrueSight"
#define DRIVER_PATH L"%SystemRoot%\\System32\\drivers\\TrueSight.sys"

// Driver IOCTLS.

#define IOCTL_READ_MEMORY 0x22E050
#define IOCTL_WRITE_MEMORY 0x22E014