#include "worker.h"

#include "bs_roformer/inference.h"
#include "bs_roformer/audio.h"
#include "bs_roformer/convert.h"

#include <QFileInfo>
#include <QDir>

void Worker::run() {
    try {
        cancel_ = false;

        emit log("Loading audio: " + inputPath_);
        AudioBuffer input_audio = bsroformer::LoadCompatible(inputPath_.toStdString());

        emit log(QString("Loaded: %1 samples, %2 channels, %3 Hz")
                     .arg(input_audio.samples)
                     .arg(input_audio.channels)
                     .arg(input_audio.sampleRate));

        // Auto-expand mono to stereo (mirror cli/main.cpp).
        if (input_audio.channels == 1) {
            emit log("Input is Mono. Expanding to Stereo...");
            std::vector<float> stereo(input_audio.samples * 2);
            for (size_t i = 0; i < input_audio.samples; ++i) {
                stereo[i * 2 + 0] = input_audio.data[i];
                stereo[i * 2 + 1] = input_audio.data[i];
            }
            input_audio.data = std::move(stereo);
            input_audio.channels = 2;
            input_audio.samples *= 2;
        } else if (input_audio.channels != 2) {
            throw std::runtime_error("Input must be Stereo (2ch) or Mono (1ch), got " +
                                     std::to_string(input_audio.channels) + " channels.");
        }

        emit log("Initializing model: " + modelPath_);
        Inference engine(modelPath_.toStdString(), backendId_.toStdString());
        emit log("Using backend: " + QString::fromStdString(engine.GetBackendName()));

        int chunk_size = (chunkSize_ > 0) ? chunkSize_ : engine.GetDefaultChunkSize();
        int num_overlap = (numOverlap_ > 0) ? numOverlap_ : engine.GetDefaultNumOverlap();
        emit log(QString("Processing (chunk_size=%1, overlap=%2)...").arg(chunk_size).arg(num_overlap));

        std::atomic<bool>* cancel_ptr = &cancel_;
        auto progress_cb = [this](float p) {
            emit progress(static_cast<int>(p * 100.0f));
        };
        auto cancel_cb = [cancel_ptr]() { return cancel_ptr->load(); };

        std::vector<std::vector<float>> stems =
            engine.Process(input_audio.data, chunk_size, num_overlap, progress_cb, cancel_cb);

        emit progress(100);
        emit log(QString("Model returned %1 stem(s).").arg(stems.size()));

        QStringList outPaths;
        int num_stems = static_cast<int>(stems.size());
        for (int i = 0; i < num_stems; ++i) {
            QString outPath = makeOutputPath(outputBase_, i, num_stems);
            AudioBuffer out;
            out.data = std::move(stems[i]);
            out.channels = 2;
            out.sampleRate = 44100;
            out.samples = out.data.size();
            AudioFile::Save(outPath.toStdString(), out);
            emit log("Saved: " + outPath);
            outPaths.append(outPath);
        }

        emit finished(outPaths);
    } catch (const std::exception& e) {
        emit error(QString::fromUtf8(e.what()));
    }
}

QString Worker::makeOutputPath(const QString& base, int stemIndex, int numStems) const {
    QDir dir(outputDir_);
    QString baseName = base;
    if (baseName.isEmpty()) baseName = "output";

    if (numStems > 1) {
        baseName += QString("_stem_%1").arg(stemIndex);
    }
    return dir.filePath(baseName + ".wav");
}
