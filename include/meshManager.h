#ifndef MESH_MANAGER_H
#define MESH_MANAGER_H
#include "Arduino.h"

#define BLINKLED 2
// Callbacks
void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void droppedConnectionCallBack(uint32_t nodeId);

// Reuseful functions
void sendingConfigurationToNode(uint32_t nodeId);
void sendEventChangedConnections(String msg);

#endif
