#pragma once
#include "wintoastlib.h"
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>

class CustomHandler : public WinToastLib::IWinToastHandler {
public:
    void toastActivated() const override;
    void toastActivated(int actionIndex) const override;
    void toastDismissed(WinToastLib::IWinToastHandler::WinToastDismissalReason state) const override;
    void toastFailed() const override;
    void toastActivated(const char* response) const;
};

class ToastUtils {
public:
    static void initialize();
    static void push(const WinToastLib::WinToastTemplate& toast);

private:
    static void processQueue();
    static std::mutex mutex_;
    static std::condition_variable cv_;
    static std::queue<WinToastLib::WinToastTemplate> queue_;
    static bool running_;
    static bool initialized_;
    static std::thread worker_;
};
