#include<QMainWindow>
#include<QVBoxLayout>
#include<QHBoxLayout>
#include<QLineEdit>
#include<QPushButton>
#include<QLabel>
#include<QFileDialog>
#include<QMessageBox>
#include<QPlainTextEdit>
#include<QGroupBox>
#include <QScrollBar>
#include <QTextCharFormat>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr) : QMainWindow(parent) { initWindow(); }
    ~MainWindow() override = default;
    void initWindow();
private:
    void setupFileSelectionUI();
    void browseModelFile();
    void browseTextureFile();
    void browseTextureDirectory();
    void openModelWithTextures();
    
    QLineEdit *modelLineEdit;
    QLineEdit *textureLineEdit;
    QLineEdit *textureDirLineEdit;
    QPlainTextEdit *consoleOutput;    
};  