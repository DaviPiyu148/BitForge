#ifndef BITFORGE_GUI_SEVEN_SEGMENT_DISPLAY_HPP
#define BITFORGE_GUI_SEVEN_SEGMENT_DISPLAY_HPP

#include "bitforge/core/types.hpp"

#include <QWidget>
#include <QString>
#include <cstdint>

namespace bitforge::gui {

// Educational 3-digit seven-segment LED display for Port 0x02 (Numeric Output)
// Represents unsigned 8-bit values (0 to 255) with leading-zero blanking
// and clear distinction between "no output yet" and genuine zero.
class SevenSegmentDisplay : public QWidget {
    Q_OBJECT

public:
    explicit SevenSegmentDisplay(QWidget* parent = nullptr);
    ~SevenSegmentDisplay() override = default;

    void setValue(byte_t val);
    void clearValue();

    [[nodiscard]] bool hasValue() const noexcept { return has_value_; }
    [[nodiscard]] byte_t value() const noexcept { return value_; }
    [[nodiscard]] QString displayText() const;

    [[nodiscard]] QSize minimumSizeHint() const override;
    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void drawDigit(QPainter& painter, int digit_x, int digit_y, int digit_w, int digit_h,
                   int value, bool show_zero, bool is_active_digit);

    bool has_value_{false};
    byte_t value_{0};
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_SEVEN_SEGMENT_DISPLAY_HPP
