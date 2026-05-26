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

const float PI = 3.14159265358979323846f;

// ======================================================
// MOVEMENT MODE
// ======================================================
enum class MovementMode
{
    LeftRight = 0,
    FrontBack,
    Orbit360,
    Vertical,
    Spiral3D
};

static std::string GetModeName(MovementMode mode)
{
    switch (mode)
    {
    case MovementMode::LeftRight:
        return "Left -> Right";
    case MovementMode::FrontBack:
        return "Front -> Back";
    case MovementMode::Orbit360:
        return "360 Orbit";
    case MovementMode::Vertical:
        return "Up -> Down";
    case MovementMode::Spiral3D:
        return "3D Spiral";
    default:
        return "Unknown";
    }
}

static float GetModeDuration(MovementMode mode)
{
    switch (mode)
    {
    case MovementMode::LeftRight:
        return 5.0f;
    case MovementMode::FrontBack:
        return 6.0f;
    case MovementMode::Orbit360:
        return 8.0f;
    case MovementMode::Vertical:
        return 6.0f;
    case MovementMode::Spiral3D:
        return 10.0f;
    default:
        return 6.0f;
    }
}

static MovementMode NextMode(MovementMode mode)
{
    int next = static_cast<int>(mode) + 1;

    if (next > static_cast<int>(MovementMode::Spiral3D))
    {
        next = 0;
    }

    return static_cast<MovementMode>(next);
}

static Vector3 GetPositionForMode(MovementMode mode, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);

    switch (mode)
    {
    case MovementMode::LeftRight:
        return Vector3(
            -40.0f + 80.0f * t,
            0.0f,
            -8.0f);

    case MovementMode::FrontBack:
        return Vector3(
            3.0f,
            0.0f,
            -40.0f + 80.0f * t);

    case MovementMode::Orbit360:
    {
        float angle = t * 2.0f * PI;

        return Vector3(
            std::cos(angle) * 18.0f,
            0.0f,
            std::sin(angle) * 18.0f);
    }

    case MovementMode::Vertical:
        return Vector3(
            6.0f,
            -16.0f + 32.0f * t,
            -10.0f);

    case MovementMode::Spiral3D:
    {
        float angle = t * 4.0f * PI;
        float radius = 18.0f;
        float y = std::sin(t * 2.0f * PI) * 12.0f;

        return Vector3(
            std::cos(angle) * radius,
            y,
            std::sin(angle) * radius);
    }

    default:
        return Vector3();
    }
}

// ======================================================
// PSEUDO 3D PROJECTION
// ======================================================
struct Projected3D
{
    sf::Vector2f ground;
    sf::Vector2f object;
    float visualScale;
};

static Projected3D Project3D(
    const Vector3 &position,
    float sceneWidth,
    float sceneHeight,
    float topBarHeight)
{
    const float scale = 18.0f;
    const float zSkew = 0.45f;
    const float yScale = 12.0f;

    float originX = sceneWidth / 2.0f;
    float originY = topBarHeight + (sceneHeight - topBarHeight) * 0.58f;

    float groundX = originX + position.x * scale + position.z * scale * zSkew;
    float groundY = originY + position.z * scale * 0.35f;

    float objectX = groundX;
    float objectY = groundY - position.y * yScale;

    float visualScale = 1.0f + std::clamp(position.y / 35.0f, -0.25f, 0.35f);

    return {
        sf::Vector2f(groundX, groundY),
        sf::Vector2f(objectX, objectY),
        visualScale};
}

static sf::Vector2f WorldToScreen(
    const Vector3 &position,
    float sceneWidth,
    float sceneHeight,
    float topBarHeight)
{
    return Project3D(position, sceneWidth, sceneHeight, topBarHeight).object;
}

// ======================================================
// UI BUTTON
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
          label(font, text, 15),
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
// VISUAL STRUCTS
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

static float ComputeDynamicOcclusion(
    const Vector3 &listener,
    const Vector3 &source,
    bool wallEnabled)
{
    (void)listener;

    if (!wallEnabled)
    {
        return 0.0f;
    }

    const float wallZ = -5.0f;
    const float wallHalfWidth = 18.0f;

    bool sourceBehindWall = source.z < wallZ;
    bool sourceInsideWallWidth = std::abs(source.x) <= wallHalfWidth;

    if (!sourceBehindWall || !sourceInsideWallWidth)
    {
        return 0.0f;
    }

    float distanceBehindWall = std::abs(source.z - wallZ);

    float amount = 0.25f + distanceBehindWall / 35.0f;
    amount = std::clamp(amount, 0.0f, 1.0f);

    float heightEscape = std::clamp(std::abs(source.y) / 18.0f, 0.0f, 1.0f);
    amount *= (1.0f - heightEscape * 0.65f);

    return std::clamp(amount, 0.0f, 1.0f);
}

static void SpawnTrailParticles(
    std::vector<TrailParticle> &particles,
    const sf::Vector2f &sourceScreenPos)
{
    for (int i = 0; i < 3; ++i)
    {
        TrailParticle p;

        p.position = sourceScreenPos;

        float spreadY = static_cast<float>((std::rand() % 70) - 35);

        p.velocity = sf::Vector2f(
            -70.0f - static_cast<float>(std::rand() % 50),
            spreadY * 0.18f);

        p.radius = 2.0f + static_cast<float>(std::rand() % 4);
        p.maxLife = 0.45f + static_cast<float>(std::rand() % 50) / 100.0f;
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
        it->radius += 150.0f * deltaTime;

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
        overlay.setFillColor(sf::Color(25, 20, 45, 70));

        window.draw(overlay);

        sf::CircleShape caveZone(260.0f);
        caveZone.setOrigin(sf::Vector2f(260.0f, 260.0f));
        caveZone.setPosition(sf::Vector2f(sceneWidth * 0.5f, topBarHeight + (sceneHeight - topBarHeight) * 0.55f));
        caveZone.setFillColor(sf::Color(60, 45, 90, 45));
        caveZone.setOutlineThickness(3.0f);
        caveZone.setOutlineColor(sf::Color(130, 100, 180, 120));

        window.draw(caveZone);
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

static void Draw3DAxes(
    sf::RenderWindow &window,
    const sf::Font &font,
    float sceneWidth,
    float sceneHeight,
    float topBarHeight)
{
    Vector3 origin3D(0.0f, 0.0f, 0.0f);
    Vector3 xEnd3D(24.0f, 0.0f, 0.0f);
    Vector3 yEnd3D(0.0f, 16.0f, 0.0f);
    Vector3 zEnd3D(0.0f, 0.0f, 24.0f);

    sf::Vector2f origin = WorldToScreen(origin3D, sceneWidth, sceneHeight, topBarHeight);
    sf::Vector2f xEnd = WorldToScreen(xEnd3D, sceneWidth, sceneHeight, topBarHeight);
    sf::Vector2f yEnd = WorldToScreen(yEnd3D, sceneWidth, sceneHeight, topBarHeight);
    sf::Vector2f zEnd = WorldToScreen(zEnd3D, sceneWidth, sceneHeight, topBarHeight);

    DrawLine(window, origin, xEnd, sf::Color(80, 160, 255));
    DrawLine(window, origin, yEnd, sf::Color(255, 210, 90));
    DrawLine(window, origin, zEnd, sf::Color(120, 230, 120));

    sf::CircleShape centerDot(5.0f);
    centerDot.setOrigin(sf::Vector2f(5.0f, 5.0f));
    centerDot.setPosition(origin);
    centerDot.setFillColor(sf::Color::White);
    window.draw(centerDot);

    sf::Text xText(font, "X", 15);
    xText.setFillColor(sf::Color(80, 160, 255));
    xText.setPosition(xEnd + sf::Vector2f(8.0f, -8.0f));
    window.draw(xText);

    sf::Text yText(font, "Y / Height", 15);
    yText.setFillColor(sf::Color(255, 210, 90));
    yText.setPosition(yEnd + sf::Vector2f(8.0f, -8.0f));
    window.draw(yText);

    sf::Text zText(font, "Z / Depth", 15);
    zText.setFillColor(sf::Color(120, 230, 120));
    zText.setPosition(zEnd + sf::Vector2f(8.0f, -8.0f));
    window.draw(zText);
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

static void DrawSourceVehicle3D(
    sf::RenderWindow &window,
    const Vector3 &worldPosition,
    float sceneWidth,
    float sceneHeight,
    float topBarHeight,
    bool moving,
    float occlusionAmount)
{
    Projected3D projected = Project3D(
        worldPosition,
        sceneWidth,
        sceneHeight,
        topBarHeight);

    sf::Vector2f ground = projected.ground;
    sf::Vector2f position = projected.object;

    float heightScale = projected.visualScale;

    bool occluded = occlusionAmount > 0.1f;

    std::uint8_t bodyAlpha = static_cast<std::uint8_t>(occluded ? 155 : 255);
    std::uint8_t glowAlpha = static_cast<std::uint8_t>(occluded ? 45 : 95);

    sf::CircleShape shadow(28.0f * heightScale);
    shadow.setOrigin(sf::Vector2f(28.0f * heightScale, 14.0f * heightScale));
    shadow.setScale(sf::Vector2f(1.4f, 0.45f));
    shadow.setPosition(ground);
    shadow.setFillColor(sf::Color(0, 0, 0, 105));
    window.draw(shadow);

    if (std::abs(worldPosition.y) > 0.1f)
    {
        DrawLine(
            window,
            ground,
            position,
            sf::Color(255, 210, 90, 155));
    }

    sf::CircleShape glow(34.0f * heightScale);
    glow.setOrigin(sf::Vector2f(34.0f * heightScale, 34.0f * heightScale));
    glow.setPosition(position);
    glow.setFillColor(sf::Color(255, 120, 70, glowAlpha));
    window.draw(glow);

    sf::RectangleShape body(sf::Vector2f(58.0f * heightScale, 26.0f * heightScale));
    body.setOrigin(sf::Vector2f(29.0f * heightScale, 13.0f * heightScale));
    body.setPosition(position);
    body.setFillColor(sf::Color(225, 235, 245, bodyAlpha));
    body.setOutlineThickness(1.5f);
    body.setOutlineColor(sf::Color(40, 40, 50, bodyAlpha));
    window.draw(body);

    sf::RectangleShape roof(sf::Vector2f(30.0f * heightScale, 13.0f * heightScale));
    roof.setOrigin(sf::Vector2f(15.0f * heightScale, 6.5f * heightScale));
    roof.setPosition(position + sf::Vector2f(-4.0f * heightScale, -13.0f * heightScale));
    roof.setFillColor(sf::Color(210, 220, 235, bodyAlpha));
    roof.setOutlineThickness(1.0f);
    roof.setOutlineColor(sf::Color(40, 40, 50, bodyAlpha));
    window.draw(roof);

    sf::RectangleShape stripe(sf::Vector2f(50.0f * heightScale, 5.0f * heightScale));
    stripe.setOrigin(sf::Vector2f(25.0f * heightScale, 2.5f * heightScale));
    stripe.setPosition(position + sf::Vector2f(0.0f, -1.0f * heightScale));
    stripe.setFillColor(sf::Color(220, 55, 55, bodyAlpha));
    window.draw(stripe);

    bool blink = moving && (static_cast<int>(std::clock() / 80) % 2 == 0);

    sf::CircleShape redLight(4.5f * heightScale);
    redLight.setOrigin(sf::Vector2f(4.5f * heightScale, 4.5f * heightScale));
    redLight.setPosition(position + sf::Vector2f(-10.0f * heightScale, -20.0f * heightScale));
    redLight.setFillColor(
        blink
            ? sf::Color(255, 40, 40, bodyAlpha)
            : sf::Color(120, 40, 40, bodyAlpha));
    window.draw(redLight);

    sf::CircleShape blueLight(4.5f * heightScale);
    blueLight.setOrigin(sf::Vector2f(4.5f * heightScale, 4.5f * heightScale));
    blueLight.setPosition(position + sf::Vector2f(6.0f * heightScale, -20.0f * heightScale));
    blueLight.setFillColor(
        !blink && moving
            ? sf::Color(60, 150, 255, bodyAlpha)
            : sf::Color(50, 80, 140, bodyAlpha));
    window.draw(blueLight);

    sf::ConvexShape nose(3);
    nose.setPoint(0, sf::Vector2f(0.0f, 0.0f));
    nose.setPoint(1, sf::Vector2f(11.0f * heightScale, -8.0f * heightScale));
    nose.setPoint(2, sf::Vector2f(11.0f * heightScale, 8.0f * heightScale));
    nose.setPosition(position + sf::Vector2f(29.0f * heightScale, 0.0f));
    nose.setFillColor(sf::Color(200, 210, 220, bodyAlpha));
    window.draw(nose);
}

static void DrawHeightBar(
    sf::RenderWindow &window,
    const sf::Font &font,
    float panelX,
    float topBarHeight,
    float heightY)
{
    float barX = panelX + 245.0f;
    float barY = topBarHeight + 330.0f;
    float barH = 180.0f;

    sf::RectangleShape bg(sf::Vector2f(18.0f, barH));

    bg.setPosition(sf::Vector2f(barX, barY));
    bg.setFillColor(sf::Color(35, 40, 50));
    bg.setOutlineThickness(1.0f);
    bg.setOutlineColor(sf::Color(100, 110, 130));

    window.draw(bg);

    float normalized = (heightY + 20.0f) / 40.0f;
    normalized = std::clamp(normalized, 0.0f, 1.0f);

    float markerY = barY + barH - normalized * barH;

    sf::RectangleShape marker(sf::Vector2f(28.0f, 4.0f));
    marker.setOrigin(sf::Vector2f(5.0f, 2.0f));
    marker.setPosition(sf::Vector2f(barX, markerY));
    marker.setFillColor(sf::Color(255, 210, 90));

    window.draw(marker);

    sf::Text label(font, "Y", 13);
    label.setFillColor(sf::Color(255, 210, 90));
    label.setPosition(sf::Vector2f(barX - 2.0f, barY - 24.0f));
    window.draw(label);
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

// ======================================================
// MAIN
// ======================================================
int main()
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    AudioEngine engine;

    if (!engine.Init(true))
    {
        std::cerr << "Failed to initialize AudioEngine." << std::endl;
        return -1;
    }

    engine.SetDopplerFactor(1.4f, 343.3f);
    engine.SetDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

    float listenerYaw = 0.0f;

    auto UpdateListenerOrientation = [&]()
    {
        Vector3 forward(
            std::sin(listenerYaw),
            0.0f,
            -std::cos(listenerYaw));

        Vector3 up(0.0f, 1.0f, 0.0f);

        AudioListener::SetOrientation(forward, up);
    };

    AudioListener::SetPosition(Vector3(0.0f, 0.0f, 0.0f));
    AudioListener::SetVelocity(Vector3(0.0f, 0.0f, 0.0f));
    UpdateListenerOrientation();

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
    siren.SetReferenceDistance(8.0f);
    siren.SetMaxDistance(120.0f);
    siren.SetRolloffFactor(0.65f);

    const unsigned int windowWidth = 1366;
    const unsigned int windowHeight = 768;

    sf::RenderWindow window(
        sf::VideoMode({windowWidth, windowHeight}),
        "HUST 3D Audio Game Engine SDK - Pseudo 3D Audio Demo");

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

    sf::Text subTitleText(font, "Pseudo-3D Visualization | True X/Y/Z Audio Position | HRTF | Doppler | Reverb | Dynamic Occlusion", 14);
    subTitleText.setFillColor(sf::Color(180, 190, 210));
    subTitleText.setPosition(sf::Vector2f(20.0f, 36.0f));

    UIButton startButton(
        font,
        "SPACE Start",
        sf::Vector2f(18.0f, 56.0f),
        sf::Vector2f(145.0f, 30.0f));

    UIButton modeButton(
        font,
        "M Mode",
        sf::Vector2f(174.0f, 56.0f),
        sf::Vector2f(190.0f, 30.0f));

    UIButton reverbButton(
        font,
        "R Reverb",
        sf::Vector2f(375.0f, 56.0f),
        sf::Vector2f(145.0f, 30.0f));

    UIButton occlusionButton(
        font,
        "O Wall",
        sf::Vector2f(532.0f, 56.0f),
        sf::Vector2f(165.0f, 30.0f));

    UIButton exitButton(
        font,
        "ESC Exit",
        sf::Vector2f(710.0f, 56.0f),
        sf::Vector2f(115.0f, 30.0f));

    reverbButton.SetEnabled(reverbReady);

    sf::RectangleShape sidePanel(sf::Vector2f(sidePanelWidth, screenH - topBarHeight));
    sidePanel.setPosition(sf::Vector2f(sceneWidth, topBarHeight));
    sidePanel.setFillColor(sf::Color(18, 22, 30));
    sidePanel.setOutlineThickness(1.0f);
    sidePanel.setOutlineColor(sf::Color(80, 90, 105));

    sf::Text panelTitle(font, "LIVE 3D AUDIO DEBUG", 17);
    panelTitle.setFillColor(sf::Color::White);
    panelTitle.setPosition(sf::Vector2f(sceneWidth + 20.0f, topBarHeight + 286.0f));

    sf::Text infoText(font, "", 14);
    infoText.setFillColor(sf::Color(220, 220, 220));
    infoText.setPosition(sf::Vector2f(sceneWidth + 20.0f, topBarHeight + 318.0f));

    sf::Text envText(font, "", 14);
    envText.setFillColor(sf::Color(235, 210, 140));
    envText.setPosition(sf::Vector2f(sceneWidth + 20.0f, topBarHeight + 250.0f));

    sf::Text controlHint(font, "M: mode | Q/E: rotate listener | Use headphones + mono WAV", 13);
    controlHint.setFillColor(sf::Color(170, 180, 200));
    controlHint.setPosition(sf::Vector2f(sceneWidth + 20.0f, screenH - 35.0f));

    Vector3 listenerWorld(0.0f, 0.0f, 0.0f);
    sf::Vector2f listenerScreen = WorldToScreen(listenerWorld, sceneWidth, screenH, topBarHeight);

    sf::CircleShape listenerShape(22.0f);
    listenerShape.setOrigin(sf::Vector2f(22.0f, 22.0f));
    listenerShape.setFillColor(sf::Color(80, 170, 255));
    listenerShape.setPosition(listenerScreen);

    sf::CircleShape listenerInner(8.0f);
    listenerInner.setOrigin(sf::Vector2f(8.0f, 8.0f));
    listenerInner.setFillColor(sf::Color::White);
    listenerInner.setPosition(listenerScreen);

    MovementMode currentMode = MovementMode::LeftRight;

    Vector3 currentPosition = GetPositionForMode(currentMode, 0.0f);
    Vector3 previousPosition = currentPosition;
    Vector3 currentVelocity(0.0f, 0.0f, 0.0f);

    bool runningDemo = false;
    float elapsedTime = 0.0f;
    float demoDuration = GetModeDuration(currentMode);

    float waveSpawnTimer = 0.0f;
    float waveSpawnInterval = 0.18f;

    float dynamicOcclusion = 0.0f;

    std::vector<SoundWaveRing> rings;
    std::vector<TrailParticle> trailParticles;

    Vector3 wallWorld(0.0f, 0.0f, -5.0f);
    sf::Vector2f wallScreen = WorldToScreen(wallWorld, sceneWidth, screenH, topBarHeight);

    sf::RectangleShape wallShape(sf::Vector2f(26.0f, 330.0f));
    wallShape.setOrigin(sf::Vector2f(13.0f, 165.0f));
    wallShape.setPosition(wallScreen);
    wallShape.setFillColor(sf::Color(130, 130, 130, 165));
    wallShape.setOutlineThickness(2.0f);
    wallShape.setOutlineColor(sf::Color(210, 150, 100, 180));

    auto StartDemo = [&]()
    {
        elapsedTime = 0.0f;
        runningDemo = true;
        waveSpawnTimer = 0.0f;
        demoDuration = GetModeDuration(currentMode);

        currentPosition = GetPositionForMode(currentMode, 0.0f);
        previousPosition = currentPosition;
        currentVelocity = Vector3(0.0f, 0.0f, 0.0f);

        rings.clear();
        trailParticles.clear();

        dynamicOcclusion = ComputeDynamicOcclusion(
            listenerWorld,
            currentPosition,
            occlusionEnabled);

        siren.Stop();
        siren.SetPosition(currentPosition);
        siren.SetVelocity(Vector3(0.0f, 0.0f, 0.0f));
        siren.SetOcclusion(dynamicOcclusion);
        siren.Play();

        std::cout << "[Demo] Started mode: " << GetModeName(currentMode) << std::endl;
    };

    auto ChangeMode = [&]()
    {
        currentMode = NextMode(currentMode);
        demoDuration = GetModeDuration(currentMode);

        runningDemo = false;
        elapsedTime = 0.0f;

        currentPosition = GetPositionForMode(currentMode, 0.0f);
        previousPosition = currentPosition;
        currentVelocity = Vector3(0.0f, 0.0f, 0.0f);

        dynamicOcclusion = ComputeDynamicOcclusion(
            listenerWorld,
            currentPosition,
            occlusionEnabled);

        siren.Stop();
        siren.SetPosition(currentPosition);
        siren.SetVelocity(Vector3(0.0f, 0.0f, 0.0f));
        siren.SetOcclusion(dynamicOcclusion);

        rings.clear();
        trailParticles.clear();

        std::cout << "[Demo] Movement mode changed to: " << GetModeName(currentMode) << std::endl;
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

        dynamicOcclusion = ComputeDynamicOcclusion(
            listenerWorld,
            currentPosition,
            occlusionEnabled);

        siren.SetOcclusion(dynamicOcclusion);

        if (occlusionEnabled)
        {
            std::cout << "[Demo] Dynamic occlusion wall ON." << std::endl;
        }
        else
        {
            std::cout << "[Demo] Dynamic occlusion wall OFF." << std::endl;
        }
    };

    auto RotateListener = [&](float deltaYaw)
    {
        listenerYaw += deltaYaw;
        UpdateListenerOrientation();

        std::cout << "[Demo] Listener yaw: " << listenerYaw << " rad" << std::endl;
    };

    auto ExitDemo = [&]()
    {
        window.close();
    };

    std::cout << "\n===== PSEUDO 3D AUDIO DEMO =====" << std::endl;
    std::cout << "SPACE : Start selected 3D movement mode" << std::endl;
    std::cout << "M     : Change movement mode" << std::endl;
    std::cout << "Q/E   : Rotate listener orientation" << std::endl;
    std::cout << "R     : Toggle Cave Reverb" << std::endl;
    std::cout << "O     : Toggle dynamic occlusion wall" << std::endl;
    std::cout << "ESC   : Exit" << std::endl;
    std::cout << "Use headphones and MONO siren.wav for best HRTF.\n"
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
        modeButton.SetHovered(modeButton.Contains(mousePosition));
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
                    StartDemo();
                }

                if (keyPressed->code == sf::Keyboard::Key::M)
                {
                    ChangeMode();
                }

                if (keyPressed->code == sf::Keyboard::Key::R)
                {
                    ToggleReverb();
                }

                if (keyPressed->code == sf::Keyboard::Key::O)
                {
                    ToggleOcclusion();
                }

                if (keyPressed->code == sf::Keyboard::Key::Q)
                {
                    RotateListener(-PI / 12.0f);
                }

                if (keyPressed->code == sf::Keyboard::Key::E)
                {
                    RotateListener(PI / 12.0f);
                }
            }

            if (event->is<sf::Event::MouseButtonPressed>())
            {
                if (startButton.Contains(mousePosition))
                {
                    StartDemo();
                }
                else if (modeButton.Contains(mousePosition))
                {
                    ChangeMode();
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
            currentPosition = GetPositionForMode(currentMode, t);

            currentVelocity = Vector3(0.0f, 0.0f, 0.0f);

            if (deltaTime > 0.0f)
            {
                currentVelocity = (currentPosition - previousPosition) / deltaTime;
            }

            dynamicOcclusion = ComputeDynamicOcclusion(
                listenerWorld,
                currentPosition,
                occlusionEnabled);

            siren.SetPosition(currentPosition);
            siren.SetVelocity(currentVelocity);
            siren.SetOcclusion(dynamicOcclusion);

            sf::Vector2f sourceScreenPos = WorldToScreen(
                currentPosition,
                sceneWidth,
                screenH,
                topBarHeight);

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

                std::cout << "[Demo] Finished mode: " << GetModeName(currentMode) << std::endl;
            }
        }
        else
        {
            dynamicOcclusion = ComputeDynamicOcclusion(
                listenerWorld,
                currentPosition,
                occlusionEnabled);

            siren.SetOcclusion(dynamicOcclusion);
        }

        UpdateTrailParticles(trailParticles, deltaTime);
        UpdateWaveRings(rings, deltaTime);

        startButton.SetActive(runningDemo);
        modeButton.SetActive(false);
        reverbButton.SetActive(reverbEnabled);
        occlusionButton.SetActive(occlusionEnabled);

        startButton.SetLabel(runningDemo ? "SPACE Running..." : "SPACE Start");
        modeButton.SetLabel("M " + GetModeName(currentMode));

        if (reverbReady)
        {
            reverbButton.SetLabel(reverbEnabled ? "R Reverb: ON" : "R Reverb: OFF");
        }
        else
        {
            reverbButton.SetLabel("R Reverb: N/A");
        }

        occlusionButton.SetLabel(occlusionEnabled ? "O Wall: ON" : "O Wall: OFF");

        envText.setString(
            reverbEnabled
                ? "Environment: Cave Zone\nLong decay reverb approximation"
                : "Environment: Normal\nDry spatial audio");

        float distanceToListener = Vector3::Distance(listenerWorld, currentPosition);
        float speed = currentVelocity.Length();

        sf::Vector2f sourceScreen = WorldToScreen(
            currentPosition,
            sceneWidth,
            screenH,
            topBarHeight);

        std::ostringstream info;

        info << std::fixed << std::setprecision(2);

        info << "Mode: " << GetModeName(currentMode) << "\n";
        info << "HRTF: " << (engine.IsHRTFEnabled() ? "ON" : "OFF") << "\n";
        info << "Doppler Factor: 1.40\n";
        info << "Reverb: " << (reverbEnabled ? "ON" : (reverbReady ? "OFF" : "N/A")) << "\n";
        info << "Wall System: " << (occlusionEnabled ? "ON" : "OFF") << "\n";
        info << "Occlusion Amount: " << dynamicOcclusion << "\n";
        info << "Running: " << (runningDemo ? "YES" : "NO") << "\n\n";

        info << "Listener\n";
        info << "Yaw: " << listenerYaw << " rad\n";
        info << "Q/E rotate listener\n\n";

        info << "Source Position\n";
        info << "X: " << currentPosition.x << "\n";
        info << "Y: " << currentPosition.y << "\n";
        info << "Z: " << currentPosition.z << "\n\n";

        info << "Source Velocity\n";
        info << "VX: " << currentVelocity.x << "\n";
        info << "VY: " << currentVelocity.y << "\n";
        info << "VZ: " << currentVelocity.z << "\n";
        info << "Speed: " << speed << "\n\n";

        info << "Distance: " << distanceToListener << "\n";
        info << "Emitter: point source\n";
        info << "Visual: 3D object body\n";

        infoText.setString(info.str());

        window.clear();

        DrawGridBackground(
            window,
            sceneWidth,
            screenH,
            topBarHeight,
            reverbEnabled);

        Draw3DAxes(
            window,
            font,
            sceneWidth,
            screenH,
            topBarHeight);

        DrawAttenuationRings(window, listenerScreen);

        DrawLine(
            window,
            listenerScreen,
            sourceScreen,
            dynamicOcclusion > 0.1f
                ? sf::Color(220, 120, 80)
                : sf::Color(150, 180, 230));

        Vector3 listenerForward(
            std::sin(listenerYaw),
            0.0f,
            -std::cos(listenerYaw));

        sf::Vector2f forwardScreen = listenerScreen + sf::Vector2f(
                                                          listenerForward.x * 65.0f + listenerForward.z * 25.0f,
                                                          listenerForward.z * 22.0f);

        DrawLine(
            window,
            listenerScreen,
            forwardScreen,
            sf::Color(255, 255, 255));

        if (occlusionEnabled)
        {
            window.draw(wallShape);
        }

        DrawTrailParticles(window, trailParticles);
        DrawWaveRings(window, rings);

        window.draw(listenerShape);
        window.draw(listenerInner);

        DrawSourceVehicle3D(
            window,
            currentPosition,
            sceneWidth,
            screenH,
            topBarHeight,
            runningDemo,
            dynamicOcclusion);

        window.draw(topBar);
        window.draw(titleText);
        window.draw(subTitleText);

        startButton.Draw(window);
        modeButton.Draw(window);
        reverbButton.Draw(window);
        occlusionButton.Draw(window);
        exitButton.Draw(window);

        window.draw(sidePanel);

        DrawMiniMap(
            window,
            sceneWidth,
            topBarHeight,
            currentPosition,
            runningDemo);

        DrawHeightBar(
            window,
            font,
            sceneWidth,
            topBarHeight,
            currentPosition.y);

        window.draw(panelTitle);
        window.draw(envText);
        window.draw(infoText);
        window.draw(controlHint);

        window.display();
    }

    siren.Stop();

    return 0;
}