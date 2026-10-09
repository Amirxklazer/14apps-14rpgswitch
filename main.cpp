// Ilot14: The Big 14 - a story game for Nintendo Switch (homebrew NRO)
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <sys/stat.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include "logo_data.h"
#include "miniz.h"
#ifdef __SWITCH__
#include <switch.h>
#endif

#define GAME_VERSION "0.2.0"
static const int W = 1280, H = 720, T = 48, MW = 44, MH = 24;
static const int LAST_DAY = 31;

// ------------------------------------------------------------------ colors
struct Col { int r, g, b; };
static Col mix(Col a, Col b, float t) { return {(int)(a.r + (b.r - a.r) * t), (int)(a.g + (b.g - a.g) * t), (int)(a.b + (b.b - a.b) * t)}; }
static const Col ORANGE = {249, 115, 22}, INK = {12, 10, 18}, WHITE = {244, 244, 250}, SKY = {90, 190, 255}, MINT = {80, 220, 160},
                 PINK = {255, 110, 170}, GOLD = {255, 205, 70}, BLUEP = {0, 138, 230}, RED = {230, 70, 70};

// ------------------------------------------------------------------ world data
enum Kind { RES, PARK, UTIL, OUTSIDE, YARD, BACK, ROOF };
struct FloorDef { const char* key; const char* name; const char* sub; Col tint; Kind kind; };
static const FloorDef FLOORS[] = {
    {"lobby", "Lobby", "L group. Home sweet home.", {40, 64, 112}, RES},
    {"common", "Common", "C group. Where everyone hangs out.", {40, 110, 96}, RES},
    {"uncommon", "Uncommon", "UC and JS group. A little less common.", {90, 70, 130}, RES},
    {"rare", "Rare", "R group. Quiet. Suspiciously quiet.", {130, 60, 96}, RES},
    {"special", "Special", "S group. Fancy carpets.", {140, 110, 40}, RES},
    {"main", "Main", "M group. The heart of the 14.", {170, 84, 36}, RES},
    {"lethal", "Lethal", "NST group. Spicy food. Be careful.", {150, 40, 52}, RES},
    {"teen", "Teen", "T group. Enter with snacks only.", {40, 120, 160}, RES},
    {"underlobby", "Underlobby", "Right below the lobby", {44, 52, 84}, RES},
    {"parking1", "Parking 1", "Underground", {70, 76, 90}, PARK},
    {"parking2", "Parking 2", "Underground", {60, 64, 78}, PARK},
    {"shaft", "Elevator shaft fix", "Hard hats required", {128, 112, 40}, UTIL},
    {"control", "Elevator control room", "Do not touch the big red button", {112, 56, 56}, UTIL},
    {"vent", "Air ventilation systems", "Windy. Very windy.", {44, 110, 124}, UTIL},
    {"botsparking", "14 Cleaning bots parking", "Charging docks", {40, 120, 84}, PARK},
    {"inn", "Further inn", "Holiday Inn branded rooms", {90, 72, 140}, RES},
    {"outside", "Parking 1 (outside)", "On the ground. Fresh air.", {84, 92, 96}, OUTSIDE},
    {"yard", "Front yard", "Right outside the big 14", {70, 130, 70}, YARD},
    {"ilot22", "\xC3\x8Elot22", "Amine's neighborhood", {80, 140, 80}, YARD},
    {"road", "Main road", "Ice cream and traffic", {110, 110, 120}, YARD},
    {"backrooms", "The Backrooms", "Yellow. Humming. Endless.", {200, 175, 70}, BACK},
    {"roof", "Rooftop", "The whole city is out here", {70, 80, 120}, ROOF},
};
static const int NF = sizeof(FLOORS) / sizeof(FLOORS[0]);
enum { F_LOBBY = 0, F_MAIN = 5, F_BOTS = 14, F_OUT = 16, F_YARD = 17, F_ILOT = 18, F_ROAD = 19, F_BACK = 20, F_ROOF = 21 };
static const int ELEV_FLOORS = 16;

struct Portal { int from, tx, ty, to; float ttx, tty; const char* label; };
static std::vector<Portal> g_portals = {
    {5, 21, 9, 16, 2.5f, 11.5f, "Secret door"},
    {16, 0, 11, 5, 21.5f, 10.5f, "Secret door"}, {16, 0, 12, 5, 21.5f, 10.5f, "Secret door"},
    {0, 43, 11, 17, 2.5f, 11.5f, "Front door"}, {0, 43, 12, 17, 2.5f, 11.5f, "Front door"},
    {17, 0, 11, 0, 41.5f, 11.5f, "Back inside"}, {17, 0, 12, 0, 41.5f, 11.5f, "Back inside"},
    {17, 43, 11, 18, 2.5f, 11.5f, "To \xC3\x8Elot22"}, {17, 43, 12, 18, 2.5f, 11.5f, "To \xC3\x8Elot22"},
    {17, 21, 23, 19, 21.5f, 2.5f, "To the main road"}, {17, 22, 23, 19, 21.5f, 2.5f, "To the main road"},
    {18, 0, 11, 17, 41.5f, 11.5f, "To the 14"}, {18, 0, 12, 17, 41.5f, 11.5f, "To the 14"},
    {19, 21, 0, 17, 21.5f, 21.5f, "To the 14"}, {19, 22, 0, 17, 21.5f, 21.5f, "To the 14"},
};

struct NpcDef { const char* key; const char* disp; const char* id; int floor, tx, ty; Col col, hair; };
static const std::vector<NpcDef> NPCS = {
    {"Raceem", "Raceem", "L-002", 0, 5, 4, SKY, {20, 20, 30}},
    {"AdamL3", "Adam", "L-003", 0, 15, 4, ORANGE, {30, 24, 20}},
    {"AdamL7", "Adam", "L-007", 0, 25, 4, MINT, {50, 36, 24}},
    {"AmirL4", "Amir", "L-004", 0, 35, 4, {170, 120, 230}, {30, 20, 20}},
    {"Aya", "Aya", "L-005", 0, 15, 18, PINK, {40, 24, 20}},
    {"Basmala", "Basmala", "L-006", 0, 25, 18, MINT, {50, 30, 40}},
    {"Fares", "Fares", "OL-007", 0, 35, 18, GOLD, {30, 20, 20}},
    {"Anes", "Anes", "C-001", 1, 5, 4, PINK, {30, 20, 24}},
    {"Iyad", "Iyad", "C-005", 1, 15, 4, SKY, {20, 20, 30}},
    {"Alicia", "Alicia", "UC-001", 2, 5, 4, PINK, {50, 30, 30}},
    {"Abdullah", "Abdullah", "UC-002", 2, 15, 4, ORANGE, {30, 24, 20}},
    {"Jalil", "Jalil", "UC-003", 2, 25, 4, MINT, {30, 20, 20}},
    {"AdamUC", "Adam", "UC-004", 2, 35, 4, SKY, {26, 20, 20}},
    {"Amira", "Amira", "UC-005", 2, 5, 18, GOLD, {30, 20, 20}},
    {"Anais", "Anais", "R-001", 3, 5, 4, {200, 120, 200}, {60, 30, 30}},
    {"Maysanne", "Maysanne", "R-002", 3, 15, 4, MINT, {40, 24, 20}},
    {"Jilali", "Jilali", "R-003", 3, 25, 4, GOLD, {30, 24, 20}},
    {"Kenzy", "Kenzy", "S-002", 4, 5, 4, ORANGE, {24, 20, 20}},
    {"Arwa", "Arwa", "M-001", 5, 5, 4, PINK, {30, 20, 20}},
    {"Dalia", "Dalia", "M-002", 5, 15, 4, MINT, {60, 30, 24}},
    {"Dania", "Dania", "M-003", 5, 25, 4, GOLD, {80, 50, 30}},
    {"Gabriel", "Gabriel", "M-004", 5, 35, 4, SKY, {20, 20, 28}},
    {"Youcef", "Youcef", "M-005", 5, 5, 18, {120, 200, 120}, {24, 20, 24}},
    {"AyaN", "Aya", "NST-001", 6, 5, 4, PINK, {30, 20, 30}},
    {"AdamNST", "Adam Tansaouti", "NST-002", 6, 15, 4, {200, 160, 90}, {30, 20, 20}},
    {"Sofia", "Sofia", "NST-003", 6, 25, 4, PINK, {40, 24, 24}},
    {"AdamT2", "Adam", "T-002", 7, 15, 4, {230, 90, 90}, {30, 22, 20}},
    {"Lilian", "Lilian", "T-003", 7, 25, 4, SKY, {60, 40, 30}},
    {"Amine", "Amine", "S-001", 18, 20, 11, {240, 200, 80}, {30, 24, 20}},
    {"Amyas", "Amyas", "T-001", 19, 35, 9, {200, 90, 60}, {26, 20, 20}},
};
static const NpcDef* findNpc(const std::string& k) {
    for (auto& n : NPCS) if (k == n.key) return &n;
    return nullptr;
}

struct Pers { const char* name; Col col; std::vector<const char*> lines; };
static const std::vector<Pers> PERSONS = {
    {"Grumpy", {200, 90, 90}, {"Bzzt. I clean. I do not enjoy it.", "Do not step on my floor.", "Another day, another dust bunny. Ugh."}},
    {"Dramatic", {190, 110, 230}, {"Oh, the DUST! The endless, endless dust!", "I was born to sweep, and sweep I shall... dramatically.", "Nobody understands the burden of the mop!"}},
    {"Shy", {120, 200, 230}, {"...h-hi. Sorry. I will clean somewhere else.", "Please do not look at me while I vacuum.", "Beep. (Quietly.)"}},
    {"Overconfident", {250, 200, 60}, {"I am the best cleaning bot in the 14. Easily.", "Spotless. As always. Because of me.", "Cleaning is just winning, but with wheels."}},
    {"Sleepy", {150, 160, 230}, {"zzz... oh. Hi. Was I cleaning?", "Five more minutes... of mopping...", "Charging dock... pleeease..."}},
    {"Poetic", {120, 230, 170}, {"Dust falls like quiet snow. I rise to meet it.", "O floor, so wide and shining...", "A single crumb. A story untold."}},
};

// ------------------------------------------------------------------ story script (see story_*.h)
struct Item {
    enum { LINE, CHOICE, CMD } t = LINE;
    std::string a, b;                                      // LINE: who,text   CMD: command line
    std::vector<std::pair<std::string, std::string>> opts;  // CHOICE: text,target
};
enum TK { TK_TALK, TK_GO, TK_CUT, TK_MINI, TK_PICK, TK_WARP };
struct Task {
    TK kind = TK_TALK;
    std::string npc, node, obj, item, mtype, ma, mb, mc, winNode;
    int floor = 0, tx = 0, ty = 0, group = 0;
    bool done = false;
};
struct DayDef {
    int n = 0;
    std::string title;
    std::vector<std::string> intro, outro;
    int sf = 0, sx = 5, sy = 18;
    std::vector<Task> tasks;
    std::map<std::string, std::string> daily;
};
struct Note { int floor, tx, ty; std::string text; };
static std::map<std::string, std::vector<Item>> NODES;
static std::vector<DayDef> DAYS;
static std::vector<Note> NOTES;
static std::vector<std::string> g_scriptErrors;

#include "story_a.h"
#include "story_b.h"
#include "story_c.h"
#include "story_d.h"

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
    return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}
static std::vector<std::string> splitBar(const std::string& s) {
    std::vector<std::string> v;
    std::string cur;
    for (char c : s) {
        if (c == '|') { v.push_back(trim(cur)); cur.clear(); }
        else cur += c;
    }
    v.push_back(trim(cur));
    return v;
}
static std::vector<std::string> splitWs(const std::string& s) {
    std::vector<std::string> v;
    std::string cur;
    for (char c : s) {
        if (c == ' ' || c == '\t') { if (!cur.empty()) v.push_back(cur); cur.clear(); }
        else cur += c;
    }
    if (!cur.empty()) v.push_back(cur);
    return v;
}
static int floorId(const std::string& k) {
    for (int i = 0; i < NF; i++) if (k == FLOORS[i].key) return i;
    return -1;
}
static void scriptErr(const std::string& s) { g_scriptErrors.push_back(s); }

static void parseScript(const char* text) {
    std::string src = text;
    std::vector<Item>* node = nullptr;
    DayDef* day = nullptr;
    int groupCounter = 0, parGroup = -1;
    bool inPar = false;
    size_t pos = 0;
    int lineNo = 0;
    while (pos <= src.size()) {
        size_t e = src.find('\n', pos);
        if (e == std::string::npos) e = src.size();
        std::string line = trim(src.substr(pos, e - pos));
        pos = e + 1;
        lineNo++;
        if (line.empty() || line[0] == '#') continue;
        char c0 = line[0];
        if (c0 == '=') {
            std::string name = trim(line.substr(1));
            node = &NODES[name];
            node->clear();
            continue;
        }
        if (c0 == '@') {
            size_t sp = line.find(' ');
            std::string cmd = line.substr(1, sp == std::string::npos ? std::string::npos : sp - 1);
            std::string rest = sp == std::string::npos ? "" : trim(line.substr(sp + 1));
            node = nullptr;
            if (cmd == "day") {
                auto p = splitBar(rest);
                DAYS.push_back(DayDef());
                day = &DAYS.back();
                day->n = atoi(p[0].c_str());
                day->title = p.size() > 1 ? p[1] : "";
                inPar = false;
            } else if (cmd == "start" && day) {
                auto p = splitBar(rest);
                day->sf = floorId(p[0]);
                if (day->sf < 0) { scriptErr("bad start floor: " + rest); day->sf = 0; }
                if (p.size() > 1) { auto t = splitWs(p[1]); if (t.size() >= 2) { day->sx = atoi(t[0].c_str()); day->sy = atoi(t[1].c_str()); } }
            } else if (cmd == "intro" && day) day->intro.push_back(rest);
            else if (cmd == "outro" && day) day->outro.push_back(rest);
            else if (cmd == "daily" && day) {
                auto p = splitBar(rest);
                if (p.size() >= 2) day->daily[p[0]] = p[1];
            } else if (cmd == "note") {
                auto p = splitBar(rest);
                if (p.size() >= 3) {
                    Note n;
                    n.floor = floorId(p[0]);
                    auto t = splitWs(p[1]);
                    n.tx = t.size() > 0 ? atoi(t[0].c_str()) : 0;
                    n.ty = t.size() > 1 ? atoi(t[1].c_str()) : 0;
                    n.text = p[2];
                    if (n.floor < 0) scriptErr("bad note floor: " + rest); else NOTES.push_back(n);
                }
            } else if (cmd == "chat") {
                auto p = splitBar(rest);
                if (p.size() < 4) { scriptErr("bad chat: " + rest); continue; }
                const std::string key = p[0];
                std::vector<Item>& top = NODES["chat_" + key];
                top.clear();
                Item g; g.t = Item::LINE; g.a = key; g.b = p[1];
                top.push_back(g);
                Item ch; ch.t = Item::CHOICE;
                for (size_t i = 2; i + 1 < p.size(); i += 2) {
                    std::string sub = "chat_" + key + "_" + std::to_string(i / 2);
                    ch.opts.push_back({p[i], sub});
                    std::vector<Item>& a = NODES[sub];
                    a.clear();
                    Item l; l.t = Item::LINE; l.a = key; l.b = p[i + 1];
                    a.push_back(l);
                    Item gt; gt.t = Item::CMD; gt.a = "goto chat_" + key;
                    a.push_back(gt);
                }
                ch.opts.push_back({"Bye!", "chat_end"});
                top.push_back(ch);
            } else scriptErr("unknown directive: " + line);
            continue;
        }
        if (c0 == '>') {
            if (!day) { scriptErr("task outside a day: " + line); continue; }
            std::string rest = trim(line.substr(1));
            if (rest == "par") { inPar = true; parGroup = ++groupCounter; continue; }
            if (rest == "endpar") { inPar = false; continue; }
            size_t sp = rest.find(' ');
            std::string kind = rest.substr(0, sp);
            auto p = splitBar(sp == std::string::npos ? "" : rest.substr(sp + 1));
            Task t;
            t.group = inPar ? parGroup : ++groupCounter;
            if (kind == "talk") {  // talk Npc | floor tx ty (or home) | node | objective
                t.kind = TK_TALK;
                t.npc = p[0];
                const NpcDef* n = findNpc(t.npc);
                if (!n) scriptErr("unknown npc: " + t.npc);
                auto q = splitWs(p.size() > 1 ? p[1] : "home");
                if (q[0] == "home" && n) { t.floor = n->floor; t.tx = n->tx; t.ty = n->ty; }
                else { t.floor = floorId(q[0]); t.tx = q.size() > 1 ? atoi(q[1].c_str()) : 0; t.ty = q.size() > 2 ? atoi(q[2].c_str()) : 0; }
                t.node = p.size() > 2 ? p[2] : "";
                t.obj = p.size() > 3 ? p[3] : "";
            } else if (kind == "go") {  // go floor | objective
                t.kind = TK_GO;
                t.floor = floorId(p[0]);
                t.obj = p.size() > 1 ? p[1] : "";
            } else if (kind == "cut") {  // cut node | objective
                t.kind = TK_CUT;
                t.node = p[0];
                t.obj = p.size() > 1 ? p[1] : "";
            } else if (kind == "mini") {  // mini type a b c | floor tx ty | objective | winNode
                t.kind = TK_MINI;
                auto m = splitWs(p[0]);
                t.mtype = m.size() > 0 ? m[0] : "";
                t.ma = m.size() > 1 ? m[1] : "";
                t.mb = m.size() > 2 ? m[2] : "";
                t.mc = m.size() > 3 ? m[3] : "";
                auto q = splitWs(p.size() > 1 ? p[1] : "");
                t.floor = floorId(q.size() ? q[0] : "");
                t.tx = q.size() > 1 ? atoi(q[1].c_str()) : 0;
                t.ty = q.size() > 2 ? atoi(q[2].c_str()) : 0;
                t.obj = p.size() > 2 ? p[2] : "";
                t.winNode = p.size() > 3 ? p[3] : "";
            } else if (kind == "pick") {  // pick Item | floor tx ty | objective
                t.kind = TK_PICK;
                t.item = p[0];
                auto q = splitWs(p.size() > 1 ? p[1] : "");
                t.floor = floorId(q.size() ? q[0] : "");
                t.tx = q.size() > 1 ? atoi(q[1].c_str()) : 0;
                t.ty = q.size() > 2 ? atoi(q[2].c_str()) : 0;
                t.obj = p.size() > 2 ? p[2] : "";
            } else if (kind == "warp") {  // warp floor tx ty
                t.kind = TK_WARP;
                auto q = splitWs(p[0]);
                t.floor = floorId(q.size() ? q[0] : "");
                t.tx = q.size() > 1 ? atoi(q[1].c_str()) : 0;
                t.ty = q.size() > 2 ? atoi(q[2].c_str()) : 0;
            } else { scriptErr("unknown task: " + line); continue; }
            if (t.floor < 0) scriptErr("bad floor in task: " + line);
            day->tasks.push_back(t);
            continue;
        }
        if (!node) { scriptErr("line outside a node: " + line); continue; }
        if (c0 == '?') {
            size_t ar = line.find("->");
            if (ar == std::string::npos) { scriptErr("bad choice: " + line); continue; }
            std::string txt = trim(line.substr(1, ar - 1)), tgt = trim(line.substr(ar + 2));
            if (node->empty() || node->back().t != Item::CHOICE) { Item it; it.t = Item::CHOICE; node->push_back(it); }
            node->back().opts.push_back({txt, tgt});
        } else if (c0 == '!') {
            Item it; it.t = Item::CMD; it.a = trim(line.substr(1));
            node->push_back(it);
        } else if (c0 == '*') {
            Item it; it.t = Item::LINE; it.a = ""; it.b = trim(line.substr(1));
            node->push_back(it);
        } else {
            size_t col = line.find(": ");
            if (col == std::string::npos) { scriptErr("bad line: " + line); continue; }
            Item it; it.t = Item::LINE; it.a = trim(line.substr(0, col)); it.b = trim(line.substr(col + 2));
            node->push_back(it);
        }
    }
}
static void loadStory() {
    NODES.clear(); DAYS.clear(); NOTES.clear(); g_scriptErrors.clear();
    NODES["chat_end"];
    parseScript(STORY_A);
    parseScript(STORY_B);
    parseScript(STORY_C);
    parseScript(STORY_D);
    std::sort(DAYS.begin(), DAYS.end(), [](const DayDef& a, const DayDef& b) { return a.n < b.n; });
}

// ------------------------------------------------------------------ input
enum { K_A = 1, K_B = 2, K_X = 4, K_Y = 8, K_UP = 16, K_DOWN = 32, K_LEFT = 64, K_RIGHT = 128, K_PLUS = 256 };
struct In {
    unsigned down = 0;     // pressed this frame
    float ax = 0, ay = 0;  // analog / dpad movement
};

// ------------------------------------------------------------------ rendering helpers
static SDL_Window* g_win = nullptr;
static SDL_Renderer* g_r = nullptr;
static TTF_Font *fS, *fM, *fL, *fXL;
static SDL_Texture* g_logo = nullptr;

static void setc(Col c, int a = 255) { SDL_SetRenderDrawColor(g_r, c.r, c.g, c.b, a); }
static void rect(int x, int y, int w, int h, Col c, int a = 255) {
    setc(c, a);
    SDL_Rect r = {x, y, w, h};
    SDL_RenderFillRect(g_r, &r);
}
static void frame(int x, int y, int w, int h, Col c, int th = 2) {
    rect(x, y, w, th, c); rect(x, y + h - th, w, th, c); rect(x, y, th, h, c); rect(x + w - th, y, th, h, c);
}
static void circleO(int cx, int cy, int rad, Col c, int th = 2) {
    setc(c);
    for (int t = 0; t < th; t++)
        for (int a = 0; a < 360; a += 2) {
            float rr = rad - t;
            SDL_RenderDrawPoint(g_r, cx + (int)(std::cos(a * 0.0174533f) * rr), cy + (int)(std::sin(a * 0.0174533f) * rr));
        }
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
static std::vector<std::string> utf8chars(const std::string& s) {
    std::vector<std::string> v;
    for (size_t i = 0; i < s.size();) {
        size_t n = 1;
        unsigned char c = (unsigned char)s[i];
        if (c >= 0xF0) n = 4; else if (c >= 0xE0) n = 3; else if (c >= 0xC0) n = 2;
        v.push_back(s.substr(i, n));
        i += n;
    }
    return v;
}
// letter-spaced text (like the blueprint menu)
static void textSp(TTF_Font* f, const std::string& s, int cx, int y, Col c, int spacing) {
    auto ch = utf8chars(s);
    int total = 0;
    for (auto& g : ch) total += textW(f, g) + spacing;
    total -= spacing;
    int x = cx - total / 2;
    for (auto& g : ch) { text(f, g, x, y, c); x += textW(f, g) + spacing; }
}
static int wrapText(TTF_Font* f, const std::string& s, int x, int y, int maxw, Col c, int lh, int align = 0) {
    std::string line, word;
    int n = 0;
    for (size_t i = 0; i <= s.size(); i++) {
        if (i == s.size() || s[i] == ' ') {
            std::string t = line.empty() ? word : line + " " + word;
            if (!line.empty() && textW(f, t) > maxw) { text(f, line, align ? x + maxw / 2 : x, y + n * lh, c, align); n++; line = word; }
            else line = t;
            word.clear();
        } else word += s[i];
    }
    if (!line.empty()) { text(f, line, align ? x + maxw / 2 : x, y + n * lh, c, align); n++; }
    return n;
}
static void dashH(int x1, int x2, int y, Col c, int a = 255) { for (int x = x1; x < x2; x += 10) rect(x, y, 5, 2, c, a); }
static void dashV(int x, int y1, int y2, Col c, int a = 255) { for (int y = y1; y < y2; y += 10) rect(x, y, 2, 5, c, a); }

// ------------------------------------------------------------------ settings / saves
static int g_textSpeed = 1;  // 0 slow 1 normal 2 fast
static bool g_scan = true;
static int g_slot = -1;
static std::map<std::string, int> g_flags;
static int g_day = 1;
static std::string g_saveDir = "";

static std::string savePath(const std::string& f) { return g_saveDir + f; }
static void saveSettings() {
    FILE* f = fopen(savePath("settings.dat").c_str(), "w");
    if (!f) return;
    fprintf(f, "textspeed=%d\nscan=%d\n", g_textSpeed, g_scan ? 1 : 0);
    fclose(f);
}
static void loadSettings() {
    FILE* f = fopen(savePath("settings.dat").c_str(), "r");
    if (!f) return;
    char k[64]; int v;
    while (fscanf(f, "%63[^=]=%d\n", k, &v) == 2) {
        if (!strcmp(k, "textspeed")) g_textSpeed = std::max(0, std::min(2, v));
        if (!strcmp(k, "scan")) g_scan = v != 0;
    }
    fclose(f);
}
struct SlotInfo { bool used = false; int day = 1; };
static SlotInfo slotInfo(int s) {
    SlotInfo si;
    FILE* f = fopen(savePath("save" + std::to_string(s + 1) + ".dat").c_str(), "r");
    if (!f) return si;
    char line[256];
    while (fgets(line, sizeof line, f)) {
        int d;
        if (sscanf(line, "day=%d", &d) == 1) { si.used = true; si.day = d; }
    }
    fclose(f);
    return si;
}
static void saveSlot(int s) {
    if (s < 0) return;
    FILE* f = fopen(savePath("save" + std::to_string(s + 1) + ".dat").c_str(), "w");
    if (!f) return;
    fprintf(f, "day=%d\n", g_day);
    for (auto& kv : g_flags) fprintf(f, "flag %s %d\n", kv.first.c_str(), kv.second);
    fclose(f);
}
static bool loadSlot(int s) {
    FILE* f = fopen(savePath("save" + std::to_string(s + 1) + ".dat").c_str(), "r");
    if (!f) return false;
    g_flags.clear();
    char line[256];
    while (fgets(line, sizeof line, f)) {
        int d; char k[128]; int v;
        if (sscanf(line, "day=%d", &d) == 1) g_day = d;
        else if (sscanf(line, "flag %127s %d", k, &v) == 2) g_flags[k] = v;
    }
    fclose(f);
    return true;
}
static void eraseSlot(int s) { remove(savePath("save" + std::to_string(s + 1) + ".dat").c_str()); }

// ------------------------------------------------------------------ maps
struct Map { std::vector<std::string> t; };
static Map g_maps[32];
static unsigned hash2(int a, int b) { unsigned h = (unsigned)a * 374761393u + (unsigned)b * 668265263u; h = (h ^ (h >> 13)) * 1274126177u; return h ^ (h >> 16); }
static unsigned g_rng = 12345;
static float frand() { g_rng = g_rng * 1664525u + 1013904223u; return (g_rng >> 8) / 16777216.0f; }

static void carve(Map& m, int x0, int y0, int x1, int y1, char c) {
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) m.t[y][x] = c;
}
static void buildBackrooms(Map& m) {
    // 14 x 7 cells, each cell is a 2x2 floor block with 1-tile walls between cells
    const int CW = 14, CH = 7;
    m.t.assign(MH, std::string(MW, '#'));
    std::vector<char> seen(CW * CH, 0);
    std::vector<int> stack = {0};
    seen[0] = 1;
    unsigned s = 777;
    auto rnd = [&]() { s = s * 1664525u + 1013904223u; return (s >> 8); };
    for (int cy = 0; cy < CH; cy++) for (int cx = 0; cx < CW; cx++) carve(m, 1 + cx * 3, 1 + cy * 3, 2 + cx * 3, 2 + cy * 3, '.');
    while (!stack.empty()) {
        int c = stack.back(), cx = c % CW, cy = c / CW;
        int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        std::vector<int> opts;
        for (int d = 0; d < 4; d++) {
            int nx = cx + dirs[d][0], ny = cy + dirs[d][1];
            if (nx >= 0 && ny >= 0 && nx < CW && ny < CH && !seen[ny * CW + nx]) opts.push_back(d);
        }
        if (opts.empty()) { stack.pop_back(); continue; }
        int d = opts[rnd() % opts.size()];
        int nx = cx + dirs[d][0], ny = cy + dirs[d][1];
        // knock out the wall between the two cells
        if (d == 0) carve(m, 3 + cx * 3, 1 + cy * 3, 3 + cx * 3, 2 + cy * 3, '.');
        if (d == 1) carve(m, 3 + nx * 3, 1 + cy * 3, 3 + nx * 3, 2 + cy * 3, '.');
        if (d == 2) carve(m, 1 + cx * 3, 3 + cy * 3, 2 + cx * 3, 3 + cy * 3, '.');
        if (d == 3) carve(m, 1 + cx * 3, 3 + ny * 3, 2 + cx * 3, 3 + ny * 3, '.');
        seen[ny * CW + nx] = 1;
        stack.push_back(ny * CW + nx);
    }
    // loops: remove a few extra walls so it is not a perfect tree
    for (int i = 0; i < 18; i++) {
        int cx = rnd() % (CW - 1), cy = rnd() % CH;
        carve(m, 3 + cx * 3, 1 + cy * 3, 3 + cx * 3, 2 + cy * 3, '.');
    }
    m.t[19][42] = 'P';  // exit door next to the last cell
    m.t[20][42] = 'P';
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
            for (int row = 0; row < 2; row++) {
                int y0 = row ? 15 : 1, y1 = row ? 22 : 8;
                m.t[y0][rx[k]] = 'f'; m.t[y0][rx[k] + 1] = 'f'; m.t[y1][rx[k] + 7] = 'f'; m.t[y1][rx[k] + 6] = 'f';
                if (hash2(fi, k * 2 + row) & 1) m.t[y1][rx[k]] = 'f';
            }
        }
        m.t[11][0] = 'E'; m.t[12][0] = 'E';
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
        if (fd.kind == PARK) { m.t[11][0] = 'E'; m.t[12][0] = 'E'; }
    } else if (fd.kind == UTIL) {
        carve(m, 1, 1, MW - 2, MH - 2, '.');
        for (int x = 5; x < MW - 4; x += 9) for (int y = 3; y < MH - 3; y += 4) {
            if (y >= 9 && y <= 14) continue;
            m.t[y][x] = 'm'; m.t[y][x + 1] = 'm'; m.t[y + 1][x] = 'm'; m.t[y + 1][x + 1] = 'm';
        }
        m.t[11][0] = 'E'; m.t[12][0] = 'E';
    } else if (fd.kind == YARD) {
        carve(m, 1, 1, MW - 2, MH - 2, '.');
        for (int y = 1; y < MH - 1; y++) for (int x = 1; x < MW - 1; x++) {
            bool path = (y >= 10 && y <= 13) || (x >= 19 && x <= 24);
            bool nearPortal = (x <= 4 || x >= MW - 5) && y >= 9 && y <= 14;
            if (path || nearPortal) continue;
            if (hash2(x + fi * 100, y) % 13 == 0) m.t[y][x] = 't';
        }
        if (fi == F_ILOT) {  // a big tree with a hornet nest
            for (int y = 2; y <= 8; y++) for (int x = 33; x <= 40; x++) m.t[y][x] = '.';
            m.t[4][36] = 'H'; m.t[4][37] = 't'; m.t[3][36] = 't'; m.t[3][37] = 't';
            carve(m, 12, 1, 26, 6, 's');  // Amine's block
            m.t[6][19] = 'd'; m.t[6][20] = 'd';
        }
        if (fi == F_ROAD) {
            carve(m, 1, 10, MW - 2, 13, '.');
            carve(m, 28, 1, 41, 8, 's');  // ice cream shop
            m.t[8][35] = 'd';
            for (int y = 1; y < MH - 1; y++) for (int x = 1; x < MW - 1; x++) if (m.t[y][x] == 't' && x >= 27 && y <= 9) m.t[y][x] = '.';
        }
        if (fi == F_YARD) {
            for (int x = 8; x < 16; x += 3) m.t[8][x] = 'b';
            for (int x = 28; x < 38; x += 3) m.t[15][x] = 'b';
        }
    } else if (fd.kind == ROOF) {
        carve(m, 1, 1, MW - 2, MH - 2, '.');
        for (int x = 6; x < MW - 4; x += 10) for (int y : {4, 18}) { m.t[y][x] = 'm'; m.t[y][x + 1] = 'm'; }
        m.t[11][0] = 'E'; m.t[12][0] = 'E';
    } else if (fd.kind == BACK) {
        buildBackrooms(m);
    }
    for (auto& p : g_portals) if (p.from == fi) m.t[p.ty][p.tx] = 'P';
}
static char tileAt(int fi, int tx, int ty) {
    if (tx < 0 || ty < 0 || tx >= MW || ty >= MH) return '#';
    return g_maps[fi].t[ty][tx];
}
static bool solidC(char c) {
    return c == '#' || c == 'f' || c == 'c' || c == 'm' || c == 'E' || c == 'P' || c == 'K' || c == 'T' || c == 't' || c == 'H' || c == 's' || c == 'b';
}
static const Portal* findPortal(int fi, int tx, int ty) {
    for (auto& p : g_portals) if (p.from == fi && p.tx == tx && p.ty == ty) return &p;
    return nullptr;
}

// ------------------------------------------------------------------ game state
enum State { S_TITLE, S_MENU_SLOTS, S_MENU_SETTINGS, S_PLAY, S_TALK, S_CHOICE, S_ELEV, S_PAUSE, S_CARD, S_MINI };
struct Bot { float x, y, vx = 0, vy = 0; int pers = 0; int floor = 0; float timer = 0; bool caught = false, mg = false; };
struct Line { std::string who, what; Col col; };
struct Place { int floor, tx, ty; };
struct Card { std::string title, sub; std::vector<std::string> paras; int then = 0; };  // then: 0 play, 1 next day, 2 title

static State g_state = S_TITLE;
static float g_time = 0;
static int g_floor = 0;
static float g_px = 0, g_py = 0;
static int g_face = 1;
static float g_walk = 0;
static std::vector<Bot> g_bots;
static std::vector<Task> g_tasks;
static std::map<std::string, Place> g_place;
static std::vector<std::string> g_inv;
static Card g_card;
static int g_menuSel = 0, g_slotSel = 0, g_setSel = 0;
static bool g_slotNew = false, g_eraseArm = false, g_ovwArm = false;
static int g_elevSel = 0;
static bool g_quit = false;
static std::string g_toast;
static float g_toastT = 0;
// transitions
static int g_fadePhase = 0;
static float g_fade = 0;
static int g_toFloor = 0;
static float g_toX = 0, g_toY = 0;
static bool g_testWin = false;  // test hook: every minigame is won instantly

static void toast(const std::string& s) { g_toast = s; g_toastT = 3.2f; }
static int flag(const std::string& k) { auto it = g_flags.find(k); return it == g_flags.end() ? 0 : it->second; }

static bool freeAt(int fi, float x, float y) {
    const float hw = 11, hh = 7;
    for (float dx : {-hw, hw}) for (float dy : {-hh, hh}) {
        if (solidC(tileAt(fi, (int)std::floor((x + dx) / T), (int)std::floor((y + dy) / T)))) return false;
    }
    return true;
}
static void teleport(int fi, float x, float y) { g_toFloor = fi; g_toX = x; g_toY = y; g_fadePhase = 1; g_fade = 0; }

// where is an NPC right now?
static void npcPos(const NpcDef& n, int& f, int& tx, int& ty) {
    auto it = g_place.find(n.key);
    if (it != g_place.end()) { f = it->second.floor; tx = it->second.tx; ty = it->second.ty; }
    else { f = n.floor; tx = n.tx; ty = n.ty; }
}

// ---- tasks
static int curGroup() {
    int g = 1 << 30;
    for (auto& t : g_tasks) if (!t.done) g = std::min(g, t.group);
    return g;
}
static bool taskActive(const Task& t) { return !t.done && t.group == curGroup(); }
static const DayDef* dayDef(int n) {
    for (auto& d : DAYS) if (d.n == n) return &d;
    return nullptr;
}

// ---- dialogue
static const std::vector<Item>* g_items = nullptr;
static size_t g_ii = 0;
static Line g_cur;
static std::vector<std::pair<std::string, std::string>> g_opts;
static int g_optSel = 0;
static float g_talkChars = 0;
static int g_curTask = -1;
static struct Pending {
    bool done = false, endday = false;
    std::string mtype, ma, mb, mc;
    bool mini = false;
    int warpF = -1; float wx = 0, wy = 0;
    std::string cardTitle, cardText; int cardThen = 0; bool card = false;
} g_pend;
static int g_autoCut = -1;
static std::vector<Item> g_tmpNode;

static void completeTask(int i);
static void finishDay();
static void startDay(int n);
static void startMini(int taskIdx, const std::string& type, const std::string& a, const std::string& b, const std::string& c);

static Col speakerCol(const std::string& who) {
    if (who == "Amir") return ORANGE;
    if (who.empty()) return mix(WHITE, INK, 0.3f);
    const NpcDef* n = findNpc(who);
    if (n) return n->col;
    return SKY;
}
static std::string speakerName(const std::string& who) {
    const NpcDef* n = findNpc(who);
    if (n) return n->disp;
    return who;
}
static void goNode(const std::string& name) {
    auto it = NODES.find(name);
    if (it == NODES.end()) { toast("missing node: " + name); g_items = nullptr; return; }
    g_items = &it->second;
    g_ii = 0;
}
static void closeDialogue() {
    g_state = S_PLAY;
    g_items = nullptr;
    if (g_curTask >= 0 && g_curTask < (int)g_tasks.size()) {
        Task& t = g_tasks[g_curTask];
        if (t.kind == TK_CUT && !t.done) g_pend.done = true;
    }
    int ct = g_curTask;
    g_curTask = -1;
    Pending p = g_pend;
    g_pend = Pending();
    if (p.done && ct >= 0) completeTask(ct);
    if (p.warpF >= 0) teleport(p.warpF, p.wx, p.wy);
    if (p.endday) { finishDay(); return; }
    if (p.mini) startMini(-1, p.mtype, p.ma, p.mb, p.mc);
    if (p.card) {
        g_card = Card();
        g_card.title = p.cardTitle;
        g_card.paras.push_back(p.cardText);
        g_card.then = p.cardThen;
        g_state = S_CARD;
    }
}
static void execCmd(const std::string& cmdline) {
    auto t = splitWs(cmdline);
    if (t.empty()) return;
    const std::string& c = t[0];
    if (c == "done") g_pend.done = true;
    else if (c == "set" && t.size() >= 2) g_flags[t[1]] = t.size() > 2 ? atoi(t[2].c_str()) : 1;
    else if (c == "add" && t.size() >= 3) g_flags[t[1]] += atoi(t[2].c_str());
    else if (c == "rel" && t.size() >= 3) g_flags["rel_" + t[1]] += atoi(t[2].c_str());
    else if (c == "goto" && t.size() >= 2) goNode(t[1]);
    else if (c == "if" && t.size() >= 6) {  // if key op value -> node
        int v = flag(t[1]), want = atoi(t[3].c_str());
        bool ok = t[2] == ">=" ? v >= want : t[2] == "==" ? v == want : t[2] == "<" ? v < want : t[2] == ">" ? v > want : false;
        if (ok) goNode(t[5]);
    } else if (c == "give" && t.size() >= 2) {
        std::string name = cmdline.substr(5);
        g_inv.push_back(trim(name));
    } else if (c == "warp" && t.size() >= 4) {
        g_pend.warpF = floorId(t[1]);
        g_pend.wx = (atoi(t[2].c_str()) + 0.5f) * T;
        g_pend.wy = (atoi(t[3].c_str()) + 0.5f) * T;
    } else if (c == "endday") g_pend.endday = true;
    else if (c == "mini" && t.size() >= 2) {
        g_pend.mini = true; g_pend.mtype = t[1];
        g_pend.ma = t.size() > 2 ? t[2] : ""; g_pend.mb = t.size() > 3 ? t[3] : ""; g_pend.mc = t.size() > 4 ? t[4] : "";
    } else if (c == "card") {
        size_t sp = cmdline.find(' ');
        auto p = splitBar(cmdline.substr(sp + 1));
        g_pend.card = true;
        g_pend.cardTitle = p[0];
        g_pend.cardText = p.size() > 1 ? p[1] : "";
        g_pend.cardThen = p.size() > 2 ? atoi(p[2].c_str()) : 0;
    }
}
static void stepDialogue() {
    for (int guard = 0; guard < 200; guard++) {
        if (!g_items || g_ii >= g_items->size()) { closeDialogue(); return; }
        const Item& it = (*g_items)[g_ii++];
        if (it.t == Item::LINE) {
            g_cur.who = it.a;
            g_cur.what = it.b;
            g_cur.col = speakerCol(it.a);
            g_talkChars = 0;
            g_state = S_TALK;
            return;
        }
        if (it.t == Item::CHOICE) {
            g_opts = it.opts;
            g_optSel = 0;
            g_state = S_CHOICE;
            return;
        }
        execCmd(it.a);
    }
    closeDialogue();
}
static void startNode(const std::string& name, int taskIdx) {
    g_curTask = taskIdx;
    g_pend = Pending();
    goNode(name);
    if (!g_items) { g_state = S_PLAY; return; }
    stepDialogue();
}
static void startLines(const std::vector<std::pair<std::string, std::string>>& lines) {
    g_tmpNode.clear();
    for (auto& l : lines) { Item it; it.t = Item::LINE; it.a = l.first; it.b = l.second; g_tmpNode.push_back(it); }
    g_curTask = -1;
    g_pend = Pending();
    g_items = &g_tmpNode;
    g_ii = 0;
    stepDialogue();
}

// ---- day flow
static void activateGroup() {
    int g = curGroup();
    if (g == (1 << 30)) return;
    for (size_t i = 0; i < g_tasks.size(); i++) {
        Task& t = g_tasks[i];
        if (t.done || t.group != g) continue;
        if (t.kind == TK_TALK) g_place[t.npc] = {t.floor, t.tx, t.ty};
        if (t.kind == TK_CUT) g_autoCut = (int)i;
    }
}
static void completeTask(int i) {
    if (i < 0 || i >= (int)g_tasks.size() || g_tasks[i].done) return;
    g_tasks[i].done = true;
    bool any = false;
    for (auto& t : g_tasks) if (!t.done) any = true;
    if (!any) { finishDay(); return; }
    activateGroup();
}
static void showCard(const std::string& title, const std::string& sub, const std::vector<std::string>& paras, int then) {
    g_card = Card();
    g_card.title = title; g_card.sub = sub; g_card.paras = paras; g_card.then = then;
    g_state = S_CARD;
}
static void finishDay() {
    const DayDef* d = dayDef(g_day);
    std::vector<std::string> paras = d ? d->outro : std::vector<std::string>();
    if (g_day >= LAST_DAY) showCard("THE END", "Day " + std::to_string(g_day) + " complete", paras, 2);
    else showCard("Day " + std::to_string(g_day) + " complete", d ? d->title : "", paras, 1);
}
static void startDay(int n) {
    g_day = n;
    const DayDef* d = dayDef(n);
    if (!d) { showCard("THE END", "", {"Thanks for playing."}, 2); return; }
    g_tasks = d->tasks;
    g_place.clear();
    g_inv.clear();
    g_autoCut = -1;
    g_bots.clear();
    activateGroup();
    // new day: everybody starts the morning at home, Amir in his room
    g_floor = d->sf;
    g_px = (d->sx + 0.5f) * T;
    g_py = (d->sy + 0.5f) * T;
    g_face = 1;
    saveSlot(g_slot);
    std::vector<std::string> paras = d->intro;
    showCard("DAY " + std::to_string(n), d->title, paras, 0);
}
static void placeBots() {
    g_bots.clear();
    for (int fi = 0; fi < NF; fi++) {
        if (fi == F_BOTS || FLOORS[fi].kind == BACK) continue;
        Bot b;
        b.floor = fi; b.pers = (fi * 5 + 1) % (int)PERSONS.size();
        for (int tries = 0; tries < 200; tries++) {
            int tx = 2 + (int)(frand() * (MW - 4)), ty = 2 + (int)(frand() * (MH - 4));
            if (tileAt(fi, tx, ty) == '.') { b.x = (tx + 0.5f) * T; b.y = (ty + 0.5f) * T; break; }
        }
        g_bots.push_back(b);
    }
}

// ------------------------------------------------------------------ minigames
// world minis: bots (catch) and hornets (swat). overlay minis: balance, code, dodge, timing.
static int g_wm = 0;  // 0 none, 1 bots, 2 hornets
static int g_wmTask = -1;
static std::string g_wmWin;
// bots
static int g_mgCaught = 0, g_mgGoal = 5;
static float g_mgTime = 0;
// hornets
struct Hornet { float x, y, vx, vy; bool alive; float ph; };
static std::vector<Hornet> g_hornets;
static int g_hKilled = 0, g_hGoal = 12, g_hHp = 5, g_hSpawned = 0;
static float g_hInv = 0, g_hCool = 0, g_hSwing = 0, g_hSpawnT = 0, g_hAllyT = 0;
static std::string g_allyA, g_allyB;
static float g_allyAx = 0, g_allyAy = 0, g_allyBx = 0, g_allyBy = 0, g_allySw = 0;
// overlay
static int g_ov = 0;  // 1 balance 2 code 3 dodge 4 timing
static bool g_ovReady = false;
static float g_ovT = 0, g_ovA = 0, g_ovB = 0;
static int g_ovTask = -1;
static std::string g_ovWin;
static std::string g_ovTitle, g_ovHelp;
// balance
static float g_tilt = 0, g_tiltV = 0, g_wph = 0;
// code
static std::vector<int> g_seq;
static int g_seqIdx = 0, g_seqShow = 0;
static float g_seqT = 0, g_padLit[4] = {0, 0, 0, 0};
static bool g_seqInput = false;
// dodge
struct Hazard { float x, y, vy, size; int kind; };
static std::vector<Hazard> g_haz;
static float g_dx = 640, g_dy = 560, g_dInv = 0, g_dSpawn = 0;
static int g_dLives = 3, g_dTheme = 0;
// timing
static float g_cur_x = 0, g_zone = 0, g_zoneW = 0, g_curDir = 1;
static int g_hits = 0, g_miss = 0, g_hitsGoal = 5;

static int toI(const std::string& s, int def) { return s.empty() ? def : atoi(s.c_str()); }
static void endMini(bool won);
static void startMini(int taskIdx, const std::string& type, const std::string& a, const std::string& b, const std::string& c) {
    g_wm = 0; g_ov = 0;
    g_wmTask = g_ovTask = taskIdx;
    if (taskIdx >= 0) { g_wmWin = g_tasks[taskIdx].winNode; g_ovWin = g_tasks[taskIdx].winNode; }
    else { g_wmWin.clear(); g_ovWin.clear(); }
    if (g_testWin) { g_wm = 99; endMini(true); return; }
    if (type == "bots") {
        g_wm = 1;
        g_mgGoal = toI(a, 5);
        g_mgTime = (float)toI(b, 45);
        g_mgCaught = 0;
        g_bots.erase(std::remove_if(g_bots.begin(), g_bots.end(), [](const Bot& x) { return x.mg; }), g_bots.end());
        for (int i = 0; i < g_mgGoal; i++) {
            Bot bt; bt.floor = g_floor; bt.mg = true; bt.pers = i % (int)PERSONS.size();
            for (int tries = 0; tries < 400; tries++) {
                int tx = 6 + (int)(frand() * (MW - 10)), ty = 3 + (int)(frand() * (MH - 6));
                float x = (tx + 0.5f) * T, y = (ty + 0.5f) * T;
                if (tileAt(g_floor, tx, ty) == '.' && std::hypot(x - g_px, y - g_py) > 300) { bt.x = x; bt.y = y; break; }
            }
            g_bots.push_back(bt);
        }
        toast("Catch them all! Corner them against the cars.");
    } else if (type == "hornets") {
        g_wm = 2;
        g_hGoal = toI(a, 12);
        g_hKilled = 0; g_hHp = toI(b, 5); g_hSpawned = 0; g_hInv = 0; g_hCool = 0; g_hSwing = 0; g_hSpawnT = 0.5f; g_hAllyT = 1;
        g_hornets.clear();
        g_allyA = c.empty() ? "" : c.substr(0, c.find(','));
        g_allyB = c.find(',') == std::string::npos ? "" : c.substr(c.find(',') + 1);
        g_allyAx = g_px - 90; g_allyAy = g_py + 40; g_allyBx = g_px + 90; g_allyBy = g_py - 40;
        toast("Press A / B / X / Y to swing. Swat them all!");
    } else if (type == "balance") {
        g_ov = 1; g_ovReady = false; g_ovT = 0; g_ovA = (float)toI(a, 20); g_ovB = (float)toI(b, 2);
        g_tilt = 0.05f; g_tiltV = 0; g_wph = frand() * 6;
        g_ovTitle = "Carry the stack!";
        g_ovHelp = "Lean the stick toward the tilt to keep the tower upright until you arrive.";
    } else if (type == "code") {
        g_ov = 2; g_ovReady = false; g_ovT = 0;
        int len = toI(a, 4);
        g_seq.clear();
        for (int i = 0; i < len; i++) g_seq.push_back((int)(frand() * 4) % 4);
        g_seqIdx = 0; g_seqShow = 0; g_seqT = 0; g_seqInput = false;
        for (int i = 0; i < 4; i++) g_padLit[i] = 0;
        g_ovTitle = "Enter the code";
        g_ovHelp = "Watch the pads, then repeat them with the D-pad (or X / A / B / Y).";
    } else if (type == "dodge") {
        g_ov = 3; g_ovReady = false; g_ovT = 0; g_ovA = (float)toI(a, 20); g_dTheme = toI(b, 0);
        g_haz.clear(); g_dx = 640; g_dy = 560; g_dLives = 3; g_dInv = 0; g_dSpawn = 0.3f;
        g_ovTitle = "Dodge!";
        g_ovHelp = "Move with the stick or D-pad. Do not get hit three times.";
    } else if (type == "timing") {
        g_ov = 4; g_ovReady = false; g_ovT = 0;
        g_hitsGoal = toI(a, 5); g_hits = 0; g_miss = 0; g_zoneW = 150; g_zone = 200 + frand() * 400; g_cur_x = 0; g_curDir = 1;
        g_ovB = (float)toI(b, 1);
        g_ovTitle = "Steady hands";
        g_ovHelp = "Press A when the marker is inside the bright zone. Three misses and you start over.";
    } else {
        toast("unknown minigame: " + type);
        return;
    }
    if (g_ov) g_state = S_MINI;
}
static void endMini(bool won) {
    int task = g_wmTask >= 0 ? g_wmTask : g_ovTask;
    std::string winNode = g_wm ? g_wmWin : g_ovWin;
    int wasWorld = g_wm;
    g_wm = 0; g_ov = 0;
    g_bots.erase(std::remove_if(g_bots.begin(), g_bots.end(), [](const Bot& x) { return x.mg; }), g_bots.end());
    g_hornets.clear();
    g_state = S_PLAY;
    g_wmTask = g_ovTask = -1;
    (void)wasWorld;
    if (won) {
        if (task >= 0) {
            g_tasks[task].done = true;  // complete after the win dialogue if there is one
            if (!winNode.empty()) {
                bool any = false;
                for (auto& t : g_tasks) if (!t.done) any = true;
                g_tasks[task].done = false;
                g_curTask = task;
                startNode(winNode, task);
                g_pend.done = true;
                (void)any;
                return;
            }
            completeTask(task);
        } else toast("Well done!");
    } else {
        toast("Not this time. Try again!");
    }
}

static void updateOverlay(float dt, const In& in) {
    if (!g_ovReady) {
        if (in.down & K_A) g_ovReady = true;
        if (in.down & K_B) { g_ov = 0; g_state = S_PLAY; g_wmTask = g_ovTask = -1; }
        return;
    }
    g_ovT += dt;
    if (g_ov == 1) {
        float w = (std::sin(g_ovT * 1.3f + g_wph) * 0.6f + std::sin(g_ovT * 2.9f + g_wph * 2) * 0.4f) * (0.45f + 0.22f * g_ovB);
        float acc = 1.6f * g_tilt + w - 4.2f * in.ax;
        g_tiltV += acc * dt;
        g_tiltV *= (1 - 0.9f * dt);
        g_tilt += g_tiltV * dt;
        if (std::fabs(g_tilt) > 1) endMini(false);
        else if (g_ovT >= g_ovA) endMini(true);
    } else if (g_ov == 2) {
        for (int i = 0; i < 4; i++) g_padLit[i] = std::max(0.0f, g_padLit[i] - dt);
        if (!g_seqInput) {
            g_seqT += dt;
            if (g_seqT > 0.9f) {
                g_seqT = 0;
                if (g_seqShow < (int)g_seq.size()) { g_padLit[g_seq[g_seqShow]] = 0.5f; g_seqShow++; }
                else { g_seqInput = true; g_seqIdx = 0; }
            }
        } else {
            int p = -1;
            if (in.down & (K_UP | K_X)) p = 0;
            else if (in.down & (K_RIGHT | K_A)) p = 1;
            else if (in.down & (K_DOWN | K_B)) p = 2;
            else if (in.down & (K_LEFT | K_Y)) p = 3;
            if (p >= 0) {
                g_padLit[p] = 0.25f;
                if (p == g_seq[g_seqIdx]) { if (++g_seqIdx >= (int)g_seq.size()) endMini(true); }
                else endMini(false);
            }
        }
    } else if (g_ov == 3) {
        float sp = 330;
        g_dx += in.ax * sp * dt; g_dy += in.ay * sp * dt;
        g_dx = std::max(260.0f, std::min(1020.0f, g_dx)); g_dy = std::max(170.0f, std::min(610.0f, g_dy));
        g_dInv -= dt;
        g_dSpawn -= dt;
        float rate = std::max(0.16f, 0.42f - g_ovT * 0.008f);
        if (g_dSpawn <= 0) {
            g_dSpawn = rate;
            Hazard h; h.x = 260 + frand() * 760; h.y = 140; h.vy = 230 + frand() * 160 + g_ovT * 4; h.size = 30 + frand() * 18; h.kind = (int)(frand() * 3);
            g_haz.push_back(h);
        }
        for (auto& h : g_haz) {
            h.y += h.vy * dt;
            if (g_dInv <= 0 && std::fabs(h.x - g_dx) < (h.size + 24) / 2 && std::fabs(h.y - g_dy) < (h.size + 30) / 2) { g_dLives--; g_dInv = 1.3f; }
        }
        g_haz.erase(std::remove_if(g_haz.begin(), g_haz.end(), [](const Hazard& h) { return h.y > 660; }), g_haz.end());
        if (g_dLives <= 0) endMini(false);
        else if (g_ovT >= g_ovA) endMini(true);
    } else if (g_ov == 4) {
        float sp = 520 + 90 * g_ovB + g_hits * 40;
        g_cur_x += g_curDir * sp * dt;
        if (g_cur_x > 700) { g_cur_x = 700; g_curDir = -1; }
        if (g_cur_x < 0) { g_cur_x = 0; g_curDir = 1; }
        if (in.down & K_A) {
            if (g_cur_x >= g_zone && g_cur_x <= g_zone + g_zoneW) {
                g_hits++;
                g_zoneW = std::max(60.0f, g_zoneW - 14);
                g_zone = frand() * (700 - g_zoneW);
                if (g_hits >= g_hitsGoal) endMini(true);
            } else if (++g_miss >= 3) endMini(false);
        }
    }
}
static void updateWorldMini(float dt, const In& in) {
    if (g_wm == 1) {
        g_mgCaught = 0;
        for (auto& b : g_bots) if (b.mg && b.caught) g_mgCaught++;
        g_mgTime -= dt;
        if (g_mgCaught >= g_mgGoal) endMini(true);
        else if (g_mgTime <= 0) endMini(false);
    } else if (g_wm == 2) {
        g_hInv -= dt; g_hCool -= dt; g_hSwing -= dt; g_hSpawnT -= dt; g_hAllyT -= dt; g_allySw -= dt;
        int alive = 0;
        for (auto& h : g_hornets) if (h.alive) alive++;
        if (g_hSpawnT <= 0 && g_hSpawned < g_hGoal && alive < 6) {
            g_hSpawnT = 0.8f;
            Hornet h;
            h.x = 36.5f * T + (frand() - 0.5f) * 40; h.y = 4.5f * T + 40; h.vx = h.vy = 0; h.alive = true; h.ph = frand() * 6;
            g_hornets.push_back(h);
            g_hSpawned++;
        }
        for (auto& h : g_hornets) {
            if (!h.alive) continue;
            float dx = g_px - h.x, dy = g_py - h.y, d = std::max(1.0f, std::hypot(dx, dy));
            float sp = 105 + 12 * std::sin(g_time * 3 + h.ph);
            float wob = std::sin(g_time * 9 + h.ph) * 60;
            float nx = h.x + (dx / d * sp - dy / d * wob) * dt, ny = h.y + (dy / d * sp + dx / d * wob) * dt;
            if (tileAt(g_floor, (int)(nx / T), (int)(h.y / T)) != 't') h.x = nx;
            if (tileAt(g_floor, (int)(h.x / T), (int)(ny / T)) != 't') h.y = ny;
            if (g_hInv <= 0 && d < 30) { g_hHp--; g_hInv = 1.2f; }
        }
        if (in.down & (K_A | K_B | K_X | K_Y)) {
            if (g_hCool <= 0) {
                g_hCool = 0.32f; g_hSwing = 0.18f;
                for (auto& h : g_hornets) if (h.alive && std::hypot(h.x - g_px, h.y - g_py) < 100) { h.alive = false; g_hKilled++; }
            }
        }
        // allies follow and swat too
        g_allyAx += (g_px - 90 - g_allyAx) * std::min(1.0f, dt * 2.5f); g_allyAy += (g_py + 40 - g_allyAy) * std::min(1.0f, dt * 2.5f);
        g_allyBx += (g_px + 90 - g_allyBx) * std::min(1.0f, dt * 2.5f); g_allyBy += (g_py - 40 - g_allyBy) * std::min(1.0f, dt * 2.5f);
        if (g_hAllyT <= 0) {
            g_hAllyT = 1.6f;
            for (auto& h : g_hornets) if (h.alive && std::hypot(h.x - g_allyAx, h.y - g_allyAy) < 170) { h.alive = false; g_hKilled++; g_allySw = 0.2f; break; }
        }
        if (g_hKilled >= g_hGoal) endMini(true);
        else if (g_hHp <= 0) endMini(false);
    }
}

// ------------------------------------------------------------------ interactions
struct Target { int kind = 0; int idx = 0; float d = 1e9; int tx = 0, ty = 0; };  // 1 npc, 2 bot, 3 tile, 4 note, 5 mini trigger, 6 pick
static Target findTarget() {
    Target best;
    for (size_t i = 0; i < NPCS.size(); i++) {
        int f, tx, ty;
        npcPos(NPCS[i], f, tx, ty);
        if (f != g_floor) continue;
        float d = std::hypot((tx + 0.5f) * T - g_px, (ty + 0.5f) * T - g_py);
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
        if (c != 'E' && c != 'P' && c != 'T') continue;
        float d = std::hypot((tx + 0.5f) * T - g_px, (ty + 0.5f) * T - g_py) - 10;
        if (d < 70 && d < best.d) { best.kind = 3; best.d = d; best.tx = tx; best.ty = ty; }
    }
    for (size_t i = 0; i < NOTES.size(); i++) {
        if (NOTES[i].floor != g_floor) continue;
        float d = std::hypot((NOTES[i].tx + 0.5f) * T - g_px, (NOTES[i].ty + 0.5f) * T - g_py);
        if (d < 60 && d < best.d) { best.kind = 4; best.idx = (int)i; best.d = d; }
    }
    for (size_t i = 0; i < g_tasks.size(); i++) {
        const Task& t = g_tasks[i];
        if (!taskActive(t) || t.floor != g_floor || (t.kind != TK_MINI && t.kind != TK_PICK)) continue;
        float d = std::hypot((t.tx + 0.5f) * T - g_px, (t.ty + 0.5f) * T - g_py);
        if (d < (t.kind == TK_PICK ? 56 : 70) && d < best.d) { best.kind = t.kind == TK_MINI ? 5 : 6; best.idx = (int)i; best.d = d; }
    }
    return best;
}
static void talkNpc(const NpcDef& n) {
    // an active task for this npc?
    for (size_t i = 0; i < g_tasks.size(); i++) {
        Task& t = g_tasks[i];
        if (taskActive(t) && t.kind == TK_TALK && t.npc == n.key) { startNode(t.node, (int)i); return; }
    }
    const DayDef* d = dayDef(g_day);
    if (d) {
        auto it = d->daily.find(n.key);
        if (it != d->daily.end()) {
            g_tmpNode.clear();
            Item l; l.t = Item::LINE; l.a = n.key; l.b = it->second; g_tmpNode.push_back(l);
            Item gt; gt.t = Item::CMD; gt.a = std::string("goto chat_") + n.key; g_tmpNode.push_back(gt);
            g_curTask = -1; g_pend = Pending(); g_items = &g_tmpNode; g_ii = 0;
            stepDialogue();
            return;
        }
    }
    startNode(std::string("chat_") + n.key, -1);
}
static void interact() {
    Target t = findTarget();
    if (t.kind == 1) talkNpc(NPCS[t.idx]);
    else if (t.kind == 2) {
        Bot& b = g_bots[t.idx];
        const Pers& p = PERSONS[b.pers];
        startLines({{std::string("Cleaning bot (") + p.name + ")", p.lines[(size_t)(g_time * 3) % p.lines.size()]}});
    } else if (t.kind == 3) {
        char c = tileAt(g_floor, t.tx, t.ty);
        if (c == 'E') { g_elevSel = g_floor < ELEV_FLOORS ? g_floor : 0; g_state = S_ELEV; }
        else if (c == 'P') {
            const Portal* p = findPortal(g_floor, t.tx, t.ty);
            if (p) teleport(p->to, p->ttx * T, p->tty * T);
            else if (g_floor == F_BACK) teleport(F_LOBBY, 41.5f * T, 11.5f * T);
        } else if (c == 'T') {
            // free-play bot roundup at the console
            startLines({{"Console", "Practice round: catch 5 bots before time runs out. Ready?"}});
            g_pend.mini = true; g_pend.mtype = "bots"; g_pend.ma = "5"; g_pend.mb = "45";
        }
    } else if (t.kind == 4) {
        startLines({{"Note", NOTES[t.idx].text}});
    } else if (t.kind == 5) {
        const Task& k = g_tasks[t.idx];
        startMini(t.idx, k.mtype, k.ma, k.mb, k.mc);
    }
}
static void pickupCheck() {
    for (size_t i = 0; i < g_tasks.size(); i++) {
        Task& t = g_tasks[i];
        if (!taskActive(t) || t.kind != TK_PICK || t.floor != g_floor) continue;
        if (std::hypot((t.tx + 0.5f) * T - g_px, (t.ty + 0.5f) * T - g_py) < 40) {
            g_inv.push_back(t.item);
            toast("Picked up: " + t.item);
            completeTask((int)i);
            return;
        }
    }
}

// ------------------------------------------------------------------ update
static void moveBot(Bot& b, float dx, float dy) {
    if (freeAt(b.floor, b.x + dx, b.y)) b.x += dx;
    if (freeAt(b.floor, b.x, b.y + dy)) b.y += dy;
}
// backrooms entity
static float g_entX = 0, g_entY = 0;
static bool g_entOn = false;
static void updatePlay(float dt, const In& in) {
    g_time += dt;
    if (g_toastT > 0) g_toastT -= dt;
    float ax = in.ax, ay = in.ay;
    float len = std::sqrt(ax * ax + ay * ay);
    if (len > 1) { ax /= len; ay /= len; }
    const float spd = 250;
    float dx = ax * spd * dt, dy = ay * spd * dt;
    if (freeAt(g_floor, g_px + dx, g_py)) g_px += dx;
    if (freeAt(g_floor, g_px, g_py + dy)) g_py += dy;
    if (std::fabs(ax) > 0.2f) g_face = ax > 0 ? 1 : -1;
    g_walk = (len > 0.1f) ? g_walk + dt * 10 : 0;
    for (auto& b : g_bots) {
        if (b.floor != g_floor) continue;
        if (b.mg) {
            float d = std::hypot(b.x - g_px, b.y - g_py);
            if (d < 30) { b.caught = true; continue; }
            float sp = 150, vx = 0, vy = 0;
            if (d < 230) {
                float ux = (b.x - g_px) / d, uy = (b.y - g_py) / d;
                vx = ux * sp + -uy * 40 * (b.pers % 2 ? 1 : -1); vy = uy * sp + ux * 40 * (b.pers % 2 ? 1 : -1);
                float nx = b.x + vx * dt, ny = b.y + vy * dt;
                if (!freeAt(b.floor, nx, b.y)) { vx = 0; vy = (vy >= 0 ? 1 : -1) * sp; }
                if (!freeAt(b.floor, b.x, ny)) { vy = 0; vx = (vx >= 0 ? 1 : -1) * sp; }
            } else {
                b.timer -= dt;
                if (b.timer <= 0) { b.timer = 0.5f + frand(); float a = frand() * 6.283f; b.vx = std::cos(a) * 50; b.vy = std::sin(a) * 50; }
                vx = b.vx; vy = b.vy;
            }
            moveBot(b, vx * dt, vy * dt);
        } else {
            b.timer -= dt;
            if (b.timer <= 0) { b.timer = 1 + frand() * 2; if (frand() < 0.4f) { b.vx = b.vy = 0; } else { float a = frand() * 6.283f; b.vx = std::cos(a) * 45; b.vy = std::sin(a) * 45; } }
            moveBot(b, b.vx * dt, b.vy * dt);
        }
    }
    // backrooms entity
    if (g_floor == F_BACK) {
        if (!g_entOn) { g_entOn = true; g_entX = 38.5f * T; g_entY = 10.5f * T; }
        float d = std::hypot(g_entX - g_px, g_entY - g_py);
        if (d < 320 && d > 1) {
            float vx = (g_px - g_entX) / d * 85, vy = (g_py - g_entY) / d * 85;
            if (freeAt(F_BACK, g_entX + vx * dt, g_entY)) g_entX += vx * dt;
            if (freeAt(F_BACK, g_entX, g_entY + vy * dt)) g_entY += vy * dt;
        }
        if (d < 34 && g_fadePhase == 0) { toast("Something got too close. You wake up at the start of the maze."); teleport(F_BACK, 1.5f * T, 1.5f * T); g_entX = 38.5f * T; g_entY = 10.5f * T; }
    } else g_entOn = false;
    if (g_wm) updateWorldMini(dt, in);
    // auto: cutscenes, "go" tasks, pickups
    if (g_wm == 0 && g_fadePhase == 0) {
        if (g_autoCut >= 0) {
            int c = g_autoCut;
            g_autoCut = -1;
            if (c < (int)g_tasks.size() && !g_tasks[c].done) { startNode(g_tasks[c].node, c); return; }
        }
        for (size_t i = 0; i < g_tasks.size(); i++) {
            Task& t = g_tasks[i];
            if (taskActive(t) && t.kind == TK_GO && t.floor == g_floor) { completeTask((int)i); return; }
            if (taskActive(t) && t.kind == TK_WARP) {
                float x = (t.tx + 0.5f) * T, y = (t.ty + 0.5f) * T;
                int f = t.floor;
                completeTask((int)i);
                if (g_state == S_PLAY) teleport(f, x, y);
                return;
            }
        }
        pickupCheck();
    }
    if ((in.down & K_A) && !(g_wm == 2)) interact();
    if (in.down & K_PLUS) g_state = S_PAUSE;
}

static void startNewGame(int slot) {
    g_slot = slot;
    g_flags.clear();
    placeBots();
    startDay(1);
}
static void startLoaded(int slot) {
    g_slot = slot;
    loadSlot(slot);
    placeBots();
    startDay(g_day);
}
static void update(float dt, const In& in) {
    g_time += (g_state == S_PLAY ? 0 : dt);
    if (g_fadePhase == 1) {
        g_fade += dt * 4;
        if (g_fade >= 1) {
            g_floor = g_toFloor; g_px = g_toX; g_py = g_toY; g_fadePhase = 2; g_fade = 1;
            if (g_wm) { g_wm = 0; g_bots.erase(std::remove_if(g_bots.begin(), g_bots.end(), [](const Bot& b) { return b.mg; }), g_bots.end()); }
            if (g_state == S_ELEV || g_state == S_TALK) g_state = S_PLAY;
        }
        return;
    }
    if (g_fadePhase == 2) { g_fade -= dt * 4; if (g_fade <= 0) { g_fade = 0; g_fadePhase = 0; } }
    switch (g_state) {
        case S_TITLE: {
            if (in.down & K_UP) g_menuSel = (g_menuSel + 3) % 4;
            if (in.down & K_DOWN) g_menuSel = (g_menuSel + 1) % 4;
            if (in.down & K_PLUS) g_quit = true;
            if (in.down & K_A) {
                if (g_menuSel == 0) { g_slotNew = true; g_slotSel = 0; g_ovwArm = false; g_eraseArm = false; g_state = S_MENU_SLOTS; }
                else if (g_menuSel == 1) { g_slotNew = false; g_slotSel = 0; g_ovwArm = false; g_eraseArm = false; g_state = S_MENU_SLOTS; }
                else if (g_menuSel == 2) { g_setSel = 0; g_state = S_MENU_SETTINGS; }
                else g_quit = true;
            }
            break;
        }
        case S_MENU_SLOTS: {
            if (in.down & K_UP) { g_slotSel = (g_slotSel + 2) % 3; g_ovwArm = g_eraseArm = false; }
            if (in.down & K_DOWN) { g_slotSel = (g_slotSel + 1) % 3; g_ovwArm = g_eraseArm = false; }
            if (in.down & K_B) g_state = S_TITLE;
            if (in.down & K_X) {
                if (!slotInfo(g_slotSel).used) break;
                if (g_eraseArm) { eraseSlot(g_slotSel); g_eraseArm = false; toast("Slot erased"); }
                else { g_eraseArm = true; toast("Press X again to erase this slot"); }
            }
            if (in.down & K_A) {
                SlotInfo si = slotInfo(g_slotSel);
                if (g_slotNew) {
                    if (si.used && !g_ovwArm) { g_ovwArm = true; toast("This slot has a save. Press A again to overwrite it."); }
                    else { eraseSlot(g_slotSel); startNewGame(g_slotSel); }
                } else {
                    if (si.used) startLoaded(g_slotSel);
                    else toast("That slot is empty. Use New Game.");
                }
            }
            break;
        }
        case S_MENU_SETTINGS: {
            if (in.down & K_UP) g_setSel = (g_setSel + 2) % 3;
            if (in.down & K_DOWN) g_setSel = (g_setSel + 1) % 3;
            if (in.down & K_B) { saveSettings(); g_state = S_TITLE; }
            if (in.down & (K_A | K_LEFT | K_RIGHT)) {
                if (g_setSel == 0) g_textSpeed = (g_textSpeed + ((in.down & K_LEFT) ? 2 : 1)) % 3;
                else if (g_setSel == 1) g_scan = !g_scan;
                else { saveSettings(); g_state = S_TITLE; }
            }
            break;
        }
        case S_PLAY: updatePlay(dt, in); break;
        case S_TALK: {
            g_time += dt;
            static const float speeds[3] = {28, 55, 110};
            g_talkChars += dt * speeds[g_textSpeed];
            if (in.down & (K_A | K_B)) {
                if (g_talkChars < (float)g_cur.what.size()) g_talkChars = (float)g_cur.what.size();
                else stepDialogue();
            }
            break;
        }
        case S_CHOICE: {
            if (in.down & K_UP) g_optSel = (g_optSel + (int)g_opts.size() - 1) % (int)g_opts.size();
            if (in.down & K_DOWN) g_optSel = (g_optSel + 1) % (int)g_opts.size();
            if (in.down & K_A) {
                std::string tgt = g_opts[g_optSel].second;
                goNode(tgt);
                stepDialogue();
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
            if (in.down & K_X) { saveSlot(g_slot); g_state = S_TITLE; }
            if (in.down & K_Y) { saveSlot(g_slot); g_quit = true; }
            break;
        case S_CARD:
            if (in.down & K_A) {
                int then = g_card.then;
                if (then == 1) startDay(g_day + 1);
                else if (then == 2) g_state = S_TITLE;
                else g_state = S_PLAY;
            }
            break;
        case S_MINI: updateOverlay(dt, in); break;
    }
}

// ------------------------------------------------------------------ drawing: world
static void person(int x, int y, Col body, Col hair, int face, float walk, bool me = false) {
    int step = (int)(std::sin(walk) * 3);
    rect(x - 15, y - 41, 30, 38, INK);
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
    if (me) rect(x - 4, y - 23, 8, 5, WHITE);
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
static void hornetSprite(int x, int y, float t) {
    int fl = (int)(std::sin(t * 40) * 3);
    rect(x - 7, y - 5, 14, 10, INK);
    rect(x - 5, y - 4, 10, 8, GOLD);
    rect(x - 1, y - 4, 3, 8, INK);
    rect(x - 12, y - 10 - fl, 8, 5, WHITE, 200); rect(x + 4, y - 10 + fl, 8, 5, WHITE, 200);
    rect(x + 6, y - 2, 4, 4, RED);
}
static void drawTile(int fi, int tx, int ty, int sx, int sy) {
    const FloorDef& fd = FLOORS[fi];
    char c = tileAt(fi, tx, ty);
    Col fl = fd.tint;
    bool grass = fd.kind == YARD;
    Col floorC = fd.kind == BACK ? mix(fl, {120, 100, 40}, 0.35f) : mix(fl, INK, grass ? 0.15f : 0.42f);
    if (fi == F_ROAD && ty >= 10 && ty <= 13) floorC = {70, 72, 80};
    if (((tx + ty) & 1) == 0) floorC = mix(floorC, WHITE, 0.05f);
    if (c == '#') {
        Col w = fd.kind == BACK ? mix(fl, {255, 240, 150}, 0.2f) : grass ? mix(fl, INK, 0.45f) : mix(fl, WHITE, 0.30f);
        rect(sx, sy, T, T, w);
        rect(sx, sy, T, 4, mix(w, WHITE, 0.35f));
        if (tileAt(fi, tx, ty + 1) != '#') rect(sx, sy + T - 10, T, 10, mix(w, INK, 0.45f));
        if (fd.kind == BACK) for (int k = 8; k < T; k += 12) rect(sx + k, sy + 6, 3, T - 12, mix(w, INK, 0.12f));
        if (fd.kind == OUTSIDE) { rect(sx + 4, sy + 8, 4, T - 8, INK); rect(sx + 22, sy + 8, 4, T - 8, INK); rect(sx + 40, sy + 8, 4, T - 8, INK); }
        return;
    }
    rect(sx, sy, T, T, floorC);
    if (fi == F_ROAD && (ty == 11 || ty == 12) && ((tx & 1) == 0) && ty == 11) rect(sx + 8, sy + T - 4, 28, 4, GOLD);
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
        case 'P': {
            const Portal* p = findPortal(fi, tx, ty);
            bool secret = p && std::string(p->label) == "Secret door";
            if (secret) {
                float pp = 0.5f + 0.5f * std::sin(g_time * 3);
                rect(sx, sy, T, T, mix({50, 20, 30}, {120, 30, 40}, pp * 0.6f));
                frame(sx + 4, sy + 4, T - 8, T - 8, mix({160, 60, 60}, {255, 140, 90}, pp), 3);
                rect(sx + T - 14, sy + T / 2, 5, 5, GOLD);
            } else {
                rect(sx, sy, T, T, fd.kind == BACK ? Col{60, 200, 90} : Col{90, 70, 50});
                frame(sx + 3, sy + 3, T - 6, T - 6, WHITE, 2);
                rect(sx + T - 14, sy + T / 2, 5, 5, GOLD);
            }
            break;
        }
        case 'K': rect(sx + 6, sy + 10, T - 12, T - 14, INK); rect(sx + 10, sy + 14, T - 20, 6, MINT); break;
        case 'T': {
            rect(sx + 4, sy + 4, T - 8, T - 8, INK);
            rect(sx + 8, sy + 8, T - 16, T - 22, {20, 80, 60});
            rect(sx + 12, sy + 12, 10, 3, MINT); rect(sx + 12, sy + 18, 18, 3, MINT);
            break;
        }
        case 't': case 'H': {
            rect(sx + 18, sy + 22, 12, 24, {90, 60, 40});
            rect(sx + 2, sy + 2, T - 4, 30, INK);
            rect(sx + 4, sy + 4, T - 8, 26, {40, 120, 60});
            rect(sx + 10, sy + 8, T - 22, 10, {60, 150, 80});
            if (c == 'H') { rect(sx + 14, sy + 24, 20, 20, INK); rect(sx + 16, sy + 26, 16, 16, {200, 170, 90}); rect(sx + 20, sy + 30, 4, 4, INK); rect(sx + 26, sy + 34, 4, 4, INK); }
            break;
        }
        case 's': {
            Col sc = tileAt(fi, tx, ty - 1) == 's' ? Col{236, 214, 200} : Col{230, 120, 150};
            rect(sx, sy, T, T, sc);
            if (tileAt(fi, tx, ty + 1) != 's') { rect(sx, sy + T - 12, T, 12, ((tx & 1) ? PINK : WHITE)); }
            else if (tx % 3 == 0) rect(sx + 10, sy + 10, 28, 24, SKY);
            if (fi == F_ILOT) rect(sx + 3, sy + 3, T - 6, 4, INK);
            break;
        }
        case 'b': rect(sx + 4, sy + 14, T - 8, 16, INK); rect(sx + 6, sy + 16, T - 12, 12, {160, 110, 70}); break;
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
    // room signs
    if (fd.kind == RES) {
        const int rx[4] = {2, 12, 22, 32};
        for (int k = 0; k < 4; k++) for (int row = 0; row < 2; row++) {
            int y0 = row ? 15 : 1, y1 = row ? 22 : 8;
            std::string label;
            if (g_floor == F_LOBBY && k == 0 && row == 1) label = "Amir  L-001";
            for (auto& n : NPCS) if (n.floor == g_floor && n.tx >= rx[k] && n.tx <= rx[k] + 7 && n.ty >= y0 && n.ty <= y1) label = std::string(n.disp) + "  " + n.id;
            if (label.empty()) continue;
            int sx = (rx[k] + 4) * T - cx;
            int sy = row ? 13 * T + 14 - cy : 10 * T + 2 - cy;
            int w = textW(fS, label) + 16;
            rect(sx - w / 2, sy, w, 26, INK, 200);
            text(fS, label, sx, sy, WHITE, 1);
        }
    }
    // task markers + sprites
    struct Spr { float y; int kind, idx; };
    std::vector<Spr> sp;
    for (size_t i = 0; i < NPCS.size(); i++) {
        int f, tx, ty;
        npcPos(NPCS[i], f, tx, ty);
        if (f == g_floor) sp.push_back({(ty + 0.5f) * T, 1, (int)i});
    }
    for (size_t i = 0; i < g_bots.size(); i++) if (g_bots[i].floor == g_floor && !g_bots[i].caught) sp.push_back({g_bots[i].y, 2, (int)i});
    for (size_t i = 0; i < g_hornets.size(); i++) if (g_hornets[i].alive) sp.push_back({g_hornets[i].y, 4, (int)i});
    if (g_wm == 2) { if (!g_allyA.empty()) sp.push_back({g_allyAy, 5, 0}); if (!g_allyB.empty()) sp.push_back({g_allyBy, 5, 1}); }
    if (g_floor == F_BACK && g_entOn) sp.push_back({g_entY, 6, 0});
    sp.push_back({g_py, 3, 0});
    std::sort(sp.begin(), sp.end(), [](const Spr& a, const Spr& b) { return a.y < b.y; });
    // props (pickups / triggers / notes)
    for (auto& n : NOTES) if (n.floor == g_floor) {
        int x = (int)((n.tx + 0.5f) * T) - cx, y = (int)((n.ty + 0.5f) * T) - cy;
        rect(x - 10, y - 12, 20, 24, INK); rect(x - 8, y - 10, 16, 20, WHITE); rect(x - 5, y - 5, 10, 2, INK); rect(x - 5, y, 10, 2, INK);
    }
    for (auto& t : g_tasks) {
        if (!taskActive(t) || t.floor != g_floor) continue;
        int x = (int)((t.tx + 0.5f) * T) - cx, y = (int)((t.ty + 0.5f) * T) - cy;
        float bob = std::sin(g_time * 4) * 4;
        if (t.kind == TK_PICK) {
            rect(x - 14, y - 12 + (int)bob, 28, 24, INK); rect(x - 11, y - 9 + (int)bob, 22, 18, GOLD);
            text(fS, t.item, x, y - 42 + (int)bob, WHITE, 1);
        } else if (t.kind == TK_MINI) {
            circleO(x, y, 26 + (int)(std::sin(g_time * 5) * 3), GOLD, 3);
            text(fM, "!", x, y - 22, GOLD, 1);
        }
    }
    for (auto& s : sp) {
        if (s.kind == 1) {
            const NpcDef& n = NPCS[s.idx];
            int f, tx, ty;
            npcPos(n, f, tx, ty);
            int x = (int)((tx + 0.5f) * T) - cx, y = (int)((ty + 0.5f) * T) - cy + 16;
            int face = (g_px < (tx + 0.5f) * T) ? -1 : 1;
            person(x, y, n.col, n.hair, face, 0);
            text(fS, std::string(n.disp) + " " + n.id, x, y - 68, WHITE, 1);
            for (auto& t : g_tasks) if (taskActive(t) && t.kind == TK_TALK && t.npc == n.key) {
                int b = (int)(std::sin(g_time * 5) * 4);
                rect(x - 9, y - 106 + b, 18, 28, ORANGE); rect(x - 9, y - 106 + b, 18, 4, WHITE);
                text(fM, "!", x, y - 108 + b, INK, 1);
            }
        } else if (s.kind == 2) {
            const Bot& b = g_bots[s.idx];
            botSprite((int)b.x - cx, (int)b.y - cy + 14, PERSONS[b.pers].col, g_time, b.mg);
        } else if (s.kind == 4) hornetSprite((int)g_hornets[s.idx].x - cx, (int)g_hornets[s.idx].y - cy, g_time + s.idx);
        else if (s.kind == 5) {
            const NpcDef* n = findNpc(s.idx == 0 ? g_allyA : g_allyB);
            if (n) {
                float ax = s.idx == 0 ? g_allyAx : g_allyBx, ay = s.idx == 0 ? g_allyAy : g_allyBy;
                person((int)ax - cx, (int)ay - cy + 14, n->col, n->hair, g_px < ax ? -1 : 1, g_time * 6);
                text(fS, n->disp, (int)ax - cx, (int)ay - cy - 54, WHITE, 1);
                if (g_allySw > 0) circleO((int)ax - cx, (int)ay - cy - 10, 60, WHITE, 3);
            }
        } else if (s.kind == 6) {
            int x = (int)g_entX - cx, y = (int)g_entY - cy;
            rect(x - 16, y - 54, 32, 56, INK);
            rect(x - 12, y - 50, 24, 20, {30, 30, 40});
            rect(x - 8, y - 44, 5, 5, WHITE); rect(x + 3, y - 44, 5, 5, WHITE); rect(x - 8, y - 36, 16, 3, WHITE);
        } else {
            person((int)g_px - cx, (int)g_py - cy + 14, ORANGE, {40, 24, 16}, g_face, g_walk, true);
            if (g_wm == 2 && g_hSwing > 0) circleO((int)g_px - cx, (int)g_py - cy - 10, 100, WHITE, 4);
            if (g_hInv > 0 && ((int)(g_time * 20) & 1)) rect((int)g_px - cx - 16, (int)g_py - cy - 28, 32, 40, RED, 90);
        }
    }
    if (fd.kind == BACK) {  // darkness with a flickering light around Amir
        float fl = 1.0f + 0.04f * std::sin(g_time * 17) + ((hash2((int)(g_time * 9), 3) % 23 == 0) ? -0.15f : 0);
        int px = (int)g_px - cx, py = (int)g_py - cy - 20;
        for (int i = 0; i < 9; i++) {
            int r = (int)((120 + i * 60) * fl);
            int a = 230 - i * 24;
            if (a < 0) a = 0;
            // four rects around the lit square
            rect(0, 0, W, py - r, INK, a / 3 + 20);
            rect(0, py + r, W, H - py - r, INK, a / 3 + 20);
            rect(0, py - r, px - r, 2 * r, INK, a / 3 + 20);
            rect(px + r, py - r, W - px - r, 2 * r, INK, a / 3 + 20);
        }
    }
    // prompt + arrows
    if (g_state == S_PLAY && !g_wm) {
        Target t = findTarget();
        if (t.kind) {
            int x = 0, y = 0; std::string label = "Talk";
            if (t.kind == 1) { int f, tx, ty; npcPos(NPCS[t.idx], f, tx, ty); x = (int)((tx + 0.5f) * T) - cx; y = (int)((ty + 0.5f) * T) - cy - 118; }
            else if (t.kind == 2) { x = (int)g_bots[t.idx].x - cx; y = (int)g_bots[t.idx].y - cy - 62; label = "Beep"; }
            else if (t.kind == 4) { x = (int)((NOTES[t.idx].tx + 0.5f) * T) - cx; y = (int)((NOTES[t.idx].ty + 0.5f) * T) - cy - 46; label = "Read"; }
            else if (t.kind == 5) { x = (int)((g_tasks[t.idx].tx + 0.5f) * T) - cx; y = (int)((g_tasks[t.idx].ty + 0.5f) * T) - cy - 62; label = "Start"; }
            else if (t.kind == 6) { x = (int)((g_tasks[t.idx].tx + 0.5f) * T) - cx; y = (int)((g_tasks[t.idx].ty + 0.5f) * T) - cy - 70; label = "Take"; }
            else {
                x = (int)((t.tx + 0.5f) * T) - cx; y = (int)((t.ty + 0.5f) * T) - cy - 34;
                char c = tileAt(g_floor, t.tx, t.ty);
                label = c == 'E' ? "Elevator" : c == 'T' ? "Console" : "Door";
                if (c == 'P') { const Portal* p = findPortal(g_floor, t.tx, t.ty); label = p ? p->label : "Exit"; }
            }
            int w = textW(fS, "A  " + label) + 24;
            rect(x - w / 2, y, w, 30, INK, 220);
            frame(x - w / 2, y, w, 30, ORANGE, 2);
            text(fS, "A  " + label, x, y + 3, WHITE, 1);
        }
        // arrow toward the first active target on this floor, when it is off screen
        for (auto& tk : g_tasks) {
            if (!taskActive(tk) || tk.floor != g_floor || tk.kind == TK_GO || tk.kind == TK_CUT || tk.kind == TK_WARP) continue;
            float tx = (tk.tx + 0.5f) * T - cx, ty = (tk.ty + 0.5f) * T - cy;
            if (tx > 40 && tx < W - 40 && ty > 100 && ty < H - 40) break;
            float ax = std::max(40.0f, std::min((float)W - 40, tx)), ay = std::max(110.0f, std::min((float)H - 40, ty));
            rect((int)ax - 16, (int)ay - 16, 32, 32, ORANGE); frame((int)ax - 16, (int)ay - 16, 32, 32, INK, 3);
            text(fS, ">", (int)ax, (int)ay - 14, INK, 1);
            break;
        }
    }
}
static void drawHud() {
    const FloorDef& fd = FLOORS[g_floor];
    rect(16, 16, 400, 66, INK, 215);
    frame(16, 16, 400, 66, ORANGE, 3);
    text(fM, fd.name, 30, 18, WHITE);
    text(fS, fd.sub, 30, 54, mix(fd.tint, WHITE, 0.6f));
    // objective box
    int w = 560;
    std::vector<std::string> objs;
    int farFloor = -1;
    for (auto& t : g_tasks) if (taskActive(t) && !t.obj.empty()) { objs.push_back(t.obj); if (farFloor < 0 && t.floor != g_floor && t.kind != TK_GO) farFloor = t.floor; if (t.kind == TK_GO && farFloor < 0) farFloor = t.floor; }
    int h = 36 + (int)objs.size() * 26;
    if (farFloor >= 0) h += 26;
    if (objs.empty()) h = 44;
    rect(W - w - 16, 16, w, h, INK, 215);
    frame(W - w - 16, 16, w, h, SKY, 3);
    const DayDef* d = dayDef(g_day);
    text(fS, "DAY " + std::to_string(g_day) + (d ? "  -  " + d->title : ""), W - w - 2, 18, SKY);
    int yy = 44;
    for (auto& o : objs) { int n = wrapText(fS, "- " + o, W - w - 2, yy, w - 28, WHITE, 24); yy += n * 24; h = std::max(h, yy - 16 + 12); }
    if (farFloor >= 0 && farFloor != g_floor) text(fS, std::string("Go to: ") + FLOORS[farFloor].name, W - w - 2, yy, GOLD);
    if (!g_inv.empty()) {
        std::string s = "Carrying: ";
        for (size_t i = 0; i < g_inv.size(); i++) s += (i ? ", " : "") + g_inv[i];
        int tw = std::min(W - 40, textW(fS, s) + 24);
        rect(16, H - 48, tw, 34, INK, 215);
        frame(16, H - 48, tw, 34, GOLD, 2);
        text(fS, s, 28, H - 44, WHITE);
    }
    if (g_wm == 1) {
        char buf[64];
        snprintf(buf, sizeof buf, "Bots %d/%d    %0.0fs", g_mgCaught, g_mgGoal, std::max(0.0f, g_mgTime));
        rect(W / 2 - 150, 116, 300, 46, INK, 225); frame(W / 2 - 150, 116, 300, 46, GOLD, 3);
        text(fM, buf, W / 2, 118, GOLD, 1);
    }
    if (g_wm == 2) {
        char buf[64];
        snprintf(buf, sizeof buf, "Hornets %d/%d", g_hKilled, g_hGoal);
        rect(W / 2 - 150, 116, 300, 46, INK, 225); frame(W / 2 - 150, 116, 300, 46, GOLD, 3);
        text(fM, buf, W / 2, 118, GOLD, 1);
        for (int i = 0; i < 5; i++) rect(W / 2 - 100 + i * 42, 170, 32, 14, i < g_hHp ? RED : mix(RED, INK, 0.8f));
    }
    if (g_toastT > 0 && !g_toast.empty()) {
        int tw = textW(fS, g_toast) + 40;
        rect(W / 2 - tw / 2, H - 100, tw, 40, INK, 230);
        frame(W / 2 - tw / 2, H - 100, tw, 40, MINT, 2);
        text(fS, g_toast, W / 2, H - 94, WHITE, 1);
    }
}
static void drawTalk() {
    int bx = 80, by = H - 220, bw = W - 160, bh = 190;
    rect(bx, by, bw, bh, INK, 238);
    frame(bx, by, bw, bh, g_cur.col, 4);
    std::string who = speakerName(g_cur.who);
    if (!who.empty()) {
        const NpcDef* n = findNpc(g_cur.who);
        if (n) who += std::string("  ") + n->id;
        rect(bx + 24, by - 22, textW(fM, who) + 36, 40, g_cur.col);
        text(fM, who, bx + 42, by - 20, INK);
    }
    size_t n = std::min((size_t)g_talkChars, g_cur.what.size());
    Col tc = g_cur.who.empty() ? mix(WHITE, SKY, 0.4f) : WHITE;
    wrapText(fM, g_cur.what.substr(0, n), bx + 30, by + 28, bw - 60, tc, 38);
    if (g_state == S_TALK && n >= g_cur.what.size()) {
        if (((int)(g_time * 2)) & 1) text(fS, "A  next", bx + bw - 24, by + bh - 34, g_cur.col, 2);
    }
}
static void drawChoice() {
    drawTalk();
    int n = (int)g_opts.size();
    int ow = 0;
    for (auto& o : g_opts) ow = std::max(ow, textW(fM, o.first) + 80);
    ow = std::max(ow, 360);
    int oh = 54, bx = W - 80 - ow, by = H - 220 - 22 - n * oh - 20;
    rect(bx - 12, by - 12, ow + 24, n * oh + 24, INK, 238);
    frame(bx - 12, by - 12, ow + 24, n * oh + 24, GOLD, 3);
    for (int i = 0; i < n; i++) {
        bool sel = i == g_optSel;
        if (sel) rect(bx, by + i * oh, ow, oh - 6, GOLD);
        text(fM, g_opts[i].first, bx + 18, by + i * oh + 4, sel ? INK : WHITE);
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

// ------------------------------------------------------------------ drawing: blueprint menus
static void bpBackground() {
    rect(0, 0, W, H, BLUEP);
    Col w = WHITE;
    int L = 390, R = 890;
    dashV(L, 20, H - 20, w, 200); dashV(R, 20, H - 20, w, 200);
    for (int y : {28, 176, 190, 262, 596, 690}) dashH(L - 30, R + 30, y, w, 200);
}
static void bpButton(int x, int y, int w, int h, const std::string& label, bool sel, bool dim = false) {
    for (int dy : {-2, h + 1}) dashH(x - 40, x + w + 40, y + dy, WHITE, 170);
    dashV(x - 1, y - 12, y + h + 12, WHITE, 140); dashV(x + w, y - 12, y + h + 12, WHITE, 140);
    if (sel) { rect(x, y, w, h, WHITE); frame(x, y, w, h, WHITE, 5); }
    else frame(x, y, w, h, WHITE, 4);
    Col tc = sel ? BLUEP : WHITE;
    if (dim) tc = mix(tc, BLUEP, 0.45f);
    textSp(fM, label, x + w / 2, y + (h - 34) / 2, tc, 3);
}
static void bpFooter(const std::string& left) {
    int y = 640;
    circleO(900 - 270 + 2, y + 16, 17, WHITE, 3); text(fS, "A", 900 - 270 + 2, y + 2, WHITE, 1);
    text(fS, "Select", 900 - 270 + 28, y + 3, WHITE);
    circleO(900 - 120, y + 16, 17, WHITE, 3); text(fS, "B", 900 - 120, y + 2, WHITE, 1);
    text(fS, "Back", 900 - 120 + 28, y + 3, WHITE);
    if (!left.empty()) text(fS, left, 410, y + 3, mix(WHITE, BLUEP, 0.2f));
}
static void drawTitle() {
    bpBackground();
    textSp(fXL, "\xC3\x8ELOT14", W / 2, 34, WHITE, 16);
    textSp(fL, "THE BIG 14", W / 2, 128, WHITE, 12);
    text(fS, "A story from the Lost of Ashes universe", W / 2, 205, WHITE, 1);
    const char* items[4] = {"New Game", "Save Slots", "Settings", "Exit to Homebrew"};
    for (int i = 0; i < 4; i++) bpButton(440, 270 + i * 80, 400, 62, items[i], i == g_menuSel);
    bpFooter("Version " GAME_VERSION);
    if (g_logo) {
        SDL_Rect d = {70, 190, 230, 345};
        SDL_RenderCopy(g_r, g_logo, nullptr, &d);
    }
    text(fS, "Day 1 to 31", 1090, 560, WHITE, 1, 200);
}
static void drawSlots() {
    bpBackground();
    textSp(fL, g_slotNew ? "NEW GAME" : "SAVE SLOTS", W / 2, 70, WHITE, 12);
    text(fS, g_slotNew ? "Pick a slot for your new story" : "Pick a save to continue", W / 2, 200, WHITE, 1);
    for (int i = 0; i < 3; i++) {
        SlotInfo si = slotInfo(i);
        std::string label = si.used ? "Slot " + std::to_string(i + 1) + "  -  Day " + std::to_string(si.day) : "Slot " + std::to_string(i + 1) + "  -  Empty";
        bpButton(410, 262 + i * 100, 460, 74, label, i == g_slotSel, !si.used && !g_slotNew);
        if (si.used) {
            const DayDef* d = dayDef(si.day);
            if (d) text(fS, d->title, 640, 262 + i * 100 + 76, WHITE, 1, 220);
        }
    }
    bpFooter("X  Erase");
    if (g_toastT > 0) text(fS, g_toast, W / 2, 590, WHITE, 1);
}
static void drawSettings() {
    bpBackground();
    textSp(fL, "SETTINGS", W / 2, 70, WHITE, 12);
    static const char* spd[3] = {"Slow", "Normal", "Fast"};
    std::string a = std::string("Text speed: ") + spd[g_textSpeed];
    std::string b = std::string("Scanlines: ") + (g_scan ? "On" : "Off");
    bpButton(410, 262, 460, 74, a, g_setSel == 0);
    bpButton(410, 362, 460, 74, b, g_setSel == 1);
    bpButton(410, 462, 460, 74, "Back", g_setSel == 2);
    bpFooter("");
}

// ------------------------------------------------------------------ drawing: cards, overlays
static void drawCard() {
    bpBackground();
    textSp(fL, g_card.title, W / 2, 50, WHITE, 8);
    if (!g_card.sub.empty()) text(fM, g_card.sub, W / 2, 130, WHITE, 1);
    int y = 210;
    for (auto& p : g_card.paras) { int n = wrapText(fM, p, W / 2 - 330, y, 660, WHITE, 38, 1); y += n * 38 + 18; }
    if (((int)(g_time * 2)) & 1) text(fS, "A  continue", W / 2, 640, WHITE, 1);
}
static void drawPause() {
    rect(0, 0, W, H, INK, 190);
    rect(W / 2 - 360, 100, 720, 520, INK, 245);
    frame(W / 2 - 360, 100, 720, 520, ORANGE, 4);
    text(fL, "Paused", W / 2, 110, WHITE, 1);
    const DayDef* d = dayDef(g_day);
    text(fM, "Day " + std::to_string(g_day) + (d ? ": " + d->title : ""), W / 2, 190, SKY, 1);
    int y = 240;
    for (auto& t : g_tasks) if (taskActive(t) && !t.obj.empty()) { int n = wrapText(fS, "- " + t.obj, W / 2 - 320, y, 640, WHITE, 26); y += n * 26 + 4; }
    std::string rel = "Friendship:";
    for (const char* k : {"Amine", "Dalia", "Gabriel"}) rel += std::string("  ") + k + " " + std::to_string(flag(std::string("rel_") + k));
    text(fS, rel, W / 2, 420, GOLD, 1);
    text(fM, "A  Resume", W / 2, 470, WHITE, 1);
    text(fM, "X  Save and back to title", W / 2, 515, WHITE, 1);
    text(fM, "Y  Save and quit", W / 2, 560, WHITE, 1);
}
static void drawOverlay() {
    rect(0, 0, W, H, INK, 170);
    rect(200, 90, 880, 560, INK, 248);
    frame(200, 90, 880, 560, ORANGE, 4);
    text(fL, g_ovTitle, W / 2, 96, WHITE, 1);
    if (!g_ovReady) {
        wrapText(fM, g_ovHelp, 260, 250, 760, WHITE, 40, 1);
        if (((int)(g_time * 2)) & 1) text(fM, "A  start      B  not now", W / 2, 520, GOLD, 1);
        return;
    }
    if (g_ov == 1) {
        int base = 520, cx = W / 2;
        const char* names[6] = {"Paper rolls", "Coffee", "Glue", "UHU stic", "Pens", "Rulers"};
        Col cols[6] = {{230, 220, 200}, {150, 100, 70}, {120, 190, 240}, {240, 120, 200}, {90, 200, 130}, {240, 200, 80}};
        person(cx, base + 60, ORANGE, {40, 24, 16}, 1, g_ovT * 6, true);
        for (int i = 0; i < 6; i++) {
            int w = 150 - i * 6;
            int off = (int)(g_tilt * (i * i * 7 + i * 14));
            int y = base - (i + 1) * 40;
            rect(cx - w / 2 + off, y, w, 36, INK);
            rect(cx - w / 2 + off + 3, y + 3, w - 6, 30, cols[i]);
            text(fS, names[i], cx + off, y + 4, INK, 1);
        }
        rect(300, 600, 680, 18, mix(INK, WHITE, 0.2f));
        rect(640 - 3, 596, 6, 26, WHITE);
        int mk = 640 + (int)(g_tilt * 340);
        rect(mk - 8, 592, 16, 34, std::fabs(g_tilt) > 0.7f ? RED : MINT);
        float left = std::max(0.0f, g_ovA - g_ovT);
        char buf[48]; snprintf(buf, sizeof buf, "%0.0fs to go", left);
        text(fM, buf, W - 260, 140, GOLD, 1);
    } else if (g_ov == 2) {
        int cx = W / 2, cy = 370;
        Col pc[4] = {{250, 210, 70}, {90, 220, 140}, {90, 160, 250}, {240, 90, 120}};
        int px[4] = {cx, cx + 120, cx, cx - 120}, py[4] = {cy - 120, cy, cy + 120, cy};
        for (int i = 0; i < 4; i++) {
            bool lit = g_padLit[i] > 0;
            rect(px[i] - 55, py[i] - 55, 110, 110, INK);
            rect(px[i] - 50, py[i] - 50, 100, 100, lit ? pc[i] : mix(pc[i], INK, 0.7f));
        }
        std::string s = g_seqInput ? "Your turn: " + std::to_string(g_seqIdx) + "/" + std::to_string((int)g_seq.size()) : "Watch...";
        text(fM, s, W / 2, 560, GOLD, 1);
    } else if (g_ov == 3) {
        frame(250, 150, 780, 480, mix(WHITE, INK, 0.5f), 3);
        for (auto& h : g_haz) {
            Col c = g_dTheme == 1 ? PINK : g_dTheme == 2 ? Col{150, 100, 70} : g_dTheme == 3 ? Col{110, 200, 120} : GOLD;
            rect((int)(h.x - h.size / 2) - 3, (int)(h.y - h.size / 2) - 3, (int)h.size + 6, (int)h.size + 6, INK);
            rect((int)(h.x - h.size / 2), (int)(h.y - h.size / 2), (int)h.size, (int)h.size, h.kind == 0 ? c : h.kind == 1 ? mix(c, WHITE, 0.4f) : mix(c, RED, 0.4f));
        }
        if (g_dInv <= 0 || ((int)(g_ovT * 20) & 1)) person((int)g_dx, (int)g_dy + 18, ORANGE, {40, 24, 16}, 1, g_ovT * 8, true);
        for (int i = 0; i < 3; i++) rect(270 + i * 40, 160, 30, 14, i < g_dLives ? RED : mix(RED, INK, 0.8f));
        char buf[48]; snprintf(buf, sizeof buf, "%0.0fs", std::max(0.0f, g_ovA - g_ovT));
        text(fM, buf, W - 290, 152, GOLD, 1);
    } else if (g_ov == 4) {
        int bx = 290, by = 360;
        rect(bx - 4, by - 4, 708, 68, INK);
        rect(bx, by, 700, 60, mix(WHITE, INK, 0.75f));
        rect(bx + (int)g_zone, by, (int)g_zoneW, 60, MINT);
        rect(bx + (int)g_cur_x - 5, by - 14, 10, 88, WHITE);
        char buf[48]; snprintf(buf, sizeof buf, "Hits %d/%d     Misses %d/3", g_hits, g_hitsGoal, g_miss);
        text(fM, buf, W / 2, 480, GOLD, 1);
    }
}
static void drawOverlayFx() {
    if (g_scan) for (int y = 0; y < H; y += 4) rect(0, y, W, 1, INK, 26);
    if (g_fadePhase) rect(0, 0, W, H, INK, (int)(255 * std::min(1.0f, g_fade)));
}
static void render() {
    SDL_SetRenderDrawBlendMode(g_r, SDL_BLENDMODE_BLEND);
    switch (g_state) {
        case S_TITLE: drawTitle(); break;
        case S_MENU_SLOTS: drawSlots(); break;
        case S_MENU_SETTINGS: drawSettings(); break;
        case S_CARD: drawCard(); break;
        default:
            drawWorld();
            drawHud();
            if (g_state == S_TALK) drawTalk();
            if (g_state == S_CHOICE) drawChoice();
            if (g_state == S_ELEV) drawElev();
            if (g_state == S_PAUSE) drawPause();
            if (g_state == S_MINI) drawOverlay();
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
    loadStory();
    for (int i = 0; i < NF; i++) buildMap(i);
    std::vector<unsigned char> px((size_t)LOGO_BIG.rawlen);
    mz_ulong n = LOGO_BIG.rawlen;
    if (mz_uncompress(px.data(), &n, LOGO_BIG.z, LOGO_BIG.zlen) == MZ_OK) {
        g_logo = SDL_CreateTexture(g_r, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, LOGO_BIG.w, LOGO_BIG.h);
        SDL_UpdateTexture(g_logo, nullptr, px.data(), LOGO_BIG.w * 4);
        SDL_SetTextureBlendMode(g_logo, SDL_BLENDMODE_BLEND);
    }
    placeBots();
    loadSettings();
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
    mkdir("sdmc:/switch", 0777);
    mkdir("sdmc:/switch/Ilot14", 0777);
    g_saveDir = "sdmc:/switch/Ilot14/";
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
#endif
    SDL_Quit();
    return 0;
}
#endif
