#include "MMW.h"
#include "PlayerState.h"
#include <map>
#include <mutex>
#include <thread>
#include <iostream>

std::map<int, PlayerState> players;
std::mutex playersMutex;

void handlePlayerInput(void* rawMsg) {
    PlayerState* state = static_cast<PlayerState*>(rawMsg);
    std::lock_guard<std::mutex> lock(playersMutex);
    players[state->id] = *state;
}

int main() {
    mmw_set_log_level(MMW_LOG_LEVEL_INFO);
    mmw_initialize("127.0.0.1", 5000);

    mmw_create_subscriber_raw("player_input", handlePlayerInput);
    mmw_create_publisher("positions");

    std::cout << "Server running..." << std::endl;

    while (true) {
        // Broadcast at fixed 60Hz
        PlayerState allStates[64];
        int count = 0;

        {
            std::lock_guard<std::mutex> lock(playersMutex);
            for (auto it = players.begin(); it != players.end(); ++it) {
                allStates[count++] = it->second;
            }
        }

        if (count > 0) {
            mmw_publish_raw("positions", allStates, count * sizeof(PlayerState), MMW_BEST_EFFORT);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60Hz
    }

    mmw_cleanup();
    return 0;
}
