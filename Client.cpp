#include "MMW.h"
#include "PlayerState.h"
#include <SFML/Graphics.hpp>
#include <map>
#include <mutex>

std::map<int, sf::RectangleShape> players;
std::mutex playersMutex;
int myId = 0;
PlayerState myState;

void handleServerUpdate(void* rawMsg) {
    PlayerState* states = static_cast<PlayerState*>(rawMsg);
    size_t count = 64; // assume max 64 players for demo
    std::lock_guard<std::mutex> lock(playersMutex);

    for (size_t i = 0; i < count; ++i) {
        int id = states[i].id;
        if (id == 0) break; // stop at empty entry
        if (players.find(id) == players.end()) {
            sf::RectangleShape box(sf::Vector2f(50,50));
            box.setFillColor(id == myId ? sf::Color::Red : sf::Color::Blue);
            players[id] = box;
        }
        players[id].setPosition(states[i].x, states[i].y);
    }
}

int main() {
    mmw_set_log_level(MMW_LOG_LEVEL_INFO);
    mmw_initialize("127.0.0.1", 5000);

    mmw_create_publisher("player_input");
    mmw_create_subscriber_raw("positions", handleServerUpdate);

    // Assign a unique ID (just use time for demo)
    myId = static_cast<int>(time(NULL) % 100000);
    myState.id = myId;
    myState.x = 100.0f;
    myState.y = 100.0f;

    sf::RenderWindow window(sf::VideoMode(800,600), "MMW Demo Client");

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
        }

        bool moved = false;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))  { myState.x -= 5; moved = true; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) { myState.x += 5; moved = true; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))    { myState.y -= 5; moved = true; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))  { myState.y += 5; moved = true; }

        if (moved) {
            mmw_publish_raw("player_input", &myState, sizeof(PlayerState), MMW_BEST_EFFORT);
        }

        window.clear();
        {
            std::lock_guard<std::mutex> lock(playersMutex);
            for (auto& kv : players) {
                window.draw(kv.second);
            }
        }
        window.display();

        sf::sleep(sf::milliseconds(16)); // ~60 FPS
    }

    mmw_cleanup();
    return 0;
}
