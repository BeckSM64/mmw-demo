#include "MMW.h"
#include <SFML/System.hpp>
#include <map>
#include <string>
#include <sstream>
#include <mutex>

struct Player {
    float x, y;
};

std::map<int, Player> players;
std::mutex playersMutex;

// Callback when a client sends input
void handlePlayerInput(const char* msg) {
    // msg format: "id:x:y"
    std::istringstream ss(msg);
    int id;
    char sep1, sep2;
    float x, y;
    ss >> id >> sep1 >> x >> sep2 >> y;

    std::lock_guard<std::mutex> lock(playersMutex);
    players[id] = {x, y};
}

int main() {
    mmw_set_log_level(MMW_LOG_LEVEL_INFO);
    mmw_initialize("127.0.0.1", 5000);

    mmw_create_subscriber("player_input", handlePlayerInput);

    // Main server loop
    while (true) {
        // Prepare the update message
        std::ostringstream out;
        {
            std::lock_guard<std::mutex> lock(playersMutex);
            for (const auto& pair : players) {
                out << pair.first << ":" << pair.second.x << "," << pair.second.y << ";";
            }
        }

        std::string updateMsg = out.str();
        if (!updateMsg.empty()) {
            mmw_publish("positions", updateMsg.c_str(), MMW_RELIABLE);
        }

        sf::sleep(sf::milliseconds(100)); // 10 updates per second
    }

    mmw_cleanup();
    return 0;
}
