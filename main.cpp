// Ilot14: The Big 14 - a top-down world game for Nintendo Switch (homebrew NRO)
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "logo_data.h"
#include "miniz.h"
#ifdef __SWITCH__
#include <switch.h>
#endif

#define GAME_VERSION "0.1.0"
static const int W = 1280, H = 720, T = 48, MW = 44, MH = 24;

// ------------------------------------------------------------------ colors
struct Col { int r, g, b; };
static Col mix(Col a, Col b, float t) { return {(int)(a.r + (b.r - a.r) * t), (int)(a.g + (b.g - a.g) * t), (int)(a.b + (b.b - a.b) * t)}; }
static const Col ORANGE = {249, 115, 22}, INK = {12, 10, 18}, WHITE = {244, 244, 250}, SKY = {90, 190, 255}, MINT = {80, 220, 160}, PINK = {255, 110, 170}, GOLD = {255, 205, 70};

// ------------------------------------------------------------------ world data (edit me!)
enum Kind { RES, PARK, UTIL, OUTSIDE };
struct FloorDef { const char* name; const char* sub; Col tint; Kind kind; };
static const FloorDef FLOORS[] = {
    {"Lobby", "Welcome to the 14", {40, 64, 112}, RES},
    {"Common", "Where everyone hangs out", {40, 110, 96}, RES},
    {"Uncommon", "A little less common", {90, 70, 130}, RES},
    {"Rare", "Quiet. Suspiciously quiet.", {130, 60, 96}, RES},
    {"Special", "Fancy carpets", {140, 110, 40}, RES},
    {"Main", "The heart of the 14", {170, 84, 36}, RES},
    {"Lethal", "Spicy food. Be careful.", {150, 40, 52}, RES},
    {"Teen", "Enter with snacks only", {40, 120, 160}, RES},
    {"Underlobby", "Right below the lobby", {44, 52, 84}, RES},
    {"Parking 1", "Underground", {70, 76, 90}, PARK},
    {"Parking 2", "Underground", {60, 64, 78}, PARK},
    {"Elevator shaft fix", "Hard hats required", {128, 112, 40}, UTIL},
    {"Elevator control room", "Do not touch the big red button", {112, 56, 56}, UTIL},
    {"Air ventilation systems", "Windy. Very windy.", {44, 110, 124}, UTIL},
    {"14 Cleaning bots parking", "Charging docks", {40, 120, 84}, PARK},
    {"Further inn", "Holiday Inn branded rooms", {90, 72, 140}, RES},
    {"Parking 1 (outside)", "On the ground. Fresh air.", {84, 92, 96}, OUTSIDE},
};
static const int NF = sizeof(FLOORS) / sizeof(FLOORS[0]);
static const int ELEV_FLOORS = 16;  // the last one (outside parking) is only reachable by the secret stairway
static const int F_COMMON = 1, F_BOTS = 14, F_OUT = 16;

struct NpcDef { int floor, tx, ty; const char* name; Col col, hair; std::vector<const char*> lines; };
static const std::vector<NpcDef> NPCS = {
    {0, 16, 11, "Aya", PINK, {40, 24, 20}, {"Welcome to the 14! The elevators are at the far left.", "Sixteen floors. Eight up, eight down. Do not ask about the ninth."}},
    {0, 36, 5, "Raceem", SKY, {20, 20, 30}, {"I live here too. Everyone lives here. It is a whole building.", "The lobby plant is not real. We do not talk about it."}},
    {1, 16, 5, "Dalia", MINT, {60, 30, 24}, {}},  // quest giver (special)
    {1, 28, 11, "Dania", GOLD, {80, 50, 30}, {"Common floor is the best floor. Fight me. Politely.", "Someone left a sock in the hallway. It has been there for days."}},
    {1, 6, 18, "Arwa", PINK, {30, 20, 20}, {"I am practising my laugh. Ha. Ha. Haaa. How was that?"}},
    {2, 26, 5, "Adam", ORANGE, {30, 24, 20}, {"There are five Adams in this building. I am the best one.", "...Do not tell the other four I said that."}},
    {3, 6, 5, "Youcef", SKY, {24, 20, 24}, {"Shh. This is the Rare floor. We keep it rare by being quiet."}},
    {4, 36, 18, "Basmala", MINT, {50, 30, 40}, {"The carpets on Special are special. Please wipe your feet.", "Yes, even the bots wipe their wheels."}},
    {5, 16, 18, "Fares", GOLD, {30, 20, 20}, {"Main floor, main character energy.", "Everything important happens near here eventually."}},
    {6, 26, 18, "Anes", PINK, {30, 20, 24}, {"Lethal floor. The only lethal thing is my cooking.", "Okay, it is also the hot sauce."}},
    {7, 6, 11, "Iyad", SKY, {20, 20, 30}, {"Teen floor rules: music loud, snacks loud, bots very confused."}},
    {7, 36, 11, "Kenzy", ORANGE, {24, 20, 20}, {"Have you seen the secret door on Common? Nobody tells me anything."}},
    {8, 16, 5, "Maysanne", MINT, {40, 24, 20}, {"It is a little dark down here, but the Underlobby has good echo.", "Say something. ...Something. ...Something."}},
    {9, 12, 11, "Jilali", GOLD, {30, 24, 20}, {"Parking 1. Underground edition. There is also an outside one.", "Rumor says there is a secret way out to it. Rumor."}},
    {10, 20, 11, "Alicia", PINK, {50, 30, 30}, {"Parking 2 is bigger than Parking 1. Do not tell Parking 1."}},
    {11, 24, 11, "Adam", SKY, {26, 20, 20}, {"The elevator shaft needs fixing. I am the fixing department.", "Hand me that wrench. No, the other wrench."}},
    {12, 20, 11, "Abdullah", ORANGE, {30, 24, 20}, {"Control room. Do NOT touch the big red button.", "I mean it. Last time all the lights went disco."}},
    {13, 24, 11, "Jalil", MINT, {30, 20, 20}, {"The wind down here is free. You are welcome.", "My hair does this on purpose."}},
    {15, 16, 5, "Sofia", PINK, {40, 24, 24}, {"Welcome to Further inn. Holiday Inn branded rooms, 14 branded vibes.", "Checkout is whenever the elevator decides."}},
    {15, 26, 18, "Amira", GOLD, {30, 20, 20}, {"The beds here are so soft. Suspiciously soft."}},
    {16, 14, 11, "Gabriel", SKY, {20, 20, 28}, {}},  // quest (special)
};

struct Pers { const char* name; Col col; std::vector<const char*> lines; };
static const std::vector<Pers> PERSONS = {
    {"Grumpy", {200, 90, 90}, {"Bzzt. I clean. I do not enjoy it.", "Do not step on my floor.", "Another day, another dust bunny. Ugh."}},
    {"Dramatic", {190, 110, 230}, {"Oh, the DUST! The endless, endless dust!", "I was born to sweep, and sweep I shall... dramatically.", "Nobody understands the burden of the mop!"}},
    {"Shy", {120, 200, 230}, {"...h-hi. Sorry. I will clean somewhere else.", "Please do not look at me while I vacuum.", "Beep. (Quietly.)"}},
    {"Overconfident", {250, 200, 60}, {"I am the best cleaning bot in the 14. Easily.", "Spotless. As always. Because of me.", "Cleaning is just winning, but with wheels."}},
    {"Sleepy", {150, 160, 230}, {"zzz... oh. Hi. Was I cleaning?", "Five more minutes... of mopping...", "Charging dock... pleeease..."}},
    {"Poetic", {120, 230, 170}, {"Dust falls like quiet snow. I rise to meet it.", "O floor, so wide and shining...", "A single crumb. A story untold."}},
};

// ------------------------------------------------------------------ input
enum { K_A = 1, K_B = 2, K_X = 4, K_Y = 8, K_UP = 16, K_DOWN = 32, K_LEFT = 64, K_RIGHT = 128, K_PLUS = 256 };
struct In {
    unsigned down = 0;   // pressed this frame
    unsigned held = 0;
    float ax = 0, ay = 0;  // analog / dpad movement
};

// ------------------------------------------------------------------ rendering helpers
static SDL_Window* g_win = nullptr;
static SDL_Renderer* g_r = nullptr;
static TTF_Font *fS, *fM, *fL, *fXL;
static SDL_Texture* g_logo = nullptr;
static int g_logoW = 0, g_logoH = 0;

static void setc(Col c, int a = 255) { SDL_SetRenderDrawColor(g_r, c.r, c.g, c.b, a); }
static void rect(int x, int y, int w, int h, Col c, int a = 255) {
    setc(c, a);
    SDL_Rect r = {x, y, w, h};
    SDL_RenderFillRect(g_r, &r);
}
static void frame(int x, int y, int w, int h, Col c, int th = 2) {
    rect(x, y, w, th, c); rect(x, y + h - th, w, th, c); rect(x, y, th, h, c); rect(x + w - th, y, th, h, c);
}
static void text(TTF_Font* f, const std::string& s, int x, int y, Col c, int align = 0, int a = 255) {
    if (s.empty()) return;
    SDL_Color sc = {(Uint8)c.r, (Uint8)c.g, (Uint8)c.b, 255};
    SDL_Surface* sf = TTF_RenderUTF8_Blended(f, s.c_str(), sc);
    if (!sf) return;
    SDL_Texture* t = SDL_CreateTextureFromSurface(g_r, sf);
    SDL_SetTextureAlphaMod(t, (Uint8)a);
    SDL_Rect d = {align == 0 ? x : align == 1 ? x - sf->w / 2 : x - sf->w, y, sf->w, sf->h};
    SDL_RenderCopy(g_r, t, nullptr, &d);
    SDL_DestroyTexture(t);
    SDL_FreeSurface(sf);
}
static int textW(TTF_Font* f, const std::string& s) { int w = 0, h = 0; TTF_SizeUTF8(f, s.c_str(), &w, &h); return w; }
// word-wrapped text, returns lines drawn
static int wrapText(TTF_Font* f, const std::string& s, int x, int y, int maxw, Col c, int lh) {
    std::string line, word;
    int n = 0;
    for (size_t i = 0; i <= s.size(); i++) {
        if (i == s.size() || s[i] == ' ') {
            std::string t = line.empty() ? word : line + " " + word;
            if (!line.empty() && textW(f, t) > maxw) { text(f, line, x, y + n * lh, c); n++; line = word; }
            else line = t;
            word.clear();
        } else word += s[i];
    }
    if (!line.empty()) { text(f, line, x, y + n * lh, c); n++; }
    return n;
}

// ------------------------------------------------------------------ world
struct Map { std::vector<std::string> t; };
static Map g_maps[32];
static unsigned hash2(int a, int b) { unsigned h = (unsigned)a * 374761393u + (unsigned)b * 668265263u; h = (h ^ (h >> 13)) * 1274126177u; return h ^ (h >> 16); }

static void carve(Map& m, int x0, int y0, int x1, int y1, char c) {
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) m.t[y][x] = c;
}
static void buildMap(int fi) {
    Map& m = g_maps[fi];
    m.t.assign(MH, std::string(MW, '#'));
    const FloorDef& fd = FLOORS[fi];
    if (fd.kind == RES) {
        carve(m, 1, 10, MW - 2, 13, '.');
        const int rx[4] = {2, 12, 22, 32};
        for (int k = 0; k < 4; k++) {
            carve(m, rx[k], 1, rx[k] + 7, 8, '.');
            carve(m, rx[k], 15, rx[k] + 7, 22, '.');
            m.t[9][rx[k] + 3] = 'd'; m.t[9][rx[k] + 4] = 'd';
            m.t[14][rx[k] + 3] = 'd'; m.t[14][rx[k] + 4] = 'd';
            // furniture in corners of every room (centers stay free for people)
            for (int row = 0; row < 2; row++) {
                int y0 = row ? 15 : 1, y1 = row ? 22 : 8;
                m.t[y0][rx[k]] = 'f'; m.t[y0][rx[k] + 1] = 'f'; m.t[y1][rx[k] + 7] = 'f'; m.t[y1][rx[k] + 6] = 'f';
                if (hash2(fi, k * 2 + row) & 1) m.t[y1][rx[k]] = 'f';
            }
        }
        m.t[11][0] = 'E'; m.t[12][0] = 'E';
        if (fi == F_COMMON) m.t[9][21] = 'S';  // the secret stairway, next to Dalia's bedroom
    } else if (fd.kind == PARK || fd.kind == OUTSIDE) {
        carve(m, 1, 1, MW - 2, MH - 2, '.');
        for (int x = 8; x < MW - 2; x += 8) for (int y : {5, 18}) m.t[y][x] = '#';
        for (int x = 4; x < MW - 6; x += 7) for (int y : {2, 20}) {
            if (fd.kind == OUTSIDE && (x + y) % 3 == 0) continue;
            m.t[y][x] = 'c'; m.t[y][x + 1] = 'c';
        }
        if (fi == F_BOTS) {
            for (int x = 4; x < MW - 3; x += 4) m.t[1][x] = 'K';
            m.t[11][4] = 'T';
        }
        if (fi == F_OUT) { m.t[11][0] = 'S'; m.t[12][0] = 'S'; }
        else { m.t[11][0] = 'E'; m.t[12][0] = 'E'; }
    } else {  // UTIL
        carve(m, 1, 1, MW - 2, MH - 2, '.');
        for (int x = 5; x < MW - 4; x += 9) for (int y = 3; y < MH - 3; y += 4) {
            if (y >= 9 && y <= 14) continue;
            m.t[y][x] = 'm'; m.t[y][x + 1] = 'm'; m.t[y + 1][x] = 'm'; m.t[y + 1][x + 1] = 'm';
        }
        m.t[11][0] = 'E'; m.t[12][0] = 'E';
    }
}
static char tileAt(int fi, int tx, int ty) {
    if (tx < 0 || ty < 0 || tx >= MW || ty >= MH) return '#';
    return g_maps[fi].t[ty][tx];
}
static bool solidC(char c) { return c == '#' || c == 'f' || c == 'c' || c == 'm' || c == 'E' || c == 'S' || c == 'K' || c == 'T'; }

// ------------------------------------------------------------------ game state
enum State { S_TITLE, S_PLAY, S_TALK, S_ELEV, S_PAUSE, S_CARD };
enum After { AT_NONE, AT_Q1, AT_Q2, AT_Q3CARD, AT_MG };
struct Bot { float x, y, vx = 0, vy = 0; int pers; int floor; float timer = 0; bool caught = false, mg = false; };
struct Line { std::string who, what; Col col; };

static State g_state = S_TITLE;
static float g_time = 0;
static int g_floor = 0;
static float g_px = 0, g_py = 0;
static int g_face = 1;
static float g_walk = 0;
static int g_quest = 0;  // 0 talk to Dalia, 1 find Gabriel, 2 catch bots, 3 return to Dalia, 4 done
static std::vector<Bot> g_bots;
static std::vector<Line> g_talk;
static size_t g_talkI = 0;
static float g_talkChars = 0;
static After g_after = AT_NONE;
static int g_elevSel = 0;
static bool g_quit = false;
// transitions
static int g_fadePhase = 0;  // 0 none, 1 out, 2 in
static float g_fade = 0;
static int g_toFloor = 0;
static float g_toX = 0, g_toY = 0;
// minigame
static bool g_mg = false;
static float g_mgTime = 0;
static int g_mgCaught = 0;
static const int MG_BOTS = 5;
static const float MG_SECONDS = 45;
static std::string g_toast;
static float g_toastT = 0;
static std::string g_card1, g_card2;

static unsigned g_rng = 12345;
static float frand() { g_rng = g_rng * 1664525u + 1013904223u; return (g_rng >> 8) / 16777216.0f; }

static void toast(const std::string& s) { g_toast = s; g_toastT = 3.0f; }

static void say(const std::string& who, const std::string& what, Col c) { g_talk.push_back({who, what, c}); }
static void startTalk(After a) { g_talkI = 0; g_talkChars = 0; g_after = a; g_state = S_TALK; }

static bool freeAt(int fi, float x, float y) {  // hitbox centered on foot point
    const float hw = 11, hh = 7;
    for (float dx : {-hw, hw}) for (float dy : {-hh, hh}) {
        if (solidC(tileAt(fi, (int)std::floor((x + dx) / T), (int)std::floor((y + dy) / T)))) return false;
    }
    return true;
}
static void teleport(int fi, float x, float y) { g_toFloor = fi; g_toX = x; g_toY = y; g_fadePhase = 1; g_fade = 0; }
static void placeBots() {
    g_bots.clear();
    for (int fi = 0; fi < NF; fi++) {
        if (fi == F_BOTS) continue;
        Bot b;
        b.floor = fi; b.pers = (fi * 5 + 1) % (int)PERSONS.size();
        for (int tries = 0; tries < 200; tries++) {
            int tx = 2 + (int)(frand() * (MW - 4)), ty = 2 + (int)(frand() * (MH - 4));
            if (tileAt(fi, tx, ty) == '.') { b.x = (tx + 0.5f) * T; b.y = (ty + 0.5f) * T; break; }
        }
        g_bots.push_back(b);
    }
}
static void newGame() {
    g_quest = 0; g_floor = 0; g_px = 3.5f * T; g_py = 11.5f * T; g_face = 1; g_mg = false;
    placeBots();
    g_toast.clear();
    g_state = S_PLAY;
    toast("Objective: find Dalia on the Common floor");
}
static const char* objective() {
    switch (g_quest) {
        case 0: return "Find Dalia on the Common floor (elevator, far left).";
        case 1: return "Look for a secret door near Dalia's room. Find Gabriel.";
        case 2: return "Take the elevator down to the 14 Cleaning bots parking. Use the console.";
        case 3: return "Tell Dalia the bots are back.";
        default: return "Free roam. Explore every floor!";
    }
}

// ---- interactions
struct Target { int kind = 0; int idx = 0; float d = 1e9; int tx = 0, ty = 0; };  // kind: 1 npc, 2 bot, 3 tile
static Target findTarget() {
    Target best;
    for (size_t i = 0; i < NPCS.size(); i++) {
        const NpcDef& n = NPCS[i];
        if (n.floor != g_floor) continue;
        float dx = (n.tx + 0.5f) * T - g_px, dy = (n.ty + 0.5f) * T - g_py;
        float d = std::sqrt(dx * dx + dy * dy);
        if (d < 80 && d < best.d) { best.kind = 1; best.idx = (int)i; best.d = d; }
    }
    for (size_t i = 0; i < g_bots.size(); i++) {
        const Bot& b = g_bots[i];
        if (b.floor != g_floor || b.mg) continue;
        float d = std::hypot(b.x - g_px, b.y - g_py);
        if (d < 70 && d < best.d) { best.kind = 2; best.idx = (int)i; best.d = d; }
    }
    int cx = (int)(g_px / T), cy = (int)(g_py / T);
    for (int ty = cy - 2; ty <= cy + 2; ty++) for (int tx = cx - 2; tx <= cx + 2; tx++) {
        char c = tileAt(g_floor, tx, ty);
        if (c != 'E' && c != 'S' && c != 'T') continue;
        float d = std::hypot((tx + 0.5f) * T - g_px, (ty + 0.5f) * T - g_py) - 10;
        if (d < 70 && d < best.d) { best.kind = 3; best.d = d; best.tx = tx; best.ty = ty; }
    }
    return best;
}
static void talkNpc(const NpcDef& n) {
    g_talk.clear();
    After a = AT_NONE;
    std::string nm = n.name;
    if (nm == "Dalia") {
        if (g_quest == 0) {
            say(nm, "Amir! Finally. My cleaning bot ran away and I think the whole building's bots are loose.", n.col);
            say(nm, "Gabriel saw one heading outside. There is a secret door next to my bedroom. We call it the Stairway to Hell.", n.col);
            say(nm, "It is not that scary. It just leads to the parking lot. Go find Gabriel!", n.col);
            a = AT_Q1;
        } else if (g_quest == 3) {
            say(nm, "You got them ALL back? Amir, you are a legend.", n.col);
            say(nm, "My bot says it was just 'exploring'. We will see about that.", n.col);
            a = AT_Q3CARD;
        } else if (g_quest >= 4) {
            say(nm, "Thank you again. The 14 is cleaner because of you.", n.col);
        } else {
            say(nm, "Find the secret door next to my room, then talk to Gabriel. Hurry!", n.col);
        }
    } else if (nm == "Gabriel") {
        if (g_quest == 1) {
            say(nm, "Amir! Over here. I saw the bots roll off. They all went down to their parking.", n.col);
            say(nm, "Go back through the secret door, take the elevator to the 14 Cleaning bots parking, and use the console.", n.col);
            say(nm, "They are fast, but you are faster. Corner them against the cars!", n.col);
            a = AT_Q2;
        } else if (g_quest >= 2) {
            say(nm, "Secret door, elevator, bots parking, console. You got this.", n.col);
        } else {
            say(nm, "Is Dalia looking for you? She has been pacing all morning.", n.col);
        }
    } else {
        for (auto* l : n.lines) say(nm, l, n.col);
    }
    startTalk(a);
}
static void interact() {
    Target t = findTarget();
    if (t.kind == 1) talkNpc(NPCS[t.idx]);
    else if (t.kind == 2) {
        Bot& b = g_bots[t.idx];
        const Pers& p = PERSONS[b.pers];
        g_talk.clear();
        say(std::string("Cleaning bot (") + p.name + ")", p.lines[(size_t)(g_time * 3) % p.lines.size()], p.col);
        startTalk(AT_NONE);
    } else if (t.kind == 3) {
        char c = tileAt(g_floor, t.tx, t.ty);
        if (c == 'E') { g_elevSel = g_floor < ELEV_FLOORS ? g_floor : 0; g_state = S_ELEV; }
        else if (c == 'S') {
            if (g_floor == F_COMMON) {
                teleport(F_OUT, 2.5f * T, 11.5f * T);
                if (g_quest < 1) toast("A secret door. You step through...");
            } else teleport(F_COMMON, 21.5f * T, 10.5f * T);
        } else if (c == 'T') {
            g_talk.clear();
            if (g_quest < 2) say("Console", "Locked. Maybe someone upstairs knows what is going on.", SKY);
            else {
                say("Console", g_quest == 2 ? "Bot roundup! 5 bots escaped. Catch them all before time runs out." : "Practice round: catch 5 bots before time runs out.", MINT);
                say("Console", "They run away when you get close. Corner them against the cars. Ready?", MINT);
            }
            startTalk(g_quest < 2 ? AT_NONE : AT_MG);
        }
    }
}
static void startMinigame() {
    g_mg = true; g_mgTime = MG_SECONDS; g_mgCaught = 0;
    g_bots.erase(std::remove_if(g_bots.begin(), g_bots.end(), [](const Bot& b) { return b.mg; }), g_bots.end());
    for (int i = 0; i < MG_BOTS; i++) {
        Bot b; b.floor = F_BOTS; b.mg = true; b.pers = i % (int)PERSONS.size();
        for (int tries = 0; tries < 400; tries++) {
            int tx = 6 + (int)(frand() * (MW - 10)), ty = 3 + (int)(frand() * (MH - 6));
            float x = (tx + 0.5f) * T, y = (ty + 0.5f) * T;
            if (tileAt(F_BOTS, tx, ty) == '.' && std::hypot(x - g_px, y - g_py) > 300) { b.x = x; b.y = y; break; }
        }
        g_bots.push_back(b);
    }
}
static void endMinigame(bool won) {
    g_mg = false;
    g_bots.erase(std::remove_if(g_bots.begin(), g_bots.end(), [](const Bot& b) { return b.mg; }), g_bots.end());
    g_talk.clear();
    if (won) {
        say("Console", "All 5 bots caught and docked! Great job, Amir.", MINT);
        if (g_quest == 2) { g_quest = 3; say("Console", "Now head back up and tell Dalia.", MINT); }
    } else say("Console", "Time is up! The bots escaped again. Use the console to retry.", PINK);
    startTalk(AT_NONE);
}

// ---- update
static void moveBot(Bot& b, float dx, float dy) {
    if (freeAt(b.floor, b.x + dx, b.y)) b.x += dx;
    if (freeAt(b.floor, b.x, b.y + dy)) b.y += dy;
}
static void updatePlay(float dt, const In& in) {
    g_time += dt;
    if (g_toastT > 0) g_toastT -= dt;
    // player
    float ax = in.ax, ay = in.ay;
    float len = std::sqrt(ax * ax + ay * ay);
    if (len > 1) { ax /= len; ay /= len; }
    const float spd = 250;
    float dx = ax * spd * dt, dy = ay * spd * dt;
    if (freeAt(g_floor, g_px + dx, g_py)) g_px += dx;
    if (freeAt(g_floor, g_px, g_py + dy)) g_py += dy;
    if (std::fabs(ax) > 0.2f) g_face = ax > 0 ? 1 : -1;
    g_walk = (len > 0.1f) ? g_walk + dt * 10 : 0;
    // bots
    for (auto& b : g_bots) {
        if (b.floor != g_floor) continue;
        if (b.mg) {
            float d = std::hypot(b.x - g_px, b.y - g_py);
            if (d < 30) { b.caught = true; continue; }
            float sp = 150, vx = 0, vy = 0;
            if (d < 230) {  // flee, with a bit of sideways slide so they do not stick to walls
                float ux = (b.x - g_px) / d, uy = (b.y - g_py) / d;
                vx = ux * sp + -uy * 40 * (b.pers % 2 ? 1 : -1); vy = uy * sp + ux * 40 * (b.pers % 2 ? 1 : -1);
                // slide along walls
                float nx = b.x + vx * dt, ny = b.y + vy * dt;
                if (!freeAt(b.floor, nx, b.y)) { vx = 0; vy = (vy >= 0 ? 1 : -1) * sp; }
                if (!freeAt(b.floor, b.x, ny)) { vy = 0; vx = (vx >= 0 ? 1 : -1) * sp; }
            } else { b.timer -= dt; if (b.timer <= 0) { b.timer = 0.5f + frand(); float a = frand() * 6.283f; b.vx = std::cos(a) * 50; b.vy = std::sin(a) * 50; } vx = b.vx; vy = b.vy; }
            moveBot(b, vx * dt, vy * dt);
        } else {
            b.timer -= dt;
            if (b.timer <= 0) { b.timer = 1 + frand() * 2; if (frand() < 0.4f) { b.vx = b.vy = 0; } else { float a = frand() * 6.283f; b.vx = std::cos(a) * 45; b.vy = std::sin(a) * 45; } }
            moveBot(b, b.vx * dt, b.vy * dt);
        }
    }
    if (g_mg) {
        g_mgCaught = 0;
        for (auto& b : g_bots) if (b.mg && b.caught) g_mgCaught++;
        g_mgTime -= dt;
        if (g_mgCaught >= MG_BOTS) endMinigame(true);
        else if (g_mgTime <= 0) endMinigame(false);
    }
    if (in.down & K_A) interact();
    if (in.down & K_PLUS) g_state = S_PAUSE;
}
static void update(float dt, const In& in) {
    // fade transitions run in every state
    if (g_fadePhase == 1) {
        g_fade += dt * 4;
        if (g_fade >= 1) {
            g_floor = g_toFloor; g_px = g_toX; g_py = g_toY; g_fadePhase = 2; g_fade = 1;
            if (g_mg) { g_mg = false; g_bots.erase(std::remove_if(g_bots.begin(), g_bots.end(), [](const Bot& b) { return b.mg; }), g_bots.end()); }
            g_state = S_PLAY;
        }
        return;
    }
    if (g_fadePhase == 2) { g_fade -= dt * 4; if (g_fade <= 0) { g_fade = 0; g_fadePhase = 0; } }
    switch (g_state) {
        case S_TITLE:
            g_time += dt;
            if (in.down & K_A) newGame();
            if (in.down & K_PLUS) g_quit = true;
            break;
        case S_PLAY: updatePlay(dt, in); break;
        case S_TALK: {
            g_time += dt;
            const std::string& cur = g_talk[g_talkI].what;
            g_talkChars += dt * 55;
            if (in.down & (K_A | K_B)) {
                if (g_talkChars < (float)cur.size()) g_talkChars = (float)cur.size();
                else if (++g_talkI >= g_talk.size()) {
                    g_state = S_PLAY;
                    After a = g_after; g_after = AT_NONE;
                    if (a == AT_Q1) { g_quest = 1; toast("Objective updated"); }
                    else if (a == AT_Q2) { g_quest = 2; toast("Objective updated"); }
                    else if (a == AT_Q3CARD) {
                        g_quest = 4;
                        g_card1 = "To be continued...";
                        g_card2 = "Lost of Ashes - the story goes on. Keep exploring the 14!";
                        g_state = S_CARD;
                    } else if (a == AT_MG) startMinigame();
                } else g_talkChars = 0;
            }
            break;
        }
        case S_ELEV:
            if (in.down & K_UP) g_elevSel = (g_elevSel + ELEV_FLOORS - 1) % ELEV_FLOORS;
            if (in.down & K_DOWN) g_elevSel = (g_elevSel + 1) % ELEV_FLOORS;
            if (in.down & (K_LEFT | K_RIGHT)) g_elevSel = (g_elevSel + 8) % ELEV_FLOORS;
            if (in.down & K_B) g_state = S_PLAY;
            if (in.down & K_A) {
                if (g_elevSel == g_floor) g_state = S_PLAY;
                else teleport(g_elevSel, 2.5f * T, 11.5f * T);
            }
            break;
        case S_PAUSE:
            if (in.down & (K_A | K_PLUS | K_B)) g_state = S_PLAY;
            if (in.down & K_X) g_state = S_TITLE;
            if (in.down & K_Y) g_quit = true;
            break;
        case S_CARD:
            if (in.down & K_A) g_state = S_PLAY;
            break;
    }
}

// ------------------------------------------------------------------ drawing
static void person(int x, int y, Col body, Col hair, int face, float walk, bool me = false) {
    // (x,y) = feet. Black outline first, like the 14 logo.
    int step = (int)(std::sin(walk) * 3);
    rect(x - 15, y - 41, 30, 38, INK);
    rect(x - 10, y - 8 + (step > 0 ? 0 : 0), 8, 8 - (step > 0 ? 0 : 0), INK);
    rect(x - 12, y - 7, 9, 6 + step, mix(body, INK, 0.5f));
    rect(x + 3, y - 7, 9, 6 - step, mix(body, INK, 0.5f));
    rect(x - 13, y - 23, 26, 17, body);
    rect(x - 13, y - 14, 26, 3, mix(body, INK, 0.25f));
    Col skin = {244, 200, 160};
    rect(x - 12, y - 40, 24, 19, skin);
    rect(x - 13, y - 41, 26, 8, hair);
    rect(x - 13 + (face > 0 ? 0 : 20), y - 36, 6, 12, hair);
    int ex = face > 0 ? 3 : -9;
    rect(x + ex, y - 32, 4, 5, INK);
    rect(x + ex + 6, y - 32, 4, 5, INK);
    if (me) rect(x - 4, y - 23, 8, 5, WHITE);  // little "14" badge
}
static void botSprite(int x, int y, Col c, float t, bool mg) {
    int bob = (int)(std::sin(t * 6 + x) * 2);
    rect(x - 16, y - 28 + bob, 32, 24, INK);
    rect(x - 14, y - 26 + bob, 28, 20, c);
    rect(x - 10, y - 22 + bob, 20, 9, INK);
    rect(x - 7, y - 20 + bob, 4, 4, SKY); rect(x + 3, y - 20 + bob, 4, 4, SKY);
    rect(x - 12, y - 4, 8, 5, INK); rect(x + 4, y - 4, 8, 5, INK);
    rect(x - 1, y - 36 + bob, 3, 9, INK); rect(x - 3, y - 39 + bob, 7, 5, mg ? GOLD : PINK);
}
static void drawTile(int fi, int tx, int ty, int sx, int sy) {
    const FloorDef& fd = FLOORS[fi];
    char c = tileAt(fi, tx, ty);
    Col fl = fd.tint;
    Col floorC = mix(fl, INK, 0.42f);
    if (((tx + ty) & 1) == 0) floorC = mix(floorC, WHITE, 0.05f);
    if (c == '#') {
        Col w = mix(fl, WHITE, 0.30f);
        rect(sx, sy, T, T, w);
        rect(sx, sy, T, 4, mix(w, WHITE, 0.35f));
        if (tileAt(fi, tx, ty + 1) != '#') rect(sx, sy + T - 10, T, 10, mix(w, INK, 0.45f));
        if (fd.kind == OUTSIDE) { rect(sx + 4, sy + 8, 4, T - 8, INK); rect(sx + 22, sy + 8, 4, T - 8, INK); rect(sx + 40, sy + 8, 4, T - 8, INK); }
        return;
    }
    rect(sx, sy, T, T, floorC);
    if (fd.kind == PARK || fd.kind == OUTSIDE) {
        if (ty >= 3 && ty <= 4 && tx % 7 == 4) rect(sx + T - 2, sy, 3, T, mix(WHITE, floorC, 0.5f));
    }
    switch (c) {
        case 'd': rect(sx, sy, T, T, mix(floorC, WHITE, 0.12f)); rect(sx, sy, T, 3, mix(fl, WHITE, 0.5f)); break;
        case 'f': {
            unsigned h = hash2(tx, ty);
            Col fc = (h & 3) == 0 ? Col{150, 90, 60} : (h & 3) == 1 ? Col{90, 120, 170} : (h & 3) == 2 ? Col{170, 70, 90} : Col{110, 150, 100};
            rect(sx + 3, sy + 6, T - 6, T - 8, INK);
            rect(sx + 6, sy + 8, T - 12, T - 18, fc);
            rect(sx + 6, sy + T - 12, T - 12, 4, mix(fc, INK, 0.4f));
            break;
        }
        case 'c': {
            unsigned h = hash2(tx / 2, ty);
            Col cc = (h & 3) == 0 ? Col{200, 60, 60} : (h & 3) == 1 ? Col{60, 120, 200} : (h & 3) == 2 ? Col{230, 200, 70} : Col{200, 200, 210};
            rect(sx, sy + 6, T, T - 10, INK);
            rect(sx + 1, sy + 8, T - 2, T - 16, cc);
            rect(sx + 8, sy + 12, T - 16, 12, mix(cc, SKY, 0.6f));
            break;
        }
        case 'm': {
            rect(sx + 2, sy + 2, T - 4, T - 4, INK);
            rect(sx + 5, sy + 5, T - 10, T - 10, {90, 100, 120});
            bool on = ((int)(g_time * 2) + tx) & 1;
            rect(sx + 10, sy + 10, 8, 8, on ? MINT : PINK);
            rect(sx + 24, sy + 10, 12, 4, INK); rect(sx + 24, sy + 18, 12, 4, INK);
            break;
        }
        case 'E': {
            rect(sx, sy, T, T, {120, 126, 140});
            rect(sx + 4, sy + 4, T / 2 - 5, T - 8, {170, 176, 190}); rect(sx + T / 2 + 1, sy + 4, T / 2 - 5, T - 8, {170, 176, 190});
            rect(sx + T / 2 - 1, sy, 2, T, INK);
            if (ty == 11) { rect(sx + T - 7, sy + 8, 4, 4, MINT); rect(sx + T - 7, sy + 16, 4, 4, ORANGE); }
            break;
        }
        case 'S': {
            float p = 0.5f + 0.5f * std::sin(g_time * 3);
            rect(sx, sy, T, T, mix({50, 20, 30}, {120, 30, 40}, p * 0.6f));
            frame(sx + 4, sy + 4, T - 8, T - 8, mix({160, 60, 60}, {255, 140, 90}, p), 3);
            rect(sx + T - 14, sy + T / 2, 5, 5, GOLD);
            break;
        }
        case 'K': rect(sx + 6, sy + 10, T - 12, T - 14, INK); rect(sx + 10, sy + 14, T - 20, 6, MINT); break;
        case 'T': {
            rect(sx + 4, sy + 4, T - 8, T - 8, INK);
            rect(sx + 8, sy + 8, T - 16, T - 22, {20, 80, 60});
            rect(sx + 12, sy + 12, 10, 3, MINT); rect(sx + 12, sy + 18, 18, 3, MINT);
            break;
        }
    }
}
static void drawWorld() {
    const FloorDef& fd = FLOORS[g_floor];
    float camX = std::max(0.0f, std::min(g_px - W / 2.0f, (float)(MW * T - W)));
    float camY = std::max(0.0f, std::min(g_py - H / 2.0f, (float)(MH * T - H)));
    int cx = (int)camX, cy = (int)camY;
    rect(0, 0, W, H, mix(fd.tint, INK, 0.8f));
    for (int ty = cy / T; ty <= (cy + H) / T; ty++)
        for (int tx = cx / T; tx <= (cx + W) / T; tx++) drawTile(g_floor, tx, ty, tx * T - cx, ty * T - cy);
    if (g_floor == F_COMMON) text(fM, "DALIA", 15 * T + T - cx, 9 * T - cy - 30, WHITE, 1);
    if (g_floor == F_COMMON) text(fS, "?", 21 * T + T / 2 - cx, 9 * T - cy - 24, GOLD, 1, (int)(120 + 100 * std::sin(g_time * 3)));
    // sorted sprites
    struct Spr { float y; int kind, idx; };
    std::vector<Spr> sp;
    for (size_t i = 0; i < NPCS.size(); i++) if (NPCS[i].floor == g_floor) sp.push_back({(NPCS[i].ty + 0.5f) * T, 1, (int)i});
    for (size_t i = 0; i < g_bots.size(); i++) if (g_bots[i].floor == g_floor && !g_bots[i].caught) sp.push_back({g_bots[i].y, 2, (int)i});
    sp.push_back({g_py, 3, 0});
    std::sort(sp.begin(), sp.end(), [](const Spr& a, const Spr& b) { return a.y < b.y; });
    for (auto& s : sp) {
        if (s.kind == 1) {
            const NpcDef& n = NPCS[s.idx];
            int x = (int)((n.tx + 0.5f) * T) - cx, y = (int)((n.ty + 0.5f) * T) - cy + 16;
            int face = (g_px < (n.tx + 0.5f) * T) ? -1 : 1;
            person(x, y, n.col, n.hair, face, 0);
            text(fS, n.name, x, y - 66, WHITE, 1);
        } else if (s.kind == 2) {
            const Bot& b = g_bots[s.idx];
            botSprite((int)b.x - cx, (int)b.y - cy + 14, PERSONS[b.pers].col, g_time, b.mg);
        } else person((int)g_px - cx, (int)g_py - cy + 14, ORANGE, {40, 24, 16}, g_face, g_walk, true);
    }
    // interaction prompt
    if (g_state == S_PLAY) {
        Target t = findTarget();
        if (t.kind) {
            int x = 0, y = 0; const char* label = "Talk";
            if (t.kind == 1) { x = (int)((NPCS[t.idx].tx + 0.5f) * T) - cx; y = (int)((NPCS[t.idx].ty + 0.5f) * T) - cy - 84; }
            else if (t.kind == 2) { x = (int)g_bots[t.idx].x - cx; y = (int)g_bots[t.idx].y - cy - 62; label = "Beep"; }
            else {
                x = (int)((t.tx + 0.5f) * T) - cx; y = (int)((t.ty + 0.5f) * T) - cy - 34;
                char c = tileAt(g_floor, t.tx, t.ty);
                label = c == 'E' ? "Elevator" : c == 'S' ? "Secret door" : "Console";
            }
            int w = textW(fS, std::string("A  ") + label) + 24;
            rect(x - w / 2, y, w, 30, INK, 220);
            frame(x - w / 2, y, w, 30, ORANGE, 2);
            text(fS, std::string("A  ") + label, x, y + 3, WHITE, 1);
        }
    }
}
static void drawHud() {
    const FloorDef& fd = FLOORS[g_floor];
    rect(16, 16, 380, 66, INK, 215);
    frame(16, 16, 380, 66, ORANGE, 3);
    text(fM, fd.name, 30, 20, WHITE);
    text(fS, fd.sub, 30, 54, mix(fd.tint, WHITE, 0.6f));
    int w = 560;
    rect(W - w - 16, 16, w, 88, INK, 215);
    frame(W - w - 16, 16, w, 88, SKY, 3);
    text(fS, "OBJECTIVE", W - w - 2, 20, SKY);
    wrapText(fS, objective(), W - w - 2, 46, w - 28, WHITE, 26);
    if (g_mg) {
        char buf[64];
        snprintf(buf, sizeof buf, "Bots %d/%d    %0.0fs", g_mgCaught, MG_BOTS, std::max(0.0f, g_mgTime));
        rect(W / 2 - 150, 116, 300, 46, INK, 225);
        frame(W / 2 - 150, 116, 300, 46, GOLD, 3);
        text(fM, buf, W / 2, 120, GOLD, 1);
    }
    if (g_toastT > 0 && !g_toast.empty()) {
        int tw = textW(fS, g_toast) + 40;
        rect(W / 2 - tw / 2, H - 62, tw, 40, INK, 230);
        frame(W / 2 - tw / 2, H - 62, tw, 40, MINT, 2);
        text(fS, g_toast, W / 2, H - 56, WHITE, 1);
    }
}
static void drawTalk() {
    const Line& l = g_talk[g_talkI];
    int bx = 80, by = H - 220, bw = W - 160, bh = 190;
    rect(bx, by, bw, bh, INK, 235);
    frame(bx, by, bw, bh, l.col, 4);
    rect(bx + 24, by - 22, textW(fM, l.who) + 36, 40, l.col);
    text(fM, l.who, bx + 42, by - 20, INK);
    size_t n = std::min((size_t)g_talkChars, l.what.size());
    wrapText(fM, l.what.substr(0, n), bx + 30, by + 28, bw - 60, WHITE, 38);
    if (n >= l.what.size()) {
        bool on = ((int)(g_time * 2)) & 1;
        if (on) text(fS, g_talkI + 1 < g_talk.size() ? "A  next" : "A  close", bx + bw - 24, by + bh - 34, l.col, 2);
    }
}
static void drawElev() {
    int pw = 760, ph = 520, px = (W - pw) / 2, py = (H - ph) / 2;
    rect(0, 0, W, H, INK, 150);
    rect(px, py, pw, ph, INK, 245);
    frame(px, py, pw, ph, ORANGE, 4);
    text(fL, "Elevator", W / 2, py + 10, WHITE, 1);
    text(fS, "UP", px + 190, py + 84, SKY, 1);
    text(fS, "DOWN", px + pw - 190, py + 84, PINK, 1);
    for (int i = 0; i < ELEV_FLOORS; i++) {
        int col = i / 8, row = i % 8;
        int x = px + 30 + col * (pw / 2), y = py + 116 + row * 48;
        bool sel = i == g_elevSel;
        if (sel) { rect(x, y, pw / 2 - 60, 42, mix(FLOORS[i].tint, INK, 0.2f)); frame(x, y, pw / 2 - 60, 42, GOLD, 3); }
        rect(x + 8, y + 9, 24, 24, FLOORS[i].tint);
        text(fS, FLOORS[i].name, x + 44, y + 7, i == g_floor ? GOLD : WHITE);
        if (i == g_floor) text(fS, "here", x + pw / 2 - 72, y + 7, GOLD, 2);
    }
    text(fS, "Up/Down choose    Left/Right switch side    A go    B cancel", W / 2, py + ph - 36, mix(WHITE, INK, 0.3f), 1);
}
static void drawTitle() {
    float t = g_time;
    for (int y = 0; y < H; y += 8) {
        float k = (float)y / H;
        rect(0, y, W, 8, mix({18, 20, 52}, {70, 30, 60}, k));
    }
    // drifting windows of the big 14
    for (int i = 0; i < 18; i++) {
        float x = std::fmod(i * 97 + t * (8 + i % 5), W + 80) - 40;
        int y = 60 + (i * 53) % 560;
        rect((int)x, y, 18, 26, mix(GOLD, INK, 0.5f + 0.4f * std::sin(t + i)), 140);
    }
    rect(0, H - 120, W, 120, INK, 200);
    if (g_logo) {
        SDL_Rect d = {110, 70, 310, 465};
        SDL_RenderCopy(g_r, g_logo, nullptr, &d);
    }
    text(fXL, "\xC3\x8Elot14", 470, 90, WHITE);
    rect(474, 200, 520, 6, ORANGE);
    text(fL, "The Big 14", 474, 214, ORANGE);
    text(fM, "A story from the Lost of Ashes universe", 474, 292, mix(WHITE, SKY, 0.5f));
    int a = 150 + (int)(105 * std::sin(t * 3));
    rect(474, 400, 340, 64, ORANGE);
    frame(474, 400, 340, 64, INK, 4);
    text(fM, "A  Start", 474 + 170, 412, INK, 1, 255);
    text(fS, "Walk: left stick / D-pad     Talk, use: A     Pause: +", 474, 490, WHITE, 0, a);
    text(fS, "Version " GAME_VERSION "     + to quit", 30, H - 44, mix(WHITE, INK, 0.2f));
}
static void drawOverlayFx() {
    for (int y = 0; y < H; y += 4) rect(0, y, W, 1, INK, 26);
    if (g_fadePhase) rect(0, 0, W, H, INK, (int)(255 * std::min(1.0f, g_fade)));
}
static void render() {
    SDL_SetRenderDrawBlendMode(g_r, SDL_BLENDMODE_BLEND);
    if (g_state == S_TITLE) drawTitle();
    else {
        drawWorld();
        drawHud();
        if (g_state == S_TALK) drawTalk();
        if (g_state == S_ELEV) drawElev();
        if (g_state == S_PAUSE) {
            rect(0, 0, W, H, INK, 170);
            rect(W / 2 - 280, 190, 560, 300, INK, 245);
            frame(W / 2 - 280, 190, 560, 300, ORANGE, 4);
            text(fL, "Paused", W / 2, 205, WHITE, 1);
            text(fM, "A  Resume", W / 2, 300, WHITE, 1);
            text(fM, "X  Back to title", W / 2, 350, WHITE, 1);
            text(fM, "Y  Quit game", W / 2, 400, WHITE, 1);
        }
        if (g_state == S_CARD) {
            rect(0, 0, W, H, INK, 235);
            text(fXL, g_card1, W / 2, 200, ORANGE, 1);
            wrapText(fM, g_card2, W / 2 - 400, 340, 800, WHITE, 40);
            text(fS, "A  continue", W / 2, 520, GOLD, 1);
        }
    }
    drawOverlayFx();
    SDL_RenderPresent(g_r);
}

// ------------------------------------------------------------------ init / main
#ifdef __SWITCH__
static PlFontData g_fd;
static TTF_Font* openFont(int sz) { return TTF_OpenFontRW(SDL_RWFromMem(g_fd.address, g_fd.size), 0, sz); }
#else
static TTF_Font* openFont(int sz) { return TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", sz); }
#endif
static bool gameInit() {
    g_win = SDL_CreateWindow("Ilot14", 0, 0, W, H, SDL_WINDOW_SHOWN);
    if (!g_win) return false;
    g_r = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!g_r) g_r = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_SOFTWARE);
    if (!g_r) return false;
    TTF_Init();
    fS = openFont(22); fM = openFont(28); fL = openFont(54); fXL = openFont(100);
    if (!fS || !fM || !fL || !fXL) return false;
    for (int i = 0; i < NF; i++) buildMap(i);
    // title logo
    std::vector<unsigned char> px((size_t)LOGO_BIG.rawlen);
    mz_ulong n = LOGO_BIG.rawlen;
    if (mz_uncompress(px.data(), &n, LOGO_BIG.z, LOGO_BIG.zlen) == MZ_OK) {
        g_logo = SDL_CreateTexture(g_r, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, LOGO_BIG.w, LOGO_BIG.h);
        SDL_UpdateTexture(g_logo, nullptr, px.data(), LOGO_BIG.w * 4);
        SDL_SetTextureBlendMode(g_logo, SDL_BLENDMODE_BLEND);
        g_logoW = LOGO_BIG.w; g_logoH = LOGO_BIG.h;
    }
    placeBots();
    return true;
}

#ifndef GAME_NO_MAIN
int main(int, char**) {
#ifdef __SWITCH__
    plInitialize(PlServiceType_User);
    plGetSharedFontByType(&g_fd, PlSharedFontType_Standard);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);
#endif
    SDL_Init(SDL_INIT_VIDEO);
    if (!gameInit()) return 1;
#ifdef __SWITCH__
    u64 last = armGetSystemTick();
    while (appletMainLoop() && !g_quit) {
        u64 now = armGetSystemTick();
        float dt = (float)(now - last) / (float)armGetSystemTickFreq();
        last = now;
        if (dt > 0.05f) dt = 0.05f;
        padUpdate(&pad);
        u64 d = padGetButtonsDown(&pad), h = padGetButtons(&pad);
        In in;
        if (d & HidNpadButton_A) in.down |= K_A;
        if (d & HidNpadButton_B) in.down |= K_B;
        if (d & HidNpadButton_X) in.down |= K_X;
        if (d & HidNpadButton_Y) in.down |= K_Y;
        if (d & (HidNpadButton_Up | HidNpadButton_StickLUp)) in.down |= K_UP;
        if (d & (HidNpadButton_Down | HidNpadButton_StickLDown)) in.down |= K_DOWN;
        if (d & (HidNpadButton_Left | HidNpadButton_StickLLeft)) in.down |= K_LEFT;
        if (d & (HidNpadButton_Right | HidNpadButton_StickLRight)) in.down |= K_RIGHT;
        if (d & HidNpadButton_Plus) in.down |= K_PLUS;
        HidAnalogStickState st = padGetStickPos(&pad, 0);
        float sx = st.x / 32767.0f, sy = -st.y / 32767.0f;
        if (std::sqrt(sx * sx + sy * sy) < 0.25f) sx = sy = 0;
        if (h & HidNpadButton_Left) sx = -1;
        if (h & HidNpadButton_Right) sx = 1;
        if (h & HidNpadButton_Up) sy = -1;
        if (h & HidNpadButton_Down) sy = 1;
        in.ax = sx; in.ay = sy;
        update(dt, in);
        render();
    }
    plExit();
#else
    (void)0;
#endif
    SDL_Quit();
    return 0;
}
#endif
