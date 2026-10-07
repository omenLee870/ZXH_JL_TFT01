#include "MQTTAsync.h"

int gui_mqtt_init(
  const char * clientid, 
  const char * address, 
  const char * topic, 
  int timeout, 
  int sleepCount,
  MQTTAsync_messageArrived* messageArrived);

int gui_mqtt_send(const char * topic, const char * payload);
int gui_mqtt_disconnect();