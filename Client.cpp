#include "MMW.h"
#include <SFML/Graphics.hpp>
#include <map>
#include <string>
#include <sstream>

struct Player {
    sf::RectangleShape box;
};

std::map<int, Player> players;

void handleServerUpdate(const char* msg) {
    // msg format: "id:x,y;id:x,y;..."
    std::istringstream ss(msg);
    std::string token;
    while (std::getline(ss, token, ';')) {
        if (token.empty()) continue;
        int id;
        float x, y;
        char colon, comma;
        std::istringstream pairStream(token);
        pairStream >> id >> colon >> x >> comma >> y;
        if (players.find(id) == players.end()) {
            players[id].box.setSize(sf::Vector2f(50,50));
            players[id].box.setFillColor(id == 1 ? sf::Color::Red : sf::Color::Blue);
        }
        players[id].box.setPosition(x, y);
    }
}

int main() {
    const int myId = 1;
    float x = 100, y = 100;

    mmw_set_log_level(MMW_LOG_LEVEL_INFO);
    mmw_initialize("127.0.0.1", 5000);
    mmw_create_subscriber("positions", handleServerUpdate);

    sf::RenderWindow window(sf::VideoMode(800, 600), "MMW Demo Client");

    players[myId].box.setSize(sf::Vector2f(50,50));
    players[myId].box.setFillColor(sf::Color::Red);

    sf::Clock clock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        float speed = 200.0f; // pixels per second
        float dt = clock.restart().asSeconds();

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))  x -= speed * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) x += speed * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))    y -= speed * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))  y += speed * dt;

        // Update my player locally
        players[myId].box.setPosition(x, y);

        // Send update to server
        std::ostringstream out;
        out << myId << ":" << x << "," << y;
        mmw_publish("player_input", out.str().c_str(), MMW_RELIABLE);

        window.clear();
        for (auto& pair : players) {
            window.draw(pair.second.box);
        }
        window.display();
    }

    mmw_cleanup();
    return 0;
}
