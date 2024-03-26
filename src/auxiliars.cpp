#include "SD.h"
#include "WiFi.h"
#include "ArduinoJson.h"
#include "LittleFS.h"
#include "FS.h"
#include "meshManager.h"
#include "structures.h"
#include "painlessMesh.h"

extern fs::FS filesystem;
extern painlessMesh mesh;
extern String ssid;
extern String password;
extern uint16_t port;
extern bool shouldReinitHub;

IPAddress localIP(192, 168, 2, 106);
IPAddress localGateway(192, 168, 2, 254);
IPAddress subnet(255, 255, 255, 0);

bool initLittleFS()
{
    if (!LittleFS.begin())
    {
        Serial.println("An Error has occurred while mounting LittleFS");
        return false;
    }
    return true;
}
bool formatLittleFS()
{
    bool status = LittleFS.format();
    if (status)
        Serial.println("LittleFS formatado corretamente");
    return status;
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
/* bool initWiFi()
{

    WiFi.mode(WIFI_STA);

    if (!WiFi.config(localIP, localGateway, subnet))
    {
        Serial.println("STA Failed to configure");
        return false;
    }
    WiFi.begin(STATION_SSID, STATION_PASSWORD);

    Serial.println("Connecting to WiFi...");
    delay(20000);
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Failed to connect.");
        return false;
    }

    Serial.println(WiFi.localIP());
    return true;
} */

void createNewFile(String path, fs::FS *filesystem)
{
    if (!filesystem->exists(path))
    {

        Serial.printf("Criando arquivo: %s\n", path.c_str());
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
        Serial.printf("%s criado com sucesso \n", path.c_str());
        file.close();
    }
    else
    {
        Serial.printf("O arquivo %s já existe\n", path.c_str());
    }
}
void createConfigMeshIfNotExists(fs::FS *filesystem)
{
    const char *path = "/configMesh.json";
    if (!filesystem->exists(path))
    {
        Serial.printf("Criando arquivo: %s\n", path);
        File file = filesystem->open(path, "w");
        if (!file)
        {
            Serial.printf("Erro ao criar o arquivo %s\n", path);
            return;
        }
        JsonDocument doc;
        doc["ssid"] = "";
        doc["password"] = "";
        doc["port"] = "";
        serializeJson(doc, file);
        Serial.printf("%s criado com sucesso \n", path);
        file.close();
    }
    else
        Serial.printf("O arquivo %s ja existe\n", path);
}
String configRedMesh(JsonObject obj)
{
    const char *path = "/configMesh.json";
    JsonDocument docRes;
    createConfigMeshIfNotExists(&filesystem);
    File file = filesystem.open(path, "w");
    if (!file)
    {
        Serial.printf("Erro ao abrir o arquivo %s\n", path);
        docRes["status"] = "error";
        docRes["msg"] = "Erro ao abrir o arquivo";
    }
    else
    {
        JsonDocument doc;
        doc["ssid"] = obj["ssid"];
        doc["password"] = obj["password"];
        doc["port"] = obj["port"];
        serializeJson(doc, file);
        file.close();
        docRes["status"] = "success";
        docRes["msg"] = "Configuracao da red mesh feita com sucesso";
        Serial.println("Reiniciando dispositivo....");
        shouldReinitHub = true;
    }
    return docRes.as<String>();
}
bool isRedMeshConfig()
{
    const char *path = "/configMesh.json";
    createConfigMeshIfNotExists(&filesystem);
    File file = filesystem.open(path, "r");
    if (!file)
    {
        Serial.printf("Erro ao abrir o arquivo %s\n", path);
        return false;
    }
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error)
    {
        Serial.printf("Erro ao deserializar o arquivo %s\n", path);
        return false;
    }
    if (doc["ssid"].as<String>() == "" || doc["password"].as<String>() == "" || doc["port"].as<String>() == "")
    {
        Serial.println("Configuracao da red mesh nao achada \n");
        return false;
    }
    ssid = doc["ssid"].as<String>();
    password = doc["password"].as<String>();
    port = doc["port"].as<uint16_t>();
    Serial.println("Configuracao da red mesh estabelecida com sucesso \n");
    return true;
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

String writeIntoFileJson(JsonObject json, String nodeId, fs::FS *filesystem)
{
    JsonDocument docRes;
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
                UpdateNodes::addNodesToUpdate(nodeId);
            }
        };
    }

    return docRes.as<String>();
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
                        UpdateNodes::addNodesToUpdate(nodeId);
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
                        UpdateNodes::addNodesToUpdate(nodeId);
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


