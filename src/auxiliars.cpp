#include "SD.h"
#include "WiFi.h"
#include "ArduinoJson.h"

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
    if (!SD.begin(5))
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

void createNewFile(const char *path)
{
    if (!SD.exists(path))
    {

        Serial.printf("Criando arquivo: %s\n", path);
        File file = SD.open(path, "w");
        if (!file)
        {
            Serial.println("Erro ao criar o arquivo");
            return;
        }
        else
        {
            JsonDocument doc;
            deserializeJson(doc, file);
            JsonArray array = doc.to<JsonArray>();
            serializeJson(doc, file);
            Serial.printf("%s criado com sucesso \n", path);
        }
        file.close();
    }
}
String sendJsonResponseFromFile(String path)
{
    if (!SD.exists(path))
    {
        JsonDocument doc;
        JsonArray array = doc.to<JsonArray>();
        String emptyArray = doc.as<String>();
        return emptyArray;
    }
    JsonDocument docRes;
    File file = SD.open(path, "r");
    deserializeJson(docRes, file);
    String response = docRes.as<String>();
    file.close();
    return response;
}

String writeIntoFileJson(JsonObject json, String nodeId)
{
    JsonDocument docRes;
    String resString;
    String path = "/" + nodeId + ".json";
    if (!SD.exists(path))
    {
        docRes["status"] = "error";
        docRes["msg"] = "The file not exists";
    }
    else
    {
        File file = SD.open(path, "r");
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

            JsonObject obj = doc.add<JsonObject>();
            obj["id"] = generateUUID();
            for (JsonPair kv : json)
            {
                obj[kv.key().c_str()] = kv.value();
            };
            file = SD.open(path, "w");
            serializeJson(doc, file);
            docRes["status"] = "ok";
            docRes["msg"] = "Success to append information to the file.";
        };
        file.close();
    }
    serializeJson(docRes, resString);
    return resString;
}