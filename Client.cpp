#include "MMW.h"
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <mutex>
#include <string>
#include <nlohmann/json.hpp>

struct Player {
    float x = 100, y = 100;
};

std::unordered_map<std::string, Player> players;
std::mutex playersMutex;
std::string myId = "player1"; // each client should have a unique id

void onStateUpdate(const char* msg) {
    try {
        auto j = nlohmann::json::parse(msg);
        std::lock_guard<std::mutex> lock(playersMutex);
        for (auto& [id, p] : j["players"].items()) {
            players[id].x = p["x"];
            players[id].y = p["y"];
        }
    } catch (...) {}
}

int main() {
    mmw_set_log_level(MMW_LOG_LEVEL_INFO);
    mmw_initialize("127.0.0.1", 5000);

    // Subscribe to game state
    mmw_create_subscriber("game/state", onStateUpdate);

    sf::RenderWindow window(sf::VideoMode(800, 600), "MMW Multiplayer Demo");

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Movement input
        float dx = 0, dy = 0;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) dy -= 5;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) dy += 5;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) dx -= 5;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) dx += 5;

        if (dx != 0 || dy != 0) {
            nlohmann::json moveMsg = { {"id", myId}, {"dx", dx}, {"dy", dy} };
            mmw_publish("game/move", moveMsg.dump().c_str(), MMW_RELIABLE);
        }

        window.clear(sf::Color::Black);

        {
            std::lock_guard<std::mutex> lock(playersMutex);
            for (auto& [id, p] : players) {
                sf::RectangleShape rect(sf::Vector2f(50, 50));
                rect.setPosition(p.x, p.y);
                rect.setFillColor(id == myId ? sf::Color::Green : sf::Color::Red);
                window.draw(rect);
            }
        }

        window.display();
        sf::sleep(sf::milliseconds(50));
    }

    mmw_cleanup();
    return 0;
}
