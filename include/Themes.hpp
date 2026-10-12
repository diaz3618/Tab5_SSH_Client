#pragma once

#include <Arduino.h>

// A terminal/UI colour scheme. Colours are 24-bit 0xRRGGBB and are converted to
// RGB565 once, when a theme is applied. No per-frame work happens for themes.
struct Theme {
    const char* id;
    const char* name;
    uint32_t bg;
    uint32_t fg;
    uint32_t ansi[16]; // 0-7 normal, 8-15 bright
    uint32_t accent;   // selection / focus highlight
};

// Pre-computed RGB565 colours used by every screen.
struct UiColors {
    uint16_t bg;      // screen background
    uint16_t fg;      // primary text
    uint16_t dim;     // secondary text
    uint16_t panel;   // header / button face
    uint16_t border;  // outlines
    uint16_t sel;     // selected row background
    uint16_t accent;  // focus ring / titles
    uint16_t good;    // positive action
    uint16_t bad;     // destructive action
    uint16_t warn;
    uint16_t onAccent; // text on an accent-coloured button
    uint16_t chrome;  // header / rail background (darker than bg)
    uint16_t dangerLine;
    uint16_t red;     // destructive text
    uint16_t green;   // positive text
    uint16_t tagOk;   // background of "ok" tags
    uint16_t tag;     // background of neutral tags
    uint16_t ansi[16];
};

constexpr uint32_t kDefaultFgColor = 256; // terminal "default foreground"
constexpr uint32_t kDefaultBgColor = 257; // terminal "default background"

size_t themeCount();
const Theme& themeAt(size_t index);
int themeIndexById(const String& id);
void themeApply(const String& id);
const Theme& themeCurrent();
extern UiColors ui;
