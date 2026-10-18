#ifndef BITFORGE_GUI_WATCH_PANEL_HPP
#define BITFORGE_GUI_WATCH_PANEL_HPP

#include "bitforge/core/types.hpp"

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QString>
#include <QStringList>
#include <vector>
#include <optional>
#include <cstdint>

namespace bitforge::gui {

class ExecutionController;

enum class WatchType {
    Register8,   // R0, R1, R2, R3
    Register16,  // IX, PC, SP
    Flag,        // Z, C, N, V
    MemoryByte   // Memory byte at 16-bit address
};

struct WatchItem {
    WatchType type{WatchType::Register8};
    QString expression{};   // Raw entered text, e.g. "R0", "[0x8000]"
    QString displayName{};  // Clean label for column 0
    int reg_index{-1};      // 0..3 for R0-R3
    word_t address{0};      // for MemoryByte
    QString flag_name{};    // "Z", "C", "N", "V"
};

// Dedicated dockable watch panel for registers, flags, and memory addresses
class WatchPanel : public QWidget {
    Q_OBJECT

public:
    explicit WatchPanel(QWidget* parent = nullptr);
    ~WatchPanel() override = default;

    void setController(ExecutionController* controller);

    bool addWatch(const QString& expression);
    void addRegisterWatch(const QString& regName);
    void addMemoryWatch(word_t address);
    void addFlagWatch(const QString& flagName);
    void removeWatch(int index);
    void clearWatches();

    [[nodiscard]] QStringList watchExpressions() const;
    void loadWatchExpressions(const QStringList& expressions);
    [[nodiscard]] std::size_t watchCount() const noexcept { return watches_.size(); }

    [[nodiscard]] QTableWidget* table() const noexcept { return table_; }
    [[nodiscard]] QLabel* emptyLabel() const noexcept { return empty_label_; }
    [[nodiscard]] QLineEdit* inputField() const noexcept { return expr_input_; }
    [[nodiscard]] QPushButton* addButton() const noexcept { return add_btn_; }
    [[nodiscard]] QPushButton* removeButton() const noexcept { return remove_btn_; }
    [[nodiscard]] QPushButton* clearButton() const noexcept { return clear_btn_; }

public slots:
    void refresh();

private slots:
    void onAddClicked();
    void onRemoveClicked();
    void onClearClicked();
    void onCustomContextMenu(const QPoint& pos);

private:
    std::optional<WatchItem> parseExpression(const QString& expr) const;
    void updateEmptyState();

    ExecutionController* controller_{nullptr};
    std::vector<WatchItem> watches_{};
    std::vector<std::uint32_t> last_step_values_{};
    std::vector<std::uint32_t> current_step_values_{};
    std::uint64_t last_step_count_{0};

    QLineEdit* expr_input_{nullptr};
    QPushButton* add_btn_{nullptr};
    QPushButton* remove_btn_{nullptr};
    QPushButton* clear_btn_{nullptr};
    QTableWidget* table_{nullptr};
    QLabel* empty_label_{nullptr};
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_WATCH_PANEL_HPP
