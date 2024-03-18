#include <Arduino.h>
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "SPIFFS.h"
#include "painlessMesh.h"
#include "meshManager.h"
#include "auxiliars.h"
#include "routesHandlers.h"
#include "tasksManager.h"

#define MESH_PREFIX "Soluforce"
#define MESH_PASSWORD "soluforcesenha"
#define MESH_PORT 5555

fs::FS filesystem = SPIFFS;

IPAddress myIP(0, 0, 0, 0);
IPAddress myAPIP(0, 0, 0, 0);

AsyncWebServer server(80);
painlessMesh mesh;
Scheduler hubScheduler;

Task taskVerifyNodesToUpdate(TASK_SECOND * 4, TASK_FOREVER, &verifyNodesToUpdate);

String actionerMessage;
JsonDocument globalMessages;
JsonObject actioner = globalMessages["actioner"].to<JsonObject>();

void setup()
{
  Serial.begin(115200);
  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  /* bool status = SPIFFS.format();
  if (status)
    Serial.println("SPIFFS formatado corretamente");
  else
    return; */

  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &hubScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.setRoot(true); // Estabelecendo o Hub como root
  mesh.setContainsRoot(true);

  hubScheduler.addTask(taskVerifyNodesToUpdate);
  taskVerifyNodesToUpdate.enable();

  myAPIP = IPAddress(mesh.getAPIP());
  Serial.println("My AP IP is " + myAPIP.toString());

  // Ruta inicial
  server.on("/", HTTP_GET, handlerServeIndexHTML);

  // Ruta dispositives/:nodeId/:typeModule
  server.on("^\\/dispositives\\/([0-9]+)\\/([a-zA-Z0-9]+)$", HTTP_GET, handlerServeIndexHTML);

  // Ruta /dispositives
  server.on("/dispositives", HTTP_GET, handlerServeIndexHTML);

  // Ruta dinamica /dispositives/:nodeID/addModule para agregar um modulo a um determinado nó.
  server.on("^\\/addModule\\/([0-9]+)$", HTTP_GET, handlerServeIndexHTML);

  // Ruta dinamica /dispositive/nodeID para ver os modulos por nós
  server.on("^\\/dispositive\\/([0-9]+)$", HTTP_GET, handlerServeIndexHTML);

  // Petição GET desde o cliente á ruta dinamica /getDispositives/<NodeID> para obter a lista de nodos conetados
  server.on("/getDispositives", HTTP_GET, handlerGetDispositives);

  // Petição GET desde o cliente á ruta dinamica /getDispositiveInfo/<NodeID> para obter a lista de modulos do nó
  server.on("^\\/getDispositiveInfo\\/([0-9]+)$", HTTP_GET, handlerGetDispositiveInfo);

  // Obtendo informação dos modulos
  server.on("^\\/getModuleInfo\\/([0-9]+)\\/([a-zA-Z0-9]+)$", HTTP_GET, handlerGetModuleInfo);

  // Apagando modulo do nó
  server.on("/deleteModule", HTTP_DELETE, handlerDeleteModule);

  // Obtendo a informação de um modulo de determinado nó
  server.on("/getModuleFromDispositiveById", HTTP_GET, handlerGetModuleFromDispositiveById);

  server.addHandler(handlerAddModule);
  server.addHandler(handlerSetModuleStatus);
  // Control de arquivos estaticos
  server.serveStatic("/", filesystem, "/");
  server.serveStatic("/addModule", filesystem, "/");
  server.serveStatic("/dispositives", filesystem, "/");
  server.serveStatic("/dispositive", filesystem, "/");

  // Iniciando o servidor
  server.begin();
}

void loop()
{
  mesh.update();
}
