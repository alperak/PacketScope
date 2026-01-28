#include "core/PipelineController.hpp"
#include "ui/MainWindow.hpp"

#include <QApplication>
#include <spdlog/spdlog.h>

int main(int argc, char* argv[]) {
    //spdlog::set_level(spdlog::level::debug);

    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}