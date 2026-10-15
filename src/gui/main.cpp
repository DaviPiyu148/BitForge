#include "bitforge/gui/main_window.hpp"

#include <QApplication>
#include <QStringList>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("BitForge");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("BitForge");

    bitforge::gui::MainWindow window;

    // If an initial file path argument is provided, load it before display
    window.loadStartupFile(app.arguments());

    window.show();

    return app.exec();
}
