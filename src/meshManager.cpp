#include <Arduino.h>
#include "painlessMesh.h"
#include "auxiliars.h"
#include "structures.h"
#include "meshManager.h"
#include "tasksManager.h"
#include "ESPAsyncWebServer.h"

extern painlessMesh mesh;
extern String actionerMessage;
extern JsonDocument globalMessages;
extern JsonObject actioner;
extern JsonObject doorSensor;
extern fs::FS filesystem;
extern AsyncEventSource events;

// Callbacks

void receivedCallback(uint32_t from, String &msg)
{
    digitalWrite(BLINKLED, HIGH);
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
            JsonDocument docRes;
            if (doc.containsKey("actioner"))
            {
                int status = doc["actioner"]["status"];
                Serial.printf("Recebendo msg desde %u, status: %d\n", from, status);
                actioner[nodeId] = status;
                JsonObject obj = docRes["actioner"].to<JsonObject>();
                obj["getInfo"] = "off";
                mesh.sendSingle(from, docRes.as<String>());
            }
            else if (doc.containsKey("doorSensor"))
            {
                int status = doc["doorSensor"]["status"];
                Serial.printf("Recebendo msg do sensor de porta desde %u, status: %d\n", from, status);
                doorSensor[nodeId] = status;
                String eventMsg = "Estado da porta mudou para " + String(status);
                events.send(eventMsg.c_str(), "doorSensorChange");
                JsonObject obj = docRes["doorSensor"].to<JsonObject>();
                obj["getInfo"] = "off";
                mesh.sendSingle(from, docRes.as<String>());
            }
            else if (doc.containsKey("validateConfigOk"))
            {
                JsonArray configNode = doc["validateConfigOk"].as<JsonArray>();
                JsonDocument configHub;
                DeserializationError error = deserializeJson(configHub, sendJsonResponseFromFile("/" + String(nodeId) + ".json", &filesystem));
                if (error)
                    Serial.println("Ocurreu algum erro ao deserializar o arquivo para validar configuracoes.");
                else
                {
                    if (configHub["config"].as<JsonArray>() == configNode)
                    {
                        Serial.printf("As configuracoes do hub e o no %u sao iguais\n", from);
                    }
                    else
                    {
                        Serial.printf("As configuracoes do hub e o no %u nao sao iguais\n", from);
                        UpdateNodes::addNodesToUpdate(String(from));
                    }
                    Serial.printf("-->Removendo no %u da lista de sincronizacao\n", from);
                    SincronizeNodesConfig::removeNodeToSincronize(from);

                    if (!SincronizeNodesConfig::needsToSincronize())
                    {
                        Serial.println("Deshabilitando validacao de configuracoes.");
                        taskValidateConfigurations.disable();
                    }
                }
            }
        }
    }
    digitalWrite(BLINKLED, LOW);
}

void newConnectionCallback(uint32_t nodeId)
{
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
    String nodeIdStr = String(nodeId);

    createNewFile("/" + nodeIdStr + ".json", &filesystem);
    mesh.sendSingle(nodeId, "root");
    sendEventChangedConnections("Node " + String(nodeId) + " connected.");
    Serial.printf("---> Agregando nó %u a lista de sinconização\n", nodeId);
    SincronizeNodesConfig::addNodesToSincronize(nodeId);
    taskValidateConfigurations.enable();
}

void changedConnectionCallback()
{
    Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset)
{
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void droppedConnectionCallBack(uint32_t nodeId)
{
    Serial.printf("Node %u lost connection.\n", nodeId);
    sendEventChangedConnections("Node " + String(nodeId) + " desconnected.");
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

void sendEventChangedConnections(String msg) { events.send(msg.c_str(), "changedConnections"); }