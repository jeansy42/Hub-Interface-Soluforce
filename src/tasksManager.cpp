#include "structures.h"
#include "meshManager.h"
#include "ESPAsyncWebServer.h"
#include "painlessMesh.h"
#include "Arduino.h"

extern AsyncEventSource events;
extern painlessMesh mesh;

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

void validateConfigurations()
{
    Serial.println("---> Iniciando validação de configurações");
    for (uint32_t nodeId : SincronizeNodesConfig::nodesToSincronize)
    {
        mesh.sendSingle(nodeId, "validateConfigurations");
    };
}


Task taskVerifyNodesToUpdate(TASK_SECOND * 4, TASK_FOREVER, &verifyNodesToUpdate);
Task taskShouldReinitHub(TASK_SECOND * 5, TASK_FOREVER, &reinitHub);
Task taskValidateConfigurations(TASK_SECOND * 4, TASK_FOREVER, &validateConfigurations);