#include "GltfScene.h"
#include "GltfUtils.h"

namespace app {
//
//  RenderNodeRegistry
//
const std::vector<int> RenderNodeRegistry::s_emptyRenderNodes;

int RenderNodeRegistry::addRenderNode(const RenderNode &node, int nodeID, int primIndex) {
    const int renderNodeID = static_cast<int>(m_renderNodes.size());
    m_renderNodes.push_back(node);
    m_renderNodeToNodeAndPrim.emplace_back(nodeID, primIndex);
    m_nodeAndPrimToRenderNode.try_emplace(makeKey(nodeID, primIndex), renderNodeID);
    m_nodeToRenderNodes[nodeID].push_back(renderNodeID);
    return renderNodeID;
}

void RenderNodeRegistry::clear() {
    m_renderNodes.clear();
    m_nodeAndPrimToRenderNode.clear();
    m_nodeToRenderNodes.clear();
    m_renderNodeToNodeAndPrim.clear();
}

std::optional<std::pair<int, int>> RenderNodeRegistry::getNodeAndPrim(int renderNodeID) const {
    if(renderNodeID < 0 || static_cast<size_t>(renderNodeID) >= m_renderNodeToNodeAndPrim.size()) {
        return std::nullopt;
    }
    return m_renderNodeToNodeAndPrim.at(renderNodeID);
}

const std::vector<int>& RenderNodeRegistry::getRenderNodeIDsForNode(int nodeID) const {
    auto it = m_nodeToRenderNodes.find(nodeID);
    if(it == m_nodeToRenderNodes.end()) {
        return s_emptyRenderNodes;
    }
    return it->second;
}

int RenderNodeRegistry::getRenderNodeID(int nodeID, int primIndex) const {
    auto it = m_nodeAndPrimToRenderNode.find(makeKey(nodeID, primIndex));
    if(it == m_nodeAndPrimToRenderNode.end()) {
        return -1;
    }
    return it->second;
}

//
//  Gltf Scene
//

bool GltfScene::load(const std::filesystem::path& filename) {
    std::error_code errorCode;
    m_filename = std::filesystem::absolute(filename, errorCode);
    if (errorCode) {
        m_filename = filename;
    }

    m_model = {};

    if (!gltfutils::loadGltf(m_filename, m_model)) {
        clearData();
        return false;
    }

    parseGltf();
    return true;
}

void GltfScene::clearData() {
    m_renderLights.clear();
    m_renderNodeRegistry.clear();
    m_renderPrimitives.clear();
    m_nodeParents.clear();
    m_nodesLocalMatrices.clear();
    m_numTriangles = 0;
}

void GltfScene::destroy() {
    clearData();
    m_filename.clear();
    m_model = {};
}

void GltfScene::parseGltf() {
    clearData();

    // populate render primitives with unique primitives
    auto primitiveKeyMap = buildPrimitiveKeyMap();

    // assume there is only one scene in gltf
    for(auto& sceneNodeID : m_model.scenes[0].nodes) {
        // create render nodes for each root node in the scene
        createRenderNodesForNode(sceneNodeID, glm::mat4(1.0f), primitiveKeyMap);
    }
}

GltfScene::PrimitiveKeyMap GltfScene::buildPrimitiveKeyMap() {
    m_renderPrimitives.clear();

    PrimitiveKeyMap primMap;
    for(size_t i = 0; i < m_model.meshes.size(); ++i) {
        for(size_t j = 0; j < m_model.meshes[i].primitives.size(); ++j) {
            tinygltf::Primitive& primitive = m_model.meshes[i].primitives[j];
            const std::string& key = gltfutils::generatePrimitiveKey(primitive);
            auto [it, inserted] = primMap.try_emplace(key, static_cast<int>(primMap.size()));
            if(inserted)
            {
                RenderPrimitive renderPrim;
                renderPrim.pPrimitive = &primitive;
                renderPrim.vertexCount = static_cast<int>(gltfutils::getVertexCount(m_model, primitive));
                renderPrim.indexCount = static_cast<int>(gltfutils::getIndexCount(m_model, primitive));
                renderPrim.meshID = static_cast<int>(i);
                m_renderPrimitives.push_back(renderPrim);
            }
        }
    }
    return primMap;
}

void GltfScene::createRenderNodesForNode(int nodeID,
                                         const glm::mat4& parentMat,
                                         PrimitiveKeyMap& primitiveKeyMap) {
    const auto& node = m_model.nodes[nodeID];
    glm::mat4 worldMatrix = parentMat * gltfutils::getNodeTransformMatrix(node);

    if(node.light > -1) {
        RenderLight renderLight;
        renderLight.lightID = node.light;

        if(node.light >= 0 && node.light < m_model.lights.size()) {
            tinygltf::Light& light = m_model.lights[node.light];

            // Add a default color if the light has no color
            if(light.color.empty()) {
                light.color = {1.0f, 1.0f, 1.0f};
            }

            // Add a default radius if the light has no radius
            if(!light.extras.Has("radius")) {
                if(!light.extras.IsObject()) {  // Avoid overwriting other extras
                    light.extras = tinygltf::Value(tinygltf::Value::Object());
                }
                auto extras = light.extras.Get<tinygltf::Value::Object>();
                extras["radius"] = tinygltf::Value(0.);
                light.extras = tinygltf::Value(extras);
            }
            renderLight.worldMatrix = worldMatrix;
            renderLight.nodeID      = nodeID;

            m_renderLights.push_back(renderLight);
        }
    }

    if(node.mesh > -1) {
        tinygltf::Mesh& mesh = m_model.meshes[node.mesh];
        for(size_t primitiveIndex = 0; primitiveIndex < mesh.primitives.size(); primitiveIndex++) {
            tinygltf::Primitive& primitive = mesh.primitives[primitiveIndex];
            int renderPrimID = primitiveKeyMap.at(gltfutils::generatePrimitiveKey(primitive));
            int numTriangles = m_renderPrimitives[renderPrimID].indexCount / 3;

            RenderNode renderNode;
            renderNode.worldMatrix  = worldMatrix;
            renderNode.materialID   = primitive.material;
            renderNode.renderPrimID = renderPrimID;
            renderNode.refNodeID    = nodeID;

            m_renderNodeRegistry.addRenderNode(renderNode, nodeID, static_cast<int>(primitiveIndex));
            m_numTriangles += numTriangles;
        }
    }

    for(const auto& child : node.children) {
        createRenderNodesForNode(child, worldMatrix, primitiveKeyMap);
    }
}

void GltfScene::updateNodeWorldMatrices() {
    // return if no nodes were modified
    if (m_dirtyFlags.nodeIDs.empty()) {
        return;
    }

    // update local matrices
    for(int nodeID : m_dirtyFlags.nodeIDs) {
        m_nodesLocalMatrices[nodeID] = gltfutils::getNodeTransformMatrix(m_model.nodes[nodeID]);
    }

    std::vector<bool> isDirty(m_dirtyFlags.nodeIDs.size(), false);
    for (auto nodeID: m_dirtyFlags.nodeIDs) {
       isDirty[nodeID] = true;
    }

    // update only root nodes' world matrix since update is applied recursively for its children
    std::vector<int> nodesToUpdate;
    nodesToUpdate.reserve(m_dirtyFlags.nodeIDs.size());

    for(auto nodeID : m_dirtyFlags.nodeIDs) {
        bool hasDirtyParent = false;
        int parent = m_nodeParents[nodeID];

        while (parent >= 0) {
            if(isDirty[parent])
            {
                hasDirtyParent = true;
                break;
            }
            parent = m_nodeParents[parent];
        }

        if (!hasDirtyParent) {
            nodesToUpdate.push_back(nodeID);
        }
    }

    // recursive update lambda
    std::function<void(int)> updateNodeTransformMat = [&](int nodeID) {
        // if parent is scene root use identity matrix
        m_nodesWorldMatrices[nodeID] = m_nodeParents[nodeID] >= 0
                                  ? m_nodesWorldMatrices[m_nodeParents[nodeID]]
                                  : glm::mat4(1.0f) * m_nodesLocalMatrices[nodeID];

        auto& node = m_model.nodes[nodeID];

        if (node.mesh >= 0) {
            for (int renderNodeID : m_renderNodeRegistry.getRenderNodeIDsForNode(nodeID)) {
                m_renderNodeRegistry.getRenderNodes()[renderNodeID].worldMatrix = m_nodesWorldMatrices[nodeID];

                // mark nodes for gpu and tlas update
                m_dirtyFlags.renderNodesVkIDs.insert(renderNodeID);
                m_dirtyFlags.renderNodesRtIDs.insert(renderNodeID);
            }
        }

        if (node.light >= 0) {
            m_renderLights[node.light].worldMatrix = m_nodesWorldMatrices[nodeID];
        }

        for(int child : node.children) {
            updateNodeTransformMat(child);
        }
    };

    for (auto nodeID : nodesToUpdate){
        updateNodeTransformMat(nodeID);
    }
}

glm::mat4 GltfScene::computeNodeWorldMatrix(int nodeID) const {
    if(nodeID < 0 || nodeID >= static_cast<int>(m_nodesLocalMatrices.size())) { // if invalid or scene root node
        return {1.0f};
    }

    std::vector<int> chain;
    for(int n = nodeID; n >= 0; n = m_nodeParents[n]) {
        chain.push_back(n);
    }

    glm::mat4 world{1.0f};
    for(auto it = chain.rbegin(); it != chain.rend(); ++it) {
        world = world * m_nodesLocalMatrices[*it];
    }

    return world;
}

void GltfScene::setSceneElementsDefaultNames()
{
    auto setDefaultName = [](auto& elements, const std::string& prefix) {
        for(size_t i = 0; i < elements.size(); ++i)
        {
            if(elements[i].name.empty())
            {
                elements[i].name = std::format("{}-{}", prefix, i);
            }
        }
    };

    setDefaultName(m_model.scenes, "Scene");
    setDefaultName(m_model.meshes, "Mesh");
    setDefaultName(m_model.materials, "Material");
    setDefaultName(m_model.nodes, "Node");
    setDefaultName(m_model.cameras, "Camera");
    setDefaultName(m_model.lights, "Light");
}

void GltfScene::markLightDirty(int lightIndex) {
    if(lightIndex >= 0 && lightIndex < static_cast<int>(m_model.lights.size())) {
        m_dirtyFlags.lightIDs.insert(lightIndex);
    }
}

void GltfScene::markNodeDirty(int nodeIndex) {
    if(nodeIndex >= 0 || nodeIndex < static_cast<int>(m_model.nodes.size())) {
        m_dirtyFlags.nodeIDs.insert(nodeIndex);

        const tinygltf::Node& node = m_model.nodes[nodeIndex];

        if(node.light >= 0)
            markLightDirty(node.light);
    }
}
}
