#include "bitforge/gui/asm_highlighter.hpp"

#include <QFont>
#include <QColor>

namespace bitforge::gui {

AsmHighlighter::AsmHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent) {
    // 1. Mnemonics (31 canonical opcodes) - Bright Blue (#569CD6)
    mnemonic_format_.setForeground(QColor(86, 156, 214));
    mnemonic_format_.setFontWeight(QFont::Bold);
    HighlightingRule mnemonic_rule;
    mnemonic_rule.pattern = QRegularExpression(
        "\\b(NOP|HALT|LDI|MOV|LD|ST|LDI16|LDX|STX|INX|DEX|ADD|SUB|INC|DEC|AND|OR|XOR|NOT|CMP|JMP|JZ|JNZ|JC|JNC|CALL|RET|PUSH|POP|IN|OUT)\\b",
        QRegularExpression::CaseInsensitiveOption
    );
    mnemonic_rule.format = mnemonic_format_;
    rules_.push_back(mnemonic_rule);

    // 2. Registers (R0-R7, PC, SP, IX) - Bright Teal (#4EC9B0)
    register_format_.setForeground(QColor(78, 201, 176));
    register_format_.setFontWeight(QFont::Bold);
    HighlightingRule register_rule;
    register_rule.pattern = QRegularExpression(
        "\\b(R[0-7]|PC|SP|IX)\\b",
        QRegularExpression::CaseInsensitiveOption
    );
    register_rule.format = register_format_;
    rules_.push_back(register_rule);

    // 3. Directives (.ORG, .ENTRY, .BYTE, .WORD, .ASCII) - Warm Gold (#E5C07B)
    directive_format_.setForeground(QColor(229, 192, 123));
    directive_format_.setFontWeight(QFont::Bold);
    HighlightingRule directive_rule;
    directive_rule.pattern = QRegularExpression(
        "(\\.(ORG|ENTRY|BYTE|WORD|ASCII))\\b",
        QRegularExpression::CaseInsensitiveOption
    );
    directive_rule.format = directive_format_;
    rules_.push_back(directive_rule);

    // 4. Labels (Identifier ending with colon) - Soft Light Yellow (#DCDCAA)
    label_format_.setForeground(QColor(220, 220, 170));
    label_format_.setFontWeight(QFont::Bold);
    HighlightingRule label_rule;
    label_rule.pattern = QRegularExpression(
        "\\b[A-Za-z_][A-Za-z0-9_]*(?=\\s*:)"
    );
    label_rule.format = label_format_;
    rules_.push_back(label_rule);

    // 5. Numeric literals (Hex, Binary, Decimal) - Soft Mint Green (#B5CEA8)
    number_format_.setForeground(QColor(181, 206, 168));
    HighlightingRule hex_rule;
    hex_rule.pattern = QRegularExpression("\\b0[xX][0-9A-Fa-f]+\\b");
    hex_rule.format = number_format_;
    rules_.push_back(hex_rule);

    HighlightingRule bin_rule;
    bin_rule.pattern = QRegularExpression("\\b0[bB][01]+\\b");
    bin_rule.format = number_format_;
    rules_.push_back(bin_rule);

    HighlightingRule dec_rule;
    dec_rule.pattern = QRegularExpression("\\b\\d+\\b");
    dec_rule.format = number_format_;
    rules_.push_back(dec_rule);

    // 6. Strings ("...") - Warm Salmon (#CE9178)
    string_format_.setForeground(QColor(206, 145, 120));
    HighlightingRule string_rule;
    string_rule.pattern = QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"");
    string_rule.format = string_format_;
    rules_.push_back(string_rule);

    // 7. Comments (;...) - Muted Olive Green (#6A9955)
    comment_format_.setForeground(QColor(106, 153, 85));
    comment_format_.setFontItalic(true);
    HighlightingRule comment_rule;
    comment_rule.pattern = QRegularExpression(";[^\n]*");
    comment_rule.format = comment_format_;
    rules_.push_back(comment_rule);
}

void AsmHighlighter::highlightBlock(const QString& text) {
    for (const auto& rule : rules_) {
        QRegularExpressionMatchIterator match_iter = rule.pattern.globalMatch(text);
        while (match_iter.hasNext()) {
            QRegularExpressionMatch match = match_iter.next();
            setFormat(static_cast<int>(match.capturedStart()),
                      static_cast<int>(match.capturedLength()),
                      rule.format);
        }
    }
}

} // namespace bitforge::gui
