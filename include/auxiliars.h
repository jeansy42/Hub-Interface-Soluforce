#if !defined(AUXILIARS_H)
#define AUXILIARS_H
bool initLittleFS();
bool formatLittleFS();
bool initSD();
/* bool initWiFi(); */
void createNewFile(String path, fs::FS *filesystem);
void createConfigMeshIfNotExists(fs::FS *filesystem);
String configRedMesh(JsonObject obj);
bool isRedMeshConfig();
String sendJsonResponseFromFile(String path, fs::FS *filesystem);
String writeIntoFileJson(JsonObject json, String nodeId, fs::FS *filesystem);
String updateModule(String moduleId, String nodeId, JsonObject json, fs::FS *filesystem);
String deleteModule(String moduleId, String nodeId, fs::FS *filesystem);
String getModuleFromDispositiveById(String moduleId, String nodeId, fs::FS *filesystem);


#endif // AUXILIARS_H
