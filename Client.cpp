#include <SFML/Graphics.hpp>
#include <iostream>
#include <thread>
#include <mutex>
#include <unordered_map>
#include "MMW.h"

// ----------------------
// Data structures
// ----------------------
struct PlayerState {
    uint32_t playerId;
    float x;
    float y;
};

struct RemotePlayer {
    PlayerState current;
    PlayerState previous;
    float alpha = 0.f;
};

// ----------------------
// Engine / networking
// ----------------------
class NetworkManager {
public:
    NetworkManager(uint32_t myId) : myId(myId) {
        mmw_set_log_level(MMW_LOG_LEVEL_OFF);
        mmw_initialize("127.0.0.1", 5000);
        mmw_create_publisher("input");
        // subscribe with static wrapper
        instance = this;
        mmw_create_subscriber_raw("state", NetworkManager::staticStateCallback);
    }

    ~NetworkManager() {
        mmw_cleanup();
    }

    void sendPosition(PlayerState& me) {
        if (me.x != lastSent.x || me.y != lastSent.y) {
            mmw_publish_raw("input", &me, sizeof(PlayerState), MMW_BEST_EFFORT);
            lastSent = me;
        }
    }

    void updateInterpolation(float dt) {
        std::lock_guard<std::mutex> lk(stateMutex);
        for (std::unordered_map<uint32_t, RemotePlayer>::iterator it = remotePlayers.begin();
             it != remotePlayers.end(); ++it) {
            it->second.alpha += dt * 10.f;
            if (it->second.alpha > 1.f) it->second.alpha = 1.f;
        }
    }

    std::unordered_map<uint32_t, RemotePlayer> getRemotePlayers() {
        std::lock_guard<std::mutex> lk(stateMutex);
        return remotePlayers;
    }

private:
    uint32_t myId;
    PlayerState lastSent;
    std::mutex stateMutex;
    std::unordered_map<uint32_t, RemotePlayer> remotePlayers;

    static NetworkManager* instance;

    static void staticStateCallback(void* data) {
        if (instance) instance->stateCallback(data);
    }

    void stateCallback(void* data) {
        PlayerState* s = reinterpret_cast<PlayerState*>(data);
        if (s->playerId == myId) return;

        std::lock_guard<std::mutex> lk(stateMutex);
        RemotePlayer& rp = remotePlayers[s->playerId];
        rp.previous = rp.current;
        rp.current = *s;
        rp.alpha = 0.f;
    }
};

// Define static member
NetworkManager* NetworkManager::instance = nullptr;

// ----------------------
// Rendering / utils
// ----------------------
float lerp(float a, float b, float t) { return a + t * (b - a); }

void resizeView(sf::RenderWindow& window, sf::View& view) {
    const float targetWidth = 1920.f;
    const float targetHeight = 1080.f;
    float windowRatio = float(window.getSize().x) / window.getSize().y;
    float targetRatio = targetWidth / targetHeight;
    float sizeX = 1.f, sizeY = 1.f, posX = 0.f, posY = 0.f;

    if (windowRatio > targetRatio) {
        sizeX = targetRatio / windowRatio;
        posX = (1.f - sizeX) / 2.f;
    } else if (windowRatio < targetRatio) {
        sizeY = windowRatio / targetRatio;
        posY = (1.f - sizeY) / 2.f;
    }

    view.setViewport(sf::FloatRect(posX, posY, sizeX, sizeY));
}

// ----------------------
// Main game
// ----------------------
int main(int argc, char** argv) {
    uint32_t myId = (argc >= 2) ? atoi(argv[1]) : 1;
    std::cout << "My ID: " << myId << std::endl;

    NetworkManager network(myId);

    sf::RenderWindow window(sf::VideoMode(1280,720), "MMW Client", sf::Style::Default);
    sf::View view(sf::FloatRect(0,0,1920,1080));
    window.setView(view);

    PlayerState me{myId, 100.f, 100.f};
    const float playerSize = 32.f;

    sf::Clock sendClock, deltaClock;

    while (window.isOpen()) {
        sf::Event e;
        float dx=0.f, dy=0.f;

        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) window.close();
            else if (e.type == sf::Event::Resized) resizeView(window, view);
        }

        float dt = deltaClock.restart().asSeconds();

        // Input
        if (window.hasFocus()) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) dx -= 200.f * dt;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) dx += 200.f * dt;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) dy -= 200.f * dt;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) dy += 200.f * dt;
        }

        me.x += dx;
        me.y += dy;

        // Send updates ~60Hz if moved
        if (sendClock.getElapsedTime().asMilliseconds() > 16) {
            network.sendPosition(me);
            sendClock.restart();
        }

        network.updateInterpolation(dt);

        // Render
        window.clear(sf::Color(50,50,50));
        window.setView(view);

        sf::RectangleShape background(sf::Vector2f(1920.f,1080.f));
        background.setFillColor(sf::Color(0,100,200));
        background.setPosition(0.f,0.f);
        window.draw(background);

        std::unordered_map<uint32_t, RemotePlayer> remotePlayers = network.getRemotePlayers();
        for (std::unordered_map<uint32_t, RemotePlayer>::iterator it = remotePlayers.begin();
             it != remotePlayers.end(); ++it) {
            const RemotePlayer& rp = it->second;
            float ix = lerp(rp.previous.x, rp.current.x, rp.alpha);
            float iy = lerp(rp.previous.y, rp.current.y, rp.alpha);
            sf::RectangleShape r(sf::Vector2f(playerSize, playerSize));
            r.setFillColor(sf::Color::Red);
            r.setOrigin(playerSize/2, playerSize/2);
            r.setPosition(ix, iy);
            window.draw(r);
        }

        // Own player
        sf::RectangleShape meBox(sf::Vector2f(playerSize, playerSize));
        meBox.setFillColor(sf::Color::Green);
        meBox.setOrigin(playerSize/2, playerSize/2);
        meBox.setPosition(me.x, me.y);
        window.draw(meBox);

        window.display();
    }

    return 0;
}
