
#ifndef MODEL_VIEW_HPP
#define MODEL_VIEW_HPP

#include <QAction>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QTextCharFormat>
#include <QToolBar>
#include <QVBoxLayout>

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
    QComboBox *backendCombo;
    QSpinBox *fpsSpinBox;
    QComboBox *recentFilesCombo;
    QCheckBox *compressCheckBox;

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