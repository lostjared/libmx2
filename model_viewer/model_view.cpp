#include"model_view.hpp"

void MainWindow::initWindow()
{
    setWindowTitle("Model Viewer");
    setGeometry(100, 100, 800, 600);
    setMinimumSize(800, 600);
    setMaximumSize(1600, 1200);
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_QuitOnClose, true);
    setAttribute(Qt::WA_NativeWindow, true);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NoSystemBackground, false);
    setAttribute(Qt::WA_PaintOnScreen, true);
    setAttribute(Qt::WA_InputMethodEnabled, true);
    setAttribute(Qt::WA_InputMethodTransparent, false);
    setAttribute(Qt::WA_WState_Visible, true);
    setAttribute(Qt::WA_WState_Hidden, false);
}