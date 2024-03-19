#include <Arduino.h>
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "LittleFS.h"
#include "painlessMesh.h"
#include "meshManager.h"
#include "auxiliars.h"
#include "routesHandlers.h"
#include "tasksManager.h"

String ssid;
String password;
uint16_t port;

fs::FS filesystem = LittleFS;

IPAddress myIP(0, 0, 0, 0);
IPAddress myAPIP(0, 0, 0, 0);

AsyncWebServer server(80);
AsyncEventSource events("/events");
painlessMesh mesh;
Scheduler hubScheduler;

Task taskVerifyNodesToUpdate(TASK_SECOND * 4, TASK_FOREVER, &verifyNodesToUpdate);
Task taskTestingEventsToBrowser(TASK_SECOND * 5, TASK_FOREVER, &testingEventsToBrowser);
Task taskShouldReinitHub(TASK_SECOND * 5, TASK_FOREVER, &reinitHub);

bool redMeshConfigState;
String actionerMessage;
JsonDocument globalMessages;
JsonObject actioner = globalMessages["actioner"].to<JsonObject>();

void setup()
{
  Serial.begin(115200);
  if (!LittleFS.begin())
  {
    Serial.println("An Error has occurred while mounting LittleFs");
    return;
  }
  createConfigMeshIfNotExists(&filesystem);
  redMeshConfigState = isRedMeshConfig();
  /* bool status = LittleFS.format();
  if (status)
    Serial.println("LittleFS formatado corretamente");
  else
    return; */
  if (redMeshConfigState)
  {
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
    mesh.init(ssid, password, &hubScheduler, port);
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onChangedConnections(&changedConnectionCallback);
    mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
    mesh.setRoot(true); // Estabelecendo o Hub como root
    mesh.setContainsRoot(true);

    hubScheduler.addTask(taskVerifyNodesToUpdate);
    hubScheduler.addTask(taskTestingEventsToBrowser);
    taskVerifyNodesToUpdate.enable();
    taskTestingEventsToBrowser.enable();

    myAPIP = IPAddress(mesh.getAPIP());
    Serial.println("My AP IP is " + myAPIP.toString());
  }
  else
  {
    Serial.println("Setting AP (Access Point)");
    WiFi.softAP("ESP-REDMESH-MANAGER", NULL);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
  }

  hubScheduler.addTask(taskShouldReinitHub);
  taskShouldReinitHub.enable();

  // Ruta inicial
  server.on("/", HTTP_GET, handlerServeIndexHTML);

  // Ruta para configuração de red mesh
  server.on("/redMesh", HTTP_GET, handlerServeIndexHTML);

  // Ruta para conferir que esta configurada a red mesh
  server.on("/isConfigRedMesh", HTTP_GET, handlerIsConfigRedMesh);

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
  server.addHandler(handlerConfigRedMesh);
  server.addHandler(&events);
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
  if (redMeshConfigState)
  {
    mesh.update();
  }
  else
  {
    hubScheduler.execute();
  }
}
