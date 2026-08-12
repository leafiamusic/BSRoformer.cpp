#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <atomic>
#include <string>
#include <vector>

class Worker : public QObject {
    Q_OBJECT

public:
    explicit Worker(QObject* parent = nullptr) : QObject(parent) {}

    // Configure the job before starting the thread.
    void setParams(const QString& modelPath,
                   const QString& inputPath,
                   const QString& outputDir,
                   const QString& outputBase,
                   int chunkSize,       // -1 => model default
                   int numOverlap,      // -1 => model default
                   const QString& backendId)
    {
        modelPath_ = modelPath;
        inputPath_ = inputPath;
        outputDir_ = outputDir;
        outputBase_ = outputBase;
        chunkSize_ = chunkSize;
        numOverlap_ = numOverlap;
        backendId_ = backendId;
    }

public slots:
    void run();
    void cancel() { cancel_ = true; }

signals:
    void progress(int percent);
    void log(const QString& message);
    void finished(const QStringList& outputPaths);
    void error(const QString& message);

private:
    QString makeOutputPath(const QString& base, int stemIndex, int numStems) const;

    QString modelPath_;
    QString inputPath_;
    QString outputDir_;
    QString outputBase_;
    int chunkSize_ = -1;
    int numOverlap_ = -1;
    QString backendId_;
    std::atomic<bool> cancel_{false};
};
