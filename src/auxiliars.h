#if !defined(AUXILIARS_H)
#define AUXILIARS_H
bool initSD();
void initWiFi(const char *ssid, const char *password);
void createNewFile(const char *path);
String sendJsonResponseFromFile(String path);
String writeIntoFileJson(JsonObject json, String nodeId);

#endif // AUXILIARS_H
