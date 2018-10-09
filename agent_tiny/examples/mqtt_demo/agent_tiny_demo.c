/*----------------------------------------------------------------------------
 * Copyright (c) <2016-2018>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of U.S. and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 *---------------------------------------------------------------------------*/

#include "agent_tiny_demo.h"
#include "atiny_log.h"
#include <pthread.h>
#include <unistd.h>

void message_cb(cloud_msg_t *msg);

#define DEFAULT_SERVER_IPV4  "127.0.0.1" //"192.168.91.131"   //"192.168.0.100"
#ifdef WITH_DTLS
#define DEFAULT_SERVER_PORT "8883"
#define AGENT_TINY_DEMO_PSK_ID "testID"
#define AGENT_TINY_DEMO_PSK_LEN (3)
unsigned char g_demo_psk[AGENT_TINY_DEMO_PSK_LEN] = {0xab, 0xcd, 0xef};
#else
#define DEFAULT_SERVER_PORT "1883"
#endif /* WITH_DTLS */

#if 1//def WITH_CA
#define MQTT_TEST_CA_CRT    \
"-----BEGIN CERTIFICATE-----\r\n"    \
"MIIDpzCCAo+gAwIBAgIJALx8oSCBKiPDMA0GCSqGSIb3DQEBDQUAMGoxFzAVBgNV\r\n"    \
"BAMMDkFuIE1RVFQgYnJva2VyMRYwFAYDVQQKDA1Pd25UcmFja3Mub3JnMRQwEgYD\r\n"    \
"VQQLDAtnZW5lcmF0ZS1DQTEhMB8GCSqGSIb3DQEJARYSbm9ib2R5QGV4YW1wbGUu\r\n"    \
"bmV0MB4XDTE4MDkyODExNDkxNloXDTMyMDkyNDExNDkxNlowajEXMBUGA1UEAwwO\r\n"    \
"QW4gTVFUVCBicm9rZXIxFjAUBgNVBAoMDU93blRyYWNrcy5vcmcxFDASBgNVBAsM\r\n"    \
"C2dlbmVyYXRlLUNBMSEwHwYJKoZIhvcNAQkBFhJub2JvZHlAZXhhbXBsZS5uZXQw\r\n"    \
"ggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDKCVvzZHV1IKD3tI8+MW/4\r\n"    \
"78WTO+2zZLMVnHC3RiZtOSV5Efr4H+OA6qduLDPlZlM7FIYyFi4NzSucySbDXVwm\r\n"    \
"7WjmmiNQUJM1EJELmc12uD1ZdRjVarc8DxU0IfBMtjHepmCuzyNx1OO7sY2TCdMY\r\n"    \
"Jfa7yBLe4IL2HyY3u4c+yqIMT8W6dAXpJFJKMLBnIPbA39aUOdJzwPS5JaWSNhfh\r\n"    \
"/BD4RoA9WeO+Svio9aoiIeUM6VtRhshclJgFZr2agCK4aIVIzGWlH0MBJWk8vwwZ\r\n"    \
"dWzucyeKG8vSh9+hatZGq+841iyA3e/wf2iK3O4SB8pyxebJsthdsHjlEtTUBozd\r\n"    \
"AgMBAAGjUDBOMB0GA1UdDgQWBBQyjKwJ4JSahePGi0GfAZc1/CkxjTAfBgNVHSME\r\n"    \
"GDAWgBQyjKwJ4JSahePGi0GfAZc1/CkxjTAMBgNVHRMEBTADAQH/MA0GCSqGSIb3\r\n"    \
"DQEBDQUAA4IBAQB7/7mBgw3FSY6VwiDUxUrAoUX8Fi5uOIWwt9GFxAZ17MaWOZZY\r\n"    \
"tJ/ZjnAfBN/lMvZCRUKbv4G/0CcIsPABSQLdIoJtsamtcCnvwvmAmc/3PNF+QXpn\r\n"    \
"R/YJzQIU9VkJztYgNDymff+mmEFdIBYnQ8P/L+cT36fsCQmXWYb5wc4ki4Tb3gAr\r\n"    \
"WHpbu+7AziW9wsM0GAOwFHkcn2eZExc0DjxHAfA+cXV1RV7Nr6J8SHkxmGg4b1oj\r\n"    \
"0CJRiXANM1CdAv5hMCznDaK4diOfnWJogN0neR5VeWmsYyTogr88s3u9B5Ogc2cm\r\n"    \
"4ee/Pwp8Tdn7vmqhGvkrTLDwIbD69HWPFWLN\r\n"                                \
"-----END CERTIFICATE-----\r\n"
const char mqtt_test_cas_pem[] = MQTT_TEST_CA_CRT;
const size_t mqtt_test_cas_pem_len = sizeof(mqtt_test_cas_pem);
#endif


#define AGENT_TINY_DEMO_CLIENT_ID "liteos_test"
#define AGENT_TINY_DEMO_USERNAME NULL
#define AGENT_TINY_DEMO_PASSWORD NULL

#define AGENT_TINY_DEMO_PUB_TOPIC "/pub_test"
#define AGENT_TINY_DEMO_SUB_TOPIC "/helloworld"

static void *g_phandle = NULL;
static atiny_device_info_t g_device_info;
static atiny_param_t g_atiny_params;

atiny_interest_uri_t g_interest_uris[ATINY_INTEREST_URI_MAX_NUM] =
{
    {
        .uri = AGENT_TINY_DEMO_SUB_TOPIC,
        .qos = CLOUD_QOS_MOST_ONCE,
        .cb = message_cb
    },
};

void message_cb(cloud_msg_t *msg)
{
    ATINY_LOG(LOG_DEBUG, "%.*s : %.*s", msg->uri_len, msg->uri, msg->payload_len,  (char *)msg->payload);
}
/*lint -e550*/
void* app_data_report(void *param)
{
    cloud_msg_t report_data;
    int ret;
    int cnt = 0;
    char payload[30] = {0};

    report_data.uri = AGENT_TINY_DEMO_PUB_TOPIC;
    report_data.qos = CLOUD_QOS_MOST_ONCE;
    report_data.method = CLOUD_METHOD_POST;
    report_data.payload = payload;
    while(1)
    {
        sprintf(payload, "publish message number %d", cnt);
        report_data.payload_len = strlen(payload);
        payload[report_data.payload_len] = '\0';
        cnt++;
        ret = atiny_data_send(g_phandle, &report_data, NULL);
        ATINY_LOG(LOG_DEBUG, "report ret:%d", ret);
        sleep(10);
    }
    return NULL;
}
/*lint +e550*/

uint32_t creat_report_task()
{
    pthread_t tidp;

    if ((pthread_create(&tidp, NULL, app_data_report, NULL)) == -1)
    {
        printf("create error!\n");
        return 1;
    }
    
    return 0;
}

void* agent_tiny_entry(void *param)
{
    uint32_t uwRet = 0;
    atiny_param_t *atiny_params;
    atiny_device_info_t *device_info = &g_device_info;

    device_info->client_id = AGENT_TINY_DEMO_CLIENT_ID;
    device_info->user_name = AGENT_TINY_DEMO_USERNAME;
    device_info->password = AGENT_TINY_DEMO_PASSWORD;
    device_info->will_flag = MQTT_WILL_FLAG_FALSE;
    device_info->will_options = NULL;
    memcpy(device_info->interest_uris, g_interest_uris, sizeof(g_interest_uris));

    atiny_params = &g_atiny_params;
    atiny_params->server_ip = DEFAULT_SERVER_IPV4;
    atiny_params->server_port = DEFAULT_SERVER_PORT;
#ifdef WITH_DTLS
    atiny_params->security_type = CLOUD_SECURITY_TYPE_PSK;
    atiny_params->u.psk.psk_id = (unsigned char *)AGENT_TINY_DEMO_PSK_ID;
    atiny_params->u.psk.psk_id_len = strlen(AGENT_TINY_DEMO_PSK_ID);
    atiny_params->u.psk.psk = g_demo_psk;
    atiny_params->u.psk.psk_len = AGENT_TINY_DEMO_PSK_LEN;
#ifdef WITH_CA
    atiny_params->security_type = CLOUD_SECURITY_TYPE_CA;
    atiny_params->u.ca.ca_crt = MQTT_TEST_CA_CRT;
    atiny_params->u.ca.server_crt = NULL;
    atiny_params->u.ca.server_key = NULL;
#endif
#else
    atiny_params->security_type = CLOUD_SECURITY_TYPE_NONE;
#endif /* WITH_DTLS */

    if(ATINY_OK != atiny_init(atiny_params, &g_phandle))
    {
        return NULL;
    }

    uwRet = creat_report_task();
    if(0 != uwRet)
    {
        return NULL;
    }

    (void)atiny_bind(device_info, g_phandle);
    return NULL;
}
