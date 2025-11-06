// client.cpp
#include <SFML/Graphics.hpp>
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

struct InputMsg {
    uint32_t playerId;
    float dx;
    float dy;
};

static std::mutex g_state_mtx;
static std::unordered_map<uint32_t, PlayerState> g_players;
static uint32_t g_myId = 1;

// Subscriber callback for authoritative state
void state_callback(void* data) {
    PlayerState* s = reinterpret_cast<PlayerState*>(data);
    std::lock_guard<std::mutex> lk(g_state_mtx);
    g_players[s->playerId] = *s;
}

int main(int argc, char** argv) {
    // if (argc >= 2) g_myId = atoi(argv[1]);
    g_myId = atoi(argv[1]);
    std::cout << "My ID: " << g_myId << std::endl;

    mmw_set_log_level(MMW_LOG_LEVEL_OFF);
    mmw_initialize("127.0.0.1", 5000);
    mmw_create_publisher("input");
    mmw_create_subscriber_raw("state", state_callback);

    sf::RenderWindow window(sf::VideoMode(600,400), "MMW Client");
    window.setFramerateLimit(60);

    float px = 100, py = 100;
    sf::RectangleShape me(sf::Vector2f(32,32));
    me.setFillColor(sf::Color::Green);

    sf::Clock sendClock;
    while (window.isOpen()) {
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) window.close();
        }

        float dx=0, dy=0;
        if (window.hasFocus()) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) dx-=2;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) dx+=2;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) dy-=2;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) dy+=2;
        }

        px += dx;
        py += dy;

        if (sendClock.getElapsedTime().asMilliseconds() > 50) {
            if (dx != 0 || dy != 0) {
                InputMsg in{g_myId, dx, dy};
                mmw_publish_raw("input", &in, sizeof(InputMsg), MMW_RELIABLE);
            }
            sendClock.restart();
        }

        window.clear();
        {
            std::lock_guard<std::mutex> lk(g_state_mtx);
            std::cout << "Size of g_players: " << g_players.size() << std::endl;
            for (auto& kv : g_players) {
                if (kv.first != g_myId) {
                    sf::RectangleShape r(sf::Vector2f(32,32));
                    r.setPosition(kv.second.x, kv.second.y);
                    r.setFillColor(sf::Color::Red);
                    window.draw(r);
                    std::cout << "Other Player current position: (" << std::to_string(kv.second.x) << ", " << std::to_string(kv.second.y) << ")" << std::endl;
                }
            }
        }
        me.setPosition(px, py);
        window.draw(me);
        std::cout << "My current position: (" << std::to_string(px) << ", " << std::to_string(py) << ")" << std::endl;

        window.display();
    }

    mmw_cleanup();
    return 0;
}
