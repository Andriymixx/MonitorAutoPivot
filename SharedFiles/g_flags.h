#ifndef G_FLAGS_H
#define G_FLAGS_H
#include <thread>
#include <functional>
// Structure of Accelerometer Axis config, 
// so Accelerometer can be placed in any comfortable position behind monitor
struct RemapConfig {
	int mapX, mapY, mapZ; // For remaping axis
	int signX, signY, signZ; // also for its sign
	// Default values
	RemapConfig() : mapX(0), mapY(1), mapZ(2), signX(1), signY(1), signZ(1) {}
};

namespace ConfigUtil {
	extern RemapConfig g_remapConfig; // Acc Axis config
	extern std::wstring selectedPort; // COM-port to automatically connect to it
	extern bool g_minimizeOnClose; // Flag for close button act as like minimize
	extern bool g_startWithWindows; // Flag ro set actual state of startup setting in app
	extern int g_selectedTheme; // Theme value for app
}

bool SaveConfigJson();
bool LoadConfigJson();
std::vector<std::wstring> ListAvailableComPorts();
std::string GetConfigPathInDocuments();
void ManageLayoutPresetsMenu();
void StopDataReceiving();
void SerialDataReaderThread( std::function<void(const std::string&)> onDataLine, std::atomic<bool>& runFlag);
void StartDataReceiving();

// Flag to know if Arduino is ready
extern std::atomic<bool> isArduinoReady;
// Flag to know if there is data receiving from Arduino
extern std::atomic<bool> dataReceivingActive;

// Flag to force stop receiving data if can not send stop command
extern std::atomic<bool> g_serialPortDead;

// Flag to not update layout if making a new one
extern std::atomic<bool> presetMakingActive;

// Flag for start receiving data if monitor became active
extern bool wasConnected;

// Flag for monitorWatcher thread
extern std::atomic<bool> stopMonitorWatcher; 
extern HANDLE hSerial;
#endif // G_FLAGS_H