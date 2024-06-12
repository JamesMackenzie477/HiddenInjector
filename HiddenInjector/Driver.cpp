#include "driver.h"

Tools::Kernel::Driver::Driver() { Load(); }

Tools::Kernel::Driver::~Driver() { Unload(); }

bool Tools::Kernel::Driver::Load()
{
	WCHAR path[MAX_PATH];
	DWORD bytes;
	SERVICE_STATUS status;

	// Writes the driver to the disk (overwrites existing).

	if (!ExpandEnvironmentStringsW(DRIVER_PATH, path, sizeof(path))) return false;

	auto hFile = CreateFileW(path, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (hFile == INVALID_HANDLE_VALUE) return false;

	auto result = WriteFile(hFile, DriverBytes, sizeof(DriverBytes), &bytes, nullptr);

	CloseHandle(hFile);

	if (!result) return false;

	// Opens the service manager.

	auto hManager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE | SC_MANAGER_CONNECT);

	if (!hManager) return false;

	// Removes the existing service.

	auto hExists = OpenServiceW(hManager, SERVICE_NAME, SERVICE_STOP | DELETE);

	if (hExists)
	{
		auto result = (!ControlService(hExists, SERVICE_CONTROL_STOP, &status) && GetLastError() != ERROR_SERVICE_NOT_ACTIVE) || (!DeleteService(hExists) && GetLastError() != ERROR_SERVICE_MARKED_FOR_DELETE);

		CloseServiceHandle(hExists);

		if (result) return false;
	}

	// Creates the driver service.

	ServiceHandle = CreateServiceW(hManager, SERVICE_NAME, SERVICE_NAME, SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, path, nullptr, nullptr, nullptr, nullptr, nullptr);

	CloseServiceHandle(hManager);

	if (!service && GetLastError() != ERROR_SERVICE_EXISTS) return false;

	// Starts the driver service.

	if (!StartServiceW(service, 0, nullptr) && GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
	{
		DeleteService(service);

		CloseServiceHandle(service);

		return false;
	}

	// Opens the driver.

	DriverHandle = CreateFileW(SYMBOLIC_LINK, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (DriverHandle == INVALID_HANDLE_VALUE)
	{
		ControlService(service, SERVICE_CONTROL_STOP, &status);

		DeleteService(service);

		CloseServiceHandle(service);

		return false;
	}

	return true;
}

bool Tools::Kernel::Driver::Unload() const
{
	SERVICE_STATUS status;

	// Closes the driver.

	if (!CloseHandle(DriverHandle)) return false;

	// Stops the driver service.

	if (!ControlService(ServiceHandle, SERVICE_CONTROL_STOP, &status)) return false;

	// Deletes the driver service.

	if (!DeleteService(ServiceHandle)) return false;

	// Closes the service.

	if (!CloseServiceHandle(ServiceHandle)) return false;

	return true;
}

Tools::Kernel::Driver& Tools::Kernel::Driver::GetInstance()
{
	static Driver instance;
	return instance;
}

bool Tools::Kernel::Driver::ReadMemory(DWORD64 Address, PVOID Buffer, DWORD32 Length) const
{
	READ_MEMORY_PARAMS params;
	DWORD bytes;

	params.Address = Address;
	params.Length = Length;

	if (!DeviceIoControl(DriverHandle, IOCTL_READ_MEMORY, &params, sizeof(params), Buffer, Length, &bytes, NULL)) return false;

	return true;
}

bool Tools::Kernel::Driver::WriteMemory(DWORD64 Address, DWORD64 Value) const
{
	WRITE_MEMORY_PARAMS params;
	DWORD bytes;

	params.Address = Address - 96;
	params.Option = 28;
	params.Value = Value;

	if (!DeviceIoControl(DriverHandle, IOCTL_WRITE_MEMORY, &params, sizeof(params), nullptr, 0, &bytes, NULL)) return false;

	return true;
}