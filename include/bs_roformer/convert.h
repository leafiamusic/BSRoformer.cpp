#pragma once

#include "bs_roformer/audio.h"
#include <string>

namespace bsroformer {

/**
 * Convert any decodable audio file to a 44100 Hz stereo 32-bit float WAV.
 *
 * Uses ffmpeg as a subprocess. The caller is responsible for deleting the
 * returned temp file. Throws std::runtime_error if ffmpeg is missing or fails.
 *
 * @param input      Path to the source audio file.
 * @param temp_dir   Directory in which to create the temporary WAV.
 * @return           Path to the generated temporary WAV file.
 */
std::string ConvertToCompatibleWav(const std::string& input, const std::string& temp_dir);

/**
 * Load an audio file in a way that is compatible with the model:
 *   - If the file is a loadable 44100 Hz mono/stereo WAV, it is loaded directly
 *     (fast path, no ffmpeg dependency).
 *   - Otherwise it is transcoded via ConvertToCompatibleWav and the temp file
 *     is loaded, then removed.
 *
 * @throws std::runtime_error on any failure.
 */
AudioBuffer LoadCompatible(const std::string& path);

} // namespace bsroformer
