#ifndef BITFORGE_GUI_MEMORY_VIEWER_HPP
#define BITFORGE_GUI_MEMORY_VIEWER_HPP

#include "bitforge/core/types.hpp"
#include "bitforge/core/constants.hpp"

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <vector>

namespace bitforge::gui {

class ExecutionController;

// Read-only, side-effect-free hexadecimal memory inspection viewer with modulo-65536 safe wrapping
class MemoryViewer : public QWidget {
    Q_OBJECT

public:
    explicit MemoryViewer(QWidget* parent = nullptr);
    ~MemoryViewer() override = default;

    void setController(ExecutionController* controller);
    void scrollToAddress(word_t address);
    [[nodiscard]] word_t currentBaseAddress() const noexcept { return base_address_; }

public slots:
    void refresh();

private slots:
    void onGoClicked();
    void onPrevPage();
    void onNextPage();

private:
    void updateTable();

    ExecutionController* controller_{nullptr};
    word_t base_address_{kDefaultEntryAddress}; // 0x0200
    word_t prev_base_address_{0xFFFF};
    std::vector<byte_t> prev_data_;

    QLineEdit* address_input_;
    QPushButton* go_button_;
    QTableWidget* table_;

    static constexpr int kRowsPerPage = 16;
    static constexpr int kBytesPerRow = 16;
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_MEMORY_VIEWER_HPP
