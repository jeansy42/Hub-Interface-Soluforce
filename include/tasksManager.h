#if !defined(TASKSMANAGER_H)
#define TASKSMANAGER_H

void verifyNodesToUpdate();
void reinitHub();
void validateConfigurations();

extern Task taskVerifyNodesToUpdate;
extern Task taskShouldReinitHub;
extern Task taskValidateConfigurations;
#endif