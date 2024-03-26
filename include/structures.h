#include <Arduino.h>
#include <set>
#ifndef STRUCTURES_H
#define STRUCTURES_H

namespace UpdateNodes
{
    extern std::set<uint32_t> nodesToUpdate;
    void addNodesToUpdate(const String nodeId);
    void removeNodeToUpdate(const uint32_t nodeId);
    bool needsToUpdate();

}
namespace SincronizeNodesConfig
{
    extern std::set<uint32_t> nodesToSincronize;
    void addNodesToSincronize(const uint32_t nodeId);
    void removeNodeToSincronize(const uint32_t nodeId);
    bool needsToSincronize();
}

#endif