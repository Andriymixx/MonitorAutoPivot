#include "toast_utils.h"
#include <iostream>

using namespace WinToastLib;

std::mutex ToastUtils::mutex_;
std::condition_variable ToastUtils::cv_;
std::queue<WinToastTemplate> ToastUtils::queue_;
bool ToastUtils::running_ = false;
bool ToastUtils::initialized_ = false;
std::thread ToastUtils::worker_;
static WinToast* globalToast = WinToast::instance();

void CustomHandler::toastActivated() const {
    //std::wcout << L"[Toast] User clicked toast" << std::endl;
}

void CustomHandler::toastActivated(int actionIndex) const {
    //std::wcout << L"[Toast] User clicked action #" << actionIndex << std::endl;
}

void CustomHandler::toastDismissed(WinToastDismissalReason state) const {
    switch (state) {
    case UserCanceled:
        //std::wcout << L"[Toast] User dismissed toast" << std::endl;
        break;
    case TimedOut:
        //std::wcout << L"[Toast] Toast timed out" << std::endl;
        break;
    case ApplicationHidden:
        //std::wcout << L"[Toast] Application hid toast" << std::endl;
        break;
    default:
        //std::wcout << L"[Toast] Toast not activated" << std::endl;
        break;
    }
}

void CustomHandler::toastFailed() const {
    //std::wcout << L"[Toast] Failed to show toast" << std::endl;
}

void CustomHandler::toastActivated(const char* response) const {
    //std::wcout << L"[Toast] Response: " << response << std::endl;
}

void ToastUtils::initialize() {
    if (initialized_) return;

    globalToast->setAppName(L"MonitorAutoPivot");
    globalToast->setAppUserModelId(L"MonitorAutoPivot.MonitorApp.MonitorAutoPivotApp.20250430");

    if (!globalToast->initialize()) {
        //std::wcout << L"[ToastUtils] Can not initialize WinToast!\n";
        return;
    }

    initialized_ = true;
    running_ = true;

    worker_ = std::thread(processQueue);
    worker_.detach();

    //std::wcout << L"[ToastUtils] WinToast initialized successfully\n";
}


void ToastUtils::push(const WinToastTemplate& toast) {
    if (!initialized_) {
        //std::wcout << L"Error when launching the toast. WinToast is not initialized.\n";
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(toast);
    }
    cv_.notify_one();
}

void ToastUtils::processQueue() {
    while (running_) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [] { return !queue_.empty(); });

        while (!queue_.empty()) {
            WinToastTemplate toast = queue_.front();
            queue_.pop();
            lock.unlock();

            if (globalToast->showToast(toast, new CustomHandler()) < 0) {
                //std::wcout << L"[ToastUtils] Can not show toast!\n";
            }

            lock.lock();
        }
    }
}
