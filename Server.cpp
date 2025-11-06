// server.cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <cstring>
#include <chrono>
#include "MMW.h"

struct PlayerState {
    uint32_t playerId;
    float x;
    float y;
};

struct InputMsg {
    uint32_t playerId;
    float dx;
    float dy;
};

static std::mutex g_state_mtx;
static std::unordered_map<uint32_t, PlayerState> g_players;

// Subscriber callback, receives void* (cast to InputMsg*)
void input_callback(void* data) {
    InputMsg* in = reinterpret_cast<InputMsg*>(data);
    std::lock_guard<std::mutex> lk(g_state_mtx);
    auto& s = g_players[in->playerId];
    s.playerId = in->playerId;
    s.x += in->dx;
    s.y += in->dy;
}

int main() {
    mmw_initialize("127.0.0.1", 5000);

    mmw_create_subscriber_raw("input", input_callback);
    mmw_create_publisher("state");

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::lock_guard<std::mutex> lk(g_state_mtx);
        for (auto& kv : g_players) {
            std::cout << "Player ID: " << std::to_string(kv.second.playerId) << ", Player Position: (" << std::to_string(kv.second.x) << ", " << std::to_string(kv.second.y) << ")" << std::endl;
            mmw_publish_raw("state", &kv.second, sizeof(PlayerState), MMW_RELIABLE);
        }
    }

    mmw_cleanup();
    return 0;
}
