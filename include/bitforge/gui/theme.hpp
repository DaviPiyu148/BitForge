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
// 1. Geometry & Metric Tokens (Canonical Design System Hierarchy)
// -----------------------------------------------------------------

// Canonical Corner Radii Scale
inline constexpr int kRadiusNone = 0;       // Code editor surfaces, dense data grids, tables, memory viewer
inline constexpr int kRadiusSmall = 3;      // Tab close buttons, spinbox arrow subcontrols, compact badges
inline constexpr int kRadiusControl = 4;    // Standard buttons, line edits, combos, spinboxes, tab tops
inline constexpr int kRadiusPanel = 6;      // Group boxes, dock frames, context menus, popup dropdowns
inline constexpr int kRadiusLarge = 8;      // Welcome action cards, tutorial container cards, preview card
inline constexpr int kRadiusPill = 12;      // Status badges, execution pills, rounded indicators

// Control Sizing & Metrics (in pixels)
inline constexpr int kControlHeight = 28;       // Standard interactive control height
inline constexpr int kControlMinHeight = 20;    // Inner content min-height
inline constexpr int kButtonMinHeight = 26;     // Minimum button height
inline constexpr int kIconButtonSize = 22;      // Standard icon-only button size (e.g. Find close)
inline constexpr int kCloseButtonSize = 18;     // Tab close button hit area (18x18)
inline constexpr int kSpinButtonWidth = 18;     // Spinbox arrow subcontrol width

// Layout Spacing & Padding Tokens
inline constexpr int kSpacingTight = 4;         // Inter-control compact spacing
inline constexpr int kSpacingStandard = 8;      // Standard layout spacing
inline constexpr int kSpacingCard = 12;         // Card internal padding
inline constexpr int kSpacingLarge = 16;        // Section/page margins
inline constexpr int kBorderWidth = 1;          // Standard structural border width
inline constexpr int kFocusBorderWidth = 1;     // Standard focus ring border width

// -----------------------------------------------------------------
// 2. Color Palette (Modern Dark Charcoal / Restrained Teal Accent)
// -----------------------------------------------------------------

// Backgrounds
inline const QColor kBgDarkest{0x18, 0x1a, 0x1f};  // #181a1f - Deepest (Menus, Status Bar, Headers)
inline const QColor kBgDark{0x21, 0x25, 0x2b};     // #21252b - Dark (Panels, Gutters, Tab Bars)
inline const QColor kBgMedium{0x28, 0x2c, 0x34};   // #282c34 - Medium (Card backgrounds, Dock widgets)
inline const QColor kBgLight{0x2c, 0x31, 0x3a};    // #2c313a - Elevated (Buttons, inputs)
inline const QColor kBgEditor{0x1e, 0x1e, 0x1e};   // #1e1e1e - Source code editor surface
inline const QColor kBgHighlight{0x3e, 0x44, 0x51};// #3e4451 - Hover / Active selections
inline const QColor kBgSelection{0x26, 0x4f, 0x78};// #264f78 - Text & row selection background
inline const QColor kBgHover{0x3e, 0x44, 0x51};    // #3e4451 - Interactive hover background
inline const QColor kBgPressed{0x1b, 0x1d, 0x23};  // #1b1d23 - Interactive pressed background
inline const QColor kBgDisabled{0x21, 0x25, 0x2b}; // #21252b - Disabled control background

// Borders & Dividers
inline const QColor kBorderDark{0x18, 0x1a, 0x1f};
inline const QColor kBorderMedium{0x3b, 0x40, 0x48}; // Standard structural border
inline const QColor kBorderLight{0x4b, 0x52, 0x63};
inline const QColor kBorderSubtle{0x28, 0x2c, 0x34}; // Subtle inner separator / gridline

// Text & Typography
inline const QColor kTextPrimary{0xdc, 0xdc, 0xdc};   // High-contrast primary text
inline const QColor kTextSecondary{0xab, 0xb2, 0xbf}; // Secondary labels & metadata
inline const QColor kTextMuted{0x7f, 0x84, 0x8e};     // Muted / disabled / placeholder text
inline const QColor kTextBright{0xff, 0xff, 0xff};    // Pure white headers & accents

// Accents
inline const QColor kAccentTeal{0x4e, 0xc9, 0xb0};     // #4ec9b0 - Primary Teal/Cyan accent
inline const QColor kAccentTealHover{0x5c, 0xd8, 0xbe};
inline const QColor kAccentTealPressed{0x3b, 0xa2, 0x8c};
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
// 3. Typography Helpers
// -----------------------------------------------------------------
QFont monoFont(int pointSize = 10, bool bold = false);
QFont uiFont(int pointSize = 9, bool bold = false);

// -----------------------------------------------------------------
// 4. Central Application Stylesheet
// -----------------------------------------------------------------
QString applicationStyleSheet();

// -----------------------------------------------------------------
// 5. Component Style Helpers (Unified Visual Design System)
// -----------------------------------------------------------------
QString primaryButtonStyle();
QString secondaryButtonStyle();
QString dangerButtonStyle();
QString compactButtonStyle();
QString inputStyle(bool has_error = false);
QString infoBannerStyle(const QColor& bg = kBgDark, const QColor& text = kTextSecondary, const QColor& border = kBorderMedium);
QString welcomeActionCardStyle(const QString& customBg = "");

QString stateBadgeStyle(bitforge::MachineState state, bool is_executing, bool is_stale);
QString flagBadgeStyle(bool active);
QString changedRegisterStyle(bool changed);
QString changedMemoryCellStyle();
QString emptyStateStyle(bool compact = false);

} // namespace bitforge::gui::theme

#endif // BITFORGE_GUI_THEME_HPP
