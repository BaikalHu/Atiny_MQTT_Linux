/*******************************************************************************
 * Copyright (c) 2014, 2017 IBM Corp.
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
 *    Ian Craggs - return codes from linux_read
 *******************************************************************************/

#include "MQTTLinux.h"
#include "atiny_log.h"

void TimerInit(Timer *timer)
{
    timer->end_time = (struct timeval)
    {
        0, 0
    };
}

char TimerIsExpired(Timer *timer)
{
    struct timeval now, res;
    gettimeofday(&now, NULL);
    timersub(&timer->end_time, &now, &res);
    return res.tv_sec < 0 || (res.tv_sec == 0 && res.tv_usec <= 0);
}


void TimerCountdownMS(Timer *timer, unsigned int timeout)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    struct timeval interval = {timeout / 1000, (timeout % 1000) * 1000};
    timeradd(&now, &interval, &timer->end_time);
}


void TimerCountdown(Timer *timer, unsigned int timeout)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    struct timeval interval = {timeout, 0};
    timeradd(&now, &interval, &timer->end_time);
}


int TimerLeftMS(Timer *timer)
{
    struct timeval now, res;
    gettimeofday(&now, NULL);
    timersub(&timer->end_time, &now, &res);
    //printf("left %d ms\n", (res.tv_sec < 0) ? 0 : res.tv_sec * 1000 + res.tv_usec / 1000);
    return (res.tv_sec < 0) ? 0 : res.tv_sec * 1000 + res.tv_usec / 1000;
}


int linux_read(Network *n, unsigned char *buffer, int len, int timeout_ms)
{
    int fd;
    void *ctx;
    struct timeval interval = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
    if (interval.tv_sec < 0 || (interval.tv_sec == 0 && interval.tv_usec <= 0))
    {
        interval.tv_sec = 0;
        interval.tv_usec = 100;
    }
    ctx = n->ctx;
    fd = ((mqtt_context_t *)ctx)->fd;

    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&interval, sizeof(struct timeval));

    int bytes = 0;
    while (bytes < len)
    {
        int rc = recv(fd, &buffer[bytes], (size_t)(len - bytes), 0);
        if (rc == -1)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
                bytes = -1;
            break;
        }
        else if (rc == 0)
        {
            bytes = 0;
            break;
        }
        else
            bytes += rc;
    }
    return bytes;
}


int linux_write(Network *n, unsigned char *buffer, int len, int timeout_ms)
{
    struct timeval tv;
    void *ctx;
    int fd;
    ctx = n->ctx;
    fd = ((mqtt_context_t *)ctx)->fd;

    tv.tv_sec = 0;  /* 30 Secs Timeout */
    tv.tv_usec = timeout_ms * 1000;  // Not init'ing this can cause strange errors

    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(struct timeval));
    int	rc = write(fd, buffer, len);
    return rc;
}


void NetworkInit(Network *n)
{
    memset(n, 0x0, sizeof(Network));
    n->proto = MQTT_PROTO_MAX;
    n->mqttread = linux_read;
    n->mqttwrite = linux_write;
}


int NetworkConnect(Network *n, char *addr, int port)
{
    int fd;
    mqtt_context_t *ctx = NULL;
    int type = SOCK_STREAM;
    struct sockaddr_in address;
    int rc = -1;
    sa_family_t family = AF_INET;
    struct addrinfo *result = NULL;
    struct addrinfo hints = {0, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL};
    
    if ((rc = getaddrinfo(addr, NULL, &hints, &result)) == 0)
    {
        struct addrinfo *res = result;

        /* prefer ip4 addresses */
        while (res)
        {
            if (res->ai_family == AF_INET)
            {
                result = res;
                break;
            }
            res = res->ai_next;
        }

        if (result->ai_family == AF_INET)
        {
            address.sin_port = htons(port);
            address.sin_family = family = AF_INET;
            address.sin_addr = ((struct sockaddr_in *)(result->ai_addr))->sin_addr;
        }
        else
            rc = -1;

        freeaddrinfo(result);
    }

    if (rc == 0)
    {
        ctx = (mqtt_context_t *)atiny_malloc(sizeof(mqtt_context_t));
        if (NULL == ctx)
        {
            rc = -1;
        }
        else
        {
            memset(ctx, 0x0, sizeof(mqtt_context_t));
            ctx->fd = socket(family, type, 0);
            if (ctx->fd != -1)
            {
                rc = connect(ctx->fd, (struct sockaddr *)&address, sizeof(address));
                if(0 != rc)
                {
                    close(ctx->fd);
                    ctx->fd = -1;
                    atiny_free(ctx);
                    ctx = NULL;
                }
                else
                {
                    ATINY_LOG(LOG_DEBUG, "connect success.");
                    n->ctx = (void *)ctx;
                }
            }
            else
            {
                rc = -1;
                atiny_free(ctx);
                ctx = NULL;
            }
        }
    }

    return rc;
}


void NetworkDisconnect(Network *n)
{
    int fd;
    void *ctx;
    ctx = n->ctx;
    fd = ((mqtt_context_t *)ctx)->fd;
    close(fd);
}
