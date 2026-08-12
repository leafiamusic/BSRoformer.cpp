#pragma once

#include <QMainWindow>
#include <QThread>
#include <QStringList>

class Worker;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onBrowseModel();
    void onBrowseInput();
    void onBrowseOutputDir();
    void onStart();
    void onCancel();
    void onProgress(int percent);
    void onLog(const QString& message);
    void onFinished(const QStringList& outputPaths);
    void onError(const QString& message);

private:
    void populateBackends();
    void saveSettings();
    void loadSettings();

    class Ui;
    Ui* ui_ = nullptr;

    QThread* thread_ = nullptr;
    Worker* worker_ = nullptr;
};
