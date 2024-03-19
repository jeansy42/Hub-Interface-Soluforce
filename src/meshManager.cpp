#include <Arduino.h>
#include "painlessMesh.h"
#include "auxiliars.h"
#include "structures.h"

extern painlessMesh mesh;
extern String actionerMessage;
extern JsonDocument globalMessages;
extern JsonObject actioner;
extern fs::FS filesystem;

// Callbacks

void receivedCallback(uint32_t from, String &msg)
{
    if (msg == "configOk")
    {
        Serial.printf("-->Removendo no %u da lista de atualizacao\n", from);
        UpdateNodes::removeNodeToUpdate(from);
    }
    else
    {
        JsonDocument doc;
        String nodeId = String(from);
        DeserializationError error = deserializeJson(doc, msg);
        if (error)
        {
            Serial.println("Falho ao deserializar a mensagem");
        }
        else
        {
            if (doc.containsKey("actioner"))
            {
                int status = doc["actioner"]["status"];
                Serial.printf("Recebendo msg from %u, status: %d\n", from, status);
                actioner[nodeId] = status;
            }
        }
    }
}

void newConnectionCallback(uint32_t nodeId)
{
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
    String nodeIdStr = String(nodeId);

    createNewFile("/" + nodeIdStr + ".json", &filesystem);
    mesh.sendSingle(nodeId, "root");
}

void changedConnectionCallback()
{
    Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset)
{
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

// Reuseful functions
void sendingConfigurationToNode(uint32_t nodeId)
{
    Serial.printf("Mandando configuração ao node %u \n", nodeId);
    String res = sendJsonResponseFromFile("/" + String(nodeId) + ".json", &filesystem);
    bool isOk = mesh.sendSingle(nodeId, res);
    if (isOk)
        Serial.println("Message sended successfully");
    else
        Serial.println("Some error ocurred");
}