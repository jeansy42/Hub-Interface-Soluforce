#include <Arduino.h>
#include <set>
#include <cstdint>
#ifndef STRUCTURES_H
#define STRUCTURES_H

namespace UpdateNodes
{
    extern std::set<uint32_t> nodesToUpdate;
    void addNodesToUpdate(const String nodeId);
    void removeNodeToUpdate(const uint32_t nodeId);
    bool needsToUpdate();
}

#endif