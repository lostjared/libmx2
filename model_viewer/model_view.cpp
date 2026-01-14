#include "model_view.hpp"
#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent) 
    : QMainWindow(parent), activeProcess(nullptr) {
    settings = new QSettings("MXModel", "ModelViewer", this);
    initWindow();
    loadSettings();
    loadRecentFiles();
}

MainWindow::~MainWindow() {
    saveSettings();
    if (activeProcess && activeProcess->state() != QProcess::NotRunning) {
        activeProcess->kill();
        activeProcess->waitForFinished();
    }
}

void MainWindow::initWindow() {
    setWindowTitle("MXMOD Model Viewer");
    setGeometry(100, 100, 1200, 800);
    setMinimumSize(900, 600);
    setAcceptDrops(true);
    
    setupMenuBar();
    setupToolBar();
    setupCentralWidget();
    setupStatusBar();
    createConnections();
    updateUIState(false);
}

void MainWindow::setupMenuBar() {
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    
    openModelAction = new QAction(style()->standardIcon(QStyle::SP_FileIcon), tr("Open &Model..."), this);
    openModelAction->setShortcut(QKeySequence::Open);
    openModelAction->setStatusTip(tr("Open a model file"));
    fileMenu->addAction(openModelAction);
    
    openTextureAction = new QAction(tr("Open &Texture..."), this);
    openTextureAction->setStatusTip(tr("Open a texture file"));
    fileMenu->addAction(openTextureAction);
    
    openTextureDirAction = new QAction(tr("Open Texture &Directory..."), this);
    openTextureDirAction->setStatusTip(tr("Open a texture directory"));
    fileMenu->addAction(openTextureDirAction);
    
    fileMenu->addSeparator();
    
    clearConsoleAction = new QAction(tr("&Clear Console"), this);
    clearConsoleAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
    fileMenu->addAction(clearConsoleAction);
    
    fileMenu->addSeparator();
    
    exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcut(QKeySequence::Quit);
    fileMenu->addAction(exitAction);
    
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    
    aboutAction = new QAction(tr("&About"), this);
    helpMenu->addAction(aboutAction);
    
    settingsAction = new QAction(tr("&Settings"), this);
    settingsAction->setShortcut(QKeySequence::Preferences);
    helpMenu->addAction(settingsAction);
}

void MainWindow::setupToolBar() {
    QToolBar *toolBar = addToolBar(tr("Main"));
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(24, 24));
    
    toolBar->addAction(openModelAction);
    toolBar->addSeparator();
    
    openButton = new QPushButton(style()->standardIcon(QStyle::SP_MediaPlay), tr("Launch Viewer"), this);
    openButton->setMinimumHeight(32);
    openButton->setStyleSheet("QPushButton { font-weight: bold; padding: 5px 15px; }");
    toolBar->addWidget(openButton);
    
    stopButton = new QPushButton(style()->standardIcon(QStyle::SP_MediaStop), tr("Stop"), this);
    stopButton->setMinimumHeight(32);
    stopButton->setEnabled(false);
    toolBar->addWidget(stopButton);
    
    toolBar->addSeparator();
    
    clearButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogResetButton), tr("Clear"), this);
    clearButton->setMinimumHeight(32);
    toolBar->addWidget(clearButton);
}

void MainWindow::setupCentralWidget() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    QGroupBox *fileGroup = new QGroupBox(tr("Model and Texture Selection"), this);
    QVBoxLayout *fileLayout = new QVBoxLayout(fileGroup);
    
    QLabel *modelLabel = new QLabel(tr("Model File:"), this);
    modelLabel->setStyleSheet("font-weight: bold;");
    fileLayout->addWidget(modelLabel);
    
    QHBoxLayout *modelLayout = new QHBoxLayout();
    modelLineEdit = new QLineEdit(this);
    modelLineEdit->setPlaceholderText(tr("Select a .mxmod or .mxmod.z file..."));
    QPushButton *modelBrowseButton = new QPushButton(style()->standardIcon(QStyle::SP_DirIcon), tr("Browse..."), this);
    modelLayout->addWidget(modelLineEdit, 1);
    modelLayout->addWidget(modelBrowseButton);
    fileLayout->addLayout(modelLayout);
    
    QLabel *textureLabel = new QLabel(tr("Texture File:"), this);
    textureLabel->setStyleSheet("font-weight: bold;");
    fileLayout->addWidget(textureLabel);
    
    QHBoxLayout *textureLayout = new QHBoxLayout();
    textureLineEdit = new QLineEdit(this);
    textureLineEdit->setPlaceholderText(tr("Select a .tex or .png file..."));
    QPushButton *textureBrowseButton = new QPushButton(style()->standardIcon(QStyle::SP_DirIcon), tr("Browse..."), this);
    textureLayout->addWidget(textureLineEdit, 1);
    textureLayout->addWidget(textureBrowseButton);
    fileLayout->addLayout(textureLayout);
    
    QLabel *dirLabel = new QLabel(tr("Texture Directory:"), this);
    dirLabel->setStyleSheet("font-weight: bold;");
    fileLayout->addWidget(dirLabel);
    
    QHBoxLayout *dirLayout = new QHBoxLayout();
    textureDirLineEdit = new QLineEdit(this);
    textureDirLineEdit->setPlaceholderText(tr("Select texture directory..."));
    QPushButton *dirBrowseButton = new QPushButton(style()->standardIcon(QStyle::SP_DirIcon), tr("Browse..."), this);
    dirLayout->addWidget(textureDirLineEdit, 1);
    dirLayout->addWidget(dirBrowseButton);
    fileLayout->addLayout(dirLayout);
    
    mainLayout->addWidget(fileGroup);
    
    QGroupBox *optionsGroup = new QGroupBox(tr("Viewer Options"), this);
    QGridLayout *optionsLayout = new QGridLayout(optionsGroup);
    optionsLayout->setSpacing(8);
    
    QLabel *recentLabel = new QLabel(tr("Recent:"), this);
    recentFilesCombo = new QComboBox(this);
    recentFilesCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    recentFilesCombo->setMinimumWidth(150);
    recentFilesCombo->addItem(tr("-- Select Recent --"));
    QPushButton *removeRecentBtn = new QPushButton(tr("Remove"), this);
    removeRecentBtn->setMaximumWidth(70);
    removeRecentBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    
    QLabel *resLabel = new QLabel(tr("Resolution:"), this);
    resolutionCombo = new QComboBox(this);
    resolutionCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    populateResolutionCombo();
    
    QLabel *fpsLabel = new QLabel(tr("FPS:"), this);
    fpsSpinBox = new QSpinBox(this);
    fpsSpinBox->setRange(30, 240);
    fpsSpinBox->setValue(60);
    fpsSpinBox->setSuffix(" fps");
    fpsSpinBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    fpsSpinBox->setMinimumWidth(100);
    
    optionsLayout->addWidget(recentLabel, 0, 0);
    optionsLayout->addWidget(recentFilesCombo, 0, 1);
    optionsLayout->addWidget(removeRecentBtn, 0, 2);
    optionsLayout->addWidget(resLabel, 0, 3);
    optionsLayout->addWidget(resolutionCombo, 0, 4);
    optionsLayout->addWidget(fpsLabel, 0, 5);
    optionsLayout->addWidget(fpsSpinBox, 0, 6);
    
    optionsLayout->setColumnStretch(1, 2);
    optionsLayout->setColumnStretch(4, 1);
    
    mainLayout->addWidget(optionsGroup);
    
    QGroupBox *outputGroup = new QGroupBox(tr("Process Output"), this);
    QVBoxLayout *outputLayout = new QVBoxLayout(outputGroup);
    
    consoleOutput = new QPlainTextEdit(this);
    consoleOutput->setReadOnly(true);
    consoleOutput->setLineWrapMode(QPlainTextEdit::NoWrap);
    consoleOutput->setMaximumBlockCount(2000);
    consoleOutput->setFont(QFont("Consolas", 9));
    consoleOutput->setStyleSheet("QPlainTextEdit { background-color: #1e1e1e; color: #d4d4d4; }");
    
    outputLayout->addWidget(consoleOutput);
    mainLayout->addWidget(outputGroup, 1);
    
    connect(modelBrowseButton, &QPushButton::clicked, this, &MainWindow::browseModelFile);
    connect(textureBrowseButton, &QPushButton::clicked, this, &MainWindow::browseTextureFile);
    connect(dirBrowseButton, &QPushButton::clicked, this, &MainWindow::browseTextureDirectory);
    connect(removeRecentBtn, &QPushButton::clicked, this, &MainWindow::removeRecentFile);
}

void MainWindow::setupStatusBar() {
    statusLabel = new QLabel(tr("Ready"), this);
    statusBar()->addWidget(statusLabel);
}

void MainWindow::createConnections() {
    connect(openButton, &QPushButton::clicked, this, &MainWindow::openModelViewer);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::stopProcess);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearConsole);
    
    connect(openModelAction, &QAction::triggered, this, &MainWindow::browseModelFile);
    connect(openTextureAction, &QAction::triggered, this, &MainWindow::browseTextureFile);
    connect(openTextureDirAction, &QAction::triggered, this, &MainWindow::browseTextureDirectory);
    connect(clearConsoleAction, &QAction::triggered, this, &MainWindow::clearConsole);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::showSettingsDialog);
    
    connect(recentFilesCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::openRecentModel);
    
    connect(modelLineEdit, &QLineEdit::textChanged, this, &MainWindow::validatePaths);
    connect(textureLineEdit, &QLineEdit::textChanged, this, &MainWindow::validatePaths);
    connect(textureDirLineEdit, &QLineEdit::textChanged, this, &MainWindow::validatePaths);
}

void MainWindow::populateResolutionCombo() {
    resolutionCombo->addItem("1280x720 (HD)");
    resolutionCombo->addItem("1920x1080 (Full HD)");
    resolutionCombo->addItem("2560x1440 (2K)");
    resolutionCombo->addItem("3840x2160 (4K)");
    resolutionCombo->setCurrentIndex(1);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (activeProcess && activeProcess->state() != QProcess::NotRunning) {
        auto reply = QMessageBox::question(this, tr("Viewer Running"),
            tr("The model viewer is still running. Do you want to close it and exit?"),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            activeProcess->kill();
            activeProcess->waitForFinished(3000);
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        event->accept();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    const QMimeData *mimeData = event->mimeData();
    
    if (mimeData->hasUrls()) {
        QList<QUrl> urls = mimeData->urls();
        if (!urls.isEmpty()) {
            QString filePath = urls.first().toLocalFile();
            QFileInfo fileInfo(filePath);
            
            if (fileInfo.suffix() == "mxmod" || filePath.endsWith(".mxmod.z")) {
                modelLineEdit->setText(filePath);
            } else if (fileInfo.suffix() == "tex" || fileInfo.suffix() == "png") {
                textureLineEdit->setText(filePath);
            } else if (fileInfo.isDir()) {
                textureDirLineEdit->setText(filePath);
            }
        }
        event->acceptProposedAction();
    }
}

void MainWindow::browseModelFile() {
    QString lastDir = settings->value("lastModelDir", QDir::homePath()).toString();
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Model File"), lastDir, 
        tr("Model Files (*.mxmod *.mxmod.z);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        modelLineEdit->setText(fileName);
        settings->setValue("lastModelDir", QFileInfo(fileName).absolutePath());
        updateRecentFiles(fileName);
        statusLabel->setText(tr("Model loaded: %1").arg(QFileInfo(fileName).fileName()));
    }
}

void MainWindow::browseTextureFile() {
    QString lastDir = settings->value("lastTextureDir", QDir::homePath()).toString();
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Texture File"), lastDir,
        tr("Texture Files (*.tex *.png *.jpg *.bmp);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        textureLineEdit->setText(fileName);
        settings->setValue("lastTextureDir", QFileInfo(fileName).absolutePath());
        statusLabel->setText(tr("Texture loaded: %1").arg(QFileInfo(fileName).fileName()));
    }
}

void MainWindow::browseTextureDirectory() {
    QString lastDir = settings->value("lastTextureDirPath", QDir::homePath()).toString();
    QString dirName = QFileDialog::getExistingDirectory(this,
        tr("Select Texture Directory"), lastDir,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (!dirName.isEmpty()) {
        textureDirLineEdit->setText(dirName);
        settings->setValue("lastTextureDirPath", dirName);
        statusLabel->setText(tr("Texture directory: %1").arg(QDir(dirName).dirName()));
    }
}

void MainWindow::validatePaths() {
    QString modelPath = modelLineEdit->text();
    QString texturePath = textureLineEdit->text();
    QString textureDir = textureDirLineEdit->text();
    
    bool valid = !modelPath.isEmpty() && !texturePath.isEmpty() && !textureDir.isEmpty();
    
    if (valid) {
        valid = QFileInfo(modelPath).exists() && 
                QFileInfo(texturePath).exists() && 
                QDir(textureDir).exists();
    }
    
    openButton->setEnabled(valid && !activeProcess);
}

void MainWindow::openModelViewer() {
    QString modelPath = modelLineEdit->text();
    QString texturePath = textureLineEdit->text();
    QString textureDir = textureDirLineEdit->text();
    
    QFileInfo modelInfo(modelPath);
    QFileInfo textureInfo(texturePath);
    QDir dir(textureDir);
    
    if (!modelInfo.exists() || !modelInfo.isFile()) {
        QMessageBox::warning(this, tr("Invalid Path"), 
            tr("The model file does not exist or is invalid."));
        return;
    }
    
    if (!textureInfo.exists() || !textureInfo.isFile()) {
        QMessageBox::warning(this, tr("Invalid Path"), 
            tr("The texture file does not exist or is invalid."));
        return;
    }
    
    if (!dir.exists()) {
        QMessageBox::warning(this, tr("Invalid Path"), 
            tr("The texture directory does not exist."));
        return;
    }
    
    activeProcess = new QProcess(this);
    QProcess *process = activeProcess;
    
    consoleOutput->clear();
    consoleOutput->appendPlainText(QString("[%1] Starting model viewer...")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
    
    connect(process, &QProcess::readyReadStandardOutput, [this, process]() {
        QByteArray output = process->readAllStandardOutput();
        QString text = QString::fromLocal8Bit(output).trimmed();
        if (!text.isEmpty()) {
            consoleOutput->appendPlainText(QString("[%1] %2")
                .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                .arg(text));
        }
        QScrollBar *scrollBar = consoleOutput->verticalScrollBar();
        scrollBar->setValue(scrollBar->maximum());
    });
    
    connect(process, &QProcess::readyReadStandardError, [this, process]() {
        QByteArray error = process->readAllStandardError();
        QString text = QString::fromLocal8Bit(error).trimmed();
        if (!text.isEmpty()) {
            QTextCharFormat originalFormat = consoleOutput->currentCharFormat();
            QTextCharFormat errorFormat = originalFormat;
            errorFormat.setForeground(QColor("#ff6b6b"));
            
            consoleOutput->setCurrentCharFormat(errorFormat);
            consoleOutput->appendPlainText(QString("[%1] ERROR: %2")
                .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                .arg(text));
            consoleOutput->setCurrentCharFormat(originalFormat);
        }
        QScrollBar *scrollBar = consoleOutput->verticalScrollBar();
        scrollBar->setValue(scrollBar->maximum());
    });
    
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, process](int exitCode, QProcess::ExitStatus) {
                QString exitMsg;
                if (exitCode != 0) {
                    exitMsg = QString("[%1] Process exited with error code %2")
                        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                        .arg(exitCode);
                    
                    QTextCharFormat originalFormat = consoleOutput->currentCharFormat();
                    QTextCharFormat errorFormat = originalFormat;
                    errorFormat.setForeground(QColor("#ff6b6b"));
                    consoleOutput->setCurrentCharFormat(errorFormat);
                    consoleOutput->appendPlainText(exitMsg);
                    consoleOutput->setCurrentCharFormat(originalFormat);
                    
                    statusLabel->setText(tr("Viewer exited with error"));
                } else {
                    exitMsg = QString("[%1] Process completed successfully")
                        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"));
                    consoleOutput->appendPlainText(exitMsg);
                    statusLabel->setText(tr("Viewer closed"));
                }
                
                if (activeProcess == process) {
                    activeProcess = nullptr;
                }
                process->deleteLater();
                updateUIState(false);
            });
    
    connect(process, &QProcess::errorOccurred, [this, process](QProcess::ProcessError error) {
        QString errorMsg;
        switch (error) {
            case QProcess::FailedToStart:
                errorMsg = "Failed to start. Is gl_mxmod installed?";
                break;
            case QProcess::Crashed:
                errorMsg = "Process crashed";
                break;
            default:
                errorMsg = process->errorString();
        }
        
        QMessageBox::critical(this, tr("Process Error"), 
            tr("Model viewer error: %1").arg(errorMsg));
        
        consoleOutput->appendPlainText(QString("[%1] ERROR: %2")
            .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
            .arg(errorMsg));
        
        if (activeProcess == process) {
            activeProcess = nullptr;
        }
        process->deleteLater();
        updateUIState(false);
        statusLabel->setText(tr("Error: %1").arg(errorMsg));
    });
    
    QStringList arguments;
    arguments << "-m" << modelPath;
    arguments << "-t" << texturePath;
    arguments << "-T" << textureDir;
    
    QString resolution = resolutionCombo->currentText().split(" ").first();
    arguments << "-r" << resolution;
    
    QString appPath = QCoreApplication::applicationDirPath();
    arguments << "-p" << appPath;
    
    consoleOutput->appendPlainText(QString("[%1] Command: %2 %3")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
        .arg(executablePath)
        .arg(arguments.join(" ")));
    consoleOutput->appendPlainText(QString("").leftJustified(60, '-'));
    
    process->start(executablePath, arguments);
    
    if (!process->waitForStarted(3000)) {
        QString errorMsg = tr("Failed to start %1. Please check:\n"
                              "1. The executable exists and is in your PATH, or\n"
                              "2. Set the correct path in Settings (Help → Settings)")
                              .arg(executablePath);
        
        QMessageBox::critical(this, tr("Startup Error"), errorMsg);
        
        consoleOutput->appendPlainText(QString("[%1] FATAL: Failed to start process: %2")
            .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
            .arg(executablePath));
        
        activeProcess = nullptr;
        process->deleteLater();
        statusLabel->setText(tr("Failed to start viewer"));
        return;
    }
    
    updateUIState(true);
    statusLabel->setText(tr("Viewer running..."));
}

void MainWindow::stopProcess() {
    if (activeProcess && activeProcess->state() != QProcess::NotRunning) {
        auto reply = QMessageBox::question(this, tr("Stop Viewer"),
            tr("Are you sure you want to stop the model viewer?"),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            consoleOutput->appendPlainText(QString("[%1] Stopping process...")
                .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
            activeProcess->terminate();
            
            if (!activeProcess->waitForFinished(2000)) {
                activeProcess->kill();
            }
            statusLabel->setText(tr("Viewer stopped"));
        }
    }
}

void MainWindow::clearConsole() {
    consoleOutput->clear();
    statusLabel->setText(tr("Console cleared"));
}

void MainWindow::updateUIState(bool processRunning) {
    openButton->setEnabled(!processRunning);
    stopButton->setEnabled(processRunning);
    modelLineEdit->setEnabled(!processRunning);
    textureLineEdit->setEnabled(!processRunning);
    textureDirLineEdit->setEnabled(!processRunning);
    resolutionCombo->setEnabled(!processRunning);
    fpsSpinBox->setEnabled(!processRunning);
    recentFilesCombo->setEnabled(!processRunning);
    openModelAction->setEnabled(!processRunning);
    openTextureAction->setEnabled(!processRunning);
    openTextureDirAction->setEnabled(!processRunning);
}

void MainWindow::loadSettings() {
    modelLineEdit->setText(settings->value("lastModel", "").toString());
    textureLineEdit->setText(settings->value("lastTexture", "").toString());
    textureDirLineEdit->setText(settings->value("lastTextureDir", "").toString());
    
    executablePath = settings->value("executablePath", "gl_mxmod").toString();
    
    int resIndex = settings->value("resolutionIndex", 1).toInt();
    if (resIndex >= 0 && resIndex < resolutionCombo->count()) {
        resolutionCombo->setCurrentIndex(resIndex);
    }
    
    int fps = settings->value("fpsLimit", 60).toInt();
    fpsSpinBox->setValue(fps);
    
    QSize size = settings->value("windowSize", QSize(1200, 800)).toSize();
    resize(size);
    
    QPoint pos = settings->value("windowPos", QPoint(100, 100)).toPoint();
    move(pos);
}

void MainWindow::saveSettings() {
    settings->setValue("lastModel", modelLineEdit->text());
    settings->setValue("lastTexture", textureLineEdit->text());
    settings->setValue("lastTextureDir", textureDirLineEdit->text());
    settings->setValue("executablePath", executablePath);
    settings->setValue("resolutionIndex", resolutionCombo->currentIndex());
    settings->setValue("fpsLimit", fpsSpinBox->value());
    settings->setValue("windowSize", size());
    settings->setValue("windowPos", pos());
    settings->setValue("recentModels", recentModels);
}

void MainWindow::loadRecentFiles() {
    recentModels = settings->value("recentModels", QStringList()).toStringList();
    recentFilesCombo->clear();
    recentFilesCombo->addItem(tr("-- Select Recent --"));
    
    for (const QString &path : recentModels) {
        QFileInfo info(path);
        if (info.exists()) {
            recentFilesCombo->addItem(info.fileName(), path);
        }
    }
}

void MainWindow::updateRecentFiles(const QString &modelPath) {
    recentModels.removeAll(modelPath);
    recentModels.prepend(modelPath);
    
    while (recentModels.size() > MAX_RECENT_FILES) {
        recentModels.removeLast();
    }
    
    settings->setValue("recentModels", recentModels);
    settings->sync();
    
    loadRecentFiles();
}

void MainWindow::openRecentModel(int index) {
    if (index > 0 && index <= recentModels.size()) {
        QString modelPath = recentFilesCombo->itemData(index).toString();
        if (QFileInfo(modelPath).exists()) {
            modelLineEdit->setText(modelPath);
            statusLabel->setText(tr("Loaded recent: %1")
                .arg(QFileInfo(modelPath).fileName()));
        } else {
            QMessageBox::warning(this, tr("File Not Found"),
                tr("The selected recent file no longer exists."));
            recentModels.removeAll(modelPath);
            loadRecentFiles();
        }
    }
    recentFilesCombo->setCurrentIndex(0);
}

void MainWindow::removeRecentFile() {
    if (recentFilesCombo->currentIndex() > 0) {
        QString modelPath = recentFilesCombo->currentData().toString();
        recentModels.removeAll(modelPath);
        loadRecentFiles();
        statusLabel->setText(tr("Removed from recent files"));
    }
}

void MainWindow::showAboutDialog() {
    QMessageBox::about(this, tr("MXMOD Model Viewer"),
        tr("<h3>MXMOD Model Viewer v1.0</h3>"
           "<p>A modern Qt-based model viewer application for .mxmod files.</p>"
           "<p><b>Features:</b></p>"
           "<ul>"
           "<li>Drag and drop support</li>"
           "<li>Recent files history</li>"
           "<li>Multiple resolution options</li>"
           "<li>Real-time console output</li>"
           "</ul>"
           "<p>© 2026 LostSideDead</p>"));
}

void MainWindow::showSettingsDialog() {
    QDialog settingsDialog(this);
    settingsDialog.setWindowTitle(tr("Settings"));
    settingsDialog.setMinimumWidth(600);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(&settingsDialog);
    
    QGroupBox *execGroup = new QGroupBox(tr("Executable Configuration"), &settingsDialog);
    QVBoxLayout *execLayout = new QVBoxLayout(execGroup);
    
    QLabel *infoLabel = new QLabel(tr("Specify the path to the gl_mxmod executable if it's not in your system PATH:"), &settingsDialog);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #aaa; font-size: 10pt;");
    execLayout->addWidget(infoLabel);
    
    execLayout->addSpacing(10);
    
    QLabel *pathLabel = new QLabel(tr("Executable Path:"), &settingsDialog);
    pathLabel->setStyleSheet("font-weight: bold;");
    execLayout->addWidget(pathLabel);
    
    QHBoxLayout *pathLayout = new QHBoxLayout();
    QLineEdit *execPathEdit = new QLineEdit(&settingsDialog);
    execPathEdit->setText(executablePath);
    execPathEdit->setPlaceholderText(tr("gl_mxmod or /full/path/to/gl_mxmod"));
    
    QPushButton *browseExecBtn = new QPushButton(tr("Browse..."), &settingsDialog);
    browseExecBtn->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
    
    pathLayout->addWidget(execPathEdit, 1);
    pathLayout->addWidget(browseExecBtn);
    execLayout->addLayout(pathLayout);
    
    QLabel *noteLabel = new QLabel(tr("Note: Leave as 'gl_mxmod' to use the system PATH. "
                                       "Use full path if the executable is in a custom location."), &settingsDialog);
    noteLabel->setWordWrap(true);
    noteLabel->setStyleSheet("color: #888; font-size: 9pt; font-style: italic;");
    execLayout->addWidget(noteLabel);
    
    mainLayout->addWidget(execGroup);
    
    QGroupBox *generalGroup = new QGroupBox(tr("General Settings"), &settingsDialog);
    QVBoxLayout *generalLayout = new QVBoxLayout(generalGroup);
    
    QLabel *recentLabel = new QLabel(tr("Maximum recent files: %1").arg(MAX_RECENT_FILES), &settingsDialog);
    generalLayout->addWidget(recentLabel);
    
    QPushButton *clearRecentBtn = new QPushButton(tr("Clear Recent Files History"), &settingsDialog);
    generalLayout->addWidget(clearRecentBtn);
    
    mainLayout->addWidget(generalGroup);
    
    mainLayout->addStretch();
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    QPushButton *saveBtn = new QPushButton(tr("Save"), &settingsDialog);
    saveBtn->setDefault(true);
    saveBtn->setMinimumWidth(100);
    
    QPushButton *cancelBtn = new QPushButton(tr("Cancel"), &settingsDialog);
    cancelBtn->setMinimumWidth(100);
    
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);
    
    connect(browseExecBtn, &QPushButton::clicked, [this, execPathEdit]() {
        QString fileName = QFileDialog::getOpenFileName(this,
            tr("Select gl_mxmod Executable"), 
            QFileInfo(execPathEdit->text()).absolutePath(),
            tr("Executables (gl_mxmod*);;All Files (*)"));
        
        if (!fileName.isEmpty()) {
            execPathEdit->setText(fileName);
        }
    });
    
    connect(clearRecentBtn, &QPushButton::clicked, [this, &settingsDialog]() {
        auto reply = QMessageBox::question(&settingsDialog, tr("Clear Recent Files"),
            tr("Are you sure you want to clear all recent files?"),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            recentModels.clear();
            loadRecentFiles();
            QMessageBox::information(&settingsDialog, tr("Cleared"), 
                tr("Recent files history has been cleared."));
        }
    });
    
    connect(saveBtn, &QPushButton::clicked, [this, execPathEdit, &settingsDialog]() {
        QString newPath = execPathEdit->text().trimmed();
        
        if (newPath.isEmpty()) {
            QMessageBox::warning(&settingsDialog, tr("Invalid Path"),
                tr("Executable path cannot be empty. Using default 'gl_mxmod'."));
            newPath = "gl_mxmod";
        }
        
        if (newPath != "gl_mxmod" && !QFileInfo(newPath).isExecutable() && !QFileInfo(newPath).exists()) {
            auto reply = QMessageBox::question(&settingsDialog, tr("File Not Found"),
                tr("The specified file does not exist or is not executable. Save anyway?"),
                QMessageBox::Yes | QMessageBox::No);
            
            if (reply == QMessageBox::No) {
                return;
            }
        }
        
        executablePath = newPath;
        saveSettings();
        statusLabel->setText(tr("Settings saved successfully"));
        settingsDialog.accept();
    });
    
    connect(cancelBtn, &QPushButton::clicked, &settingsDialog, &QDialog::reject);
    
    settingsDialog.exec();
}
