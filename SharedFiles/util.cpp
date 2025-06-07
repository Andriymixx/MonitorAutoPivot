// util.cpp

#include "util.h"
#include <iostream>
#include <Windows.h>
#include <vector>
#include <string>
#include "monitor_manager.h"
#include <thread>
#include <conio.h>
#include <sstream>
#include <mutex>
#include "config_util.h"
#include "g_flags.h"
#include <functional>
#include <cwctype>
#include <wintoastlib.h>
#include <toast_utils.h>
using namespace WinToastLib;
using namespace MonitorManager;

std::atomic<bool> dataReceivingActive = false;
std::atomic<bool> g_serialPortDead = false;
std::atomic<bool> presetMakingActive = false;
std::thread dataReceivingThread;

std::vector<std::wstring> ListAvailableComPorts() {
	std::vector<std::wstring> ports;
	wchar_t lpTargetPath[5000];
	for (int i = 1; i <= 255; ++i) {
		std::wstring portName = L"COM" + std::to_wstring(i);
		DWORD test = QueryDosDevice(portName.c_str(), lpTargetPath, 5000);
		if (test != 0) {
			ports.push_back(portName);
		}
	}
	return ports;
}

HANDLE openSerialPort(const std::wstring& portName) {
	HANDLE serialHandle = CreateFileW(portName.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (serialHandle == INVALID_HANDLE_VALUE) {
		return INVALID_HANDLE_VALUE;
	}

	COMMTIMEOUTS timeouts = { 0 };
	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;
	SetCommTimeouts(serialHandle, &timeouts);

	DCB dcbSerialParams = { 0 };
	dcbSerialParams.DCBlength = sizeof(DCB);

	if (!GetCommState(serialHandle, &dcbSerialParams)) {
		CloseHandle(serialHandle);
		return INVALID_HANDLE_VALUE;
	}

	dcbSerialParams.BaudRate = CBR_115200;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;
	dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;

	if (!SetCommState(serialHandle, &dcbSerialParams)) {
		CloseHandle(serialHandle);
		return INVALID_HANDLE_VALUE;
	}

	PurgeComm(serialHandle, PURGE_RXCLEAR | PURGE_TXCLEAR);
	return serialHandle;
}

// Reading data from serial
std::string readSerialLine() {
	char buffer[256] = { 0 };
	DWORD bytesRead = 0;
	DWORD i = 0;
	char ch;

	while (true) {
		// Check state of COM-port
		DWORD errors;
		COMSTAT status;
		if (!ClearCommError(hSerial, &errors, &status)) {
			// If connection lost
			OnSerialDead();
			return "";

		}

		if (status.cbInQue == 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}

		if (!ReadFile(hSerial, &ch, 1, &bytesRead, NULL) || bytesRead == 0) {
			return "";
		}

		if (ch == '\n') break;
		if (i < sizeof(buffer) - 1) {
			buffer[i++] = ch;
		}
	}

	buffer[i] = '\0';
	return std::string(buffer);
}

void StartDataReceiving() {
	if (dataReceivingActive) {
		return;
	}
	dataReceivingActive = true;
	g_serialPortDead = false;
	dataReceivingThread = std::thread(RecvDataFromAcc);
}

void StopDataReceiving() {
	dataReceivingActive = false;
	if (hSerial != INVALID_HANDLE_VALUE) {
		COMSTAT status;
		DWORD errors;
		if (ClearCommError(hSerial, &errors, &status)) {
			const char* stopCommand = "STOP\n";
			DWORD written;
			BOOL success = WriteFile(hSerial, stopCommand,
				(DWORD)strlen(stopCommand), &written, NULL);

			if (!success || written != strlen(stopCommand)) {
				OnSerialDead();
			}
			FlushFileBuffers(hSerial);
		}
	}
	if (dataReceivingThread.joinable()) {
		if (std::this_thread::get_id() != dataReceivingThread.get_id()) {
			dataReceivingThread.join();
		}
		else {
			dataReceivingThread.detach();
		}
	}
}

void OnSerialDead() {
	OutputDebugString(L"Serial dead\n");
	g_serialPortDead = true;
	dataReceivingActive = false;
	hSerial = NULL;
	if (dataReceivingThread.joinable()) {
		try {
			if (std::this_thread::get_id() != dataReceivingThread.get_id()) {
				dataReceivingThread.join();
			}
			else {
				dataReceivingThread.detach();
			}
		}
		catch (...) {
			OutputDebugString(L"Could not end dataReceivingThread\n");
		}
	}
	dataReceivingThread = std::thread();
	WinToastTemplate templ(WinToastTemplate::Text02);
	templ.setTextField(L"Error", WinToastTemplate::FirstLine);
	templ.setTextField(L"Lost connection on COM-port", WinToastTemplate::SecondLine);
	templ.setDuration(WinToastTemplate::Duration::Short);
	ToastUtils::push(templ);
}

void SerialDataReaderThread(std::function<void(const std::string&)> onDataLine,
	std::atomic<bool>& runFlag) {
	while (runFlag) {
		std::string data = readSerialLine();

		if (!data.empty()) {
			onDataLine(data);
		}
		else if (g_serialPortDead) {
			break;
		}
	}
}

void RecvDataFromAcc() {
	const char* startCommand = "SEND_DATA\n";
	DWORD bytesWritten;
	WriteFile(hSerial, startCommand, (DWORD)strlen(startCommand), &bytesWritten, NULL);
	std::function<void(const std::string&)> onData = [&](const std::string& data) {
		std::wstring wdata = ConvertUTF8ToWString(data);
		if (data.find("[DATA]") != std::string::npos) {
			std::wstring val = wdata.substr(wdata.find(L"[DATA]") + 7);
			try {
				int orientation = std::stoi(val);
				MonitorManager::currentOrientation = orientation;
				if (orientation != -1) {
					setOrientation(MonitorManager::selectedMonitor, orientation);
				}
			}
			catch (...) {}
		}
		else if (data.find("[SENDING_STOPPED]") != std::string::npos) {
			dataReceivingActive = false;
		}
		};
	SerialDataReaderThread(onData, dataReceivingActive);
}

std::wstring ConvertUTF8ToWString(const std::string& str) {
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstrTo[0], size_needed);
	return wstrTo;
}

void SendAutoRemapConfig(const RemapConfig& config) {
	std::ostringstream ss;
	ss << "AUTO_REMAP/"
		<< config.mapX << "/"
		<< config.mapY << "/"
		<< config.mapZ << "/"
		<< config.signX << "/"
		<< config.signY << "/"
		<< config.signZ << "\n";

	std::string command = ss.str();
	DWORD written;
	WriteFile(hSerial, command.c_str(), (DWORD)command.size(), &written, NULL);
	FlushFileBuffers(hSerial);

	std::atomic<bool> waitResponse = true;

	std::function<void(const std::string&)> onData = [&](const std::string& data) {
		std::string clean = data;
		if (!clean.empty() && clean.back() == '\r') clean.pop_back();

		if (clean.find("[REMAP_LOADED]") != std::string::npos) {
			waitResponse = false;
			OutputDebugString(L"REMAP_LOADED\n");
		}
		};

	SerialDataReaderThread(onData, waitResponse);
}

bool WaitForArduinoReady(uint32_t timeoutMillis)
{
	char buffer[64];
	DWORD bytesRead = 0;
	auto start = std::chrono::steady_clock::now();

	while (std::chrono::duration_cast<std::chrono::milliseconds>
		(std::chrono::steady_clock::now() - start).count() < timeoutMillis)
	{
		DWORD errors;
		COMSTAT status;
		ClearCommError(hSerial, &errors, &status);
		if (status.cbInQue > 0) {
			if (ReadFile(hSerial, buffer, min(sizeof(buffer) - 1,
				status.cbInQue), &bytesRead, NULL)) {
				buffer[bytesRead] = '\0';
				// Arduino is ready to work
				if (strstr(buffer, "[READY]") != NULL) {
					return true;
				}
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	hSerial = INVALID_HANDLE_VALUE;
	return false;
}

