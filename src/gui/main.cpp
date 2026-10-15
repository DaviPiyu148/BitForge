#include "bitforge/gui/main_window.hpp"

#include <QApplication>
#include <QStringList>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("BitForge");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("BitForge");

    bitforge::gui::MainWindow window;
    window.show();

    // If an initial file path argument is provided, load it
    const QStringList args = app.arguments();
    if (args.size() > 1 && !args[1].startsWith("-")) {
        window.loadFile(args[1]);
    }

    return app.exec();
}
