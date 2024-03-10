#include "ESPAsyncWebServer.h"
#include "SD.h"
#include "painlessMesh.h"
#include "AsyncJson.h"
#include "auxiliars.h"


extern painlessMesh mesh;
extern JsonDocument globalMessages;
extern fs::FS filesystem;

void handlerServeIndexHTML(AsyncWebServerRequest *request)
{
    request->send(SD, "/index.html", String(), false);
}
void handlerGetDispositives(AsyncWebServerRequest *request)
{
    std::list<uint32_t> nodes = mesh.getNodeList(false);
    JsonDocument doc;
    JsonArray jsonArray = doc.to<JsonArray>();
    for (const auto &node : nodes)
    {
        jsonArray.add(node);
    };
    String nodeString = doc.as<String>();

    request->send(200, "application/json", nodeString);
}
void handlerGetDispositiveInfo(AsyncWebServerRequest *request)
{
    String pathToDispositive = "/" + request->pathArg(0) + ".json";
    String res = sendJsonResponseFromFile(pathToDispositive, &filesystem);
    request->send(200, "application/json", res);
}
void handlerDeleteModule(AsyncWebServerRequest *request)
{
    if (!(request->hasParam("moduleId") && request->hasParam("nodeId")))
    {
        request->send(400, "application/json", "{\"error\":\"Bad request, query param moduleId and nodeId are required.\"}");
    }
    else
    {
        String nodeId = request->getParam("nodeId")->value();
        String moduleId = request->getParam("moduleId")->value();
        String res = deleteModule(moduleId, nodeId, &filesystem);
        request->send(200, "application/json", res);
    }
}
void handlerGetModuleInfo(AsyncWebServerRequest *request)
{
    String nodeId = request->pathArg(0);
    String module = request->pathArg(1);
    if (globalMessages.containsKey(module) && globalMessages[module].containsKey(nodeId))
    {
        String res = globalMessages[module][nodeId].as<String>();
        request->send(200, "text/plain", res);
    }
    else
    {
        request->send(404, "text/plain", "Error: Not found");
    }
}

// Gerenciadores
AsyncCallbackJsonWebHandler *handlerSetModuleStatus = new AsyncCallbackJsonWebHandler("/setModuleStatus", [](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                      {
    JsonObject jsonObj = json.as<JsonObject>();
    if (!(jsonObj.containsKey("nodeId") && jsonObj.containsKey("type") && jsonObj.containsKey("action")))
    {
      request->send(400, "text/plain", "Error: Bad request");
    }
    else
    {
      uint32_t nodeId = jsonObj["nodeId"];
      String type = jsonObj["type"];
      int action = jsonObj["action"];
      if(mesh.isConnected(nodeId)){
        JsonDocument doc;
        String res;
        JsonObject typeObj= doc[type].to<JsonObject>();
        typeObj["setStatus"]=action;
        serializeJson(doc,res);
        mesh.sendSingle(nodeId, res);
        request->send(200, "application/json", res);
      }else
      {
        request->send(503, "application/json", "{\"error\":\"Node is not connected.\"}");
      };
      } });

AsyncCallbackJsonWebHandler *handlerAddModule = new AsyncCallbackJsonWebHandler("/addModule", [](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                {
    JsonObject jsonObj = json.as<JsonObject>();
    if (jsonObj.containsKey("name") && jsonObj.containsKey("description") && jsonObj.containsKey("type"))
    {
        String nodeId = jsonObj["nodeId"];
        jsonObj.remove("nodeId");
        String res;

            if (request->method() == HTTP_POST)
        {
            res = writeIntoFileJson(jsonObj, nodeId,&filesystem);
            request->send(200, "application/json", res);

        }else if(request->method()==HTTP_PUT)
        {
            if(!request->hasParam("moduleId"))
            {
                request->send(400, "application/json", "{\"error\":\"Bad request, query param moduleId is required.\"}");
            }
            else
            {
                String moduleId = request->getParam("moduleId")->value();
                res=updateModule(moduleId,nodeId,json,&filesystem);
                request->send(200, "application/json", res);
                
            }
            
        }
    }else
    {
        request->send(400, "application/json", "{\"error\":\"Bad request\"}");
    }; });