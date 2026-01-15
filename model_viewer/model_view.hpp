/*
    Coded by Jared Bruni
    (C) 2026 GPL v3
*/
#ifndef MODEL_VIEW_HPP
#define MODEL_VIEW_HPP

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QGroupBox>
#include <QScrollBar>
#include <QTextCharFormat>
#include <QCloseEvent>
#include <QProcess>
#include <QComboBox>
#include <QSpinBox>
#include <QSettings>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QListWidget>
#include <QSplitter>
#include <QStyle>
#include <QIcon>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void browseModelFile();
    void browseTextureFile();
    void browseTextureDirectory();
    void openModelViewer();
    void stopProcess();
    void clearConsole();
    void openRecentModel(int index);
    void showAboutDialog();
    void showSettingsDialog();
    void loadSettings();
    void saveSettings();
    void updateRecentFiles(const QString &modelPath);
    void removeRecentFile();

private:
    void initWindow();
    void setupMenuBar();
    void setupToolBar();
    void setupCentralWidget();
    void setupStatusBar();
    void createConnections();
    void updateUIState(bool processRunning);
    void validatePaths();
    void loadRecentFiles();
    void populateResolutionCombo();
    
protected:
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    QLineEdit *modelLineEdit;
    QLineEdit *textureLineEdit;
    QLineEdit *textureDirLineEdit;
    QPlainTextEdit *consoleOutput;
    QProcess *activeProcess;
    
    QPushButton *openButton;
    QPushButton *stopButton;
    QPushButton *clearButton;
    
    QComboBox *resolutionCombo;
    QSpinBox *fpsSpinBox;
    QComboBox *recentFilesCombo;
    
    QAction *openModelAction;
    QAction *openTextureAction;
    QAction *openTextureDirAction;
    QAction *clearConsoleAction;
    QAction *exitAction;
    QAction *aboutAction;
    QAction *settingsAction;
    
    QLabel *statusLabel;
    
    QSettings *settings;
    QString executablePath;
    QStringList recentModels;
    
    static constexpr int MAX_RECENT_FILES = 10;
};

#endif