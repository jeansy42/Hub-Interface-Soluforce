#include "ESPAsyncWebServer.h"
#include "painlessMesh.h"
#include "AsyncJson.h"
#include "auxiliars.h"
#include "meshManager.h"
#include "structures.h"

extern painlessMesh mesh;
extern JsonDocument globalMessages;
extern fs::FS filesystem;

extern String ssid;
extern String password;
extern uint16_t port;

extern bool redMeshConfigState;

void handlerServeIndexHTML(AsyncWebServerRequest *request)
{
    request->send(filesystem, "/index.html", String(), false);
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
        request->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Bad request, query param moduleId and nodeId are required.\"}");
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
        JsonDocument docRes;
        docRes["status"] = res;
        docRes["msg"] = "Informação do modulo recebida con sucesso.";
        request->send(200, "application/json", docRes.as<String>());
    }
    else
    {
        request->send(404, "application/json", "{\"status\":\"error\",\"msg\":\"Module não achado\"}");
    }
}

void handlerGetModuleFromDispositiveById(AsyncWebServerRequest *request)
{
    if (request->hasParam("moduleId") && request->hasParam("nodeId"))
    {
        String moduleId = request->getParam("moduleId")->value();
        String nodeId = request->getParam("nodeId")->value();
        String res = getModuleFromDispositiveById(moduleId, nodeId, &filesystem);
        request->send(200, "application/json", res);
    }
    else
    {
        request->send(400, "application/json", "{\"error\":\"Bad request, query param moduleId and nodeId are required.\"}");
    }
}

void handlerIsConfigRedMesh(AsyncWebServerRequest *request)
{
    if (redMeshConfigState)
    {
        request->send(200, "text/plain", "true");
    }
    else
    {
        request->send(200, "text/plain", "false");
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
        String msg;
        JsonObject typeObj= doc[type].to<JsonObject>();
        typeObj["setStatus"]=action;
        serializeJson(doc,msg);
        bool state=mesh.sendSingle(nodeId, msg);
        if(state){request->send(200, "application/json", "{\"status\":\"success\",\"msg\":\"Mensagem enviado ao nó com sucesso\"}");}
        else
        {
            request->send(503, "application/json", "{\"status\":\"error\",\"msg\":\"Erro ao enviar o mensagem ao nó\"}");
        }
      }else
      {
          request->send(503, "application/json", "{\"status\":\"error\",\"msg\":\"O nó solicitado não está conetado à red mesh\"}");
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

AsyncCallbackJsonWebHandler *handlerConfigRedMesh = new AsyncCallbackJsonWebHandler("/redMeshConfig", [](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                    {
    JsonObject jsonObj = json.as<JsonObject>();
    if (jsonObj.containsKey("ssid") && jsonObj.containsKey("password") && jsonObj.containsKey("port")){
        String res=configRedMesh(jsonObj);
        request->send(200,"application/json",res);
        }else{
        request->send(400, "application/json", "{\"error\":\"Bad request, ssid, password and port are required.\"}");
    } });
