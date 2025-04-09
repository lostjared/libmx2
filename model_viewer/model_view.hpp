#include<QMainWindow>


class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr) : QMainWindow(parent) { initWindow(); }
    ~MainWindow() override = default;
    void initWindow();
};  