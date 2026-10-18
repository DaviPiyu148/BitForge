#include "bitforge/gui/main_window.hpp"

#include <QApplication>
#include <QIcon>
#include <QStringList>
#include <QTimer>
#include <iostream>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("BitForge");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("BitForge");

    QIcon app_icon(":/icons/bitforge.ico");
    if (app_icon.isNull()) {
        app_icon = QIcon(":/icons/bitforge-256.png");
    }
    app.setWindowIcon(app_icon);

    const QStringList args = app.arguments();

    if (args.contains("--help") || args.contains("-h")) {
        std::cout << "BitForge GUI — 8-Bit Computer Emulator, Assembler, and Workbench\n\n"
                  << "Usage:\n"
                  << "  bitforge_gui [options] [source.asm]\n\n"
                  << "Options:\n"
                  << "  -h, --help        Display this help text\n"
                  << "  -v, --version     Display version information\n"
                  << "  --smoke-test      Run automated headless startup verification and exit\n"
                  << "  -platform <name>  Qt platform plugin (e.g. windows, offscreen, minimal)\n";
        return 0;
    }

    if (args.contains("--version") || args.contains("-v")) {
        std::cout << "BitForge GUI 0.1.0 (Architecture Specification v1.7)\n";
        return 0;
    }

    bitforge::gui::MainWindow window;

    if (args.contains("--smoke-test")) {
        if (!window.controller() || !window.editor() || !window.registersPanel() ||
            !window.devicesPanel() || !window.instructionInspector() ||
            !window.breakpointsPanel() || !window.tracePanel() ||
            !window.memoryViewer() || !window.stackViewer() || !window.watchPanel()) {
            std::cerr << "[BitForge Smoke Test] FAILED: One or more GUI components failed to initialize.\n";
            return 2;
        }

        window.show();
        QTimer::singleShot(200, &app, [&app]() {
            std::cout << "[BitForge Smoke Test] SUCCESS: MainWindow and all panels initialized cleanly.\n" << std::flush;
            app.exit(0);
        });
        return app.exec();
    }

    // If an initial file path argument is provided, load it before display
    window.loadStartupFile(args);

    window.show();

    return app.exec();
}
