#include <iostream>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <cstring>
#include "MMW.h"

struct PlayerState {
    uint32_t playerId;
    float x;
    float y;
};

static std::mutex g_state_mtx;
static std::unordered_map<uint32_t, PlayerState> g_players;

// Subscriber callback: receives the client's current position
void position_callback(void* data) {
    PlayerState* s = reinterpret_cast<PlayerState*>(data);
    std::lock_guard<std::mutex> lk(g_state_mtx);
    g_players[s->playerId] = *s;
}

int main() {
    mmw_initialize("127.0.0.1", 5000);

    mmw_create_subscriber_raw("input", position_callback);
    mmw_create_publisher("state");

    // Server loop: broadcast all player positions at ~60Hz
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        std::lock_guard<std::mutex> lk(g_state_mtx);
        for (auto& kv : g_players) {
            mmw_publish_raw("state", &kv.second, sizeof(PlayerState), MMW_RELIABLE);
        }
    }

    mmw_cleanup();
    return 0;
}
