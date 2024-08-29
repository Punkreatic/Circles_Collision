#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <random>
#include <unordered_map>


struct Circle {
    sf::CircleShape shape;
    sf::Vector2f velocity;
    float radius;

    Circle(float r, sf::Vector2f position, sf::Vector2f vel)
        : radius(r), velocity(vel) {
        shape.setRadius(r);
        shape.setPosition(position);
        shape.setOrigin(r, r);
        shape.setFillColor(sf::Color(150, 50, 250));
        shape.setOutlineThickness(15);
        shape.setOutlineColor(sf::Color(250, 50, 100));
    }

    //обновление позиции круга
    void update(float deltaTime) {
        shape.move(velocity * deltaTime);
    }

    //проверка на столкновение с границами экрана
    void checkCollisionWithWindow(const sf::RenderWindow& window) {
        sf::Vector2f position = shape.getPosition();
        float windowWidth = static_cast<float>(window.getSize().x);
        float windowHeight = static_cast<float>(window.getSize().y);
        float thickness = shape.getOutlineThickness();

        if (position.x - radius < 0 || position.x + radius + thickness > windowWidth) {
            velocity.x = -velocity.x;
            shape.setPosition(std::max(radius, std::min(position.x, windowWidth - radius - thickness)), position.y);
        }
        if (position.y - radius < 0 || position.y + radius + thickness > windowHeight) {
            velocity.y = -velocity.y;
            shape.setPosition(position.x, std::max(radius, std::min(position.y, windowHeight - radius - thickness)));
        }
    }

    //проверка на столкновение с другим кругом
    bool checkCollisionWith(const Circle& other) const {
        float distance = sqrt(pow(shape.getPosition().x - other.shape.getPosition().x, 2) +
            pow(shape.getPosition().y - other.shape.getPosition().y, 2));
        return distance < (radius + other.radius);
    }

    //разрешение столкновения с другим кругом
    void resolveCollision(Circle& other) {
        sf::Vector2f delta = shape.getPosition() - other.shape.getPosition();
        float distance = sqrt(delta.x * delta.x + delta.y * delta.y);
        if (distance == 0) return;

        //Нормализация вектора и расчет перекрытия
        sf::Vector2f normalizedDelta = delta / distance;
        float overlap = (radius + other.radius) - distance;
        float moveAmount = overlap * 0.5f;

        //Перемещение кругов
        shape.setPosition(shape.getPosition() + normalizedDelta * moveAmount);
        other.shape.setPosition(other.shape.getPosition() - normalizedDelta * moveAmount);
        //Обмен скоростями
        std::swap(velocity, other.velocity);
    }
};

//обработки столкновений между кругами через сетку
class Grid {
public:
    Grid(float width, float height, float cellSize) : cellSize(cellSize) {
        cols = static_cast<int>(width / cellSize);
        rows = static_cast<int>(height / cellSize);
        cells.resize(cols * rows);  //Создается вектор для хранения всех ячеек
    }

    void clear() {
        for (auto& cell : cells) {
            cell.clear();
        }
    }

    void addCircle(Circle* circle) {
        int col = static_cast<int>(circle->shape.getPosition().x / cellSize);
        int row = static_cast<int>(circle->shape.getPosition().y / cellSize);
        if (col >= 0 && col < cols && row >= 0 && row < rows) {
            cells[col + row * cols].push_back(circle);
        }
    }

    //Находим потенциальные столкновения
    std::vector<Circle*> getPotentialCollisions(const Circle& circle) {
        std::vector<Circle*> potentialCollisions;

        int col = static_cast<int>(circle.shape.getPosition().x / cellSize);
        int row = static_cast<int>(circle.shape.getPosition().y / cellSize);

        for (int i = col - 1; i <= col + 1; ++i) {
            for (int j = row - 1; j <= row + 1; ++j) {
                if (i >= 0 && i < cols && j >= 0 && j < rows) {
                    for (Circle* other : cells[i + j * cols]) {
                        potentialCollisions.push_back(other);
                    }
                }
            }
        }

        return potentialCollisions;
    }

private:
    std::vector<std::vector<Circle*>> cells;
    float cellSize;
    int cols;
    int rows;
};

void checkCollisions(std::vector<Circle>& circles, Grid& grid) {
    for (Circle& circle : circles) {
        auto potentialCollisions = grid.getPotentialCollisions(circle);
        for (Circle* other : potentialCollisions) {
            if (&circle != other && circle.checkCollisionWith(*other)) {
                circle.resolveCollision(*other);
            }
        }
    }
}

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 800), "Ball Collision");
    window.setFramerateLimit(60);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> radiusDist(40.f, 100.f);
    std::uniform_real_distribution<float> velocityDist(-300.f, 300.f);

    std::vector<Circle> circles;

    for (int i = 0; i < 5; ++i) {
        float radius = radiusDist(gen);
        float posX = radius + std::uniform_real_distribution<float>(0.f, 800.f - 2 * radius)(gen);
        float posY = radius + std::uniform_real_distribution<float>(0.f, 800.f - 2 * radius)(gen);
        sf::Vector2f position(posX, posY);
        sf::Vector2f velocity(velocityDist(gen), velocityDist(gen));
        circles.emplace_back(radius, position, velocity);
    }

    Grid grid(window.getSize().x, window.getSize().y, 100.0f);
    sf::Clock clock;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        float deltaTime = clock.restart().asSeconds();
        grid.clear();

        for (auto& circle : circles) {
            circle.update(deltaTime);
            circle.checkCollisionWithWindow(window);
            grid.addCircle(&circle); // Добавляем круги в сетку
        }

        checkCollisions(circles, grid);

        window.clear();
        for (const auto& circle : circles) {
            window.draw(circle.shape);
        }
        window.display();
    }

    return 0;
}