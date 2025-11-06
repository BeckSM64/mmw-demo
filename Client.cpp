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

// Subscriber callback for authoritative state (other players)
void state_callback(void* data) {
    PlayerState* s = reinterpret_cast<PlayerState*>(data);
    if (s->playerId == g_myId) return; // ignore own player
    std::lock_guard<std::mutex> lk(g_state_mtx);
    g_players[s->playerId] = *s;
}

// Letterbox / aspect ratio resize
void resizeView(sf::RenderWindow& window, sf::View& view) {
    const float targetWidth = 1920.f;
    const float targetHeight = 1080.f;
    float windowRatio = float(window.getSize().x) / window.getSize().y;
    float targetRatio = targetWidth / targetHeight;
    float sizeX = 1.f;
    float sizeY = 1.f;
    float posX = 0.f;
    float posY = 0.f;

    if (windowRatio > targetRatio) {
        // Wider window, black bars left/right
        sizeX = targetRatio / windowRatio;
        posX = (1.f - sizeX) / 2.f;
    } else if (windowRatio < targetRatio) {
        // Taller window, black bars top/bottom
        sizeY = windowRatio / targetRatio;
        posY = (1.f - sizeY) / 2.f;
    }

    view.setViewport(sf::FloatRect(posX, posY, sizeX, sizeY));
}

int main(int argc, char** argv) {
    if (argc >= 2) g_myId = atoi(argv[1]);
    std::cout << "My ID: " << g_myId << std::endl;

    mmw_set_log_level(MMW_LOG_LEVEL_OFF);
    mmw_initialize("127.0.0.1", 5000);
    mmw_create_publisher("input");
    mmw_create_subscriber_raw("state", state_callback);

    sf::RenderWindow window(sf::VideoMode(1280,720), "MMW Client", sf::Style::Default);
    sf::View view(sf::FloatRect(0, 0, 1920, 1080)); // virtual 16:9 world
    window.setView(view);

    // Own player
    PlayerState me{g_myId, 100.f, 100.f};
    sf::RectangleShape playerBox(sf::Vector2f(32,32));
    playerBox.setFillColor(sf::Color::Green);

    sf::Clock sendClock;
    while (window.isOpen()) {
        sf::Event e;
        float dx=0, dy=0;

        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed)
                window.close();
            else if (e.type == sf::Event::Resized)
                resizeView(window, view);
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

        // Send own updated position to server
        if (sendClock.getElapsedTime().asMilliseconds() > 16) {
            mmw_publish_raw("input", &me, sizeof(PlayerState), MMW_RELIABLE);
            sendClock.restart();
        }

        // Render
        window.clear(sf::Color(50,50,50)); // bar color (outside game world)
        window.setView(view);

        // Draw game world background
        sf::RectangleShape background(sf::Vector2f(1920.f, 1080.f));
        background.setFillColor(sf::Color(0,100,200)); // game world background
        background.setPosition(0.f, 0.f);
        window.draw(background);

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

        // Draw own player
        playerBox.setPosition(me.x, me.y);
        window.draw(playerBox);

        window.display();
    }

    mmw_cleanup();
    return 0;
}
