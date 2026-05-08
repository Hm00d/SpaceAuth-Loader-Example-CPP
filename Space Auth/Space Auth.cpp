#include <iostream>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "auth.hpp"

using namespace SpaceAuth;

namespace {

	std::string name = SA_OBF("Your Application Name");
	std::string appid = SA_OBF("YOUR_APP_PUBLIC_ID");
	std::string version = SA_OBF("1.0.0");
    std::string url = SA_OBF("https://hmood.wtf");

    api SpaceAuthApp(name, appid, version, url);

    void print_status() {
        std::cout << SA_OBF("\nStatus: ") << SpaceAuthApp.response.message << SA_OBF("\n");
    }

    void pause_for_exit() {
        std::cout << SA_OBF("\nPress Enter to exit...");
        std::cout.flush();
        std::string line;
        std::getline(std::cin, line);
    }

}  // namespace

int main() {
    try {
#ifdef _WIN32
        SetConsoleTitleA(name.c_str());
#endif

        std::cout << SA_OBF("\nConnecting...");
        SpaceAuthApp.init();
        if (!SpaceAuthApp.response.success) {
            print_status();
            pause_for_exit();
            return 1;
        }
        print_status();

        protected_loader_runtime runtime(SpaceAuthApp);
        if (SpaceAuthApp.app_data.anti_crack_enabled &&
            !runtime.start_anti_crack(2, SA_OBF("startup_security_check_failed"))) {
            pause_for_exit();
            return 1;
        }

        const auto blacklist = SpaceAuthApp.checkblack();
        if (blacklist.blacklisted) {
            std::cout << SA_OBF("\nThis device is blacklisted.");
            if (!blacklist.reason.empty()) {
                std::cout << SA_OBF("\nReason: ") << blacklist.reason;
            }
            pause_for_exit();
            return 1;
        }

        std::string license_key;
        std::cout << SA_OBF("\nEnter license key: ");
        std::getline(std::cin >> std::ws, license_key);

        SpaceAuthApp.license(license_key);
        if (!SpaceAuthApp.response.success) {
            print_status();
            pause_for_exit();
            return 1;
        }
        print_status();

        SpaceAuthApp.log(SA_OBF("Loader login completed."), SA_OBF("loader.license_login"));

        runtime.start_live_guard(20, 300);
        runtime.start_diagnostics(7);

        std::cout << SA_OBF("\nDownloading protected file...");
        const std::string saved_path = download_and_launch(
            SpaceAuthApp,
            SA_OBF("keeps"),
            SA_OBF("connects.exe"),
            true);
        if (!SpaceAuthApp.response.success || saved_path.empty()) {
            print_status();
            pause_for_exit();
            return 1;
        }
		
        std::cout << SA_OBF("\nProtected file launched.");
        std::cout << SA_OBF("\nAnti Crack: ")
            << (SpaceAuthApp.app_data.anti_crack_enabled ? SA_OBF("active") : SA_OBF("disabled"));
        std::cout << SA_OBF("\n");

        pause_for_exit();
        runtime.stop();
        SpaceAuthApp.logout();
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << SA_OBF("\nLoader failed: ") << error.what() << SA_OBF("\n");
        pause_for_exit();
        return 1;
    }
}
