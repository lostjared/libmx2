/*
    Coded by Jared Bruni
    (C) 2026 GPL v3
*/
#include "model_view.hpp"
#include <QApplication>
#include <QFile>

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    
    app.setApplicationName("Professional Model Viewer");
    app.setOrganizationName("MXModel");
    app.setOrganizationDomain("mxmodel.io");
    app.setApplicationVersion("1.0.0");
    
    // Load stylesheet from embedded resource
    QFile styleFile(":/styles/data/stylesheet.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = QLatin1String(styleFile.readAll());
        app.setStyleSheet(style);
        styleFile.close();
    }
    
    MainWindow viewer;
    viewer.show();
    
    return app.exec();
}