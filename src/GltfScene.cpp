#include "GltfScene.h"
#include "GltfUtils.h"

namespace app {
Scene::Scene() = default;
Scene::~Scene() = default;

bool Scene::loadGLTF(const std::filesystem::path& filename) {
    std::error_code errorCode;
    m_filename = std::filesystem::absolute(filename, errorCode);
    if (errorCode) {
        m_filename = filename;
    }

    m_model = {};

    if (!loadGltf(m_filename, m_model)) {
        clearData();
        return false;
    }

    parseGltf();
    return true;
}

void Scene::clearData() {
    m_renderLights.clear();
    //m_renderNodeRegistry.clear();
    m_renderPrimitives.clear();
    m_nodeParents.clear();
    m_nodesLocalMatrices.clear();
    m_numTriangles = 0;
}

void Scene::destroy() {
    clearData();
    m_filename.clear();
    m_model = {};
}

void Scene::traverseNode(int nodeID, const glm::mat4 &parentMatrix) {

}

void Scene::traverseScene(const std::function<void(int nodeID, const glm::mat4 &worldMatrix)> &callback) {

}

void Scene::parseGltf() {

}
}
