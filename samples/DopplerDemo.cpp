#include <SFML/Graphics.hpp>

#include "AudioEngine.h"
#include "AudioEnvironment.h"
#include "AudioListener.h"
#include "AudioSource.h"
#include "Vector3.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

// ======================================================
// BASIC UI BUTTON
// ======================================================
class UIButton
{
private:
    sf::RectangleShape box;
    sf::Text label;

    bool enabled;
    bool active;
    bool hovered;

public:
    UIButton(
        const sf::Font &font,
        const std::string &text,
        const sf::Vector2f &position,
        const sf::Vector2f &size)
        : box(size),
          label(font, text, 16),
          enabled(true),
          active(false),
          hovered(false)
    {
        box.setPosition(position);
        box.setOutlineThickness(1.0f);
        box.setOutlineColor(sf::Color(120, 120, 120));
        label.setFillColor(sf::Color::White);

        CenterText();
        UpdateVisual();
    }

    void SetLabel(const std::string &text)
    {
        label.setString(text);
        CenterText();
    }

    void SetEnabled(bool value)
    {
        enabled = value;
        UpdateVisual();
    }

    void SetActive(bool value)
    {
        active = value;
        UpdateVisual();
    }

    void SetHovered(bool value)
    {
        hovered = value;
        UpdateVisual();
    }

    bool Contains(const sf::Vector2f &point) const
    {
        if (!enabled)
        {
            return false;
        }

        sf::FloatRect bounds = box.getGlobalBounds();

        return point.x >= bounds.position.x &&
               point.x <= bounds.position.x + bounds.size.x &&
               point.y >= bounds.position.y &&
               point.y <= bounds.position.y + bounds.size.y;
    }

    void Draw(sf::RenderWindow &window) const
    {
        window.draw(box);
        window.draw(label);
    }

private:
    void CenterText()
    {
        sf::FloatRect textBounds = label.getLocalBounds();

        label.setOrigin(
            sf::Vector2f(
                textBounds.position.x + textBounds.size.x / 2.0f,
                textBounds.position.y + textBounds.size.y / 2.0f));

        sf::FloatRect boxBounds = box.getGlobalBounds();

        label.setPosition(
            sf::Vector2f(
                boxBounds.position.x + boxBounds.size.x / 2.0f,
                boxBounds.position.y + boxBounds.size.y / 2.0f - 1.0f));
    }

    void UpdateVisual()
    {
        if (!enabled)
        {
            box.setFillColor(sf::Color(55, 55, 55));
            box.setOutlineColor(sf::Color(80, 80, 80));
            label.setFillColor(sf::Color(150, 150, 150));
            return;
        }

        if (active)
        {
            box.setFillColor(sf::Color(40, 120, 70));
            box.setOutlineColor(sf::Color(120, 220, 160));
            label.setFillColor(sf::Color::White);
            return;
        }

        if (hovered)
        {
            box.setFillColor(sf::Color(70, 80, 100));
            box.setOutlineColor(sf::Color(170, 190, 230));
            label.setFillColor(sf::Color::White);
            return;
        }

        box.setFillColor(sf::Color(42, 48, 60));
        box.setOutlineColor(sf::Color(110, 120, 140));
        label.setFillColor(sf::Color::White);
    }
};

// ======================================================
// VISUAL EFFECT DATA
// ======================================================
struct SoundWaveRing
{
    sf::Vector2f center;
    float radius;
    float maxRadius;
    float life;
    float maxLife;
};

struct TrailParticle
{
    sf::Vector2f position;
    sf::Vector2f velocity;
    float radius;
    float life;
    float maxLife;
};

// ======================================================
// HELPERS
// ======================================================
static bool TryLoadFont(sf::Font &font)
{
    if (font.openFromFile("assets/arial.ttf"))
    {
        return true;
    }

    if (font.openFromFile("assets/ui_font.ttf"))
    {
        return true;
    }

    if (font.openFromFile("C:/Windows/Fonts/arial.ttf"))
    {
        return true;
    }

    if (font.openFromFile("C:/Windows/Fonts/segoeui.ttf"))
    {
        return true;
    }

    return false;
}

static bool TryLoadSiren(AudioSource &source)
{
    if (source.LoadWav("assets/siren.wav"))
    {
        return true;
    }

    if (source.LoadWav("../assets/siren.wav"))
    {
        return true;
    }

    if (source.LoadWav("siren.wav"))
    {
        return true;
    }

    return false;
}

static sf::Vector2f WorldToScreen(
    const Vector3 &position,
    float sceneWidth,
    float sceneHeight)
{
    float scale = 18.0f;

    float x = sceneWidth / 2.0f + position.x * scale;
    float y = sceneHeight / 2.0f + position.z * scale;

    return sf::Vector2f(x, y);
}

static void DrawLine(
    sf::RenderWindow &window,
    const sf::Vector2f &a,
    const sf::Vector2f &b,
    const sf::Color &color)
{
    sf::Vertex line[2];

    line[0].position = a;
    line[0].color = color;

    line[1].position = b;
    line[1].color = color;

    window.draw(line, 2, sf::PrimitiveType::Lines);
}

static float Clamp01(float value)
{
    return std::max(0.0f, std::min(1.0f, value));
}

static void SpawnTrailParticles(
    std::vector<TrailParticle> &particles,
    const sf::Vector2f &sourceScreenPos)
{
    for (int i = 0; i < 3; ++i)
    {
        TrailParticle p;

        p.position = sourceScreenPos;

        float spreadY = static_cast<float>((std::rand() % 60) - 30);

        p.velocity = sf::Vector2f(
            -90.0f - static_cast<float>(std::rand() % 40),
            spreadY * 0.15f);

        p.radius = 2.0f + static_cast<float>(std::rand() % 4);
        p.maxLife = 0.45f + static_cast<float>(std::rand() % 40) / 100.0f;
        p.life = p.maxLife;

        particles.push_back(p);
    }
}

static void SpawnWaveRing(
    std::vector<SoundWaveRing> &rings,
    const sf::Vector2f &sourceScreenPos)
{
    SoundWaveRing ring;

    ring.center = sourceScreenPos;
    ring.radius = 12.0f;
    ring.maxRadius = 150.0f;
    ring.maxLife = 0.9f;
    ring.life = ring.maxLife;

    rings.push_back(ring);
}

static void UpdateTrailParticles(
    std::vector<TrailParticle> &particles,
    float deltaTime)
{
    for (auto it = particles.begin(); it != particles.end();)
    {
        it->life -= deltaTime;

        if (it->life <= 0.0f)
        {
            it = particles.erase(it);
        }
        else
        {
            it->position += it->velocity * deltaTime;
            ++it;
        }
    }
}

static void UpdateWaveRings(
    std::vector<SoundWaveRing> &rings,
    float deltaTime)
{
    for (auto it = rings.begin(); it != rings.end();)
    {
        it->life -= deltaTime;
        it->radius += 140.0f * deltaTime;

        if (it->life <= 0.0f || it->radius >= it->maxRadius)
        {
            it = rings.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

static void DrawGridBackground(
    sf::RenderWindow &window,
    float sceneWidth,
    float sceneHeight,
    float topBarHeight,
    bool reverbEnabled)
{
    sf::Color bgColor = reverbEnabled
                            ? sf::Color(18, 22, 32)
                            : sf::Color(15, 18, 24);

    sf::RectangleShape bg(sf::Vector2f(sceneWidth, sceneHeight - topBarHeight));
    bg.setPosition(sf::Vector2f(0.0f, topBarHeight));
    bg.setFillColor(bgColor);
    window.draw(bg);

    float gridSpacing = 40.0f;

    for (float x = 0.0f; x <= sceneWidth; x += gridSpacing)
    {
        sf::RectangleShape line(sf::Vector2f(1.0f, sceneHeight - topBarHeight));
        line.setPosition(sf::Vector2f(x, topBarHeight));
        line.setFillColor(sf::Color(35, 40, 52));
        window.draw(line);
    }

    for (float y = topBarHeight; y <= sceneHeight; y += gridSpacing)
    {
        sf::RectangleShape line(sf::Vector2f(sceneWidth, 1.0f));
        line.setPosition(sf::Vector2f(0.0f, y));
        line.setFillColor(sf::Color(35, 40, 52));
        window.draw(line);
    }

    if (reverbEnabled)
    {
        sf::RectangleShape overlay(sf::Vector2f(sceneWidth, sceneHeight - topBarHeight));
        overlay.setPosition(sf::Vector2f(0.0f, topBarHeight));
        overlay.setFillColor(sf::Color(25, 20, 45, 60));
        window.draw(overlay);

        sf::CircleShape caveShade1(220.0f);
        caveShade1.setFillColor(sf::Color(20, 18, 28, 130));
        caveShade1.setPosition(sf::Vector2f(-120.0f, topBarHeight - 80.0f));
        window.draw(caveShade1);

        sf::CircleShape caveShade2(250.0f);
        caveShade2.setFillColor(sf::Color(20, 18, 28, 110));
        caveShade2.setPosition(sf::Vector2f(sceneWidth - 220.0f, sceneHeight - 280.0f));
        window.draw(caveShade2);
    }
}

static void DrawAttenuationRings(
    sf::RenderWindow &window,
    const sf::Vector2f &center)
{
    const float radii[] = {
        90.0f,
        180.0f,
        270.0f};

    for (float radius : radii)
    {
        sf::CircleShape ring(radius);

        ring.setOrigin(sf::Vector2f(radius, radius));
        ring.setPosition(center);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(1.5f);
        ring.setOutlineColor(sf::Color(70, 90, 120, 140));

        window.draw(ring);
    }
}

static void DrawAxes(
    sf::RenderWindow &window,
    const sf::Vector2f &center,
    float sceneWidth,
    float sceneHeight,
    float topBarHeight)
{
    DrawLine(
        window,
        sf::Vector2f(0.0f, center.y),
        sf::Vector2f(sceneWidth, center.y),
        sf::Color(70, 140, 220));

    DrawLine(
        window,
        sf::Vector2f(center.x, topBarHeight),
        sf::Vector2f(center.x, sceneHeight),
        sf::Color(120, 220, 120));
}

static void DrawWaveRings(
    sf::RenderWindow &window,
    const std::vector<SoundWaveRing> &rings)
{
    for (const auto &ring : rings)
    {
        float alphaRatio = Clamp01(ring.life / ring.maxLife);

        sf::CircleShape shape(ring.radius);

        shape.setOrigin(sf::Vector2f(ring.radius, ring.radius));
        shape.setPosition(ring.center);
        shape.setFillColor(sf::Color::Transparent);
        shape.setOutlineThickness(2.0f);

        std::uint8_t alpha = static_cast<std::uint8_t>(alphaRatio * 160.0f);

        shape.setOutlineColor(sf::Color(120, 220, 255, alpha));

        window.draw(shape);
    }
}

static void DrawTrailParticles(
    sf::RenderWindow &window,
    const std::vector<TrailParticle> &particles)
{
    for (const auto &p : particles)
    {
        float alphaRatio = Clamp01(p.life / p.maxLife);

        sf::CircleShape shape(p.radius);

        shape.setOrigin(sf::Vector2f(p.radius, p.radius));
        shape.setPosition(p.position);

        std::uint8_t alpha = static_cast<std::uint8_t>(alphaRatio * 180.0f);

        shape.setFillColor(sf::Color(255, 180, 110, alpha));

        window.draw(shape);
    }
}

static void DrawSourceVehicle(
    sf::RenderWindow &window,
    const sf::Vector2f &position,
    bool moving,
    bool occluded)
{
    std::uint8_t bodyAlpha = static_cast<std::uint8_t>(occluded ? 150 : 255);
    std::uint8_t glowAlpha = static_cast<std::uint8_t>(occluded ? 55 : 95);

    sf::CircleShape glow(30.0f);
    glow.setOrigin(sf::Vector2f(30.0f, 30.0f));
    glow.setPosition(position);
    glow.setFillColor(sf::Color(255, 120, 70, glowAlpha));
    window.draw(glow);

    sf::RectangleShape body(sf::Vector2f(54.0f, 24.0f));
    body.setOrigin(sf::Vector2f(27.0f, 12.0f));
    body.setPosition(position);
    body.setFillColor(sf::Color(225, 235, 245, bodyAlpha));
    body.setOutlineThickness(1.5f);
    body.setOutlineColor(sf::Color(40, 40, 50, bodyAlpha));
    window.draw(body);

    sf::RectangleShape roof(sf::Vector2f(28.0f, 12.0f));
    roof.setOrigin(sf::Vector2f(14.0f, 6.0f));
    roof.setPosition(position + sf::Vector2f(-4.0f, -12.0f));
    roof.setFillColor(sf::Color(210, 220, 235, bodyAlpha));
    roof.setOutlineThickness(1.0f);
    roof.setOutlineColor(sf::Color(40, 40, 50, bodyAlpha));
    window.draw(roof);

    sf::RectangleShape stripe(sf::Vector2f(48.0f, 5.0f));
    stripe.setOrigin(sf::Vector2f(24.0f, 2.5f));
    stripe.setPosition(position + sf::Vector2f(0.0f, -1.0f));
    stripe.setFillColor(sf::Color(220, 55, 55, bodyAlpha));
    window.draw(stripe);

    sf::RectangleShape lightBarBase(sf::Vector2f(18.0f, 5.0f));
    lightBarBase.setOrigin(sf::Vector2f(9.0f, 2.5f));
    lightBarBase.setPosition(position + sf::Vector2f(-3.0f, -18.0f));
    lightBarBase.setFillColor(sf::Color(30, 30, 35, bodyAlpha));
    window.draw(lightBarBase);

    sf::CircleShape redLight(4.0f);
    redLight.setOrigin(sf::Vector2f(4.0f, 4.0f));
    redLight.setPosition(position + sf::Vector2f(-9.0f, -18.0f));
    redLight.setFillColor(
        moving
            ? sf::Color(255, 70, 70, bodyAlpha)
            : sf::Color(140, 70, 70, bodyAlpha));
    window.draw(redLight);

    sf::CircleShape blueLight(4.0f);
    blueLight.setOrigin(sf::Vector2f(4.0f, 4.0f));
    blueLight.setPosition(position + sf::Vector2f(3.0f, -18.0f));
    blueLight.setFillColor(
        moving
            ? sf::Color(70, 150, 255, bodyAlpha)
            : sf::Color(70, 90, 140, bodyAlpha));
    window.draw(blueLight);

    sf::ConvexShape nose(3);
    nose.setPoint(0, sf::Vector2f(0.0f, 0.0f));
    nose.setPoint(1, sf::Vector2f(10.0f, -8.0f));
    nose.setPoint(2, sf::Vector2f(10.0f, 8.0f));
    nose.setPosition(position + sf::Vector2f(27.0f, 0.0f));
    nose.setFillColor(sf::Color(200, 210, 220, bodyAlpha));
    window.draw(nose);
}

static void DrawMiniMap(
    sf::RenderWindow &window,
    float panelX,
    float topBarHeight,
    const Vector3 &sourcePos,
    bool runningDemo)
{
    float radarX = panelX + 20.0f;
    float radarY = topBarHeight + 20.0f;
    float radarSize = 250.0f;

    sf::RectangleShape bg(sf::Vector2f(radarSize, radarSize));
    bg.setPosition(sf::Vector2f(radarX, radarY));
    bg.setFillColor(sf::Color(22, 26, 34));
    bg.setOutlineThickness(1.5f);
    bg.setOutlineColor(sf::Color(90, 100, 120));
    window.draw(bg);

    sf::Vector2f center(
        radarX + radarSize / 2.0f,
        radarY + radarSize / 2.0f);

    for (float radius : {35.0f, 70.0f, 105.0f})
    {
        sf::CircleShape ring(radius);

        ring.setOrigin(sf::Vector2f(radius, radius));
        ring.setPosition(center);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(1.0f);
        ring.setOutlineColor(sf::Color(70, 90, 120, 150));

        window.draw(ring);
    }

    DrawLine(
        window,
        sf::Vector2f(radarX, center.y),
        sf::Vector2f(radarX + radarSize, center.y),
        sf::Color(70, 90, 120));

    DrawLine(
        window,
        sf::Vector2f(center.x, radarY),
        sf::Vector2f(center.x, radarY + radarSize),
        sf::Color(70, 90, 120));

    sf::CircleShape listener(6.0f);
    listener.setOrigin(sf::Vector2f(6.0f, 6.0f));
    listener.setPosition(center);
    listener.setFillColor(sf::Color(80, 180, 255));
    window.draw(listener);

    float radarScale = 3.0f;

    sf::Vector2f sourceRadar(
        center.x + sourcePos.x * radarScale,
        center.y + sourcePos.z * radarScale);

    sf::CircleShape source(5.0f);
    source.setOrigin(sf::Vector2f(5.0f, 5.0f));
    source.setPosition(sourceRadar);
    source.setFillColor(
        runningDemo
            ? sf::Color(255, 120, 80)
            : sf::Color(130, 130, 130));

    window.draw(source);
}

int main()
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    AudioEngine engine;

    if (!engine.Init(true))
    {
        std::cerr << "Failed to initialize AudioEngine." << std::endl;
        return -1;
    }

    engine.SetDopplerFactor(1.0f, 343.3f);
    engine.SetDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

    AudioListener::SetPosition(Vector3(0.0f, 0.0f, 0.0f));
    AudioListener::SetVelocity(Vector3(0.0f, 0.0f, 0.0f));
    AudioListener::SetOrientation(
        Vector3(0.0f, 0.0f, -1.0f),
        Vector3(0.0f, 1.0f, 0.0f));

    AudioEnvironment caveEnvironment;

    bool reverbReady = caveEnvironment.Init(ReverbPreset::Cave);
    bool reverbEnabled = false;
    bool occlusionEnabled = false;

    AudioSource siren;

    if (!TryLoadSiren(siren))
    {
        std::cerr << "Cannot load siren.wav. Please put siren.wav in assets/ folder." << std::endl;
        return -1;
    }

    siren.SetLooping(true);
    siren.SetGain(1.0f);
    siren.SetReferenceDistance(6.0f);
    siren.SetMaxDistance(120.0f);
    siren.SetRolloffFactor(0.7f);

    const unsigned int windowWidth = 1366;
    const unsigned int windowHeight = 768;

    sf::RenderWindow window(
        sf::VideoMode({windowWidth, windowHeight}),
        "HUST 3D Audio Game Engine SDK - Enhanced Demo");

    window.setFramerateLimit(60);

    const float screenW = static_cast<float>(windowWidth);
    const float screenH = static_cast<float>(windowHeight);

    const float topBarHeight = 96.0f;
    const float sidePanelWidth = 300.0f;
    const float sceneWidth = screenW - sidePanelWidth;

    sf::Font font;

    if (!TryLoadFont(font))
    {
        std::cerr << "[UI] Cannot load font. Copy a .ttf into assets/arial.ttf or assets/ui_font.ttf" << std::endl;
    }

    sf::RectangleShape topBar(sf::Vector2f(screenW, topBarHeight));
    topBar.setPosition(sf::Vector2f(0.0f, 0.0f));
    topBar.setFillColor(sf::Color(25, 28, 36));

    sf::Text titleText(font, "HUST 3D Audio Game Engine SDK", 20);
    titleText.setFillColor(sf::Color::White);
    titleText.setPosition(sf::Vector2f(18.0f, 10.0f));

    sf::Text subTitleText(font, "Spatial Audio | HRTF | Doppler | Reverb | Occlusion", 14);
    subTitleText.setFillColor(sf::Color(180, 190, 210));
    subTitleText.setPosition(sf::Vector2f(20.0f, 36.0f));

    UIButton startButton(
        font,
        "SPACE  Start Doppler",
        sf::Vector2f(18.0f, 56.0f),
        sf::Vector2f(205.0f, 30.0f));

    UIButton reverbButton(
        font,
        "R  Reverb",
        sf::Vector2f(235.0f, 56.0f),
        sf::Vector2f(165.0f, 30.0f));

    UIButton occlusionButton(
        font,
        "O  Occlusion",
        sf::Vector2f(412.0f, 56.0f),
        sf::Vector2f(170.0f, 30.0f));

    UIButton exitButton(
        font,
        "ESC  Exit",
        sf::Vector2f(594.0f, 56.0f),
        sf::Vector2f(120.0f, 30.0f));

    reverbButton.SetEnabled(reverbReady);

    sf::RectangleShape sidePanel(sf::Vector2f(sidePanelWidth, screenH - topBarHeight));
    sidePanel.setPosition(sf::Vector2f(sceneWidth, topBarHeight));
    sidePanel.setFillColor(sf::Color(18, 22, 30));
    sidePanel.setOutlineThickness(1.0f);
    sidePanel.setOutlineColor(sf::Color(80, 90, 105));

    sf::Text panelTitle(font, "LIVE DEBUG PANEL", 18);
    panelTitle.setFillColor(sf::Color::White);
    panelTitle.setPosition(sf::Vector2f(sceneWidth + 20.0f, topBarHeight + 286.0f));

    sf::Text infoText(font, "", 15);
    infoText.setFillColor(sf::Color(220, 220, 220));
    infoText.setPosition(sf::Vector2f(sceneWidth + 20.0f, topBarHeight + 320.0f));

    sf::Text envText(font, "", 15);
    envText.setFillColor(sf::Color(235, 210, 140));
    envText.setPosition(sf::Vector2f(sceneWidth + 20.0f, topBarHeight + 250.0f));

    Vector3 listenerWorld(0.0f, 0.0f, 0.0f);
    sf::Vector2f listenerScreen = WorldToScreen(listenerWorld, sceneWidth, screenH);

    sf::CircleShape listenerShape(22.0f);
    listenerShape.setOrigin(sf::Vector2f(22.0f, 22.0f));
    listenerShape.setFillColor(sf::Color(80, 170, 255));
    listenerShape.setPosition(listenerScreen);

    sf::CircleShape listenerInner(8.0f);
    listenerInner.setOrigin(sf::Vector2f(8.0f, 8.0f));
    listenerInner.setFillColor(sf::Color::White);
    listenerInner.setPosition(listenerScreen);

    sf::ConvexShape listenerDirectionArrow(3);
    listenerDirectionArrow.setPoint(0, sf::Vector2f(0.0f, -10.0f));
    listenerDirectionArrow.setPoint(1, sf::Vector2f(-8.0f, 8.0f));
    listenerDirectionArrow.setPoint(2, sf::Vector2f(8.0f, 8.0f));
    listenerDirectionArrow.setFillColor(sf::Color::White);
    listenerDirectionArrow.setPosition(listenerScreen + sf::Vector2f(0.0f, -32.0f));

    Vector3 currentPosition(-35.0f, 0.0f, -8.0f);
    Vector3 previousPosition = currentPosition;
    Vector3 currentVelocity(0.0f, 0.0f, 0.0f);

    bool runningDemo = false;
    float elapsedTime = 0.0f;
    float demoDuration = 7.0f;

    float waveSpawnTimer = 0.0f;
    float waveSpawnInterval = 0.18f;

    std::vector<SoundWaveRing> rings;
    std::vector<TrailParticle> trailParticles;

    float wallWorldX = 0.0f;
    sf::Vector2f wallScreen = WorldToScreen(Vector3(wallWorldX, 0.0f, 0.0f), sceneWidth, screenH);

    sf::RectangleShape wallShape(sf::Vector2f(22.0f, 300.0f));
    wallShape.setOrigin(sf::Vector2f(11.0f, 150.0f));
    wallShape.setPosition(sf::Vector2f(wallScreen.x, screenH / 2.0f));
    wallShape.setFillColor(sf::Color(130, 130, 130, 180));

    sf::RectangleShape pathLine(sf::Vector2f(sceneWidth - 80.0f, 3.0f));
    pathLine.setOrigin(sf::Vector2f((sceneWidth - 80.0f) / 2.0f, 1.5f));
    pathLine.setPosition(WorldToScreen(Vector3(0.0f, 0.0f, -8.0f), sceneWidth, screenH));
    pathLine.setFillColor(sf::Color(90, 90, 100));

    sf::Text xAxisText(font, "X", 15);
    xAxisText.setFillColor(sf::Color(90, 160, 250));
    xAxisText.setPosition(sf::Vector2f(sceneWidth - 20.0f, listenerScreen.y - 24.0f));

    sf::Text zAxisText(font, "Z", 15);
    zAxisText.setFillColor(sf::Color(130, 220, 130));
    zAxisText.setPosition(sf::Vector2f(listenerScreen.x + 10.0f, topBarHeight + 6.0f));

    auto StartDopplerDemo = [&]()
    {
        elapsedTime = 0.0f;
        runningDemo = true;
        waveSpawnTimer = 0.0f;

        currentPosition = Vector3(-35.0f, 0.0f, -8.0f);
        previousPosition = currentPosition;
        currentVelocity = Vector3(0.0f, 0.0f, 0.0f);

        rings.clear();
        trailParticles.clear();

        siren.Stop();
        siren.SetPosition(currentPosition);
        siren.SetVelocity(Vector3(0.0f, 0.0f, 0.0f));
        siren.Play();

        std::cout << "[Demo] Doppler fly-by started." << std::endl;
    };

    auto ToggleReverb = [&]()
    {
        if (!reverbReady)
        {
            std::cout << "[Demo] Reverb is not available on this device." << std::endl;
            return;
        }

        reverbEnabled = !reverbEnabled;

        if (reverbEnabled)
        {
            siren.AttachEnvironment(caveEnvironment);
            std::cout << "[Demo] Cave reverb ON." << std::endl;
        }
        else
        {
            siren.DetachEnvironment();
            std::cout << "[Demo] Cave reverb OFF." << std::endl;
        }
    };

    auto ToggleOcclusion = [&]()
    {
        occlusionEnabled = !occlusionEnabled;

        if (occlusionEnabled)
        {
            siren.SetOcclusion(1.0f);
            std::cout << "[Demo] Occlusion ON. Sound should become muffled." << std::endl;
        }
        else
        {
            siren.SetOcclusion(0.0f);
            std::cout << "[Demo] Occlusion OFF." << std::endl;
        }
    };

    auto ExitDemo = [&]()
    {
        window.close();
    };

    std::cout << "\n===== HUST 3D Audio Game Engine SDK - Enhanced Demo =====" << std::endl;
    std::cout << "SPACE or button : Start Doppler fly-by demo" << std::endl;
    std::cout << "R or button     : Toggle Cave Reverb" << std::endl;
    std::cout << "O or button     : Toggle Occlusion" << std::endl;
    std::cout << "ESC or button   : Exit" << std::endl;
    std::cout << "Use headphones for best HRTF 360-degree effect." << std::endl;
    std::cout << "Use MONO siren.wav for true 3D spatialization.\n"
              << std::endl;

    sf::Clock clock;

    while (window.isOpen())
    {
        float deltaTime = clock.restart().asSeconds();

        sf::Vector2i mousePixel = sf::Mouse::getPosition(window);

        sf::Vector2f mousePosition(
            static_cast<float>(mousePixel.x),
            static_cast<float>(mousePixel.y));

        startButton.SetHovered(startButton.Contains(mousePosition));
        reverbButton.SetHovered(reverbButton.Contains(mousePosition));
        occlusionButton.SetHovered(occlusionButton.Contains(mousePosition));
        exitButton.SetHovered(exitButton.Contains(mousePosition));

        while (const std::optional<sf::Event> event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }

            if (const auto *keyPressed = event->getIf<sf::Event::KeyPressed>())
            {
                if (keyPressed->code == sf::Keyboard::Key::Escape)
                {
                    ExitDemo();
                }

                if (keyPressed->code == sf::Keyboard::Key::Space)
                {
                    StartDopplerDemo();
                }

                if (keyPressed->code == sf::Keyboard::Key::R)
                {
                    ToggleReverb();
                }

                if (keyPressed->code == sf::Keyboard::Key::O)
                {
                    ToggleOcclusion();
                }
            }

            if (event->is<sf::Event::MouseButtonPressed>())
            {
                if (startButton.Contains(mousePosition))
                {
                    StartDopplerDemo();
                }
                else if (reverbButton.Contains(mousePosition))
                {
                    ToggleReverb();
                }
                else if (occlusionButton.Contains(mousePosition))
                {
                    ToggleOcclusion();
                }
                else if (exitButton.Contains(mousePosition))
                {
                    ExitDemo();
                }
            }
        }

        if (runningDemo)
        {
            elapsedTime += deltaTime;
            waveSpawnTimer += deltaTime;

            float t = elapsedTime / demoDuration;

            if (t >= 1.0f)
            {
                t = 1.0f;
                runningDemo = false;
            }

            previousPosition = currentPosition;

            float x = -35.0f + 70.0f * t;

            currentPosition = Vector3(x, 0.0f, -8.0f);

            currentVelocity = Vector3(0.0f, 0.0f, 0.0f);

            if (deltaTime > 0.0f)
            {
                currentVelocity = (currentPosition - previousPosition) / deltaTime;
            }

            siren.SetPosition(currentPosition);
            siren.SetVelocity(currentVelocity);

            sf::Vector2f sourceScreenPos = WorldToScreen(currentPosition, sceneWidth, screenH);

            SpawnTrailParticles(trailParticles, sourceScreenPos);

            if (waveSpawnTimer >= waveSpawnInterval)
            {
                SpawnWaveRing(rings, sourceScreenPos);
                waveSpawnTimer = 0.0f;
            }

            if (!runningDemo)
            {
                siren.Stop();
                siren.SetVelocity(Vector3(0.0f, 0.0f, 0.0f));

                std::cout << "[Demo] Doppler fly-by finished." << std::endl;
            }
        }

        UpdateTrailParticles(trailParticles, deltaTime);
        UpdateWaveRings(rings, deltaTime);

        startButton.SetActive(runningDemo);
        reverbButton.SetActive(reverbEnabled);
        occlusionButton.SetActive(occlusionEnabled);

        startButton.SetLabel(
            runningDemo
                ? "SPACE  Running..."
                : "SPACE  Start Doppler");

        if (reverbReady)
        {
            reverbButton.SetLabel(
                reverbEnabled
                    ? "R  Reverb: ON"
                    : "R  Reverb: OFF");
        }
        else
        {
            reverbButton.SetLabel("R  Reverb: N/A");
        }

        occlusionButton.SetLabel(
            occlusionEnabled
                ? "O  Occlusion: ON"
                : "O  Occlusion: OFF");

        envText.setString(
            reverbEnabled
                ? "Environment Mode: Cave"
                : "Environment Mode: Normal");

        float distanceToListener = Vector3::Distance(listenerWorld, currentPosition);
        sf::Vector2f sourceScreen = WorldToScreen(currentPosition, sceneWidth, screenH);

        std::ostringstream info;

        info << std::fixed << std::setprecision(2);
        info << "HRTF: " << (engine.IsHRTFEnabled() ? "ON" : "OFF") << "\n";
        info << "Doppler: ON\n";
        info << "Reverb: " << (reverbEnabled ? "ON" : (reverbReady ? "OFF" : "N/A")) << "\n";
        info << "Occlusion: " << (occlusionEnabled ? "ON" : "OFF") << "\n";
        info << "Running: " << (runningDemo ? "YES" : "NO") << "\n\n";

        info << "Source Position\n";
        info << "X: " << currentPosition.x << "\n";
        info << "Y: " << currentPosition.y << "\n";
        info << "Z: " << currentPosition.z << "\n\n";

        info << "Source Velocity\n";
        info << "VX: " << currentVelocity.x << "\n";
        info << "VY: " << currentVelocity.y << "\n";
        info << "VZ: " << currentVelocity.z << "\n\n";

        info << "Distance to Listener\n";
        info << distanceToListener << "\n";

        infoText.setString(info.str());

        window.clear();

        DrawGridBackground(window, sceneWidth, screenH, topBarHeight, reverbEnabled);
        DrawAttenuationRings(window, listenerScreen);
        DrawAxes(window, listenerScreen, sceneWidth, screenH, topBarHeight);

        window.draw(pathLine);

        DrawLine(
            window,
            listenerScreen,
            sourceScreen,
            occlusionEnabled
                ? sf::Color(220, 120, 80)
                : sf::Color(150, 180, 230));

        if (occlusionEnabled)
        {
            window.draw(wallShape);
        }

        DrawTrailParticles(window, trailParticles);
        DrawWaveRings(window, rings);

        window.draw(listenerShape);
        window.draw(listenerInner);
        window.draw(listenerDirectionArrow);

        DrawSourceVehicle(window, sourceScreen, runningDemo, occlusionEnabled);

        window.draw(xAxisText);
        window.draw(zAxisText);

        window.draw(topBar);
        window.draw(titleText);
        window.draw(subTitleText);

        startButton.Draw(window);
        reverbButton.Draw(window);
        occlusionButton.Draw(window);
        exitButton.Draw(window);

        window.draw(sidePanel);

        DrawMiniMap(window, sceneWidth, topBarHeight, currentPosition, runningDemo);

        window.draw(panelTitle);
        window.draw(envText);
        window.draw(infoText);

        window.display();
    }

    siren.Stop();

    return 0;
}