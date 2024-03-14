#include "structures.h"
#if !defined(AUXILIARS_H)
#define AUXILIARS_H
bool initSPIFFS();
bool initSD();
void initWiFi(const char *ssid, const char *password);
void createNewFile(String path, fs::FS *filesystem);
String sendJsonResponseFromFile(String path, fs::FS *filesystem);
isConfigNode writeIntoFileJson(JsonObject json, String nodeId, fs::FS *filesystem);
String updateModule(String moduleId, String nodeId, JsonObject json, fs::FS *filesystem);
String deleteModule(String moduleId, String nodeId, fs::FS *filesystem);
String getModuleFromDispositiveById(String moduleId, String nodeId, fs::FS *filesystem);

#endif // AUXILIARS_H
