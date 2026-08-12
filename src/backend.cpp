#include "bs_roformer/backend.h"
#include <ggml.h>
#include <ggml-backend.h>

#include <string>

namespace bsroformer {

std::vector<BackendInfo> ListBackends() {
    std::vector<BackendInfo> backends;

    // "auto" always first: engine picks best (respecting BSR_FORCE_CPU).
    backends.push_back({"", "Auto (default)", "", false});

    // Enumerate every registered device (CPU, Vulkan, CUDA, Metal, ...) by name
    // so the user can pick a specific compute device (e.g. an AMD GPU via Vulkan).
    const size_t n_regs = ggml_backend_reg_count();
    for (size_t r = 0; r < n_regs; ++r) {
        ggml_backend_reg_t reg = ggml_backend_reg_get(r);
        const char* reg_name = ggml_backend_reg_name(reg);
        if (!reg_name) continue;

        const size_t n_dev = ggml_backend_reg_dev_count(reg);
        for (size_t d = 0; d < n_dev; ++d) {
            ggml_backend_dev_t dev = ggml_backend_reg_dev_get(reg, d);
            if (!dev) continue;

            const char* dev_name = ggml_backend_dev_name(dev);
            const char* dev_desc = ggml_backend_dev_description(dev);
            const enum ggml_backend_dev_type type = ggml_backend_dev_type(dev);

            // Prefer the human-readable device description (e.g. the GPU model
            // name) and fall back to the short device name.
            const std::string friendly = dev_desc ? std::string(dev_desc)
                                                  : (dev_name ? std::string(dev_name) : "");

            BackendInfo info;
            info.id = std::string(reg_name) + ":" + std::to_string(d);
            info.name = std::string(reg_name) + " — " + friendly;
            info.description = friendly;
            info.is_cpu = (type == GGML_BACKEND_DEVICE_TYPE_CPU);
            backends.push_back(std::move(info));
        }
    }

    return backends;
}

} // namespace bsroformer
