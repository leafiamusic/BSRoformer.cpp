#include "mainwindow.h"
#include "worker.h"

#include "bs_roformer/inference.h"

#include <QApplication>
#include <QBoxLayout>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QTextEdit>

class MainWindow::Ui {
public:
    QLineEdit* modelPath = nullptr;
    QLineEdit* inputPath = nullptr;
    QLineEdit* outputDir = nullptr;
    QLineEdit* outputBase = nullptr;
    QLineEdit* chunkSize = nullptr;
    QLineEdit* numOverlap = nullptr;
    QComboBox* backend = nullptr;
    QPushButton* startBtn = nullptr;
    QPushButton* cancelBtn = nullptr;
    QProgressBar* progress = nullptr;
    QTextEdit* log = nullptr;
};

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    ui_ = new Ui();

    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QVBoxLayout(central);

    // ---- Inputs group ----
    auto* inputs = new QGroupBox("Files", central);
    auto* inputsForm = new QFormLayout(inputs);

    ui_->modelPath = new QLineEdit(inputs);
    auto* modelBtn = new QPushButton("Browse...", inputs);
    auto* modelRow = new QHBoxLayout();
    modelRow->addWidget(ui_->modelPath);
    modelRow->addWidget(modelBtn);
    inputsForm->addRow("Model (.gguf):", modelRow);

    ui_->inputPath = new QLineEdit(inputs);
    auto* inputBtn = new QPushButton("Browse...", inputs);
    auto* inputRow = new QHBoxLayout();
    inputRow->addWidget(ui_->inputPath);
    inputRow->addWidget(inputBtn);
    inputsForm->addRow("Input audio:", inputRow);

    ui_->outputDir = new QLineEdit(inputs);
    auto* outDirBtn = new QPushButton("Browse...", inputs);
    auto* outDirRow = new QHBoxLayout();
    outDirRow->addWidget(ui_->outputDir);
    outDirRow->addWidget(outDirBtn);
    inputsForm->addRow("Output directory:", outDirRow);

    ui_->outputBase = new QLineEdit(inputs);
    inputsForm->addRow("Output base name:", ui_->outputBase);

    root->addWidget(inputs);

    // ---- Options group ----
    auto* opts = new QGroupBox("Options", central);
    auto* optsForm = new QFormLayout(opts);

    ui_->backend = new QComboBox(opts);
    optsForm->addRow("Compute backend:", ui_->backend);

    ui_->chunkSize = new QLineEdit(opts);
    ui_->chunkSize->setPlaceholderText("model default");
    optsForm->addRow("Chunk size (samples):", ui_->chunkSize);

    ui_->numOverlap = new QLineEdit(opts);
    ui_->numOverlap->setPlaceholderText("model default");
    optsForm->addRow("Overlap:", ui_->numOverlap);

    root->addWidget(opts);

    // ---- Buttons ----
    auto* btnRow = new QHBoxLayout();
    ui_->startBtn = new QPushButton("Start", central);
    ui_->cancelBtn = new QPushButton("Cancel", central);
    ui_->cancelBtn->setEnabled(false);
    btnRow->addWidget(ui_->startBtn);
    btnRow->addWidget(ui_->cancelBtn);
    btnRow->addStretch(1);
    root->addLayout(btnRow);

    // ---- Progress ----
    ui_->progress = new QProgressBar(central);
    ui_->progress->setRange(0, 100);
    ui_->progress->setValue(0);
    root->addWidget(ui_->progress);

    // ---- Log ----
    ui_->log = new QTextEdit(central);
    ui_->log->setReadOnly(true);
    ui_->log->setLineWrapMode(QTextEdit::NoWrap);
    root->addWidget(ui_->log, 1);

    // ---- Connections ----
    connect(modelBtn, &QPushButton::clicked, this, &MainWindow::onBrowseModel);
    connect(inputBtn, &QPushButton::clicked, this, &MainWindow::onBrowseInput);
    connect(outDirBtn, &QPushButton::clicked, this, &MainWindow::onBrowseOutputDir);
    connect(ui_->startBtn, &QPushButton::clicked, this, &MainWindow::onStart);
    connect(ui_->cancelBtn, &QPushButton::clicked, this, &MainWindow::onCancel);

    setWindowTitle("BSRoformer GUI");
    resize(640, 520);

    populateBackends();
    loadSettings();

    // Worker + thread (created lazily in onStart to keep it simple).
    thread_ = nullptr;
    worker_ = nullptr;
}

MainWindow::~MainWindow() {
    saveSettings();
    if (thread_) {
        thread_->quit();
        thread_->wait();
    }
    delete ui_;
}

void MainWindow::populateBackends() {
    ui_->backend->clear();
    auto backends = Inference::ListBackends();
    for (const auto& b : backends) {
        QString label = QString::fromStdString(b.name);
        if (!b.description.empty()) {
            label += " (" + QString::fromStdString(b.description) + ")";
        }
        ui_->backend->addItem(label, QString::fromStdString(b.id));
    }
    // Default to "Auto".
    int idx = ui_->backend->findData("");
    if (idx >= 0) ui_->backend->setCurrentIndex(idx);
}

void MainWindow::loadSettings() {
    QSettings s;
    ui_->modelPath->setText(s.value("modelPath").toString());
    ui_->inputPath->setText(s.value("inputPath").toString());
    ui_->outputDir->setText(s.value("outputDir").toString());
    ui_->outputBase->setText(s.value("outputBase").toString());
    ui_->chunkSize->setText(s.value("chunkSize").toString());
    ui_->numOverlap->setText(s.value("numOverlap").toString());
    QString backend = s.value("backend", "").toString();
    int idx = ui_->backend->findData(backend);
    if (idx >= 0) ui_->backend->setCurrentIndex(idx);
}

void MainWindow::saveSettings() {
    QSettings s;
    s.setValue("modelPath", ui_->modelPath->text());
    s.setValue("inputPath", ui_->inputPath->text());
    s.setValue("outputDir", ui_->outputDir->text());
    s.setValue("outputBase", ui_->outputBase->text());
    s.setValue("chunkSize", ui_->chunkSize->text());
    s.setValue("numOverlap", ui_->numOverlap->text());
    s.setValue("backend", ui_->backend->currentData().toString());
}

void MainWindow::onBrowseModel() {
    QString path = QFileDialog::getOpenFileName(this, "Select model (.gguf)",
                                                ui_->modelPath->text(), "GGUF models (*.gguf)");
    if (!path.isEmpty()) ui_->modelPath->setText(path);
}

void MainWindow::onBrowseInput() {
    QString path = QFileDialog::getOpenFileName(this, "Select input audio",
                                                ui_->inputPath->text(),
                                                "Audio files (*.wav *.mp3 *.flac *.ogg *.m4a *.aac);;All files (*)");
    if (!path.isEmpty()) ui_->inputPath->setText(path);
}

void MainWindow::onBrowseOutputDir() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select output directory",
                                                    ui_->outputDir->text());
    if (!dir.isEmpty()) ui_->outputDir->setText(dir);
}

void MainWindow::onStart() {
    QString model = ui_->modelPath->text().trimmed();
    QString input = ui_->inputPath->text().trimmed();
    QString outDir = ui_->outputDir->text().trimmed();
    QString base = ui_->outputBase->text().trimmed();

    if (model.isEmpty() || input.isEmpty() || outDir.isEmpty()) {
        QMessageBox::warning(this, "Missing inputs",
                             "Please provide a model, an input audio file, and an output directory.");
        return;
    }

    bool ok = false;
    int chunk = -1, overlap = -1;
    QString cs = ui_->chunkSize->text().trimmed();
    QString ov = ui_->numOverlap->text().trimmed();
    if (!cs.isEmpty()) {
        chunk = cs.toInt(&ok);
        if (!ok || chunk <= 0) {
            QMessageBox::warning(this, "Invalid", "Chunk size must be a positive integer or empty.");
            return;
        }
    }
    if (!ov.isEmpty()) {
        overlap = ov.toInt(&ok);
        if (!ok || overlap < 1) {
            QMessageBox::warning(this, "Invalid", "Overlap must be an integer >= 1 or empty.");
            return;
        }
    }

    // Tear down any previous run.
    if (thread_) {
        thread_->quit();
        thread_->wait();
        delete thread_;
        thread_ = nullptr;
    }

    thread_ = new QThread(this);
    worker_ = new Worker();
    worker_->setParams(model, input, outDir, base, chunk, overlap,
                       ui_->backend->currentData().toString());
    worker_->moveToThread(thread_);

    connect(thread_, &QThread::started, worker_, &Worker::run);
    connect(worker_, &Worker::progress, this, &MainWindow::onProgress);
    connect(worker_, &Worker::log, this, &MainWindow::onLog);
    connect(worker_, &Worker::finished, this, &MainWindow::onFinished);
    connect(worker_, &Worker::error, this, &MainWindow::onError);
    connect(worker_, &Worker::finished, thread_, &QThread::quit);
    connect(worker_, &Worker::error, thread_, &QThread::quit);
    connect(thread_, &QThread::finished, worker_, &Worker::deleteLater);
    connect(thread_, &QThread::finished, this, [this]() {
        ui_->startBtn->setEnabled(true);
        ui_->cancelBtn->setEnabled(false);
        thread_ = nullptr;
    });

    ui_->progress->setValue(0);
    ui_->startBtn->setEnabled(false);
    ui_->cancelBtn->setEnabled(true);
    onLog("Starting...");
    thread_->start();
}

void MainWindow::onCancel() {
    if (worker_) {
        worker_->cancel();
        onLog("Cancel requested...");
    }
}

void MainWindow::onProgress(int percent) {
    ui_->progress->setValue(percent);
}

void MainWindow::onLog(const QString& message) {
    ui_->log->append(message);
}

void MainWindow::onFinished(const QStringList& outputPaths) {
    onLog(QString("Done. Wrote %1 file(s).").arg(outputPaths.size()));
    QMessageBox::information(this, "Complete",
                             "Separation finished.\n" + outputPaths.join("\n"));
}

void MainWindow::onError(const QString& message) {
    onLog("Error: " + message);
    QMessageBox::critical(this, "Error", message);
}
