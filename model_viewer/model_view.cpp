#include"model_view.hpp"
#include<QProcess>
#include<QMessageBox>
#include<QCoreApplication>
void MainWindow::initWindow() {
    setWindowTitle("Model Viewer");
    setGeometry(100, 100, 800, 600);
    setMinimumSize(800, 600);
    setMaximumSize(1600, 1200);
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);
    setupFileSelectionUI();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (activeProcess && activeProcess->state() != QProcess::NotRunning) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("The model viewer is still running.");
        msgBox.setInformativeText("Please close the model viewer program before exiting this application.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::setupFileSelectionUI() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    QLabel *modelLabel = new QLabel("Model File:", this);
    modelLineEdit = new QLineEdit(this);
    modelLineEdit->setReadOnly(true);
    QPushButton *modelBrowseButton = new QPushButton("Browse", this);
    connect(modelBrowseButton, &QPushButton::clicked, this, &MainWindow::browseModelFile);
    
    QHBoxLayout *modelLayout = new QHBoxLayout();
    modelLayout->addWidget(modelLabel);
    modelLayout->addWidget(modelLineEdit, 1);
    modelLayout->addWidget(modelBrowseButton);
    mainLayout->addLayout(modelLayout);
    
    QLabel *textureLabel = new QLabel("Texture File:", this);
    textureLineEdit = new QLineEdit(this);
    textureLineEdit->setReadOnly(true);
    QPushButton *textureBrowseButton = new QPushButton("Browse", this);
    connect(textureBrowseButton, &QPushButton::clicked, this, &MainWindow::browseTextureFile);
    
    QHBoxLayout *textureLayout = new QHBoxLayout();
    textureLayout->addWidget(textureLabel);
    textureLayout->addWidget(textureLineEdit, 1);
    textureLayout->addWidget(textureBrowseButton);
    mainLayout->addLayout(textureLayout);
    
    QLabel *dirLabel = new QLabel("Texture Directory:", this);
    textureDirLineEdit = new QLineEdit(this);
    textureDirLineEdit->setReadOnly(true);
    QPushButton *dirBrowseButton = new QPushButton("Browse", this);
    connect(dirBrowseButton, &QPushButton::clicked, this, &MainWindow::browseTextureDirectory);
    
    QHBoxLayout *dirLayout = new QHBoxLayout();
    dirLayout->addWidget(dirLabel);
    dirLayout->addWidget(textureDirLineEdit, 1);
    dirLayout->addWidget(dirBrowseButton);
    mainLayout->addLayout(dirLayout);
    
    QGroupBox *outputGroup = new QGroupBox("Process Output", this);
    QVBoxLayout *outputLayout = new QVBoxLayout(outputGroup);
    
    consoleOutput = new QPlainTextEdit(this);
    consoleOutput->setReadOnly(true);
    consoleOutput->setLineWrapMode(QPlainTextEdit::NoWrap);
    consoleOutput->setMaximumBlockCount(1000);
    consoleOutput->setFont(QFont("Consolas", 9));
    
    outputLayout->addWidget(consoleOutput);
    outputLayout->setContentsMargins(5, 5, 5, 5); 
    mainLayout->addWidget(outputGroup, 1);
    openButton = new QPushButton("Open", this);
    openButton->setMinimumHeight(40);
    connect(openButton, &QPushButton::clicked, this, &MainWindow::openModelWithTextures);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(openButton);
    mainLayout->setContentsMargins(10, 10, 10, 10);
}

void MainWindow::browseModelFile() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Model File"), "", tr("Model Files (*.mxmod *.mxmod.z);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        modelLineEdit->setText(fileName);
    }
}

void MainWindow::browseTextureFile() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Texture File"), "", tr("Texture Files (*.tex *.png );;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        textureLineEdit->setText(fileName);
    }
}

void MainWindow::browseTextureDirectory() {
    QString dirName = QFileDialog::getExistingDirectory(this,
        tr("Select Texture Directory"), "",
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (!dirName.isEmpty()) {
        textureDirLineEdit->setText(dirName);
    }
}

void MainWindow::openModelWithTextures() {
    QString modelPath = modelLineEdit->text();
    QString texturePath = textureLineEdit->text();
    QString textureDir = textureDirLineEdit->text();
    
    
    if (modelPath.isEmpty()) {
        QMessageBox::warning(this, tr("Missing Path"), tr("Please select a model file."));
        return;
    }
    
    if (texturePath.isEmpty()) {
        QMessageBox::warning(this, tr("Missing Path"), tr("Please select a texture file."));
        return;
    }
    
    if (textureDir.isEmpty()) {
        QMessageBox::warning(this, tr("Missing Path"), tr("Please select a texture directory."));
        return;
    }
    
    QFileInfo modelInfo(modelPath);
    QFileInfo textureInfo(texturePath);
    QDir dir(textureDir);
    
    if (!modelInfo.exists() || !modelInfo.isFile()) {
        QMessageBox::warning(this, tr("Invalid Path"), tr("The selected model file does not exist."));
        return;
    }
    
    if (!textureInfo.exists() || !textureInfo.isFile()) {
        QMessageBox::warning(this, tr("Invalid Path"), tr("The selected texture file does not exist."));
        return;
    }
    
    if (!dir.exists()) {
        QMessageBox::warning(this, tr("Invalid Path"), tr("The selected texture directory does not exist."));
        return;
    }
    
    activeProcess = new QProcess(this);
    QProcess *process = activeProcess;
    
    consoleOutput->clear();
    consoleOutput->appendPlainText("Starting model viewer...");
    
    connect(process, &QProcess::readyReadStandardOutput, [this, process]() {
        QByteArray output = process->readAllStandardOutput();
        consoleOutput->appendPlainText(QString::fromLocal8Bit(output).trimmed());
        
        QScrollBar *scrollBar = consoleOutput->verticalScrollBar();
        scrollBar->setValue(scrollBar->maximum());
    });
    
    connect(process, &QProcess::readyReadStandardError, [this, process]() {
        QByteArray error = process->readAllStandardError();
        
        QTextCharFormat originalFormat = consoleOutput->currentCharFormat();
        QTextCharFormat errorFormat = originalFormat;
        errorFormat.setForeground(Qt::red);
        
        consoleOutput->setCurrentCharFormat(errorFormat);
        consoleOutput->appendPlainText(QString::fromLocal8Bit(error).trimmed());
        consoleOutput->setCurrentCharFormat(originalFormat);
        
        QScrollBar *scrollBar = consoleOutput->verticalScrollBar();
        scrollBar->setValue(scrollBar->maximum());
    });
    
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, process](int exitCode, QProcess::ExitStatus) {
                if (exitCode != 0) {
                    QMessageBox::warning(this, tr("Process Error"), 
                        tr("The model viewer exited with code %1").arg(exitCode));
                    
                    QTextCharFormat originalFormat = consoleOutput->currentCharFormat();
                    QTextCharFormat errorFormat = originalFormat;
                    errorFormat.setForeground(Qt::red);
                    
                    consoleOutput->setCurrentCharFormat(errorFormat);
                    consoleOutput->appendPlainText(QString("Process exited with error code %1").arg(exitCode));
                    consoleOutput->setCurrentCharFormat(originalFormat);
                } else {
                    consoleOutput->appendPlainText("Process completed successfully.");
                }
                
                if (activeProcess == process) {
                    activeProcess = nullptr;
                }
                process->deleteLater();
                openButton->setEnabled(true);
            });
    
    connect(process, &QProcess::errorOccurred, [this, process](QProcess::ProcessError) {
        QMessageBox::critical(this, tr("Process Error"), 
            tr("Failed to start model viewer: %1").arg(process->errorString()));
        
        if (activeProcess == process) {
            activeProcess = nullptr;
        }
        process->deleteLater();
        openButton->setEnabled(true);
    });
    
    QStringList arguments;
    arguments << "-m" << modelPath;
    arguments << "-t" << texturePath; 
    arguments << "-T" << textureDir;
    arguments << "-r" << "1280x720";
    QString app_path = QCoreApplication::applicationDirPath();
    arguments << "-p" << app_path;
    
    consoleOutput->appendPlainText("Command: gl_mxmod " + arguments.join(" "));
    consoleOutput->appendPlainText("---------------------------------");
    
    process->start("gl_mxmod", arguments);
    
    if (!process->waitForStarted(3000)) {
        QMessageBox::critical(this, tr("Error"), 
            tr("Failed to start the model viewer program. Please check if the application is installed correctly."));
        
        QTextCharFormat originalFormat = consoleOutput->currentCharFormat();
        QTextCharFormat errorFormat = originalFormat;
        errorFormat.setForeground(Qt::red);
        
        consoleOutput->setCurrentCharFormat(errorFormat);
        consoleOutput->appendPlainText("ERROR: Failed to start process. Check if gl_mxmod is installed and in your PATH.");
        consoleOutput->setCurrentCharFormat(originalFormat);
        
        activeProcess = nullptr;
        process->deleteLater();
        return;
    }

    openButton->setEnabled(false);
}
