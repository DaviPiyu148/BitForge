#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QBuffer>
#include <QFile>
#include <QDir>
#include <iostream>
#include <vector>
#include <cstdint>

#pragma pack(push, 1)
struct IcoHeader {
    std::uint16_t reserved{0};
    std::uint16_t type{1}; // 1 = ICO
    std::uint16_t count{0};
};

struct IcoEntry {
    std::uint8_t width{0};  // 0 means 256
    std::uint8_t height{0}; // 0 means 256
    std::uint8_t color_count{0};
    std::uint8_t reserved{0};
    std::uint16_t planes{1};
    std::uint16_t bit_count{32};
    std::uint32_t bytes_in_res{0};
    std::uint32_t image_offset{0};
};
#pragma pack(pop)

QImage drawBitForgeIcon(int size) {
    QImage img(size, size, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const qreal s = size / 256.0;

    // Helper to scale points
    auto sc = [s](qreal v) { return v * s; };
    auto scPt = [s](qreal x, qreal y) { return QPointF(x * s, y * s); };
    auto scRect = [s](qreal x, qreal y, qreal w, qreal h) { return QRectF(x * s, y * s, w * s, h * s); };

    // 1. Gold IC Pins
    QLinearGradient goldPin(scPt(0, 0), scPt(256, 256));
    goldPin.setColorAt(0.0, QColor(255, 213, 107));
    goldPin.setColorAt(0.6, QColor(209, 154, 102));
    goldPin.setColorAt(1.0, QColor(176, 123, 62));

    p.setPen(Qt::NoPen);
    p.setBrush(goldPin);

    // Top / Bottom pins
    const qreal pin_w = 16.0;
    const qreal pin_h = 22.0;
    const qreal pin_r = std::max(1.0, 3.0 * s);
    const qreal pin_coords[4] = {58.0, 94.0, 146.0, 182.0};

    for (qreal px : pin_coords) {
        // Top
        p.drawRoundedRect(scRect(px, 6.0, pin_w, pin_h), pin_r, pin_r);
        // Bottom
        p.drawRoundedRect(scRect(px, 228.0, pin_w, pin_h), pin_r, pin_r);
    }
    // Left / Right pins
    for (qreal py : pin_coords) {
        // Left
        p.drawRoundedRect(scRect(6.0, py, pin_h, pin_w), pin_r, pin_r);
        // Right
        p.drawRoundedRect(scRect(228.0, py, pin_h, pin_w), pin_r, pin_r);
    }

    // 2. Main Chip Body
    QLinearGradient chipGrad(scPt(24, 24), scPt(232, 232));
    chipGrad.setColorAt(0.0, QColor(30, 34, 42));
    chipGrad.setColorAt(0.5, QColor(22, 24, 30));
    chipGrad.setColorAt(1.0, QColor(14, 16, 20));

    QLinearGradient rimGrad(scPt(24, 24), scPt(232, 232));
    rimGrad.setColorAt(0.0, QColor(78, 201, 176));
    rimGrad.setColorAt(0.35, QColor(59, 64, 72));
    rimGrad.setColorAt(0.70, QColor(40, 44, 52));
    rimGrad.setColorAt(1.0, QColor(97, 175, 239));

    const qreal chip_r = std::max(2.0, 26.0 * s);
    const qreal pen_w = std::max(1.0, 4.0 * s);
    p.setPen(QPen(rimGrad, pen_w));
    p.setBrush(chipGrad);
    p.drawRoundedRect(scRect(24, 24, 208, 208), chip_r, chip_r);

    // 3. Pin 1 Index Dot
    if (size >= 32) {
        p.setPen(QPen(QColor(62, 68, 81), std::max(1.0, 1.5 * s)));
        p.setBrush(QColor(33, 37, 43));
        p.drawEllipse(scPt(48, 48), sc(6), sc(6));
    }

    // 4. PCB Circuit Traces (for medium to large sizes)
    if (size >= 48) {
        p.setPen(QPen(QColor(40, 44, 52), std::max(1.0, 2.0 * s), Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);

        QPainterPath traces;
        traces.moveTo(scPt(66, 32)); traces.lineTo(scPt(66, 56)); traces.lineTo(scPt(86, 76));
        traces.moveTo(scPt(190, 32)); traces.lineTo(scPt(190, 56)); traces.lineTo(scPt(170, 76));
        traces.moveTo(scPt(32, 66)); traces.lineTo(scPt(56, 66)); traces.lineTo(scPt(76, 86));
        traces.moveTo(scPt(224, 66)); traces.lineTo(scPt(200, 66)); traces.lineTo(scPt(180, 86));
        traces.moveTo(scPt(32, 190)); traces.lineTo(scPt(56, 190)); traces.lineTo(scPt(76, 170));
        traces.moveTo(scPt(224, 190)); traces.lineTo(scPt(200, 190)); traces.lineTo(scPt(180, 170));
        traces.moveTo(scPt(66, 224)); traces.lineTo(scPt(66, 200)); traces.lineTo(scPt(86, 180));
        traces.moveTo(scPt(190, 224)); traces.lineTo(scPt(190, 200)); traces.lineTo(scPt(170, 180));
        p.drawPath(traces);
    }

    // 5. Central CPU Die
    QLinearGradient coreGrad(scPt(58, 58), scPt(58, 198));
    coreGrad.setColorAt(0.0, QColor(33, 37, 43));
    coreGrad.setColorAt(1.0, QColor(18, 20, 24));

    p.setPen(QPen(QColor(62, 68, 81), std::max(1.0, 2.0 * s)));
    p.setBrush(coreGrad);
    p.drawRoundedRect(scRect(58, 58, 140, 140), sc(14), sc(14));

    // Inner subtle teal border
    if (size >= 32) {
        p.setPen(QPen(QColor(78, 201, 176, 90), std::max(1.0, 1.0 * s)));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(scRect(64, 64, 128, 128), sc(10), sc(10));
    }

    // 6. Stylized Anvil Base (for size >= 32)
    if (size >= 32) {
        QPainterPath anvil;
        anvil.moveTo(scPt(82, 160));
        anvil.lineTo(scPt(174, 160));
        anvil.lineTo(scPt(166, 150));
        anvil.lineTo(scPt(150, 146));
        anvil.lineTo(scPt(106, 146));
        anvil.lineTo(scPt(90, 150));
        anvil.closeSubpath();

        p.setPen(QPen(QColor(78, 201, 176, 180), std::max(1.0, 1.2 * s)));
        p.setBrush(QColor(44, 49, 58));
        p.drawPath(anvil);
    }

    // 7. Dynamic Forge Lightning / Spark Glyph (Bold & High Contrast)
    QLinearGradient sparkGrad(scPt(100, 70), scPt(156, 156));
    sparkGrad.setColorAt(0.0, QColor(255, 213, 107)); // Gold energy top
    sparkGrad.setColorAt(0.35, QColor(78, 201, 176)); // Vibrant teal
    sparkGrad.setColorAt(0.85, QColor(97, 175, 239)); // Electric cyan
    sparkGrad.setColorAt(1.0, QColor(0, 173, 181));

    QPainterPath spark;
    if (size <= 24) {
        // High visibility bold shape at small sizes
        spark.moveTo(scPt(138, 66));
        spark.lineTo(scPt(98, 124));
        spark.lineTo(scPt(126, 124));
        spark.lineTo(scPt(114, 168));
        spark.lineTo(scPt(160, 114));
        spark.lineTo(scPt(130, 114));
        spark.closeSubpath();
    } else {
        spark.moveTo(scPt(138, 72));
        spark.lineTo(scPt(102, 118));
        spark.lineTo(scPt(128, 118));
        spark.lineTo(scPt(116, 158));
        spark.lineTo(scPt(156, 108));
        spark.lineTo(scPt(130, 108));
        spark.closeSubpath();
    }

    p.setPen(QPen(QColor(255, 220, 120), std::max(1.0, 1.5 * s), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(sparkGrad);
    p.drawPath(spark);

    // 8. 8-Bit Bus LED Row (for size >= 48)
    if (size >= 48) {
        const bool bits[8] = {true, false, true, true, false, true, false, true};
        const qreal led_r = std::max(1.5, 3.5 * s);
        for (int b = 0; b < 8; ++b) {
            qreal lx = 74.0 + b * 15.0;
            p.setPen(Qt::NoPen);
            p.setBrush(bits[b] ? QColor(78, 201, 176) : QColor(40, 44, 52));
            p.drawEllipse(scPt(lx, 174), led_r, led_r);
        }
    }

    return img;
}

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    const std::vector<int> sizes = {256, 128, 64, 48, 32, 24, 16};
    const QString out_dir = "resources/icons";
    QDir().mkpath(out_dir);

    std::vector<QByteArray> png_buffers;
    png_buffers.reserve(sizes.size());

    for (int sz : sizes) {
        QImage img = drawBitForgeIcon(sz);
        QString png_path = QString("%1/bitforge-%2.png").arg(out_dir).arg(sz);
        img.save(png_path, "PNG");
        std::cout << "Generated " << png_path.toStdString() << " (" << sz << "x" << sz << ")\n";

        QByteArray buf;
        QBuffer qbuf(&buf);
        qbuf.open(QIODevice::WriteOnly);
        img.save(&qbuf, "PNG");
        png_buffers.push_back(buf);
    }

    // Default 256px png copy for general icon usage
    QFile::copy(out_dir + "/bitforge-256.png", out_dir + "/bitforge.png");

    // Generate crisp, prominent, highly visible tab close icons
    auto drawCloseIcon = [](int state) { // 0 = normal, 1 = hover, 2 = pressed
        const int s = 18;
        QImage img(s, s, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::transparent);
        QPainter p(&img);
        p.setRenderHint(QPainter::Antialiasing, true);

        if (state == 1) { // hover
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(62, 68, 81));
            p.drawRoundedRect(QRectF(0.5, 0.5, 17.0, 17.0), 3.5, 3.5);
        } else if (state == 2) { // pressed
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(224, 108, 117));
            p.drawRoundedRect(QRectF(0.5, 0.5, 17.0, 17.0), 3.5, 3.5);
        }

        // Bold, bright, high-contrast, large cross
        QColor col = (state == 0) ? QColor(230, 235, 242) : QColor(255, 255, 255);
        p.setPen(QPen(col, 2.5, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(3.0, 3.0), QPointF(15.0, 15.0));
        p.drawLine(QPointF(15.0, 3.0), QPointF(3.0, 15.0));
        return img;
    };
    drawCloseIcon(0).save(out_dir + "/close-tab.png", "PNG");
    drawCloseIcon(1).save(out_dir + "/close-tab-hover.png", "PNG");
    drawCloseIcon(2).save(out_dir + "/close-tab-pressed.png", "PNG");
    std::cout << "Generated tab close icons: close-tab.png, close-tab-hover.png, close-tab-pressed.png\n";

    // Generate crisp spinbox arrow icons
    auto drawSpinArrow = [](bool up, bool hover) {
        QImage img(10, 8, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::transparent);
        QPainter p(&img);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(Qt::NoPen);
        QColor col = hover ? QColor(78, 201, 176) : QColor(171, 178, 191);
        p.setBrush(col);

        QPolygonF poly;
        if (up) {
            poly << QPointF(5.0, 1.5) << QPointF(1.5, 6.5) << QPointF(8.5, 6.5);
        } else {
            poly << QPointF(1.5, 1.5) << QPointF(8.5, 1.5) << QPointF(5.0, 6.5);
        }
        p.drawPolygon(poly);
        return img;
    };
    drawSpinArrow(true, false).save(out_dir + "/spin-up.png", "PNG");
    drawSpinArrow(true, true).save(out_dir + "/spin-up-hover.png", "PNG");
    drawSpinArrow(false, false).save(out_dir + "/spin-down.png", "PNG");
    drawSpinArrow(false, true).save(out_dir + "/spin-down-hover.png", "PNG");
    std::cout << "Generated spinbox arrows: spin-up.png, spin-down.png\n";

    // Build multi-resolution Windows .ico
    const QString ico_path = out_dir + "/bitforge.ico";
    QFile ico_file(ico_path);
    if (!ico_file.open(QIODevice::WriteOnly)) {
        std::cerr << "Failed to open " << ico_path.toStdString() << " for writing!\n";
        return 1;
    }

    IcoHeader header;
    header.reserved = 0;
    header.type = 1;
    header.count = static_cast<std::uint16_t>(sizes.size());
    ico_file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // Calculate initial data offset
    std::uint32_t current_offset = sizeof(IcoHeader) + sizeof(IcoEntry) * static_cast<std::uint32_t>(sizes.size());

    for (std::size_t i = 0; i < sizes.size(); ++i) {
        int sz = sizes[i];
        IcoEntry entry;
        entry.width = (sz == 256) ? 0 : static_cast<std::uint8_t>(sz);
        entry.height = (sz == 256) ? 0 : static_cast<std::uint8_t>(sz);
        entry.color_count = 0;
        entry.reserved = 0;
        entry.planes = 1;
        entry.bit_count = 32;
        entry.bytes_in_res = static_cast<std::uint32_t>(png_buffers[i].size());
        entry.image_offset = current_offset;

        ico_file.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
        current_offset += entry.bytes_in_res;
    }

    // Write PNG data payloads
    for (const auto& buf : png_buffers) {
        ico_file.write(buf);
    }

    ico_file.close();
    std::cout << "Successfully created multi-resolution Windows ICO: " << ico_path.toStdString() << " (" << sizes.size() << " resolutions: 256, 128, 64, 48, 32, 24, 16)\n";

    return 0;
}
