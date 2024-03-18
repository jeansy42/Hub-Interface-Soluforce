#include "structures.h"
#include "meshManager.h"


void verifyNodesToUpdate()
{
    if (UpdateNodes::needsToUpdate())
    {
        for (uint32_t nodeId : UpdateNodes::nodesToUpdate)
        {
            sendingConfigurationToNode(nodeId);
        }
    }
}