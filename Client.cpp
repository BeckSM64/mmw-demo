#include <SFML/Graphics.hpp>
#include <iostream>
#include <thread>
#include <mutex>
#include <unordered_map>
#include "MMW.h"

struct PlayerState {
    uint32_t playerId;
    float x;
    float y;
};

static std::mutex g_state_mtx;
static std::unordered_map<uint32_t, PlayerState> g_players;
static uint32_t g_myId = 1;

// Callback: update other players' positions
void state_callback(void* data) {
    PlayerState* s = reinterpret_cast<PlayerState*>(data);
    if (s->playerId == g_myId) return; // ignore our own position
    std::lock_guard<std::mutex> lk(g_state_mtx);
    g_players[s->playerId] = *s;
}

int main(int argc, char** argv) {
    if (argc >= 2) g_myId = atoi(argv[1]);
    std::cout << "My ID: " << g_myId << std::endl;

    mmw_set_log_level(MMW_LOG_LEVEL_OFF);
    mmw_initialize("127.0.0.1", 5000);
    mmw_create_publisher("input");
    mmw_create_subscriber_raw("state", state_callback);

    sf::RenderWindow window(sf::VideoMode(600,400), "MMW Client");
    window.setFramerateLimit(60);

    // Local player position
    PlayerState me{g_myId, 100.0f, 100.0f};

    sf::RectangleShape playerBox(sf::Vector2f(32,32));
    playerBox.setFillColor(sf::Color::Green);

    sf::Clock sendClock;
    while (window.isOpen()) {
        sf::Event e;
        float dx=0, dy=0;

        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) window.close();
        }

        // Apply input locally
        if (window.hasFocus()) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) dx -= 2;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) dx += 2;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) dy -= 2;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) dy += 2;
        }

        me.x += dx;
        me.y += dy;

        // Send our updated position to server at ~60Hz
        if (sendClock.getElapsedTime().asMilliseconds() > 16) {
            mmw_publish_raw("input", &me, sizeof(PlayerState), MMW_RELIABLE);
            sendClock.restart();
        }

        // Render
        window.clear();

        // Draw other players
        {
            std::lock_guard<std::mutex> lk(g_state_mtx);
            for (auto& kv : g_players) {
                sf::RectangleShape r(sf::Vector2f(32,32));
                r.setPosition(kv.second.x, kv.second.y);
                r.setFillColor(sf::Color::Red);
                window.draw(r);
            }
        }

        // Draw ourselves
        playerBox.setPosition(me.x, me.y);
        window.draw(playerBox);

        window.display();
    }

    mmw_cleanup();
    return 0;
}
