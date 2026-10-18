#ifndef BITFORGE_GUI_THEME_HPP
#define BITFORGE_GUI_THEME_HPP

#include "bitforge/core/types.hpp"
#include "bitforge/core/cpu_state.hpp"

#include <QString>
#include <QColor>
#include <QFont>
#include <QFontDatabase>

namespace bitforge::gui::theme {

// -----------------------------------------------------------------
// 1. Color Palette (Modern Dark Charcoal / Restrained Teal Accent)
// -----------------------------------------------------------------

// Backgrounds
inline const QColor kBgDarkest{0x18, 0x1a, 0x1f};  // #181a1f - Deepest (Menus, Status Bar, Headers)
inline const QColor kBgDark{0x21, 0x25, 0x2b};     // #21252b - Dark (Panels, Gutters, Tab Bars)
inline const QColor kBgMedium{0x28, 0x2c, 0x34};   // #282c34 - Medium (Card backgrounds, Dock widgets)
inline const QColor kBgLight{0x2c, 0x31, 0x3a};    // #2c313a - Elevated (Buttons, inputs)
inline const QColor kBgEditor{0x1e, 0x1e, 0x1e};   // #1e1e1e - Source code editor surface
inline const QColor kBgHighlight{0x3e, 0x44, 0x51};// #3e4451 - Hover / Active selections

// Borders & Dividers
inline const QColor kBorderDark{0x18, 0x1a, 0x1f};
inline const QColor kBorderMedium{0x3b, 0x40, 0x48}; // Standard structural border
inline const QColor kBorderLight{0x4b, 0x52, 0x63};

// Text & Typography
inline const QColor kTextPrimary{0xdc, 0xdc, 0xdc};   // High-contrast primary text
inline const QColor kTextSecondary{0xab, 0xb2, 0xbf}; // Secondary labels & metadata
inline const QColor kTextMuted{0x6c, 0x75, 0x7d};     // Muted / disabled text
inline const QColor kTextBright{0xff, 0xff, 0xff};    // Pure white headers & accents

// Accents
inline const QColor kAccentTeal{0x4e, 0xc9, 0xb0};     // #4ec9b0 - Primary Teal/Cyan accent
inline const QColor kAccentTealHover{0x5c, 0xd8, 0xbe};
inline const QColor kAccentBlue{0x61, 0xaf, 0xef};     // #61afef - Electric blue (PC arrow, running)
inline const QColor kAccentAmber{0xe5, 0xc0, 0x7b};    // #e5c07b - Warning / Paused / PC highlight

// Runtime Machine States
inline const QColor kStateReady{0x98, 0xc3, 0x79};     // #98c379 - Emerald green
inline const QColor kStateRunning{0x61, 0xaf, 0xef};   // #61afef - Active blue
inline const QColor kStatePaused{0xe5, 0xc0, 0x7b};    // #e5c07b - Amber
inline const QColor kStateHalted{0x7f, 0x84, 0x8e};    // #7f848e - Slate gray
inline const QColor kStateFaulted{0xe0, 0x6c, 0x75};   // #e06c75 - Crimson red
inline const QColor kStateStale{0xd1, 0x9a, 0x66};     // #d19a66 - Warm orange
inline const QColor kStateBreakpoint{0xe0, 0x6c, 0x75};// Crimson red

// Mutation / Changed Value Highlighting (Warm Amber)
inline const QColor kChangedBg{0x3e, 0x35, 0x20};      // Warm dark amber fill
inline const QColor kChangedText{0xff, 0xd5, 0x4f};    // Bright gold text
inline const QColor kChangedBorder{0xb2, 0x89, 0x00};  // Gold accent border

// -----------------------------------------------------------------
// 2. Typography Helpers
// -----------------------------------------------------------------
QFont monoFont(int pointSize = 10, bool bold = false);
QFont uiFont(int pointSize = 9, bool bold = false);

// -----------------------------------------------------------------
// 3. Central Application Stylesheet
// -----------------------------------------------------------------
QString applicationStyleSheet();

// -----------------------------------------------------------------
// 4. Component Style Helpers
// -----------------------------------------------------------------
QString stateBadgeStyle(bitforge::MachineState state, bool is_executing, bool is_stale);
QString flagBadgeStyle(bool active);
QString changedRegisterStyle(bool changed);
QString changedMemoryCellStyle();
QString emptyStateStyle();

} // namespace bitforge::gui::theme

#endif // BITFORGE_GUI_THEME_HPP
