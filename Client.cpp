// Client.cpp
#include <SFML/Graphics.hpp>
#include <iostream>
#include <thread>
#include <mutex>
#include <unordered_map>
#include "MMW.h"

// ---------------------------
// Data structures
// ---------------------------
struct PlayerState {
    uint32_t playerId;
    float x;
    float y;
};

// ---------------------------
// Player class
// ---------------------------
class Player {
public:
    Player(uint32_t id, float x, float y, bool isLocal)
        : playerId(id), current({id, x, y}), previous(current), alpha(1.f), local(isLocal) {}

    void updateRemote(const PlayerState& newState) {
        previous = current;
        current = newState;
        alpha = 0.f;
    }

    PlayerState getState() const { return current; }

    uint32_t playerId;
    PlayerState current;
    PlayerState previous;
    float alpha;
    bool local;
};

// ---------------------------
// Network manager
// ---------------------------
class NetworkManager {
public:
    NetworkManager(uint32_t myId) : myId(myId) {
        mmw_set_log_level(MMW_LOG_LEVEL_OFF);
        mmw_initialize("127.0.0.1", 5000);
        mmw_create_publisher("input");
        mmw_create_subscriber_raw("state", NetworkManager::stateCallbackStatic);
        instance = this;
    }

    ~NetworkManager() { mmw_cleanup(); }

    void sendPosition(const PlayerState& state) {
        // Only send if moved
        mmw_publish_raw("input", (void*)&state, sizeof(PlayerState), MMW_BEST_EFFORT);
    }

    Player* getPlayer(uint32_t id) {
        std::lock_guard<std::mutex> lk(mtx);
        auto it = players.find(id);
        return it != players.end() ? it->second : NULL;
    }

    void addPlayer(Player* p) {
        std::lock_guard<std::mutex> lk(mtx);
        players[p->playerId] = p;
    }

    std::unordered_map<uint32_t, Player*> players;
    std::mutex mtx;
    uint32_t myId;

private:
    static NetworkManager* instance;

    static void stateCallbackStatic(void* data) {
        if (!instance) return;
        instance->stateCallback(data);
    }

    void stateCallback(void* data) {
        PlayerState* s = reinterpret_cast<PlayerState*>(data);
        if (s->playerId == myId) return;

        Player* rp = getPlayer(s->playerId);
        if (!rp) {
            rp = new Player(s->playerId, s->x, s->y, false);
            addPlayer(rp);
        } else {
            rp->updateRemote(*s);
        }
    }
};

NetworkManager* NetworkManager::instance = nullptr;

// ---------------------------
// Utilities
// ---------------------------
float lerp(float a, float b, float t) { return a + t * (b - a); }

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

// ---------------------------
// Main
// ---------------------------
int main(int argc, char** argv) {
    uint32_t myId = 1;
    if (argc >= 2) myId = atoi(argv[1]);
    std::cout << "My ID: " << myId << std::endl;

    NetworkManager network(myId);

    sf::RenderWindow window(sf::VideoMode(1280, 720), "MMW Client", sf::Style::Default);
    sf::View view(sf::FloatRect(0, 0, 1920, 1080));
    window.setView(view);

    Player me(myId, 100.f, 100.f, true);
    const float playerSize = 32.f;

    sf::Clock sendClock;
    sf::Clock deltaClock;

    while (window.isOpen()) {
        sf::Event e;
        float dx = 0, dy = 0;

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

        me.current.x += dx;
        me.current.y += dy;

        // Send own updated position only if moved
        if ((dx != 0.f || dy != 0.f) && sendClock.getElapsedTime().asMilliseconds() > 16) {
            network.sendPosition(me.getState());
            sendClock.restart();
        }

        // Update remote players
        {
            std::lock_guard<std::mutex> lk(network.mtx);
            for (auto& kv : network.players) {
                Player* rp = kv.second;
                rp->alpha += dt * 10.f;
                if (rp->alpha > 1.f) rp->alpha = 1.f;
            }
        }

        // Render
        window.clear(sf::Color(50, 50, 50));
        window.setView(view);

        sf::RectangleShape background(sf::Vector2f(1920.f, 1080.f));
        background.setFillColor(sf::Color(0, 100, 200));
        background.setPosition(0.f, 0.f);
        window.draw(background);

        // Remote players
        {
            std::lock_guard<std::mutex> lk(network.mtx);
            for (auto& kv : network.players) {
                Player* rp = kv.second;
                if (rp->local) continue;
                float ix = lerp(rp->previous.x, rp->current.x, rp->alpha);
                float iy = lerp(rp->previous.y, rp->current.y, rp->alpha);
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
        playerBox.setPosition(me.current.x, me.current.y);
        window.draw(playerBox);

        window.display();
    }

    return 0;
}
