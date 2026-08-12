#pragma once

#include <string>
#include <vector>

namespace bsroformer {

/**
 * Describes a compute backend device available for inference.
 */
struct BackendInfo {
    std::string id;          // Stable identifier ("", "cpu", "gpu")
    std::string name;        // Human readable label for UI
    std::string description; // Extra detail (e.g. device name)
    bool is_cpu = false;     // True for CPU devices
};

/**
 * Enumerate the compute backends available on this machine.
 *
 * The first entry is always an "auto" entry (id == "") which lets the engine
 * pick the default best backend (respecting BSR_FORCE_CPU).
 *
 * The exact set depends on how ggml was built:
 *   - "cpu": always available
 *   - "gpu": available when a GPU backend (CUDA / Vulkan / Metal / ...) was
 *            compiled in and a device is present.
 */
std::vector<BackendInfo> ListBackends();

} // namespace bsroformer
