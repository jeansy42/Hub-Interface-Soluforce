#include <Arduino.h>
#include "painlessMesh.h"
#include "auxiliars.h"

painlessMesh mesh;
void receivedCallback(uint32_t from, String &msg)
{
    Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId)
{
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
    char nodeIdStr[18];
    sprintf(nodeIdStr, "/%u.json", nodeId);
    createNewFile(nodeIdStr);
}

void changedConnectionCallback()
{
    Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset)
{
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}
