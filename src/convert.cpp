#include "bs_roformer/convert.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

#include <filesystem>

namespace fs = std::filesystem;

namespace bsroformer {

namespace {

// Quote a path for safe use inside a shell command.
std::string ShellQuote(const std::string& s) {
#if defined(_WIN32)
    // Windows: wrap in double quotes, escape existing quotes.
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else out += c;
    }
    out += "\"";
    return out;
#else
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    out += "'";
    return out;
#endif
}

// Run a command, capturing stderr. Returns ffmpeg exit code and its stderr.
int RunCapture(const std::string& cmd, std::string& stderr_out) {
    std::string full = cmd + " 2>&1";
    FILE* pipe = popen(full.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("Failed to launch ffmpeg subprocess (popen failed).");
    }
    char buf[1024];
    while (fgets(buf, sizeof(buf), pipe) != nullptr) {
        stderr_out += buf;
    }
    int status = pclose(pipe);
#if defined(_WIN32)
    return status;
#else
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return -1;
#endif
}

} // namespace

std::string ConvertToCompatibleWav(const std::string& input, const std::string& temp_dir) {
    if (!fs::exists(input)) {
        throw std::runtime_error("Input file does not exist: " + input);
    }

    fs::path dir = temp_dir;
    if (dir.empty() || !fs::is_directory(dir)) {
        dir = fs::temp_directory_path();
    }
    fs::path tmp = dir / (fs::path(input).stem().string() + "_bsr_tmp.wav");

    // pcm_f32le matches the float32 loading path used by AudioFile (dr_wav).
    std::string cmd = "ffmpeg -y -i " + ShellQuote(input) +
                      " -ar 44100 -ac 2 -c:a pcm_f32le " + ShellQuote(tmp.string());

    std::string err;
    int rc = RunCapture(cmd, err);
    if (rc != 0) {
        std::string msg = "ffmpeg failed (exit " + std::to_string(rc) + ") converting input to WAV.";
        if (!err.empty()) {
            msg += "\nffmpeg output:\n" + err;
        } else {
            msg += "\nIs ffmpeg installed and on your PATH?";
        }
        // Clean up partial output if any.
        std::error_code ec;
        fs::remove(tmp, ec);
        throw std::runtime_error(msg);
    }

    if (!fs::exists(tmp)) {
        throw std::runtime_error("ffmpeg did not produce the expected output file: " + tmp.string());
    }

    return tmp.string();
}

AudioBuffer LoadCompatible(const std::string& path) {
    // Fast path: try loading directly. Only accept 44100 Hz mono/stereo.
    try {
        AudioBuffer buf = AudioFile::Load(path);
        if (buf.sampleRate == 44100 && (buf.channels == 1 || buf.channels == 2)) {
            return buf;
        }
        // Wrong sample rate or channel count: fall through to conversion.
    } catch (const std::exception&) {
        // Not a directly loadable WAV (e.g. MP3/FLAC): fall through.
    }

    std::string tmp = ConvertToCompatibleWav(path, fs::temp_directory_path().string());
    try {
        AudioBuffer buf = AudioFile::Load(tmp);
        std::error_code ec;
        fs::remove(tmp, ec);
        return buf;
    } catch (...) {
        std::error_code ec;
        fs::remove(tmp, ec);
        throw;
    }
}

} // namespace bsroformer
