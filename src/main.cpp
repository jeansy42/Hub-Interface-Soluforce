#include <Arduino.h>
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "SD.h"
#include "auxiliars.h"
#include "painlessMesh.h"
#include "meshManager.h"

#define MESH_PREFIX "Soluforce"
#define MESH_PASSWORD "soluforcesenha"
#define MESH_PORT 5555

IPAddress myIP(0, 0, 0, 0);
IPAddress myAPIP(0, 0, 0, 0);

AsyncWebServer server(80);

/* Scheduler userScheduler; */
extern painlessMesh mesh;

void setup()
{
  Serial.begin(115200);

  if (!initSD())
    return;

  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.setRoot(true); // Estabelecendo o Hub como root
  mesh.setContainsRoot(true);

  myAPIP = IPAddress(mesh.getAPIP());
  Serial.println("My AP IP is " + myAPIP.toString());

  // Ruta inicial
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SD, "/index.html", String(), false); });

  // Ruta dispositives/:nodeId/:typeModule
  server.on("^\\/dispositives\\/([0-9]+)\\/([a-zA-Z0-9]+)$", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SD, "/index.html", String(), false); });

  // Ruta /dispositives
  server.on("/dispositives", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SD, "/index.html", String(), false); });

  // Ruta dinamica /dispositives/:nodeID/addModule
  server.on("^\\/addModule\\/([0-9]+)$", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SD, "/index.html", String(), false); });

  // Ruta dinamica /dispositive/nodeID
  server.on("^\\/dispositive\\/([0-9]+)$", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SD, "/index.html", String(), false); });

  // Petição GET desde o cliente á ruta dinamica /getDispositives/<NodeID> para obter a lista de nodos conetados
  server.on("/getDispositives", HTTP_GET, [](AsyncWebServerRequest *request)
            { std::list<uint32_t> nodes = mesh.getNodeList(false);
              JsonDocument  doc;
              JsonArray jsonArray = doc.to<JsonArray>();
              for (const auto &node : nodes)
              {
                jsonArray.add(node);
              };
              String nodeString=doc.as<String>(); 
              
              
              request->send(200,"application/json", nodeString); });

  // Petição GET desde o cliente á ruta dinamica /getDispositiveInfo/<NodeID> para obter a lista de modulos do nodo
  server.on("^\\/getDispositiveInfo\\/([0-9]+)$", HTTP_GET, [](AsyncWebServerRequest *request)
            { String pathToDispositive="/"+ request->pathArg(0) + ".json";
              String res=sendJsonResponseFromFile(pathToDispositive);
              request->send(200,"application/json", res); });

  // Gerenciando rutas que recebem json como request
  AsyncCallbackJsonWebHandler *
      handler = new AsyncCallbackJsonWebHandler("/addModule", [](AsyncWebServerRequest *request, JsonVariant &json)
                                                { 
    JsonObject jsonObj = json.as<JsonObject>();
    String nodeID = jsonObj["nodeId"];
    jsonObj.remove("nodeId");
    String res = writeIntoFileJson(jsonObj, nodeID);
    request->send(200, "application/json", res); });

  server.addHandler(handler);
  // Control de arquivos estaticos
  server.serveStatic("/", SD, "/");
  server.serveStatic("/addModule", SD, "/");
  server.serveStatic("/dispositives", SD, "/");
  server.serveStatic("/dispositive", SD, "/");

  // Iniciando o servidor
  server.begin();
}

void loop()
{
  mesh.update();
}
