#include <SFML/Graphics.hpp>
#include "../include/AudioEngine.h"
#include "../include/AudioListener.h"
#include "../include/AudioSource.h"
#include <iostream>
#include <cmath>
#include <algorithm>

const float PI = 3.14159265358979323846f;

int main() {
    // 1. KHỞI TẠO ÂM THANH
    AudioEngine engine;
    engine.Init();
    engine.SetDopplerFactor(1.0f);

    AudioSource sound;
    if (!sound.LoadWav("siren.wav")) {
        std::cerr << "Lỗi: Không tìm thấy file siren.wav!" << std::endl;
        return -1;
    }
    sound.SetLooping(false);
    AudioListener::SetPosition(0.0f, 0.0f, 0.0f);

    // 2. KHỞI TẠO ĐỒ HỌA (FULLSCREEN)
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(desktop, "HUST 3D Audio Paths", sf::Style::Fullscreen);
    window.setFramerateLimit(60);

    float screenW = desktop.width;
    float screenH = desktop.height;
    float margin = 100.0f; // Lề an toàn để hình không bao giờ tràn màn hình

    // --- CHUẨN BỊ 8 HÌNH DÁNG VÀ MÀU SẮC KHÁC NHAU ---
    sf::RectangleShape shape1(sf::Vector2f(120, 40));     // 1. Chữ nhật ngang
    sf::CircleShape shape2(50);                           // 2. Hình tròn
    sf::CircleShape shape3(55, 3);                        // 3. Hình tam giác
    sf::CircleShape shape4(55, 5);                        // 4. Hình ngũ giác
    sf::RectangleShape shape5(sf::Vector2f(40, 120));     // 5. Chữ nhật dọc
    sf::CircleShape shape6(55, 6);                        // 6. Hình lục giác
    sf::CircleShape shape7(60, 4);                        // 7. Hình thoi (vuông nghiêng)
    sf::CircleShape shape8(60, 8);                        // 8. Hình bát giác

    sf::Shape* shapes[8] = { &shape1, &shape2, &shape3, &shape4, &shape5, &shape6, &shape7, &shape8 };
    sf::Color colors[8] = {
        sf::Color::Cyan, sf::Color::Red, sf::Color::Green, sf::Color::Yellow,
        sf::Color::Magenta, sf::Color::White, sf::Color(255, 165, 0), sf::Color(100, 150, 255)
    };
    std::string pathNames[8] = {
        "Trai -> Phai", "Phai -> Trai", "Cheo Len (Trai duoi -> Phai tren)", "Cheo Xuong (Trai tren -> Phai duoi)",
        "Tren -> Duoi", "Duoi -> Tren", "Quỹ đạo Tròn (Circle)", "Quỹ đạo Vuông (Square)"
    };

    // Đặt tâm (Origin) của tất cả các hình vào chính giữa để vẽ chính xác
    for (int i = 0; i < 8; ++i) {
        shapes[i]->setFillColor(colors[i]);
        sf::FloatRect bounds = shapes[i]->getLocalBounds();
        shapes[i]->setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
    }

    // 3. BIẾN TRẠNG THÁI TRÒ CHƠI
    int spacePressCount = 0;
    bool isMoving = false;
    int currentPattern = 0; // 0 đến 7

    float elapsedTime = 0.0f;
    float travelTime = 2.0f; // Sẽ thay đổi tùy độ dài quỹ đạo
    
    // Lưu tọa độ ảo khung hình trước để tính Đạo Hàm Vận Tốc (Doppler)
    float prevVirtualX = 0.0f;
    float prevVirtualY = 0.0f;
    bool firstFrame = true;

    sf::Clock clock;

    std::cout << "=========================================" << std::endl;
    std::cout << "[GAME] KICH HOAT CHUOI QUY DAO 8 HUONG!" << std::endl;
    std::cout << "[GAME] Nhan SPACE de ban (10 lan). Nhan ESC de thoat." << std::endl;
    std::cout << "=========================================" << std::endl;

    // 4. VÒNG LẶP GAME
    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        sf::Event event;

        // XỬ LÝ INPUT
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) window.close();
            
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space) {
                if (!isMoving && spacePressCount < 10) {
                    spacePressCount++;
                    // Pattern từ 0 đến 7. Nếu lượt 9, 10 thì quay vòng lại pattern đầu
                    currentPattern = (spacePressCount - 1) % 8; 
                    
                    isMoving = true;
                    elapsedTime = 0.0f;
                    firstFrame = true; // Báo hiệu frame đầu tiên của quỹ đạo mới

                    // Thời gian bay: Các hình lượn vòng dài hơn cần nhiều thời gian hơn
                    if (currentPattern >= 6) travelTime = 4.0f; 
                    else travelTime = 2.0f; 

                    sound.Play();
                    std::cout << "[BAM] Lan " << spacePressCount << " | Kieu: " << pathNames[currentPattern] << std::endl;
                } else if (spacePressCount == 10) {
                    std::cout << "[THONG BAO] Da xong 10 luot! Nhan ESC de thoat." << std::endl;
                    spacePressCount++;
                }
            }
        }

        // CẬP NHẬT LOGIC KHÔNG GIAN (UPDATE)
        if (isMoving) {
            elapsedTime += deltaTime;
            float ratio = elapsedTime / travelTime;
            if (ratio >= 1.0f) {
                ratio = 1.0f;
                isMoving = false;
            }

            float curX = 0.0f, curY = 0.0f;

            // KIỂM SOÁT 8 QUỸ ĐẠO DỰA VÀO RATIO (0.0 -> 1.0)
            if (currentPattern == 0) { // L -> R
                curX = margin + ratio * (screenW - 2 * margin);
                curY = screenH / 2.0f;
            } else if (currentPattern == 1) { // R -> L
                curX = (screenW - margin) - ratio * (screenW - 2 * margin);
                curY = screenH / 2.0f;
            } else if (currentPattern == 2) { // Chéo Lên
                curX = margin + ratio * (screenW - 2 * margin);
                curY = (screenH - margin) - ratio * (screenH - 2 * margin);
            } else if (currentPattern == 3) { // Chéo Xuống
                curX = margin + ratio * (screenW - 2 * margin);
                curY = margin + ratio * (screenH - 2 * margin);
            } else if (currentPattern == 4) { // Trên -> Dưới
                curX = screenW / 2.0f;
                curY = margin + ratio * (screenH - 2 * margin);
            } else if (currentPattern == 5) { // Dưới -> Trên
                curX = screenW / 2.0f;
                curY = (screenH - margin) - ratio * (screenH - 2 * margin);
            } else if (currentPattern == 6) { // Tròn (Circle)
                float angle = ratio * 2.0f * PI; // Quay đủ 360 độ
                float R = std::min(screenW, screenH) / 2.0f - margin;
                curX = screenW / 2.0f + R * std::cos(angle);
                curY = screenH / 2.0f + R * std::sin(angle);
            } else if (currentPattern == 7) { // Vuông (Square)
                float seg = ratio * 4.0f; // 4 cạnh của hình vuông
                float R = std::min(screenW, screenH) / 2.0f - margin;
                float left = screenW/2 - R, right = screenW/2 + R;
                float top = screenH/2 - R, bot = screenH/2 + R;

                if (seg < 1.0f) { curX = left + seg * (right - left); curY = top; }
                else if (seg < 2.0f) { curX = right; curY = top + (seg - 1.0f) * (bot - top); }
                else if (seg < 3.0f) { curX = right - (seg - 2.0f) * (right - left); curY = bot; }
                else { curX = left; curY = bot - (seg - 3.0f) * (bot - top); }
            }

            // --- ĐỒNG BỘ ÂM THANH 3D VÀ ĐẠO HÀM VẬN TỐC ---
            // Ánh xạ X, Y của màn hình sang hệ trục OpenAL: X(-20 tới 20), Y(10 tới -10)
            float virtualX = ((curX / screenW) - 0.5f) * 40.0f;
            float virtualY = (0.5f - (curY / screenH)) * 20.0f; 

            if (firstFrame) {
                prevVirtualX = virtualX;
                prevVirtualY = virtualY;
                firstFrame = false;
            }

            // Tính đạo hàm: V = ds/dt
            float vX = 0.0f, vY = 0.0f;
            if (deltaTime > 0.0f) {
                vX = (virtualX - prevVirtualX) / deltaTime;
                vY = (virtualY - prevVirtualY) / deltaTime;
            }

            // Cập nhật lên Audio Engine
            sound.SetPosition(virtualX, virtualY, 0.0f);
            sound.SetVelocity(vX, vY, 0.0f);

            // Cập nhật đồ họa
            shapes[currentPattern]->setPosition(curX, curY);

            // Lưu tọa độ cho frame sau
            prevVirtualX = virtualX;
            prevVirtualY = virtualY;
        }

        // VẼ HÌNH LÊN MÀN HÌNH (RENDER)
        window.clear(sf::Color::Black);
        if (isMoving) {
            window.draw(*shapes[currentPattern]);
        }
        window.display();
    }

    return 0;
}
