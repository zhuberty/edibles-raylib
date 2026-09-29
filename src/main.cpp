// Game 3 -- Edibles (Snake)
//
// Ported from the original Python/pygame "Edibles" game
// (see C:\Users\Zach\dev\Edibles) to C++ / raylib.
//
// Faithful port of the core gameplay: Title screen, One Player mode,
// Two Player mode, and a Controls/Config screen for picking snake colors.
// The original's PAdLib particle-effects / shadow-occluder decorations were
// not ported since they are purely cosmetic flourishes provided by a
// pygame-only library with no raylib equivalent in this codebase.
//
// Press ESC to exit back to the arcade menu.

#include "raylib.h"
#include "resource_dir.h"
#include "arcade_input.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

static const int LOGICAL_W = 800;
static const int LOGICAL_H = 800;
static const int SCALE     = 2;
static const int CELL      = 10 * SCALE;

struct NamedColor { const char* name; Color rgb; };

static const NamedColor kColors[] = {
    { "Green",  {41,  237, 41,  255} },
    { "Orange", {255, 150, 44,  255} },
    { "Purple", {100, 61,  227, 255} },
    { "Blue",   {38,  219, 219, 255} },
    { "Pink",   {237, 41,  152, 255} },
    { "Yellow", {255, 230, 44,  255} },
    { "Rose",   {251, 43,  67,  255} },
};
static const int kColorCount = (int)(sizeof(kColors) / sizeof(kColors[0]));

static const Color kApple = {255, 44, 44, 255};
static const Color kGrid  = {56, 56, 56, 255};
static const Color kBlue  = {49, 167, 255, 255};
static const Color kRed   = {255, 64, 34, 255};

struct Segment { int x, y; };

static int MyRound(int x, int base)
{
    if (base == 0) return x;
    return (int)(base * (long)((x + (x >= 0 ? base/2 : -base/2)) / base));
}

static int RandRange(int minV, int maxV)
{
    if (maxV <= minV) return minV;
    return minV + GetRandomValue(0, maxV - minV);
}

enum class SceneId { Title, OnePlayer, TwoPlayer, Config };

struct GameState
{
    SceneId scene = SceneId::Title;

    int colorIndexOne = 0;
    int colorIndexTwo = 1;
    Color p1Color = kColors[0].rgb;
    Color p2Color = kColors[1].rgb;

    Music music{};
    bool musicLoaded = false;
    bool musicPlaying = false;

    int titleSelection = 1;

    int dx1 = CELL, dy1 = 0;
    Segment head1{};
    Segment apple{};
    std::vector<Segment> tail1;
    bool gameOver1 = false;
    double moveAccum1 = 0.0;

    int dxA = CELL, dyA = 0;
    int dxB = -CELL, dyB = 0;
    Segment headA{}, headB{};
    Segment apple2{};
    std::vector<Segment> tailA, tailB;
    bool p1Wins = false, p2Wins = false, draw2 = false;
    double moveAccum2 = 0.0;

    bool p1HiliLeft = false, p1HiliRight = false;
    bool p2HiliLeft = false, p2HiliRight = false;
};

static GameState g;
static Font gFont;

static void PlayMusicLoop()
{
    if (!g.musicLoaded) return;
    SeekMusicStream(g.music, 0.0f);
    PlayMusicStream(g.music);
    g.musicPlaying = true;
}

static void StopMusicIfPlaying()
{
    if (g.musicLoaded && g.musicPlaying)
    {
        StopMusicStream(g.music);
        g.musicPlaying = false;
    }
}

static void OnePlayerSpawnApple(); // forward declaration

static void InitOnePlayer()
{
    g.dx1 = CELL; g.dy1 = 0;
    g.head1 = { 1, 1 };
    g.apple = { -999, -999 }; // off-screen sentinel
    g.tail1.clear();
    for (int i = 1; i < 3; ++i)
        g.tail1.push_back({ g.head1.x - 10 * i * SCALE, g.head1.y });
    g.gameOver1 = false;
    g.moveAccum1 = 0.0;
    OnePlayerSpawnApple();
}

static void OnePlayerSpawnApple()
{
    int prevX = g.apple.x, prevY = g.apple.y;
    int currX, currY;
    for (;;)
    {
        currX = GetRandomValue(0, LOGICAL_W / CELL - 1) * CELL + 1;
        currY = GetRandomValue(0, LOGICAL_H / CELL - 1) * CELL + 1;
        if (currX == prevX && currY == prevY) continue;

        bool onTail = false;
        for (auto& s : g.tail1) if (s.x == currX && s.y == currY) { onTail = true; break; }
        if (onTail) continue;
        break;
    }
    g.apple.x = currX;
    g.apple.y = currY;
}

static void OnePlayerDidEat()
{
    if (g.head1.x == g.apple.x && g.head1.y == g.apple.y)
    {
        if (!g.tail1.empty()) g.tail1.push_back(g.tail1.back());
        OnePlayerSpawnApple();
    }
}

static void OnePlayerIsCollide()
{
    if (g.head1.x < 0 || g.head1.x > LOGICAL_W || g.head1.y < 0 || g.head1.y > LOGICAL_H)
    {
        g.dx1 = 0; g.dy1 = 0; g.gameOver1 = true;
        StopMusicIfPlaying();
    }
    for (auto& s : g.tail1)
    {
        if (g.head1.x == s.x && g.head1.y == s.y)
        {
            g.dx1 = 0; g.dy1 = 0; g.gameOver1 = true;
            StopMusicIfPlaying();
        }
    }
}

static void OnePlayerEvent()
{
    if (arcade::IsActionPressed(arcade::Player::Any, arcade::Action::Up) && g.dy1 == 0) { g.dy1 = -CELL; g.dx1 = 0; }
    if (arcade::IsActionPressed(arcade::Player::Any, arcade::Action::Down) && g.dy1 == 0) { g.dy1 = CELL; g.dx1 = 0; }
    if (arcade::IsActionPressed(arcade::Player::Any, arcade::Action::Left) && g.dx1 == 0) { g.dx1 = -CELL; g.dy1 = 0; }
    if (arcade::IsActionPressed(arcade::Player::Any, arcade::Action::Right) && g.dx1 == 0) { g.dx1 = CELL; g.dy1 = 0; }

    if (arcade::IsActionPressed(arcade::Player::Any, arcade::Action::Restart))
    {
        StopMusicIfPlaying();
        InitOnePlayer();
        PlayMusicLoop();
    }
    if (arcade::IsActionPressed(arcade::Player::Any, arcade::Action::Back))
    {
        StopMusicIfPlaying();
        g.scene = SceneId::Title;
        InitOnePlayer();
    }
}

static void OnePlayerUpdate(float dt)
{
    if (g.gameOver1) return;

    const double stepTime = 1.0 / 15.0;
    g.moveAccum1 += dt;
    if (g.moveAccum1 < stepTime) return;
    g.moveAccum1 -= stepTime;

    OnePlayerDidEat();

    for (size_t i = g.tail1.size() - 1; i > 0; --i)
        g.tail1[i] = g.tail1[i - 1];
    if (!g.tail1.empty()) g.tail1[0] = g.head1;

    g.head1.x += g.dx1;
    g.head1.y += g.dy1;

    OnePlayerIsCollide();
}

static void OnePlayerDraw()
{
    ClearBackground(BLACK);

    for (int i = 0; i <= LOGICAL_W / (10 * SCALE); ++i)
        DrawLine(i * 10 * SCALE, 0, i * 10 * SCALE, LOGICAL_H, kGrid);
    for (int i = 0; i <= LOGICAL_H / (10 * SCALE); ++i)
        DrawLine(0, i * 10 * SCALE, LOGICAL_W, i * 10 * SCALE, kGrid);

    DrawRectangle(g.apple.x, g.apple.y, 9 * SCALE, 9 * SCALE, kApple);
    DrawRectangle(g.head1.x, g.head1.y, 9 * SCALE, 9 * SCALE, g.p1Color);
    for (auto& s : g.tail1)
        DrawRectangle(s.x, s.y, 9 * SCALE, 9 * SCALE, g.p1Color);

    if (g.gameOver1)
    {
        const char* txt = "Game Over...";
        Vector2 sz = MeasureTextEx(gFont, txt, 45 * SCALE, 1);
        DrawTextEx(gFont, txt, { (LOGICAL_W - sz.x) / 2, (LOGICAL_H - sz.y) / 2 }, 45 * SCALE, 1, WHITE);
    }

    DrawText("[ESC] Menu   [R] Restart   [CTRL] Title", 10, LOGICAL_H - 24, 18, GRAY);
}

static bool OnTailList(const std::vector<Segment>& tail, int x, int y); // forward declaration

static void InitTwoPlayer()
{
    g.dxA = CELL; g.dyA = 0;
    g.dxB = -CELL; g.dyB = 0;
    g.headA = { 2 * CELL + 1, 1 };
    g.headB = { 37 * CELL + 1, 1 };
    g.apple2 = { -999, -999 }; // will be placed randomly below
    g.tailA.clear(); g.tailB.clear();
    for (int i = 1; i < 3; ++i) g.tailA.push_back({ g.headA.x - 10 * i * SCALE, g.headA.y });
    for (int i = 1; i < 3; ++i) g.tailB.push_back({ g.headB.x + 10 * i * SCALE, g.headB.y });
    g.p1Wins = g.p2Wins = g.draw2 = false;
    g.moveAccum2 = 0.0;
    // Place apple randomly at start
    const int nc = LOGICAL_W / CELL;
    int cx2, cy2;
    do {
        cx2 = GetRandomValue(0, nc - 1) * CELL + 1;
        cy2 = GetRandomValue(0, nc - 1) * CELL + 1;
    } while (OnTailList(g.tailA, cx2, cy2) || OnTailList(g.tailB, cx2, cy2));
    g.apple2.x = cx2; g.apple2.y = cy2;
}

static bool OnTailList(const std::vector<Segment>& tail, int x, int y)
{
    for (auto& s : tail) if (s.x == x && s.y == y) return true;
    return false;
}

static void TwoPlayerDidEat()
{
    if (g.headA.x != g.apple2.x || g.headA.y != g.apple2.y) return;
    const int numCells2 = LOGICAL_W / CELL;
    int prevX = g.apple2.x, prevY = g.apple2.y;
    int currX, currY;
    for (;;)
    {
        currX = GetRandomValue(0, numCells2 - 1) * CELL + 1;
        currY = GetRandomValue(0, numCells2 - 1) * CELL + 1;
        if (currX == prevX && currY == prevY) continue;
        if (OnTailList(g.tailA, currX, currY)) continue;
        if (OnTailList(g.tailB, currX, currY)) continue;
        break;
    }
    g.apple2.x = currX; g.apple2.y = currY;
    g.tailA.push_back(g.tailA.back());
    g.tailB.push_back(g.tailB.back());
}

static void TwoPlayerIsCollide()
{
    if (g.headA.x == g.headB.x && g.headA.y == g.headB.y)
    {
        g.draw2 = true;
        StopMusicIfPlaying();
        return;
    }
    for (auto& s : g.tailA)
    {
        if (g.headB.x == s.x && g.headB.y == s.y) { g.p1Wins = true; StopMusicIfPlaying(); return; }
        if ((g.headA.x == s.x && g.headA.y == s.y) ||
            g.headA.x < 0 || g.headA.x > LOGICAL_W || g.headA.y < 0 || g.headA.y > LOGICAL_H)
        { g.p2Wins = true; StopMusicIfPlaying(); return; }
    }
    for (auto& s : g.tailB)
    {
        if (g.headA.x == s.x && g.headA.y == s.y) { g.p2Wins = true; StopMusicIfPlaying(); return; }
        if ((g.headB.x == s.x && g.headB.y == s.y) ||
            g.headB.x < 0 || g.headB.x > LOGICAL_W || g.headB.y < 0 || g.headB.y > LOGICAL_H)
        { g.p1Wins = true; StopMusicIfPlaying(); return; }
    }
}

static void TwoPlayerEvent()
{
    if (arcade::IsActionPressed(arcade::Player::One, arcade::Action::Up) && g.dyA == 0) { g.dyA = -CELL; g.dxA = 0; }
    if (arcade::IsActionPressed(arcade::Player::One, arcade::Action::Down) && g.dyA == 0) { g.dyA = CELL; g.dxA = 0; }
    if (arcade::IsActionPressed(arcade::Player::One, arcade::Action::Left) && g.dxA == 0) { g.dxA = -CELL; g.dyA = 0; }
    if (arcade::IsActionPressed(arcade::Player::One, arcade::Action::Right) && g.dxA == 0) { g.dxA = CELL; g.dyA = 0; }

    if (arcade::IsActionPressed(arcade::Player::Two, arcade::Action::Up) && g.dyB == 0) { g.dyB = -CELL; g.dxB = 0; }
    if (arcade::IsActionPressed(arcade::Player::Two, arcade::Action::Down) && g.dyB == 0) { g.dyB = CELL; g.dxB = 0; }
    if (arcade::IsActionPressed(arcade::Player::Two, arcade::Action::Left) && g.dxB == 0) { g.dxB = -CELL; g.dyB = 0; }
    if (arcade::IsActionPressed(arcade::Player::Two, arcade::Action::Right) && g.dxB == 0) { g.dxB = CELL; g.dyB = 0; }

    if (arcade::IsActionPressed(arcade::Player::Any, arcade::Action::Restart))
    {
        StopMusicIfPlaying();
        InitTwoPlayer();
        PlayMusicLoop();
    }
    if (arcade::IsActionPressed(arcade::Player::Any, arcade::Action::Back))
    {
        StopMusicIfPlaying();
        g.scene = SceneId::Title;
        InitTwoPlayer();
    }
}

static void TwoPlayerUpdate(float dt)
{
    if (g.p1Wins || g.p2Wins || g.draw2) return;

    const double stepTime = 1.0 / 15.0;
    g.moveAccum2 += dt;
    if (g.moveAccum2 < stepTime) return;
    g.moveAccum2 -= stepTime;

    TwoPlayerDidEat();

    for (size_t i = g.tailA.size() - 1; i > 0; --i) g.tailA[i] = g.tailA[i - 1];
    if (!g.tailA.empty()) g.tailA[0] = g.headA;
    g.headA.x += g.dxA; g.headA.y += g.dyA;
    TwoPlayerIsCollide();
    if (g.p1Wins || g.p2Wins || g.draw2) return;

    for (size_t i = g.tailB.size() - 1; i > 0; --i) g.tailB[i] = g.tailB[i - 1];
    if (!g.tailB.empty()) g.tailB[0] = g.headB;
    g.headB.x += g.dxB; g.headB.y += g.dyB;
    TwoPlayerIsCollide();
}

static void TwoPlayerDraw()
{
    ClearBackground(BLACK);

    for (int i = 0; i <= LOGICAL_W / (10 * SCALE); ++i)
        DrawLine(i * 10 * SCALE, 0, i * 10 * SCALE, LOGICAL_H, kGrid);
    for (int i = 0; i <= LOGICAL_H / (10 * SCALE); ++i)
        DrawLine(0, i * 10 * SCALE, LOGICAL_W, i * 10 * SCALE, kGrid);

    DrawRectangle(g.apple2.x, g.apple2.y, 9 * SCALE, 9 * SCALE, kApple);

    DrawRectangle(g.headA.x, g.headA.y, 9 * SCALE, 9 * SCALE, g.p1Color);
    for (auto& s : g.tailA) DrawRectangle(s.x, s.y, 9 * SCALE, 9 * SCALE, g.p1Color);

    DrawRectangle(g.headB.x, g.headB.y, 9 * SCALE, 9 * SCALE, g.p2Color);
    for (auto& s : g.tailB) DrawRectangle(s.x, s.y, 9 * SCALE, 9 * SCALE, g.p2Color);

    const char* msg = nullptr;
    if (g.draw2) msg = "Draw...";
    else if (g.p1Wins) msg = "Player One Wins";
    else if (g.p2Wins) msg = "Player Two Wins";
    if (msg)
    {
        Vector2 sz = MeasureTextEx(gFont, msg, 45 * SCALE, 1);
        DrawTextEx(gFont, msg, { (LOGICAL_W - sz.x) / 2, (LOGICAL_H - sz.y) / 2 }, 45 * SCALE, 1, WHITE);
    }

    DrawText("[ESC] Menu   [R] Restart   [CTRL] Title", 10, LOGICAL_H - 24, 18, GRAY);
}

static Rectangle CenteredRect(float cx, float cy, float w, float h)
{
    return { cx - w / 2, cy - h / 2, w, h };
}

static void TitleEvent()
{
    if (arcade::IsActionPressed(arcade::Player::Any, arcade::Action::Down))
        g.titleSelection = (g.titleSelection == 3) ? 1 : g.titleSelection + 1;
    if (arcade::IsActionPressed(arcade::Player::Any, arcade::Action::Up))
        g.titleSelection = (g.titleSelection == 1) ? 3 : g.titleSelection - 1;

    if (arcade::IsActionPressed(arcade::Player::Any, arcade::Action::Confirm))
    {
        if (g.titleSelection == 1) { InitOnePlayer(); g.scene = SceneId::OnePlayer; PlayMusicLoop(); }
        else if (g.titleSelection == 2) { InitTwoPlayer(); g.scene = SceneId::TwoPlayer; PlayMusicLoop(); }
        else if (g.titleSelection == 3) { g.scene = SceneId::Config; }
    }
}

static void TitleUpdate(float dt) { (void)dt; }

static void TitleDraw()
{
    ClearBackground(BLACK);

    const char* title = "Edibles";
    Vector2 tsz = MeasureTextEx(gFont, title, 65 * SCALE, 1);
    float cx = LOGICAL_W / 2.0f;
    float cy = LOGICAL_H / 2.0f;
    DrawTextEx(gFont, title, { cx - tsz.x / 2, cy / 2 - tsz.y / 2 }, 65 * SCALE, 1, kBlue);

    struct Btn { const char* label; float yOff; int idx; };
    Btn btns[3] = {
        { "start one player", 73 * SCALE, 1 },
        { "start two player", 143 * SCALE, 2 },
        { "controls n config", 213 * SCALE, 3 },
    };

    for (auto& b : btns)
    {
        Rectangle r = CenteredRect(cx, cy / 2 + b.yOff, 221 * SCALE, 45 * SCALE);
        bool sel = (g.titleSelection == b.idx);
        DrawRectangleRoundedLinesEx(r, 0.3f, 8, 3.0f, kRed);
        if (sel)
            DrawRectangleRounded(r, 0.3f, 8, Fade(kRed, 0.5f));

        Vector2 tsz2 = MeasureTextEx(gFont, b.label, 22 * SCALE, 1);
        DrawTextEx(gFont, b.label, { r.x + r.width / 2 - tsz2.x / 2, r.y + r.height / 2 - tsz2.y / 2 },
                   22 * SCALE, 1, sel ? kBlue : kRed);
    }

    const char* hint = "UP/DOWN or W/S select   ENTER start   ESC quit";
    Vector2 hsz = MeasureTextEx(gFont, hint, 14 * SCALE, 1);
    DrawTextEx(gFont, hint, { cx - hsz.x / 2, LOGICAL_H - 40.0f }, 14 * SCALE, 1, GRAY);
}

static void RebuildConfigColors()
{
    g.p1Color = kColors[g.colorIndexOne].rgb;
    g.p2Color = kColors[g.colorIndexTwo].rgb;
}

static void ConfigEvent()
{
    if (arcade::IsActionPressed(arcade::Player::One, arcade::Action::Right))
    {
        g.colorIndexOne = (g.colorIndexOne == kColorCount - 1) ? 0 : g.colorIndexOne + 1;
        if (g.colorIndexOne == g.colorIndexTwo)
            g.colorIndexOne = (g.colorIndexOne == kColorCount - 1) ? 0 : g.colorIndexOne + 1;
        g.p1HiliRight = true; g.p1HiliLeft = false;
        RebuildConfigColors();
    }
    if (arcade::IsActionPressed(arcade::Player::One, arcade::Action::Left))
    {
        g.colorIndexOne = (g.colorIndexOne == 0) ? kColorCount - 1 : g.colorIndexOne - 1;
        if (g.colorIndexOne == g.colorIndexTwo)
            g.colorIndexOne = (g.colorIndexOne == 0) ? kColorCount - 1 : g.colorIndexOne - 1;
        g.p1HiliLeft = true; g.p1HiliRight = false;
        RebuildConfigColors();
    }
    if (arcade::IsActionPressed(arcade::Player::Two, arcade::Action::Right))
    {
        g.colorIndexTwo = (g.colorIndexTwo == kColorCount - 1) ? 0 : g.colorIndexTwo + 1;
        if (g.colorIndexTwo == g.colorIndexOne)
            g.colorIndexTwo = (g.colorIndexTwo == kColorCount - 1) ? 0 : g.colorIndexTwo + 1;
        g.p2HiliRight = true; g.p2HiliLeft = false;
        RebuildConfigColors();
    }
    if (arcade::IsActionPressed(arcade::Player::Two, arcade::Action::Left))
    {
        g.colorIndexTwo = (g.colorIndexTwo == 0) ? kColorCount - 1 : g.colorIndexTwo - 1;
        if (g.colorIndexTwo == g.colorIndexOne)
            g.colorIndexTwo = (g.colorIndexTwo == 0) ? kColorCount - 1 : g.colorIndexTwo - 1;
        g.p2HiliLeft = true; g.p2HiliRight = false;
        RebuildConfigColors();
    }

    if (arcade::IsActionPressed(arcade::Player::Any, arcade::Action::Back))
    {
        g.scene = SceneId::Title;
        g.p1HiliLeft = g.p1HiliRight = g.p2HiliLeft = g.p2HiliRight = false;
    }
}

static void ConfigUpdate(float dt) { (void)dt; }

static void DrawColorRow(float cx, float y, int selectedIdx, bool hiliLeft, bool hiliRight, const char* label)
{
    Vector2 lsz = MeasureTextEx(gFont, label, 20 * SCALE, 1);
    DrawTextEx(gFont, label, { cx - lsz.x / 2, y }, 20 * SCALE, 1, WHITE);

    float swatchY = y + 40 * SCALE;
    float swatchSize = 40.0f * SCALE;
    DrawRectangle((int)(cx - swatchSize / 2), (int)swatchY, (int)swatchSize, (int)swatchSize, kColors[selectedIdx].rgb);
    DrawRectangleLinesEx({ cx - swatchSize / 2, swatchY, swatchSize, swatchSize }, 3, WHITE);

    Vector2 nsz = MeasureTextEx(gFont, kColors[selectedIdx].name, 18 * SCALE, 1);
    DrawTextEx(gFont, kColors[selectedIdx].name, { cx - nsz.x / 2, swatchY + swatchSize + 8 }, 18 * SCALE, 1, WHITE);

    float arrowY = swatchY + swatchSize / 2;
    float lSize = hiliLeft ? 16.0f : 12.0f;
    float rSize = hiliRight ? 16.0f : 12.0f;
    Vector2 lTip = { cx - swatchSize / 2 - 20, arrowY };
    DrawTriangle({ lTip.x + lSize, arrowY - lSize }, { lTip.x + lSize, arrowY + lSize }, lTip, WHITE);
    Vector2 rTip = { cx + swatchSize / 2 + 20, arrowY };
    DrawTriangle({ rTip.x - rSize, arrowY - rSize }, rTip, { rTip.x - rSize, arrowY + rSize }, WHITE);
}

static void ConfigDraw()
{
    ClearBackground(BLACK);

    const char* header = "Controls & Config";
    Vector2 hsz = MeasureTextEx(gFont, header, 45 * SCALE, 1);
    DrawTextEx(gFont, header, { LOGICAL_W / 2.0f - hsz.x / 2, 20.0f * SCALE }, 45 * SCALE, 1, WHITE);

    DrawColorRow(LOGICAL_W * 0.33f, 130 * SCALE, g.colorIndexOne, g.p1HiliLeft, g.p1HiliRight, "Player One Color (A / D)");
    DrawColorRow(LOGICAL_W * 0.67f, 130 * SCALE, g.colorIndexTwo, g.p2HiliLeft, g.p2HiliRight, "Player Two Color (Left / Right)");

    const char* ctrls1 = "P1: W A S D    move";
    const char* ctrls2 = "P2: Arrow Keys move";
    const char* ctrls3 = "R restart    CTRL back to title    ESC quit to menu";
    DrawTextEx(gFont, ctrls1, { LOGICAL_W * 0.33f - MeasureTextEx(gFont, ctrls1, 18*SCALE, 1).x/2, 280.0f * SCALE }, 18 * SCALE, 1, GRAY);
    DrawTextEx(gFont, ctrls2, { LOGICAL_W * 0.67f - MeasureTextEx(gFont, ctrls2, 18*SCALE, 1).x/2, 280.0f * SCALE }, 18 * SCALE, 1, GRAY);
    DrawTextEx(gFont, ctrls3, { LOGICAL_W / 2.0f - MeasureTextEx(gFont, ctrls3, 16*SCALE, 1).x/2, 340.0f * SCALE }, 16 * SCALE, 1, GRAY);

    DrawRectangleRounded({ 0, 0, (float)(63 * SCALE), (float)(30 * SCALE) }, 0.5f, 8, WHITE);
    DrawTextEx(gFont, "ctrl", { (float)(25 * SCALE), (float)(7 * SCALE) }, 20 * SCALE, 1, BLACK);
}

int main(void)
{
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, "Game 3 - Edibles");
    SetExitKey(KEY_ESCAPE);
    SetTargetFPS(60);

    SearchAndSetResourceDir("resources");

    InitAudioDevice();

    gFont = LoadFontEx("fonts/Condition.ttf", 96, 0, 0);

    if (FileExists("music/snakesong.wav"))
    {
        g.music = LoadMusicStream("music/snakesong.wav");
        g.musicLoaded = true;
    }

    Image icon = { 0 };
    if (FileExists("ed.png"))
    {
        icon = LoadImage("ed.png");
        SetWindowIcon(icon);
    }

    RebuildConfigColors();

    RenderTexture2D target = LoadRenderTexture(LOGICAL_W, LOGICAL_H);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        if (g.musicLoaded && g.musicPlaying)
            UpdateMusicStream(g.music);

        switch (g.scene)
        {
            case SceneId::Title:     TitleEvent();     TitleUpdate(dt);     break;
            case SceneId::OnePlayer: OnePlayerEvent(); OnePlayerUpdate(dt); break;
            case SceneId::TwoPlayer: TwoPlayerEvent(); TwoPlayerUpdate(dt); break;
            case SceneId::Config:    ConfigEvent();    ConfigUpdate(dt);    break;
        }

        BeginTextureMode(target);
        switch (g.scene)
        {
            case SceneId::Title:     TitleDraw();     break;
            case SceneId::OnePlayer: OnePlayerDraw(); break;
            case SceneId::TwoPlayer: TwoPlayerDraw(); break;
            case SceneId::Config:    ConfigDraw();    break;
        }
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);

        int W = GetScreenWidth();
        int H = GetScreenHeight();
        float scale = (float)((W < H) ? W : H) / (float)LOGICAL_W;
        if ((float)H / LOGICAL_H < scale) scale = (float)H / LOGICAL_H;
        float destW = LOGICAL_W * scale;
        float destH = LOGICAL_H * scale;
        Rectangle src  = { 0, 0, (float)target.texture.width, -(float)target.texture.height };
        Rectangle dest = { (W - destW) / 2.0f, (H - destH) / 2.0f, destW, destH };
        DrawTexturePro(target.texture, src, dest, { 0, 0 }, 0.0f, WHITE);

        EndDrawing();
    }

    UnloadRenderTexture(target);
    if (icon.data != nullptr) UnloadImage(icon);
    if (g.musicLoaded) UnloadMusicStream(g.music);
    UnloadFont(gFont);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
