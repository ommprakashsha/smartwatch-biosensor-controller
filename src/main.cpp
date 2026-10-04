#include <iostream>
#include <csignal>
#include <memory>
#include <thread>
#include <chrono>
#include "BioWatchController.hpp"
#include "Logger.hpp"

static BioWatchController* g_controller = nullptr;

void signalHandler(int signum) {
    if (g_controller) {
        Logger::getInstance().info("Signal caught (SIGINT/SIGTERM). Initiating clean shutdown...");
        g_controller->stop();
    }
}

int main(int argc, char* argv[]) {
    std::cout << "==========================================================" << std::endl;
    std::cout << "  V-BioWatch: Smartwatch Biosensor & Haptic Daemon (C++)   " << std::endl;
    std::cout << "==========================================================" << std::endl;

    Logger::getInstance().init("smartwatch_health.log");
    Logger::getInstance().info("Smartwatch Biosensor Subsystem starting up...");

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    try {
        BioWatchController controller("/dev/smart_watch_bio");
        g_controller = &controller;

        // Custom threshold limit (defaults to 130 BPM, or passed via CLI argument)
        int32_t hrLimit = 130;
        if (argc > 1) {
            hrLimit = std::stoi(argv[1]);
        }
        controller.setHeartRateThreshold(hrLimit);

        // Start multi-threaded vitals monitoring
        controller.start();

        Logger::getInstance().info("System active. Press Ctrl+C to terminate cleanly.");

        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

    } catch (const DeviceException& ex) {
        Logger::getInstance().error(std::string("Device Exception: ") + ex.what());
        return 1;
    } catch (const std::exception& ex) {
        Logger::getInstance().error(std::string("Fatal Error: ") + ex.what());
        return 1;
    }

    Logger::getInstance().info("Smartwatch Biosensor Subsystem terminated cleanly.");
    return 0;
}
