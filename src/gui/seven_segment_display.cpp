#include "bitforge/gui/seven_segment_display.hpp"

#include <QPainter>
#include <QPolygonF>
#include <QFontDatabase>

namespace bitforge::gui {

namespace {

// Bitmask for segments: bit 0=a, 1=b, 2=c, 3=d, 4=e, 5=f, 6=g
constexpr std::uint8_t kDigitPatterns[10] = {
    0b00111111, // 0: a, b, c, d, e, f
    0b00000110, // 1: b, c
    0b01011011, // 2: a, b, d, e, g
    0b01001111, // 3: a, b, c, d, g
    0b01100110, // 4: b, c, f, g
    0b01101101, // 5: a, c, d, f, g
    0b01111101, // 6: a, c, d, e, f, g
    0b00000111, // 7: a, b, c
    0b01111111, // 8: a, b, c, d, e, f, g
    0b01101111  // 9: a, b, c, d, f, g
};

} // namespace

SevenSegmentDisplay::SevenSegmentDisplay(QWidget* parent)
    : QWidget(parent) {
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

void SevenSegmentDisplay::setValue(byte_t val) {
    has_value_ = true;
    value_ = val;
    update();
}

void SevenSegmentDisplay::clearValue() {
    has_value_ = false;
    value_ = 0;
    update();
}

QString SevenSegmentDisplay::displayText() const {
    if (!has_value_) {
        return "---";
    }
    return QString::number(value_);
}

QSize SevenSegmentDisplay::minimumSizeHint() const {
    return QSize(160, 80);
}

QSize SevenSegmentDisplay::sizeHint() const {
    return QSize(220, 96);
}

void SevenSegmentDisplay::paintEvent(QPaintEvent* /* event */) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRect r = rect();

    // 1. Dark bezel background
    painter.setPen(QPen(QColor(44, 44, 48), 1));
    painter.setBrush(QColor(18, 18, 20));
    painter.drawRoundedRect(r.adjusted(1, 1, -1, -1), 6.0, 6.0);

    // 2. Title header
    painter.setFont(QFont("Segoe UI", 8, QFont::Bold));
    painter.setPen(QColor(160, 160, 168));
    painter.drawText(QRect(r.left() + 8, r.top() + 4, r.width() - 16, 16),
                     Qt::AlignLeft | Qt::AlignVCenter, "Numeric Display — Port 0x02");

    // 3. Three 7-segment digits
    const int digit_y = r.top() + 22;
    const int digit_h = r.height() - 42;
    const int total_avail_w = r.width() - 24;
    const int digit_spacing = 8;
    const int digit_w = std::min(36, (total_avail_w - (digit_spacing * 2)) / 3);

    // Center the 3 digits horizontally
    const int total_digits_w = (digit_w * 3) + (digit_spacing * 2);
    const int start_x = r.left() + (r.width() - total_digits_w) / 2;

    int d2 = 0; // Hundreds
    int d1 = 0; // Tens
    int d0 = 0; // Units

    if (has_value_) {
        d2 = (value_ / 100) % 10;
        d1 = (value_ / 10) % 10;
        d0 = value_ % 10;
    }

    const bool d2_active = has_value_ && (value_ >= 100);
    const bool d1_active = has_value_ && (value_ >= 10);
    const bool d0_active = has_value_; // Units digit is always active when has_value_ is true (even for 0)

    drawDigit(painter, start_x, digit_y, digit_w, digit_h, d2, false, d2_active);
    drawDigit(painter, start_x + digit_w + digit_spacing, digit_y, digit_w, digit_h, d1, false, d1_active);
    drawDigit(painter, start_x + (digit_w + digit_spacing) * 2, digit_y, digit_w, digit_h, d0, true, d0_active);

    // 4. Subtitle / readout label
    painter.setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    painter.setPen(has_value_ ? QColor(255, 120, 120) : QColor(100, 100, 108));
    QString sub_text = has_value_ ? QString("Value: %1 (0x%2)")
                                        .arg(value_)
                                        .arg(value_, 2, 16, QLatin1Char('0')).toUpper()
                                  : QString("[No Output Yet]");
    painter.drawText(QRect(r.left() + 8, r.bottom() - 18, r.width() - 16, 16),
                     Qt::AlignCenter, sub_text);
}

void SevenSegmentDisplay::drawDigit(QPainter& painter, int digit_x, int digit_y,
                                    int digit_w, int digit_h, int digit_val,
                                    bool /* show_zero */, bool is_active_digit) {
    const std::uint8_t pattern = is_active_digit && digit_val >= 0 && digit_val <= 9
                                     ? kDigitPatterns[digit_val]
                                     : 0;

    const QColor active_color(255, 59, 48);     // Bright digital red
    const QColor inactive_color(38, 20, 22);    // Faint dimmed outline

    const qreal w = digit_w;
    const qreal h = digit_h;
    const qreal t = std::max(2.5, w * 0.15);    // Segment thickness
    const qreal g = 1.0;                        // Gap between segments

    const qreal x0 = digit_x;
    const qreal y0 = digit_y;
    const qreal half_h = h / 2.0;

    auto drawHorizontal = [&](qreal sx, qreal sy, bool on) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(on ? active_color : inactive_color);
        QPolygonF poly;
        poly << QPointF(sx + t * 0.5 + g, sy)
             << QPointF(sx + w - t * 0.5 - g, sy)
             << QPointF(sx + w - t - g, sy + t)
             << QPointF(sx + t + g, sy + t);
        painter.drawPolygon(poly);
    };

    auto drawMiddle = [&](qreal sx, qreal sy, bool on) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(on ? active_color : inactive_color);
        QPolygonF poly;
        poly << QPointF(sx + t * 0.5 + g, sy)
             << QPointF(sx + w - t * 0.5 - g, sy)
             << QPointF(sx + w - t - g, sy + t * 0.5)
             << QPointF(sx + w - t * 0.5 - g, sy + t)
             << QPointF(sx + t * 0.5 + g, sy + t)
             << QPointF(sx + t + g, sy + t * 0.5);
        painter.drawPolygon(poly);
    };

    auto drawBottom = [&](qreal sx, qreal sy, bool on) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(on ? active_color : inactive_color);
        QPolygonF poly;
        poly << QPointF(sx + t + g, sy)
             << QPointF(sx + w - t - g, sy)
             << QPointF(sx + w - t * 0.5 - g, sy + t)
             << QPointF(sx + t * 0.5 + g, sy + t);
        painter.drawPolygon(poly);
    };

    auto drawVertical = [&](qreal sx, qreal sy, qreal len, bool on) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(on ? active_color : inactive_color);
        QPolygonF poly;
        poly << QPointF(sx, sy + t * 0.5 + g)
             << QPointF(sx + t, sy + t + g)
             << QPointF(sx + t, sy + len - t - g)
             << QPointF(sx, sy + len - t * 0.5 - g);
        painter.drawPolygon(poly);
    };

    // Segment a (top)
    drawHorizontal(x0, y0, (pattern & 0b00000001) != 0);

    // Segment b (top-right)
    drawVertical(x0 + w - t, y0, half_h, (pattern & 0b00000010) != 0);

    // Segment c (bottom-right)
    drawVertical(x0 + w - t, y0 + half_h, half_h, (pattern & 0b00000100) != 0);

    // Segment d (bottom)
    drawBottom(x0, y0 + h - t, (pattern & 0b00001000) != 0);

    // Segment e (bottom-left)
    drawVertical(x0, y0 + half_h, half_h, (pattern & 0b00010000) != 0);

    // Segment f (top-left)
    drawVertical(x0, y0, half_h, (pattern & 0b00100000) != 0);

    // Segment g (middle)
    drawMiddle(x0, y0 + half_h - t * 0.5, (pattern & 0b01000000) != 0);
}

} // namespace bitforge::gui
