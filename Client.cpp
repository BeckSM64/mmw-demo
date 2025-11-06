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

struct RemotePlayer {
    PlayerState current;
    PlayerState previous;
    float alpha = 0.f; // interpolation factor [0,1]
};

static std::mutex g_state_mtx;
static std::unordered_map<uint32_t, RemotePlayer> g_players;
static uint32_t g_myId = 1;

// Subscriber callback for server updates
void state_callback(void* data) {
    PlayerState* s = reinterpret_cast<PlayerState*>(data);
    if (s->playerId == g_myId) return; // ignore own player

    std::lock_guard<std::mutex> lk(g_state_mtx);
    auto& rp = g_players[s->playerId];
    rp.previous = rp.current;
    rp.current = *s;
    rp.alpha = 0.f; // reset interpolation
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
        sizeX = targetRatio / windowRatio;
        posX = (1.f - sizeX) / 2.f;
    } else if (windowRatio < targetRatio) {
        sizeY = windowRatio / targetRatio;
        posY = (1.f - sizeY) / 2.f;
    }

    view.setViewport(sf::FloatRect(posX, posY, sizeX, sizeY));
}

// Linear interpolation helper
float lerp(float a, float b, float t) {
    return a + t * (b - a);
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
    const float playerSize = 32.f;

    sf::Clock sendClock;
    sf::Clock deltaClock;

    while (window.isOpen()) {
        sf::Event e;
        float dx=0, dy=0;

        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed)
                window.close();
            else if (e.type == sf::Event::Resized)
                resizeView(window, view);
        }

        float dt = deltaClock.restart().asSeconds();

        // Apply input locally
        if (window.hasFocus()) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) dx -= 200.f * dt;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) dx += 200.f * dt;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) dy -= 200.f * dt;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) dy += 200.f * dt;
        }

        me.x += dx;
        me.y += dy;

        // Send own updated position to server ~60Hz
        if (sendClock.getElapsedTime().asMilliseconds() > 16) {
            if (sizeof(PlayerState) > 0) {
                mmw_publish_raw("input", &me, sizeof(PlayerState), MMW_RELIABLE);
            }
            sendClock.restart();
        }

        // Update interpolation for remote players
        {
            std::lock_guard<std::mutex> lk(g_state_mtx);
            for (auto& kv : g_players) {
                kv.second.alpha += dt * 10.f; // interpolation speed
                if (kv.second.alpha > 1.f) kv.second.alpha = 1.f;
            }
        }

        // Render
        window.clear(sf::Color(50,50,50)); // letterbox bars
        window.setView(view);

        // Game world background
        sf::RectangleShape background(sf::Vector2f(1920.f, 1080.f));
        background.setFillColor(sf::Color(0,100,200));
        background.setPosition(0.f, 0.f);
        window.draw(background);

        // Draw remote players (interpolated)
        {
            std::lock_guard<std::mutex> lk(g_state_mtx);
            for (auto& kv : g_players) {
                const RemotePlayer& rp = kv.second;
                float ix = lerp(rp.previous.x, rp.current.x, rp.alpha);
                float iy = lerp(rp.previous.y, rp.current.y, rp.alpha);

                sf::RectangleShape r(sf::Vector2f(playerSize, playerSize));
                r.setFillColor(sf::Color::Red);
                r.setOrigin(playerSize/2, playerSize/2);
                r.setPosition(ix, iy);
                window.draw(r);
            }
        }

        // Draw own player
        sf::RectangleShape playerBox(sf::Vector2f(playerSize, playerSize));
        playerBox.setFillColor(sf::Color::Green);
        playerBox.setOrigin(playerSize/2, playerSize/2);
        playerBox.setPosition(me.x, me.y);
        window.draw(playerBox);

        window.display();
    }

    mmw_cleanup();
    return 0;
}
