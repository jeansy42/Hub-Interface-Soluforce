#ifndef MESH_MANAGER_H
#define MESH_MANAGER_H

#include "Arduino.h"
#include "painlessMesh.h"
// Callbacks
void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);

// Reuseful functions
void sendingConfigurationToNode(String nodeId);

#endif
