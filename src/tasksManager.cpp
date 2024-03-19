#include "structures.h"
#include "meshManager.h"
#include "ESPAsyncWebServer.h"

extern AsyncEventSource events;
bool shouldReinitHub = false;

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

void reinitHub()
{
    if (shouldReinitHub)
        ESP.restart();
}

void testingEventsToBrowser()
{
    events.send("Esto e um teste", "teste", millis());
}