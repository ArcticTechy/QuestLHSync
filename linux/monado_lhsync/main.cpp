// Skeleton for a Linux QuestLHSync service targeting Monado / WiVRn.
// Reuses the network protocol from the original HeadsetLink.
// Solver and Monado space-overseer integration are stubs.

#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>

// TODO: include / port net.h, sync.h logic or link against a shared library version

std::atomic<bool> running{true};

int main(int argc, char** argv) {
    std::string host;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--host" && i + 1 < argc) {
            host = argv[++i];
        }
    }

    std::cout << "QuestLHSync Monado/WiVRn skeleton\n";
    if (!host.empty()) {
        std::cout << "Target host: " << host << "\n";
    } else {
        std::cout << "No --host given; discovery not yet implemented in skeleton\n";
    }

    std::cout << "\nPlanned behaviour:\n";
    std::cout << "  1. Discover / connect to headset lhsyncd (UDP 47281 / TCP 47280)\n";
    std::cout << "  2. Receive camera calibration + frame/spot lines\n";
    std::cout << "  3. Obtain HMD pose history from Monado\n";
    std::cout << "  4. Run alignment solver (yaw + translation)\n";
    std::cout << "  5. Apply result via xrt_space_overseer_set_tracking_origin_offset\n";
    std::cout << "     on the steamvr_lh tracking origin\n\n";

    // Placeholder loop
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "." << std::flush;
    }

    return 0;
}
