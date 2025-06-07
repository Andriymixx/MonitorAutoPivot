// util.h
#ifndef UTIL_H
#define UTIL_H
#include <Windows.h>
#include <string>
#include "g_flags.h"

std::string readSerialLine();
HANDLE openSerialPort(const std::wstring& portName);
std::wstring ConvertUTF8ToWString(const std::string& str);
void SendRemapRequest();
void SendAutoRemapConfig(const RemapConfig& config);
std::wstring ConvertUTF8ToWString(const std::string& str);
bool WaitForArduinoReady(uint32_t timeoutMillis);
void CreateLayout();
void RecvDataFromAcc();
void SerialDataReaderThread(std::function<void(const std::string&)> onDataLine, std::atomic<bool>& runFlag);
void OnSerialDead();
#endif // !UTIL_H