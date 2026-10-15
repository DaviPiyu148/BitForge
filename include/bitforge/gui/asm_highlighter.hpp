#ifndef BITFORGE_GUI_ASM_HIGHLIGHTER_HPP
#define BITFORGE_GUI_ASM_HIGHLIGHTER_HPP

#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>
#include <vector>

namespace bitforge::gui {

// Pure presentation syntax highlighter for BitForge 8-bit assembly source
class AsmHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit AsmHighlighter(QTextDocument* parent = nullptr);
    ~AsmHighlighter() override = default;

protected:
    void highlightBlock(const QString& text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    std::vector<HighlightingRule> rules_;

    QTextCharFormat mnemonic_format_;
    QTextCharFormat register_format_;
    QTextCharFormat directive_format_;
    QTextCharFormat label_format_;
    QTextCharFormat number_format_;
    QTextCharFormat string_format_;
    QTextCharFormat comment_format_;
};

} // namespace bitforge::gui

#endif // BITFORGE_GUI_ASM_HIGHLIGHTER_HPP
