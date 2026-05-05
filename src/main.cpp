#include <SFML/Graphics.hpp>
#include "../include/AudioEngine.h"
#include "../include/AudioListener.h"
#include "../include/AudioSource.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>

const float PI = 3.14159265358979323846f;

// CẤU TRÚC HẠT (PARTICLE) CHO HIỆU ỨNG LẤP LÁNH
struct Particle {
    sf::CircleShape shape;
    sf::Vector2f velocity;
    float lifetime;
    float initialLifetime;
};

int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    AudioEngine engine;
    engine.Init();
    engine.SetDopplerFactor(1.0f);

    AudioSource sound;
    if (!sound.LoadWav("siren.wav")) return -1;
    sound.SetLooping(false);
    AudioListener::SetPosition(0.0f, 0.0f, 0.0f);

    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(desktop, "HUST 3D Audio & Particle Trail", sf::Style::Fullscreen);
    window.setFramerateLimit(60);

    float screenW = desktop.width;
    float screenH = desktop.height;
    float fov = 600.0f;

    sf::RectangleShape shape1(sf::Vector2f(100, 100)); 
    sf::CircleShape shape2(60);                        
    sf::CircleShape shape3(70, 3);                     
    sf::CircleShape shape4(70, 5);                     
    sf::RectangleShape shape5(sf::Vector2f(50, 150));  
    sf::CircleShape shape6(70, 6);                     
    sf::CircleShape shape7(80, 4);                     
    sf::CircleShape shape8(80, 8);                     

    sf::Shape* shapes[8] = { &shape1, &shape2, &shape3, &shape4, &shape5, &shape6, &shape7, &shape8 };
    sf::Color colors[8] = {
        sf::Color::Cyan, sf::Color::Red, sf::Color::Green, sf::Color::Yellow,
        sf::Color::Magenta, sf::Color::White, sf::Color(255, 165, 0), sf::Color(100, 150, 255)
    };
    
    for (int i = 0; i < 8; ++i) {
        shapes[i]->setFillColor(colors[i]);
        sf::FloatRect bounds = shapes[i]->getLocalBounds();
        shapes[i]->setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
    }

    // Danh sách chứa các hạt lấp lánh đang hiển thị
    std::vector<Particle> trailParticles;

    int spacePressCount = 0;
    bool isMoving = false;
    int currentPattern = 0;
    float elapsedTime = 0.0f;
    float travelTime = 5.0f; // ĐÃ TĂNG LÊN ĐỂ BAY CHẬM HƠN

    float prevAlX = 0, prevAlY = 0, prevAlZ = 0;
    bool firstFrame = true;

    sf::Clock clock;

    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        sf::Event event;

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed || 
               (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)) 
                window.close();
            
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space) {
                if (!isMoving && spacePressCount < 10) {
                    spacePressCount++;
                    currentPattern = (spacePressCount - 1) % 8; 
                    isMoving = true;
                    elapsedTime = 0.0f;
                    firstFrame = true;
                    trailParticles.clear(); // Xóa vết cũ khi bắn hạt mới

                    // Các quỹ đạo vòng lượn sẽ bay chậm hơn nữa (7 giây)
                    if (currentPattern == 3 || currentPattern == 5 || currentPattern == 7) travelTime = 7.0f;
                    else travelTime = 5.0f;

                    sound.Play();
                }
            }
        }

        window.clear(sf::Color::Black);

        if (isMoving) {
            elapsedTime += deltaTime;
            float t = elapsedTime / travelTime;
            if (t >= 1.0f) { t = 1.0f; isMoving = false; }

            float curX = 0.0f, curY = 0.0f, curZ = 0.0f;
            float angle = t * 2.0f * PI;

            // QUỸ ĐẠO
            switch (currentPattern) {
                case 0: curX = -20.0f + (t * 40.0f); curY = 0.0f; curZ = 5.0f; break;
                case 1: curX = 0.0f; curY = 0.0f; curZ = 50.0f - (t * 65.0f); break;
                case 2: curX = 20.0f - (t * 40.0f); curY = 10.0f - (t * 20.0f); curZ = 40.0f - (t * 38.0f); break;
                case 3: curX = 15.0f * std::sin(angle); curY = 0.0f; curZ = 15.0f * std::cos(angle); break;
                case 4: curX = 0.0f; curY = 20.0f - (t * 40.0f); curZ = 30.0f - (t * 25.0f); break;
                case 5: curX = 0.0f; curY = 15.0f * std::sin(angle); curZ = 15.0f * std::cos(angle); break;
                case 6: curX = 10.0f * std::sin(t * 4.0f * PI); curY = 0.0f; curZ = 40.0f - (t * 35.0f); break;
                case 7: curX = 10.0f * std::sin(angle * 2.0f); curZ = 10.0f * std::cos(angle * 2.0f); curY = 15.0f - (t * 30.0f); break;
            }

            if (std::abs(curZ) < 0.1f) curZ = 0.1f * (curZ < 0 ? -1 : 1);

            // ÂM THANH
            float alX = curX, alY = curY, alZ = -curZ; 
            if (firstFrame) { prevAlX = alX; prevAlY = alY; prevAlZ = alZ; firstFrame = false; }

            float vX = 0, vY = 0, vZ = 0;
            if (deltaTime > 0) {
                vX = (alX - prevAlX) / deltaTime;
                vY = (alY - prevAlY) / deltaTime;
                vZ = (alZ - prevAlZ) / deltaTime;
            }

            sound.SetPosition(alX, alY, alZ);
            sound.SetVelocity(vX, vY, vZ);
            prevAlX = alX; prevAlY = alY; prevAlZ = alZ;

            // ĐỒ HỌA & HIỆU ỨNG HẠT
            if (curZ > 0.0f) {
                float screenX = screenW / 2.0f + (curX / curZ) * fov;
                float screenY = screenH / 2.0f - (curY / curZ) * fov;
                float baseScale = 5.0f / curZ; 
                
                // 1. SINH RA HẠT MỚI (Tạo vệt lấp lánh)
                // Sinh ra 3 hạt mỗi frame
                for(int i = 0; i < 3; i++) {
                    Particle p;
                    // Kích thước hạt ngẫu nhiên, to nhỏ tùy khoảng cách Z
                    p.shape.setRadius(((std::rand() % 5) + 2) * baseScale); 
                    p.shape.setOrigin(p.shape.getRadius(), p.shape.getRadius());
                    
                    // Vị trí hạt mọc ra xung quanh vật thể chính
                    float offsetX = (std::rand() % 40 - 20) * baseScale;
                    float offsetY = (std::rand() % 40 - 20) * baseScale;
                    p.shape.setPosition(screenX + offsetX, screenY + offsetY);
                    
                    // Thỉnh thoảng có hạt màu trắng lóe sáng
                    sf::Color pColor = colors[currentPattern];
                    if (std::rand() % 4 == 0) pColor = sf::Color::White;
                    p.shape.setFillColor(pColor);
                    
                    // Bay tản ra xung quanh nhẹ nhàng
                    p.velocity = sf::Vector2f((std::rand() % 100 - 50) / 10.0f, (std::rand() % 100 - 50) / 10.0f);
                    p.initialLifetime = 0.5f + (std::rand() % 50) / 100.0f; // Sống từ 0.5 đến 1.0 giây
                    p.lifetime = p.initialLifetime;
                    
                    trailParticles.push_back(p);
                }

                // 2. CẬP NHẬT VỊ TRÍ HÌNH CHÍNH
                shapes[currentPattern]->setPosition(screenX, screenY);
                shapes[currentPattern]->setScale(baseScale, baseScale);
            }
        }

        // 3. CẬP NHẬT VÀ VẼ HẠT LẤP LÁNH
        for (auto it = trailParticles.begin(); it != trailParticles.end(); ) {
            it->lifetime -= deltaTime;
            if (it->lifetime <= 0) {
                it = trailParticles.erase(it); // Xóa hạt đã hết vòng đời
            } else {
                // Di chuyển hạt
                it->shape.move(it->velocity * deltaTime * 60.0f);
                
                // Fade out (mờ dần theo thời gian)
                sf::Color c = it->shape.getFillColor();
                c.a = static_cast<sf::Uint8>((it->lifetime / it->initialLifetime) * 255);
                it->shape.setFillColor(c);
                
                window.draw(it->shape);
                ++it;
            }
        }

        // Vẽ vật thể chính đè lên trên lớp hạt
        if (isMoving && prevAlZ < 0.0f) { // prevAlZ < 0 nghĩa là curZ > 0 (trước mặt)
            window.draw(*shapes[currentPattern]);
        }

        window.display();
    }

    return 0;
}
