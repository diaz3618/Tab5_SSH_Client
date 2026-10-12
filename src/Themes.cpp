#include "Themes.hpp"

#include <math.h>

namespace {
// Palettes follow each scheme's published terminal (ANSI) mapping.
const Theme kThemes[] = {
    {"classic", "Classic", 0x000000, 0xC0C0C0,
     {0x000000, 0x800000, 0x008000, 0x808000, 0x000080, 0x800080, 0x008080, 0xC0C0C0,
      0x808080, 0xFF0000, 0x00FF00, 0xFFFF00, 0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF}, 0x00A0A0},
    {"tokyo-night", "Tokyo Night", 0x1A1B26, 0xC0CAF5,
     {0x15161E, 0xF7768E, 0x9ECE6A, 0xE0AF68, 0x7AA2F7, 0xBB9AF7, 0x7DCFFF, 0xA9B1D6,
      0x414868, 0xF7768E, 0x9ECE6A, 0xE0AF68, 0x7AA2F7, 0xBB9AF7, 0x7DCFFF, 0xC0CAF5}, 0x7AA2F7},
    {"catppuccin-mocha", "Catppuccin Mocha", 0x1E1E2E, 0xCDD6F4,
     {0x45475A, 0xF38BA8, 0xA6E3A1, 0xF9E2AF, 0x89B4FA, 0xF5C2E7, 0x94E2D5, 0xBAC2DE,
      0x585B70, 0xF38BA8, 0xA6E3A1, 0xF9E2AF, 0x89B4FA, 0xF5C2E7, 0x94E2D5, 0xA6ADC8}, 0xCBA6F7},
    {"catppuccin-latte", "Catppuccin Latte", 0xEFF1F5, 0x4C4F69,
     {0x5C5F77, 0xD20F39, 0x40A02B, 0xDF8E1D, 0x1E66F5, 0xEA76CB, 0x179299, 0xACB0BE,
      0x6C6F85, 0xD20F39, 0x40A02B, 0xDF8E1D, 0x1E66F5, 0xEA76CB, 0x179299, 0xBCC0CC}, 0x8839EF},
    {"dracula", "Dracula", 0x282A36, 0xF8F8F2,
     {0x21222C, 0xFF5555, 0x50FA7B, 0xF1FA8C, 0xBD93F9, 0xFF79C6, 0x8BE9FD, 0xF8F8F2,
      0x6272A4, 0xFF6E6E, 0x69FF94, 0xFFFFA5, 0xD6ACFF, 0xFF92DF, 0xA4FFFF, 0xFFFFFF}, 0xBD93F9},
    {"nord", "Nord", 0x2E3440, 0xD8DEE9,
     {0x3B4252, 0xBF616A, 0xA3BE8C, 0xEBCB8B, 0x81A1C1, 0xB48EAD, 0x88C0D0, 0xE5E9F0,
      0x4C566A, 0xBF616A, 0xA3BE8C, 0xEBCB8B, 0x81A1C1, 0xB48EAD, 0x8FBCBB, 0xECEFF4}, 0x88C0D0},
    {"gruvbox-dark", "Gruvbox Dark", 0x282828, 0xEBDBB2,
     {0x282828, 0xCC241D, 0x98971A, 0xD79921, 0x458588, 0xB16286, 0x689D6A, 0xA89984,
      0x928374, 0xFB4934, 0xB8BB26, 0xFABD2F, 0x83A598, 0xD3869B, 0x8EC07C, 0xEBDBB2}, 0xFABD2F},
    {"solarized-dark", "Solarized Dark", 0x002B36, 0x839496,
     {0x073642, 0xDC322F, 0x859900, 0xB58900, 0x268BD2, 0xD33682, 0x2AA198, 0xEEE8D5,
      0x002B36, 0xCB4B16, 0x586E75, 0x657B83, 0x839496, 0x6C71C4, 0x93A1A1, 0xFDF6E3}, 0x268BD2},
    {"solarized-light", "Solarized Light", 0xFDF6E3, 0x657B83,
     {0x073642, 0xDC322F, 0x859900, 0xB58900, 0x268BD2, 0xD33682, 0x2AA198, 0xEEE8D5,
      0x002B36, 0xCB4B16, 0x586E75, 0x657B83, 0x839496, 0x6C71C4, 0x93A1A1, 0xFDF6E3}, 0x268BD2},
    {"one-dark", "One Dark", 0x282C34, 0xABB2BF,
     {0x282C34, 0xE06C75, 0x98C379, 0xE5C07B, 0x61AFEF, 0xC678DD, 0x56B6C2, 0xABB2BF,
      0x5C6370, 0xE06C75, 0x98C379, 0xE5C07B, 0x61AFEF, 0xC678DD, 0x56B6C2, 0xFFFFFF}, 0x61AFEF},
    {"rose-pine", "Rose Pine", 0x191724, 0xE0DEF4,
     {0x26233A, 0xEB6F92, 0x31748F, 0xF6C177, 0x9CCFD8, 0xC4A7E7, 0xEBBCBA, 0xE0DEF4,
      0x6E6A86, 0xEB6F92, 0x31748F, 0xF6C177, 0x9CCFD8, 0xC4A7E7, 0xEBBCBA, 0xE0DEF4}, 0xC4A7E7},
    {"monokai", "Monokai", 0x272822, 0xF8F8F2,
     {0x272822, 0xF92672, 0xA6E22E, 0xF4BF75, 0x66D9EF, 0xAE81FF, 0xA1EFE4, 0xF8F8F2,
      0x75715E, 0xF92672, 0xA6E22E, 0xF4BF75, 0x66D9EF, 0xAE81FF, 0xA1EFE4, 0xF9F8F5}, 0xA6E22E},
};
constexpr size_t kThemeTotal = sizeof(kThemes) / sizeof(kThemes[0]);
size_t currentIndex = 1;

uint16_t to565(uint32_t rgb)
{
    return static_cast<uint16_t>(((rgb >> 8) & 0xF800) | ((rgb >> 5) & 0x07E0) | ((rgb >> 3) & 0x001F));
}

double luminance(uint32_t c)
{
    auto channel = [](int v) {
        const double s = v / 255.0;
        return s <= 0.03928 ? s / 12.92 : pow((s + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * channel((c >> 16) & 0xFF) + 0.7152 * channel((c >> 8) & 0xFF) + 0.0722 * channel(c & 0xFF);
}

double contrastRatio(uint32_t a, uint32_t b)
{
    const double la = luminance(a);
    const double lb = luminance(b);
    return (max(la, lb) + 0.05) / (min(la, lb) + 0.05);
}

uint32_t mix(uint32_t a, uint32_t b, int percentB);

// Moves a colour toward black (light background) or white (dark background)
// until it reaches the requested contrast ratio against the background.
uint32_t ensureContrast(uint32_t c, uint32_t bg, double minRatio)
{
    const uint32_t target = luminance(bg) > 0.4 ? 0x000000 : 0xFFFFFF;
    for (int pct = 0; pct <= 100; pct += 4) {
        const uint32_t candidate = mix(c, target, pct);
        if (contrastRatio(candidate, bg) >= minRatio) return candidate;
    }
    return target;
}

uint32_t mix(uint32_t a, uint32_t b, int percentB)
{
    auto ch = [&](int shift) -> uint32_t {
        const int ca = (a >> shift) & 0xFF;
        const int cb = (b >> shift) & 0xFF;
        return static_cast<uint32_t>((ca * (100 - percentB) + cb * percentB) / 100) << shift;
    };
    return ch(16) | ch(8) | ch(0);
}
}

UiColors ui;

size_t themeCount() { return kThemeTotal; }
const Theme& themeAt(size_t index) { return kThemes[index < kThemeTotal ? index : 1]; }
const Theme& themeCurrent() { return kThemes[currentIndex]; }

int themeIndexById(const String& id)
{
    for (size_t i = 0; i < kThemeTotal; ++i) {
        if (id == kThemes[i].id) return static_cast<int>(i);
    }
    return -1;
}

void themeApply(const String& id)
{
    int found = themeIndexById(id);
    currentIndex = found >= 0 ? static_cast<size_t>(found) : 1;
    const Theme& t = kThemes[currentIndex];
    // Published terminal palettes (especially the light ones) contain colours that are
    // unreadable as text on their own background, so text colours are nudged to a
    // minimum contrast. Classic keeps its original VGA-style colours.
    const bool adjust = String(t.id) != "classic";
    const bool lightBg = luminance(t.bg) > 0.4;
    auto text = [&](uint32_t c, double minRatio) { return adjust ? ensureContrast(c, t.bg, minRatio) : c; };
    const uint32_t fg = text(t.fg, 8.0);
    const uint32_t accent = text(t.accent, 3.5);
    ui.bg = to565(t.bg);
    ui.fg = to565(fg);
    ui.dim = to565(text(mix(t.bg, fg, 62), 4.8));
    ui.panel = to565(mix(t.bg, fg, 14));
    ui.border = to565(mix(t.bg, fg, lightBg ? 38 : 30));
    ui.sel = to565(mix(t.bg, accent, lightBg ? 22 : 30));
    ui.accent = to565(accent);
    ui.onAccent = to565(contrastRatio(0x000000, accent) >= contrastRatio(0xFFFFFF, accent) ? 0x000000 : 0xFFFFFF);
    ui.good = to565(mix(t.bg, t.ansi[2], 55));
    ui.bad = to565(mix(t.bg, t.ansi[1], 55));
    ui.warn = to565(text(t.ansi[3], 4.5));
    ui.chrome = to565(mix(t.bg, 0x000000, lightBg ? 10 : 22));
    ui.red = to565(text(t.ansi[1], 4.5));
    ui.dangerLine = to565(mix(t.bg, text(t.ansi[1], 4.5), 45));
    ui.green = to565(text(t.ansi[2], 4.5));
    ui.tagOk = to565(mix(t.bg, text(t.ansi[2], 4.5), 22));
    ui.tag = to565(mix(t.bg, fg, 14));
    for (int i = 0; i < 16; ++i) {
        const bool keep = !adjust || i == 0 || i == 8;
        ui.ansi[i] = to565(keep ? t.ansi[i] : ensureContrast(t.ansi[i], t.bg, lightBg ? 4.2 : 3.2));
    }
}
