/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Allan Stockdill-Mander - initial API and implementation and/or initial documentation
 *******************************************************************************/

#if !defined(__MQTT_LINUX_)
#define __MQTT_LINUX_

#if defined(WIN32_DLL) || defined(WIN64_DLL)
  #define DLLImport __declspec(dllimport)
  #define DLLExport __declspec(dllexport)
#elif defined(LINUX_SO)
  #define DLLImport extern
  #define DLLExport  __attribute__ ((visibility ("default")))
#else
  #define DLLImport
  #define DLLExport
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "atiny_log.h"

typedef struct Timer
{
	struct timeval end_time;
} Timer;

void TimerInit(Timer*);
char TimerIsExpired(Timer*);
void TimerCountdownMS(Timer*, unsigned int);
void TimerCountdown(Timer*, unsigned int);
int TimerLeftMS(Timer*);

typedef enum mqtt_proto
{
    MQTT_PROTO_NONE = 0,
    MQTT_PROTO_TLS_PSK,
    MQTT_PROTO_TLS_CA,
    MQTT_PROTO_MAX
}mqtt_proto_e;

typedef struct mqtt_security_psk
{
    unsigned char *psk_id;
    int psk_id_len;
    unsigned char *psk;
    int psk_len;
}mqtt_security_psk_t;

typedef struct mqtt_security_ca
{
    char *ca_crt;
    char *server_crt;
    char *server_key;
}mqtt_security_ca_t;

typedef struct mqtt_context
{
    int fd;
}mqtt_context_t;


typedef struct Network
{
    mqtt_proto_e proto;
    void *ctx;
	int (*mqttread) (struct Network*, unsigned char*, int, int);
	int (*mqttwrite) (struct Network*, unsigned char*, int, int);
    union
    {
        mqtt_security_psk_t psk;
        mqtt_security_ca_t ca;
    };

} Network;

int linux_read(Network*, unsigned char*, int, int);
int linux_write(Network*, unsigned char*, int, int);

DLLExport void NetworkInit(Network*);
DLLExport int NetworkConnect(Network*, char*, int);
DLLExport void NetworkDisconnect(Network*);

#endif
