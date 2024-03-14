#include "SD.h"
#include "WiFi.h"
#include "ArduinoJson.h"
#include "SPIFFS.h"
#include "FS.h"
#include "meshManager.h"
#include "structures.h"

bool initSPIFFS()
{
    if (!SPIFFS.begin())
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return false;
    }
    return true;
}
String generateUUID()
{
    uint32_t rand1 = esp_random();
    uint32_t rand2 = esp_random();
    uint32_t rand3 = esp_random();
    uint32_t rand4 = esp_random();

    String uuid = String(rand1, HEX) + String(rand2, HEX) + "-" + String(rand3, HEX) + "-" + String(rand4, HEX);
    return uuid;
}
bool initSD()
{
    if (!SD.begin())
    {
        Serial.println("Card Mount Failed");
        return false;
    }
    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE)
    {
        Serial.println("No SD card attached");
        return false;
    }

    Serial.print("SD Card Type: ");
    if (cardType == CARD_MMC)
    {
        Serial.println("MMC");
    }
    else if (cardType == CARD_SD)
    {
        Serial.println("SDSC");
    }
    else if (cardType == CARD_SDHC)
    {
        Serial.println("SDHC");
    }
    else
    {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
    return true;
}
void initWiFi(const char *ssid, const char *password)
{
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Connecting to WiFi..");
    }

    Serial.println(WiFi.localIP());
}

void createNewFile(String path, fs::FS *filesystem)
{
    if (!filesystem->exists(path))
    {

        Serial.printf("Criando arquivo: %s\n", path);
        File file = filesystem->open(path, "w");
        if (!file)
        {
            Serial.println("Erro ao criar o arquivo");
            return;
        }

        JsonDocument doc;
        deserializeJson(doc, file);
        doc["config"].to<JsonArray>();
        serializeJson(doc, file);
        Serial.printf("%s criado com sucesso \n", path);
        file.close();
    }
    else
    {
        Serial.printf("O arquivo %s já existe\n", path);
    }
}
String sendJsonResponseFromFile(String path, fs::FS *filesystem)
{
    String response;
    JsonDocument doc;
    if (!filesystem->exists(path))
    {
        JsonArray array = doc.to<JsonArray>();
        serializeJson(doc, response);
        return response;
    }
    else
    {
        File file = filesystem->open(path, "r");
        if (file)
        {
            DeserializationError error = deserializeJson(doc, file);
            file.close();
            if (error)
            {
                return ("{\"error \":\"Error to deserialize archive json\"}");
            }
            else
            {
                serializeJson(doc, response);
                return response;
            }
        }
        else
        {
            return ("{\"error \":\"Error to open the file\"}");
        }
    }
}

isConfigNode writeIntoFileJson(JsonObject json, String nodeId, fs::FS *filesystem)
{
    isConfigNode result;
    JsonDocument docRes;
    bool isOk = false;
    String resString;
    String path = "/" + nodeId + ".json";
    if (!filesystem->exists(path))
    {
        docRes["status"] = "error";
        docRes["msg"] = "The file not exists";
    }
    else
    {
        File file = filesystem->open(path, "r");
        if (!file)
        {
            docRes["status"] = "error";
            docRes["msg"] = " Failed to open the file.";
        }
        else
        {
            JsonDocument doc;
            deserializeJson(doc, file);
            file.close();

            JsonObject obj = doc["config"].add<JsonObject>();
            obj["id"] = generateUUID();
            for (JsonPair kv : json)
            {
                obj[kv.key().c_str()] = kv.value();
            };
            file = filesystem->open(path, "w");
            if (!file)
            {
                docRes["status"] = "error";
                docRes["msg"] = " Failed to open the file.";
            }
            else
            {
                serializeJson(doc, file);
                docRes["status"] = "ok";
                docRes["msg"] = "Success to append information to the file.";
                file.close();
                isOk = true;
            }
        };
    }
    serializeJson(docRes, resString);
    result.isOk = isOk;
    result.res = resString;
    return result;
}

String deleteModule(String moduleId, String nodeId, fs::FS *filesystem)
{
    String path = "/" + nodeId + ".json";
    JsonDocument docRes;
    String resString;

    if (!filesystem->exists(path))
    {
        docRes["status"] = "error";
        docRes["msg"] = "The file does not exist";
    }
    else
    {
        File file = filesystem->open(path, "r");
        if (!file)
        {
            docRes["status"] = "error";
            docRes["msg"] = "Failed to open the file.";
        }
        else
        {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, file);
            file.close();
            if (error)
            {
                docRes["status"] = "error";
                docRes["msg"] = "Failed to deserialize the file.";
            }
            else
            {
                JsonArray array = doc["config"].as<JsonArray>();
                bool moduleDeleted = false;
                for (int i = 0; i < array.size(); i++)
                {
                    JsonObject obj = array[i].as<JsonObject>();
                    if (obj["id"].as<String>() == moduleId)
                    {
                        array.remove(i);
                        moduleDeleted = true;
                        break;
                    }
                }
                if (moduleDeleted)
                {
                    file = filesystem->open(path, "w");
                    if (file)
                    {
                        serializeJson(doc, file);
                        file.close();
                        docRes["status"] = "success";
                        docRes["msg"] = "Module deleted successfully.";
                        // Enviando configuração ao no
                        sendingConfigurationToNode(nodeId);
                    }
                    else
                    {
                        docRes["status"] = "error";
                        docRes["msg"] = "Failed to open the file for writing.";
                    }
                }
                else
                {
                    docRes["status"] = "error";
                    docRes["msg"] = "Module does not exist.";
                }
            }
        }
    }
    serializeJson(docRes, resString);
    return resString;
}

String updateModule(String moduleId, String nodeId, JsonObject json, fs::FS *filesystem)
{
    String path = "/" + nodeId + ".json";
    JsonDocument docRes;
    String resString;

    if (!filesystem->exists(path))
    {
        docRes["status"] = "error";
        docRes["msg"] = "The file does not exist";
    }
    else
    {
        File file = filesystem->open(path, "r");
        if (!file)
        {
            docRes["status"] = "error";
            docRes["msg"] = "Failed to open the file.";
        }
        else
        {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, file);
            file.close();
            if (error)
            {
                docRes["status"] = "error";
                docRes["msg"] = "Failed to deserialize the file.";
            }
            else
            {
                JsonArray array = doc["config"].as<JsonArray>();
                bool moduleUpdated = false;
                for (int i = 0; i < array.size(); i++)
                {
                    JsonObject obj = array[i].as<JsonObject>();
                    if (obj["id"].as<String>() == moduleId)
                    {
                        String name = json["name"];
                        String description = json["description"];
                        String type = json["type"];
                        obj["name"] = name;
                        obj["description"] = description;
                        obj["type"] = type;
                        moduleUpdated = true;
                        break;
                    }
                }
                if (moduleUpdated)
                {
                    file = filesystem->open(path, "w");
                    if (file)
                    {
                        serializeJson(doc, file);
                        file.close();
                        docRes["status"] = "success";
                        docRes["msg"] = "Module updated successfully.";
                        // Enviando configuração ao no
                        sendingConfigurationToNode(nodeId);
                    }
                    else
                    {
                        docRes["status"] = "error";
                        docRes["msg"] = "Failed to open the file for writing.";
                    }
                }
                else
                {
                    docRes["status"] = "error";
                    docRes["msg"] = "Module does not exist.";
                }
            }
        }
    }
    serializeJson(docRes, resString);
    return resString;
}

String getModuleFromDispositiveById(String moduleId, String nodeId, fs::FS *filesystem)
{
    String path = "/" + nodeId + ".json";
    JsonDocument docRes;
    String resString;
    String jsonModule;

    if (!filesystem->exists(path))
    {
        docRes["status"] = "error";
        docRes["msg"] = "The file does not exist";
    }
    else
    {
        File file = filesystem->open(path, "r");
        if (!file)
        {
            docRes["status"] = "error";
            docRes["msg"] = "Failed to open the file.";
        }
        else
        {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, file);
            file.close();
            if (error)
            {
                docRes["status"] = "error";
                docRes["msg"] = "Failed to deserialize the file.";
            }
            else
            {
                JsonArray array = doc["config"].as<JsonArray>();
                for (int i = 0; i < array.size(); i++)
                {
                    JsonObject obj = array[i].as<JsonObject>();
                    if (obj["id"].as<String>() == moduleId)
                    {
                        jsonModule = array[i].as<String>();
                        return jsonModule;
                    }
                }
                docRes["status"] = "error";
                docRes["msg"] = "Module does not exist.";
            }
        }
    }
    serializeJson(docRes, resString);
    return resString;
}
