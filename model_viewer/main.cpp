#include "model_view.hpp"
#include <QApplication>

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    
    app.setApplicationName("Professional Model Viewer");
    app.setOrganizationName("MXModel");
    app.setOrganizationDomain("mxmodel.io");
    app.setApplicationVersion("1.0.0");
    
    app.setStyle("Fusion");
    
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(35, 35, 35));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);
    
    app.setPalette(darkPalette);
    app.setStyleSheet(
        "QToolTip { color: #ffffff; background-color: #2b2b2b; border: 1px solid #767676; }"
        "QGroupBox { font-weight: bold; border: 2px solid #555; border-radius: 5px; margin-top: 10px; padding-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 5px; }"
        "QPushButton { padding: 8px 16px; border: 1px solid #555; border-radius: 4px; background-color: #454545; }"
        "QPushButton:hover { background-color: #505050; border: 1px solid #666; }"
        "QPushButton:pressed { background-color: #3a3a3a; }"
        "QPushButton:disabled { background-color: #353535; color: #666; }"
        "QLineEdit { padding: 6px; border: 1px solid #555; border-radius: 3px; background-color: #3a3a3a; }"
        "QLineEdit:focus { border: 1px solid #2a82da; }"
        "QComboBox { padding: 6px; border: 1px solid #555; border-radius: 3px; background-color: #3a3a3a; }"
        "QSpinBox { padding: 6px; border: 1px solid #555; border-radius: 3px; background-color: #3a3a3a; }"
    );
    
    MainWindow viewer;
    viewer.show();
    
    return app.exec();
}