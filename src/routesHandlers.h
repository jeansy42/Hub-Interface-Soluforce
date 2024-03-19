#include "ESPAsyncWebServer.h"
#include "AsyncJson.h"

#if !defined(ROUTES_HANDLERS_H)
#define ROUTES_HANDLERS_H

void handlerServeIndexHTML(AsyncWebServerRequest *request);
void handlerGetDispositives(AsyncWebServerRequest *request);
void handlerGetDispositiveInfo(AsyncWebServerRequest *request);
void handlerGetModuleInfo(AsyncWebServerRequest *request);
void handlerDeleteModule(AsyncWebServerRequest *request);
void handlerGetModuleFromDispositiveById(AsyncWebServerRequest *request);
void handlerIsConfigRedMesh(AsyncWebServerRequest *request);

// Gerenciadores de rutas que receben json
extern AsyncCallbackJsonWebHandler *handlerSetModuleStatus;
extern AsyncCallbackJsonWebHandler *handlerAddModule;
extern AsyncCallbackJsonWebHandler *handlerConfigRedMesh;

#endif