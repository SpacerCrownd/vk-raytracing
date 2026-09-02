#ifndef VK_RAYTRACING_SCENE_H
#define VK_RAYTRACING_SCENE_H

#include <filesystem>
#include <unordered_set>
#include <vector>

#include <tinygltf/tiny_gltf.h>
#include <glm/glm.hpp>

// scene data
namespace app {
// a render node is the instance of a gltf primitive in the scene graph
struct RenderNode {
    glm::mat4 worldMatrix  = glm::mat4(1.0f);
    int       materialID   = 0;   // Reference to the material
    int       renderPrimID = -1;  // Reference to the unique primitive
    int       refNodeID    = -1;  // Reference to the tinygltf::Node
};

// a render primitive corresponds to a unique geometry object (a primitive (i.e. a submesh) in gltf)
struct RenderPrimitive {
    tinygltf::Primitive* pPrimitive  = nullptr;
    int                  vertexCount = 0;
    int                  indexCount  = 0;
    int                  meshID      = 0;
};

struct RenderLight {
    glm::mat4 worldMatrix = glm::mat4(1.0f);
    int lightID           = 0;
    int nodeID            = -1;
};

// bidirectional reference between render node (app scene object representation) <-> node/primitive (gltf object)
// each gltf primitive instance will have its own render node in the scene graph
// nodeID -> primitiveIndex, 1:N relationship
class RenderNodeRegistry {
public:
    const std::vector<RenderNode>&     getRenderNodes() const { return m_renderNodes; }
    std::vector<RenderNode>&           getRenderNodes() { return m_renderNodes; }
    int                                getRenderNodeID(int nodeID, int primIndex) const;
    std::optional<std::pair<int, int>> getNodeAndPrim(int renderNodeID) const;
    const std::vector<int>&            getRenderNodeIDsForNode(int nodeID) const;

    int addRenderNode(const RenderNode& node, int nodeID, int primIndex);

    void clear();
private:
    std::vector<RenderNode> m_renderNodes;

    // Forward: (nodeID, primIndex) -> renderNodeID
    std::unordered_map<uint64_t, int> m_nodeAndPrimToRenderNode;

    // Reverse: renderNodeID -> (nodeID, primIndex)
    std::vector<std::pair<int, int>> m_renderNodeToNodeAndPrim;

    // Grouped by node: nodeID -> [renderNodeIDs]
    std::unordered_map<int, std::vector<int>> m_nodeToRenderNodes;

    static const std::vector<int> s_emptyRenderNodes;

    // fit two 32bit integers into one uint64
    // (nodeID, primIndex)
    static uint64_t makeKey(int nodeID, int primIndex)
    {
        return (static_cast<uint64_t>(static_cast<uint32_t>(nodeID)) << 32) | static_cast<uint32_t>(primIndex);
    }
};

// nodeID -> Gltf Node
// RenderNode -> rendering node created from (nodeID, primIndex)
class GltfScene { // NOTE: in case of scene editing implementation, add dirty flag tracking for optimized scene update
public:
    bool                         load(const std::filesystem::path& filename);
    const std::filesystem::path& getFilename() const { return m_filename; }

    const tinygltf::Model& getModel() const { return m_model; }
    tinygltf::Model        getModel() { return m_model; }

    const std::vector<glm::mat4>& getNodesWorldMatrices() const { return m_nodesWorldMatrices; }
    const std::vector<glm::mat4>& getNodesLocalMatrices() const { return m_nodesLocalMatrices; }
    void                          updateNodeWorldMatrices();

    const std::vector<RenderPrimitive>& getRenderPrimitives() const { return m_renderPrimitives; }
    std::vector<RenderPrimitive>&       getRenderPrimitives() { return m_renderPrimitives; }
    size_t                              getNumRenderPrimitives() const { return m_renderPrimitives.size(); }

    const std::vector<RenderLight>& getRenderLights() const { return m_renderLights; }

    const std::vector<RenderNode>& getRenderNodes() const { return m_renderNodeRegistry.getRenderNodes(); }
    const RenderNodeRegistry&      getRenderNodeRegistry() const { return m_renderNodeRegistry; }
    RenderNodeRegistry&            getRenderNodeRegistry() { return m_renderNodeRegistry; }

    struct DirtyFlags {
        std::unordered_set<int> renderNodesVkIDs; // indices of render nodes that need to be updated on gpu buffers
        std::unordered_set<int> renderNodesRtIDs; // indices of render nodes that need to be updated on tlas
        std::unordered_set<int> lightIDs;
        std::unordered_set<int> nodeIDs;
        // possible additions:
        // material editing
        // ...

        void clear() {
            renderNodesVkIDs.clear();
            renderNodesRtIDs.clear();
            lightIDs.clear();
            nodeIDs.clear();
        }

        bool isEmpty() const {
            return renderNodesVkIDs.empty() && renderNodesRtIDs.empty() && lightIDs.empty() && nodeIDs.empty();
        }
    };

    const DirtyFlags& getDirtyFlags() const { return m_dirtyFlags; }
    DirtyFlags&       getDirtyFlags() { return m_dirtyFlags; }
    void              clearDirtyFlags() { m_dirtyFlags.clear(); }

    void destroy();

    int getNumTriangles() const { return m_numTriangles; }

    // TODO: In the future add scene management methods (e.g. move render nodes, add, remove scene elements, lights, etc...)
private:
    // gltf data
    tinygltf::Model       m_model;
    std::filesystem::path m_filename;

    // Render data built from model
    RenderNodeRegistry           m_renderNodeRegistry;
    std::vector<RenderPrimitive> m_renderPrimitives;
    std::vector<RenderLight>     m_renderLights;

    // transform matrices
    std::vector<glm::mat4> m_nodesWorldMatrices; // index is nodeID
    std::vector<glm::mat4> m_nodesLocalMatrices;
    std::vector<int>       m_nodeParents;

    DirtyFlags m_dirtyFlags;

    int m_numTriangles{0};

    void parseGltf();
    void clearData();

    using PrimitiveKeyMap = std::map<std::string, int>; // this is used to make sure primitives are loaded once while parsing

    PrimitiveKeyMap buildPrimitiveKeyMap(); // cycle through meshes and import unique primitives
    void            createRenderNodesForNode(int nodeID,
                                             const glm::mat4& parentMat,
                                             PrimitiveKeyMap& primitiveKeyMap);
    glm::mat4       computeNodeWorldMatrix(int nodeID) const;

    void setSceneElementsDefaultNames();

    void markLightDirty(int lightIndex);
    void markNodeDirty(int nodeIndex); // needs to be called when modifying node transform in-editor
};
}

#endif //VK_RAYTRACING_SCENE_H