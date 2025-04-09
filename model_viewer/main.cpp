#include"model_view.hpp"
#include<QApplication>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    MainWindow viewer;
    viewer.show();
    return app.exec();
}