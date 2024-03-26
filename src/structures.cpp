#include <set>
#include <Arduino.h>

namespace UpdateNodes
{
    std::set<uint32_t> nodesToUpdate;

    void addNodesToUpdate(const String nodeId)
    {
        uint32_t nodeIdInt = strtoul(nodeId.c_str(), NULL, 10);
        nodesToUpdate.insert(nodeIdInt);
    }
    void removeNodeToUpdate(const uint32_t nodeId)
    {
        nodesToUpdate.erase(nodeId);
    }
    bool needsToUpdate() { return !nodesToUpdate.empty(); }

}

namespace SincronizeNodesConfig
{
    std::set<uint32_t> nodesToSincronize;
    void addNodesToSincronize(uint32_t nodeId)
    {
        nodesToSincronize.insert(nodeId);
    }
    void removeNodeToSincronize(const uint32_t nodeId) { nodesToSincronize.erase(nodeId); }
    bool needsToSincronize() { return !nodesToSincronize.empty(); }
}