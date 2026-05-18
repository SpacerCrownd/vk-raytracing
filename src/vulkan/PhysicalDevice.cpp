#include "PhysicalDevice.h"

bool PhysicalDevice::IsExtensionSupported(const char* pExt) const {
    bool res = false;
    std::string reqExtension(pExt);

    for (const vk::ExtensionProperties& extension : m_extensions) {
        std::string curExtension(extension.extensionName.data());
        if (reqExtension == curExtension) {
            res = true;
            break;
        }
    }

    printf("Extension %s %s supporterd\n", pExt, res ? "is" : "is not");
    return res;
}
