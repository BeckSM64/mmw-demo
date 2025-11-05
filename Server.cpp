#include "MMW.h"
#include <SFML/System.hpp>
#include <unordered_map>
#include <mutex>
#include <string>
#include <nlohmann/json.hpp> // optional if you want to serialize as JSON

struct Player {
    float x = 100, y = 100;
};

std::unordered_map<std::string, Player> players;
std::mutex playersMutex;

void onClientMove(const char* msg) {
    try {
        auto j = nlohmann::json::parse(msg);
        std::string playerId = j["id"];
        float dx = j["dx"];
        float dy = j["dy"];

        std::lock_guard<std::mutex> lock(playersMutex);
        players[playerId].x += dx;
        players[playerId].y += dy;

        // broadcast the updated state to all players
        nlohmann::json stateJson;
        for (auto& [id, p] : players) {
            stateJson["players"][id] = { {"x", p.x}, {"y", p.y} };
        }
        mmw_publish("game/state", stateJson.dump().c_str(), MMW_RELIABLE);

    } catch (...) {
        // ignore bad messages
    }
}

int main() {
    mmw_set_log_level(MMW_LOG_LEVEL_INFO);
    mmw_initialize("127.0.0.1", 5000);

    // Subscribe to all player movements
    mmw_create_subscriber("game/move", onClientMove);

    // Run forever
    while (true) {
        sf::sleep(sf::milliseconds(100));
    }

    mmw_cleanup();
    return 0;
}
