// QuestLHSync Linux / Monado client with control UI
// Speaks the existing headset protocol (UDP discovery + TCP stream).
// Terminal-based control panel (works without extra UI libraries).
// Solver and Monado offset application are still stubs — the structure is ready.

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

constexpr int kUdpPort = 47281;
constexpr int kTcpPort = 47280;

struct Status {
    std::mutex m;
    std::string state = "Idle";
    std::string headset;
    std::string addr;
    std::string last_msg;
    int frames = 0;
    bool paused = false;
    bool connected = false;
    // stub alignment
    double yaw = 0, tx = 0, ty = 0, tz = 0;
};

Status g_status;

void log_status(const std::string& s) {
    std::lock_guard<std::mutex> g(g_status.m);
    g_status.last_msg = s;
    std::cerr << "[lhsync] " << s << "\n";
}

// Simple non-blocking keyboard
int kbhit() {
    struct timeval tv{0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0;
}

char getch() {
    char c = 0;
    if (read(STDIN_FILENO, &c, 1) == 1) return c;
    return 0;
}

void set_raw_mode(bool on) {
    static struct termios orig;
    static bool saved = false;
    if (!saved) {
        tcgetattr(STDIN_FILENO, &orig);
        saved = true;
    }
    if (on) {
        struct termios raw = orig;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig);
    }
}

// UDP discovery
std::string discover() {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) return {};
    int on = 1;
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
    sockaddr_in any{};
    any.sin_family = AF_INET;
    bind(s, (sockaddr*)&any, sizeof(any));

    sockaddr_in dst{};
    dst.sin_family = AF_INET;
    dst.sin_port = htons(kUdpPort);
    dst.sin_addr.s_addr = INADDR_BROADCAST;
    const char* q = "QLHS?";
    sendto(s, q, 5, 0, (sockaddr*)&dst, sizeof(dst));

    fd_set rf;
    FD_ZERO(&rf);
    FD_SET(s, &rf);
    timeval tv{1, 0};
    if (select(s + 1, &rf, nullptr, nullptr, &tv) > 0) {
        char buf[512] = {};
        sockaddr_in from{};
        socklen_t fl = sizeof(from);
        int n = recvfrom(s, buf, sizeof(buf) - 1, 0, (sockaddr*)&from, &fl);
        if (n > 0) {
            char ip[64];
            inet_ntop(AF_INET, &from.sin_addr, ip, sizeof(ip));
            close(s);
            return ip;
        }
    }
    close(s);
    return {};
}

void session(const std::string& ip) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) return;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(kTcpPort);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    if (connect(s, (sockaddr*)&addr, sizeof(addr)) < 0) {
        log_status("TCP connect failed to " + ip);
        close(s);
        return;
    }
    {
        std::lock_guard<std::mutex> g(g_status.m);
        g_status.connected = true;
        g_status.addr = ip;
        g_status.state = "Connected";
    }
    log_status("Connected to " + ip);

    std::string buf;
    char tmp[8192];
    while (true) {
        {
            std::lock_guard<std::mutex> g(g_status.m);
            if (g_status.paused) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
        }
        fd_set rf;
        FD_ZERO(&rf);
        FD_SET(s, &rf);
        timeval tv{0, 200000};
        int r = select(s + 1, &rf, nullptr, nullptr, &tv);
        if (r > 0) {
            int n = recv(s, tmp, sizeof(tmp), 0);
            if (n <= 0) break;
            buf.append(tmp, n);
            size_t pos;
            while ((pos = buf.find('\n')) != std::string::npos) {
                std::string line = buf.substr(0, pos);
                buf.erase(0, pos + 1);
                if (line.size() >= 2 && line[1] == ' ') {
                    if (line[0] == 'F') {
                        std::lock_guard<std::mutex> g(g_status.m);
                        g_status.frames++;
                        // stub: pretend we update alignment slowly
                        g_status.yaw += 0.01;
                        if (g_status.yaw > 360) g_status.yaw -= 360;
                    } else if (line[0] == 'H') {
                        log_status("Hello: " + line);
                    } else if (line[0] == 'C') {
                        log_status("Calibration received");
                    }
                }
            }
        }
        // check if we should quit (handled by main thread via atomic later)
    }
    close(s);
    std::lock_guard<std::mutex> g(g_status.m);
    g_status.connected = false;
    g_status.state = "Disconnected";
    log_status("Disconnected");
}

void ui_thread() {
    set_raw_mode(true);
    while (true) {
        // clear screen
        std::cout << "\033[2J\033[H";
        std::cout << "=== QuestLHSync Monado/WiVRn Control ===\n\n";
        {
            std::lock_guard<std::mutex> g(g_status.m);
            std::cout << "State:     " << g_status.state << "\n";
            std::cout << "Headset:   " << (g_status.headset.empty() ? "-" : g_status.headset) << "\n";
            std::cout << "Address:   " << (g_status.addr.empty() ? "-" : g_status.addr) << "\n";
            std::cout << "Frames:    " << g_status.frames << "\n";
            std::cout << "Paused:    " << (g_status.paused ? "YES" : "no") << "\n";
            std::cout << "Alignment: yaw=" << g_status.yaw << "°  t=("
                      << g_status.tx << ", " << g_status.ty << ", " << g_status.tz << ")\n";
            std::cout << "Last:      " << g_status.last_msg << "\n";
        }
        std::cout << "\nControls:\n";
        std::cout << "  [p] Pause / resume corrections\n";
        std::cout << "  [r] Force recalibrate (stub)\n";
        std::cout << "  [q] Quit\n";
        std::cout << "\n(Monado offset application not yet wired — see docs/MONADO.md)\n";
        std::cout.flush();

        if (kbhit()) {
            char c = getch();
            if (c == 'q' || c == 'Q') break;
            if (c == 'p' || c == 'P') {
                std::lock_guard<std::mutex> g(g_status.m);
                g_status.paused = !g_status.paused;
                log_status(g_status.paused ? "Paused" : "Resumed");
            }
            if (c == 'r' || c == 'R') {
                std::lock_guard<std::mutex> g(g_status.m);
                g_status.yaw = g_status.tx = g_status.ty = g_status.tz = 0;
                g_status.frames = 0;
                log_status("Force recalibrate (stub)");
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    set_raw_mode(false);
}

int main(int argc, char** argv) {
    std::string host;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--host" && i + 1 < argc) host = argv[++i];
    }

    std::thread net([&] {
        while (true) {
            std::string ip = host;
            if (ip.empty()) {
                log_status("Discovering...");
                ip = discover();
                if (ip.empty()) {
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    continue;
                }
            }
            session(ip);
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    });

    ui_thread();
    // crude exit: the net thread is detached in spirit; process exits
    std::cout << "\nExiting.\n";
    _exit(0);
}
