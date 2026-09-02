#ifndef VK_RT_UTILS_H
#define VK_RT_UTILS_H

#include "Vulkan.h"
#include <vector>
#include <hash_combine/hash_combine.h>

#define VK_CHECK_RESULT(res, msg) \
if(res != vk::Result::eSuccess) { \
	fprintf(stderr, "[ERROR] Error in %s:%d - %s, code %x", __FILE__, __LINE__, msg, res); \
	throw std::runtime_error(msg); \
}

namespace ptvk::utils {
// debug utils
const char* getDebugSeverityStr(vk::DebugUtilsMessageSeverityFlagBitsEXT severity);
const char* getDebugType(vk::DebugUtilsMessageTypeFlagsEXT type);

void printImageUsageFlags(const vk::ImageUsageFlags& flags);
void printMemoryPropertyFlags(const vk::Flags<vk::MemoryPropertyFlagBits>& flags);
void printMemoryPropertyFlags(const VkMemoryPropertyFlags &flags);

// format query
vk::Format findSupportedFormat(const vk::raii::PhysicalDevice& device,
                               const std::vector<vk::Format>& candidates,
                               vk::ImageTiling tiling,
                               vk::FormatFeatureFlags features);
vk::Format findDepthFormat(const vk::raii::PhysicalDevice& device);

// image utils
void imageLayoutTransition(const vk::raii::CommandBuffer& cmd, vk::Image image, vk::ImageLayout currentLayout, vk::ImageLayout newLayout); // generic inefficient transition memory barriers
void imageLayoutTransition(const vk::raii::CommandBuffer &cmd,
                           vk::Image image,
                           vk::PipelineStageFlags2 srcStageMask,
                           vk::PipelineStageFlags2 dstStageMask,
                           vk::AccessFlags2 srcAccessMask,
                           vk::AccessFlags2 dstAccessMask,
                           vk::ImageLayout oldLayout,
                           vk::ImageLayout newLayout,
                           const vk::ImageSubresourceRange &subresourceRange);
void blitImage(const vk::raii::CommandBuffer& cmd, vk::Image source, vk::Image destination, vk::Extent2D srcSize, vk::Extent2D dstSize);

// img barrier utils
constexpr vk::AccessFlags2 inferAccessFromStage(vk::PipelineStageFlags2 stage, bool read) {
    vk::AccessFlags2 access{};

    if (stage & (vk::PipelineStageFlagBits2::eAllCommands | vk::PipelineStageFlagBits2::eAllGraphics)) {
        access |= read
            ? vk::AccessFlagBits2::eMemoryRead
            : vk::AccessFlagBits2::eMemoryWrite;
    }

    if (stage & (vk::PipelineStageFlagBits2::eComputeShader |
                vk::PipelineStageFlagBits2::eFragmentShader |
                vk::PipelineStageFlagBits2::eVertexShader |
                vk::PipelineStageFlagBits2::eMeshShaderEXT |
                vk::PipelineStageFlagBits2::eRayTracingShaderKHR |
                vk::PipelineStageFlagBits2::eTaskShaderEXT |
                vk::PipelineStageFlagBits2::ePreRasterizationShaders |
                vk::PipelineStageFlagBits2::eTessellationControlShader |
                vk::PipelineStageFlagBits2::eTessellationEvaluationShader |
                vk::PipelineStageFlagBits2::eGeometryShader))
    {
        access |= read
            ? (vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eUniformRead)
            : vk::AccessFlagBits2::eShaderWrite;
    }

    if (stage & vk::PipelineStageFlagBits2::eHost)
    {
        access |= read
            ? vk::AccessFlagBits2::eHostRead
            : vk::AccessFlagBits2::eHostWrite;
    }

    if (stage & vk::PipelineStageFlagBits2::eTransfer)
    {
        access |= read
            ? vk::AccessFlagBits2::eTransferRead
            : vk::AccessFlagBits2::eTransferWrite;
    }

    if (stage & vk::PipelineStageFlagBits2::eVertexAttributeInput)
        access |= vk::AccessFlagBits2::eVertexAttributeRead;

    if (stage & vk::PipelineStageFlagBits2::eIndexInput)
        access |= vk::AccessFlagBits2::eIndexRead;

    if (stage & vk::PipelineStageFlagBits2::eDrawIndirect)
        access |= vk::AccessFlagBits2::eIndirectCommandRead;

    if (stage & (vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests)) {
        access |= read
            ? vk::AccessFlagBits2::eDepthStencilAttachmentRead
            : vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
    }

    if (stage & vk::PipelineStageFlagBits2::eColorAttachmentOutput) {
        access |= read
            ? vk::AccessFlagBits2::eColorAttachmentRead
            : vk::AccessFlagBits2::eColorAttachmentWrite;
    }

    if (stage & vk::PipelineStageFlagBits2::eCommandPreprocessEXT) {
        access |= read
            ? vk::AccessFlagBits2::eCommandPreprocessReadEXT
            : vk::AccessFlagBits2::eCommandPreprocessWriteEXT;
    }

    if (stage & vk::PipelineStageFlagBits2::eFragmentShadingRateAttachmentKHR)
        access |= vk::AccessFlagBits2::eFragmentShadingRateAttachmentReadKHR;

    if (stage & vk::PipelineStageFlagBits2::eVideoDecodeKHR) {
        access |= read
            ? vk::AccessFlagBits2::eVideoDecodeReadKHR
            : vk::AccessFlagBits2::eVideoDecodeWriteKHR;
    }

    if (stage & vk::PipelineStageFlagBits2::eVideoEncodeKHR) {
        access |= read
            ? vk::AccessFlagBits2::eVideoEncodeReadKHR
            : vk::AccessFlagBits2::eVideoEncodeWriteKHR;
    }

    if (stage & vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR) {
        access |= read
            ? vk::AccessFlagBits2::eAccelerationStructureReadKHR
            : vk::AccessFlagBits2::eAccelerationStructureWriteKHR;
    }

    if (stage & vk::PipelineStageFlagBits2::eAccelerationStructureCopyKHR)
        access |= vk::AccessFlagBits2::eIndirectCommandRead;

    assert(access || stage == vk::PipelineStageFlagBits2::eNone);

    return access;
}

constexpr std::tuple<vk::PipelineStageFlags2, vk::AccessFlags2> inferPipelineStageAccessTuple(vk::ImageLayout layout) {
    switch (layout) {
        case vk::ImageLayout::eUndefined:
            return {
            vk::PipelineStageFlagBits2::eNone,
            vk::AccessFlagBits2::eNone
        };

        case vk::ImageLayout::eColorAttachmentOptimal:
            return {
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eColorAttachmentRead |
            vk::AccessFlagBits2::eColorAttachmentWrite
        };

        case vk::ImageLayout::eShaderReadOnlyOptimal:
            return {
            vk::PipelineStageFlagBits2::eFragmentShader |
            vk::PipelineStageFlagBits2::eComputeShader |
            vk::PipelineStageFlagBits2::ePreRasterizationShaders,
            vk::AccessFlagBits2::eShaderRead
        };

        case vk::ImageLayout::eTransferDstOptimal:
            return {
            vk::PipelineStageFlagBits2::eTransfer,
            vk::AccessFlagBits2::eTransferWrite
        };

        case vk::ImageLayout::eTransferSrcOptimal:
            return {
            vk::PipelineStageFlagBits2::eTransfer,
            vk::AccessFlagBits2::eTransferRead
        };

        case vk::ImageLayout::eGeneral:
            return {
            vk::PipelineStageFlagBits2::eComputeShader |
            vk::PipelineStageFlagBits2::eFragmentShader |
            vk::PipelineStageFlagBits2::ePreRasterizationShaders |
            vk::PipelineStageFlagBits2::eTransfer,
            vk::AccessFlagBits2::eShaderRead |
            vk::AccessFlagBits2::eShaderWrite |
            vk::AccessFlagBits2::eTransferRead |
            vk::AccessFlagBits2::eTransferWrite
        };

        case vk::ImageLayout::ePresentSrcKHR:
            return {
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eNone
        };

        case vk::ImageLayout::eDepthAttachmentOptimal:
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            return {
            vk::PipelineStageFlagBits2::eEarlyFragmentTests |
            vk::PipelineStageFlagBits2::eLateFragmentTests,
            vk::AccessFlagBits2::eDepthStencilAttachmentRead |
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite
        };

        case vk::ImageLayout::eDepthReadOnlyOptimal:
            return {
            vk::PipelineStageFlagBits2::eEarlyFragmentTests |
            vk::PipelineStageFlagBits2::eLateFragmentTests,
            vk::AccessFlagBits2::eDepthStencilAttachmentRead
        };

        case vk::ImageLayout::eAttachmentOptimal:
            return {
            vk::PipelineStageFlagBits2::eEarlyFragmentTests |
            vk::PipelineStageFlagBits2::eLateFragmentTests |
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eDepthStencilAttachmentRead |
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite |
            vk::AccessFlagBits2::eColorAttachmentRead |
            vk::AccessFlagBits2::eColorAttachmentWrite
        };

        default:
            assert(false && "Unsupported layout transition!");

            return {
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite
            };
    }
}

struct ImageMemoryBarrierParams {
    vk::Image image{};

    vk::ImageLayout oldLayout = vk::ImageLayout::eUndefined;
    vk::ImageLayout newLayout = vk::ImageLayout::eUndefined;

    vk::ImageSubresourceRange subresourceRange{
        vk::ImageAspectFlagBits::eColor,
        0,
        vk::RemainingMipLevels,
        0,
        vk::RemainingArrayLayers
    };

    std::optional<vk::PipelineStageFlags2> srcStageMask;
    std::optional<vk::PipelineStageFlags2> dstStageMask;

    std::optional<vk::AccessFlags2> srcAccessMask;
    std::optional<vk::AccessFlags2> dstAccessMask;

    bool srcAccessRead = false;
    bool dstAccessRead = false;
};

constexpr vk::ImageMemoryBarrier2 makeImageMemoryBarrier(const ImageMemoryBarrierParams& params) {
    vk::ImageMemoryBarrier2 barrier{
        .srcStageMask = params.srcStageMask.value(),
        .srcAccessMask = params.srcAccessMask.value(),
        .dstStageMask = params.dstStageMask.value(),
        .dstAccessMask = params.dstAccessMask.value(),
        .oldLayout = params.oldLayout,
        .newLayout = params.newLayout,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = params.image,
        .subresourceRange = params.subresourceRange,
    };

    if (!params.srcStageMask && !params.srcAccessMask) {
        const auto [stageMask, accessMask] = inferPipelineStageAccessTuple(params.oldLayout);

        barrier.srcStageMask  = stageMask;
        barrier.srcAccessMask = accessMask;
    } else if (!params.srcAccessMask) {
        barrier.srcAccessMask = inferAccessFromStage(params.srcStageMask.value(), params.srcAccessRead);
    }

    if (!params.dstStageMask && !params.dstAccessMask) {
        const auto [stageMask, accessMask] = inferPipelineStageAccessTuple(params.newLayout);

        barrier.dstStageMask  = stageMask;
        barrier.dstAccessMask = accessMask;
    } else if (!params.dstAccessMask) {
        barrier.dstAccessMask = inferAccessFromStage(params.dstStageMask.value(), params.dstAccessRead);
    }

    return barrier;
}

// hashing utility
template <typename T>
void hashCombine(std::size_t& seed, const T& val)
{
    boost::hash_combine(seed, val);
}

template <typename T, typename... Types>
void hashCombine(std::size_t& seed, const T& val, const Types&... args)
{
    hashCombine(seed, val);
    hashCombine(seed, args...);
}

template <typename... Types>
std::size_t hashVal(const Types&... args)
{
    std::size_t seed = 0;
    hashCombine(seed, args...);
    return seed;
}
}

#endif
