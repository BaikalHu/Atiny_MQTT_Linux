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
#include "cJSON.h"
#include <pthread.h>
#include <unistd.h>

void dev_message_cb(cloud_msg_t *msg);
void twins_message_cb(cloud_msg_t *msg);
void dev_message_update_cb(cloud_msg_t *msg);
void twins_message_update_cb(cloud_msg_t *msg);
void dev_message_handle_cb(cloud_msg_t *msg);


#define DEFAULT_SERVER_IPV4 "122.112.225.88" //"127.0.0.1" //"192.168.91.131"   //"192.168.0.100"
#ifdef WITH_DTLS
#define DEFAULT_SERVER_PORT "8883"
#define AGENT_TINY_DEMO_PSK_ID "testID"
#define AGENT_TINY_DEMO_PSK_LEN (3)
unsigned char g_demo_psk[AGENT_TINY_DEMO_PSK_LEN] = {0xab, 0xcd, 0xef};
#define SERVER_NAME "kubernetes.default.svc"
const char server_name[] = SERVER_NAME;
#else
#define DEFAULT_SERVER_PORT "1883"
#endif /* WITH_DTLS */

#ifdef WITH_CA_UNI
#define MQTT_TEST_CA_CRT    \
"-----BEGIN CERTIFICATE-----\r\n"    \
"MIICxjCCAa6gAwIBAgIJAJk1DbZBu8FDMA0GCSqGSIb3DQEBCwUAMBMxETAPBgNV\r\n"    \
"BAMMCE15VGVzdENBMB4XDTE3MTEwMjEzNDI0N1oXDTE5MTEwMjEzNDI0N1owEzER\r\n"    \
"MA8GA1UEAwwITXlUZXN0Q0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIB\r\n"    \
"AQDshDho6ef1JClDJ24peSsXdFnFO3xIB7+BSp1YPcOvmRECKUG0mLORw3hNm15m\r\n"    \
"8eGOn1iLGE/xKlaZ74/xjyq8f7qIGZCmvZj59m+eiJCAmy8SiUJZtSVoOlOzepJd\r\n"    \
"PoDgcBvDKA4ogZ3iJHMUNI3EdlD6nrKEJF2qe2JUrL0gv65uo2/N7XVNvE87Dk3J\r\n"    \
"83KyCAmeu+x+moS1ILnjs2DuPEGSxZqzf7IQMbXuNWJYAOZg9t4Fg0YjTiAaWw3G\r\n"    \
"JKAoMY4tI3JCqlvwGR4lH7kfk3WsD4ofGlFhxU4nEG0xgnJl8BcoJWD1A2RjGe1f\r\n"    \
"qCijqPSe93l2wt8OpbyHzwc7AgMBAAGjHTAbMAwGA1UdEwQFMAMBAf8wCwYDVR0P\r\n"    \
"BAQDAgEGMA0GCSqGSIb3DQEBCwUAA4IBAQAi+t5jBrMxFzoF76kyRd3riNDlWp0w\r\n"    \
"NCewkohBkwBHsQfHzSnc6c504jdyzkEiD42UcI8asPsJcsYrQ+Uo6OBn049u49Wn\r\n"    \
"zcSERVSVec1/TAPS/egFTU9QMWtPSAm8AEaQ6YYAuiwOLCcC+Cm/a3e3dWSRWt8o\r\n"    \
"LqKX6CWTlmKWe182MhFPpZYxZQLGapti4R4mb5QusUbc6tXbkcX82GjDPTOuAw7b\r\n"    \
"mWpzVd5xnlp7Vz+50u+YaAYUmCobg0hR/AuTrA4GDMlgzTnuZQhF6o8iVkypXOtS\r\n"    \
"Ufz6X3tVVErVVc7UUfzSnupHj1M2h4rzlQ3oqHoAEnXcJmV4f/Pf/6FW\r\n"    \
"-----END CERTIFICATE-----\r\n"
const char mqtt_test_cas_pem[] = MQTT_TEST_CA_CRT;
const size_t mqtt_test_cas_pem_len = sizeof(mqtt_test_cas_pem);
#endif

#if 0
#ifdef WITH_CA_BI
#define MQTT_TEST_CA_CRT    \
"-----BEGIN CERTIFICATE-----\r\n"	 \
"MIICxjCCAa6gAwIBAgIJAJk1DbZBu8FDMA0GCSqGSIb3DQEBCwUAMBMxETAPBgNV\r\n"	  \
"BAMMCE15VGVzdENBMB4XDTE3MTEwMjEzNDI0N1oXDTE5MTEwMjEzNDI0N1owEzER\r\n"	  \
"MA8GA1UEAwwITXlUZXN0Q0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIB\r\n"	  \
"AQDshDho6ef1JClDJ24peSsXdFnFO3xIB7+BSp1YPcOvmRECKUG0mLORw3hNm15m\r\n"	  \
"8eGOn1iLGE/xKlaZ74/xjyq8f7qIGZCmvZj59m+eiJCAmy8SiUJZtSVoOlOzepJd\r\n"	  \
"PoDgcBvDKA4ogZ3iJHMUNI3EdlD6nrKEJF2qe2JUrL0gv65uo2/N7XVNvE87Dk3J\r\n"	  \
"83KyCAmeu+x+moS1ILnjs2DuPEGSxZqzf7IQMbXuNWJYAOZg9t4Fg0YjTiAaWw3G\r\n"	  \
"JKAoMY4tI3JCqlvwGR4lH7kfk3WsD4ofGlFhxU4nEG0xgnJl8BcoJWD1A2RjGe1f\r\n"	  \
"qCijqPSe93l2wt8OpbyHzwc7AgMBAAGjHTAbMAwGA1UdEwQFMAMBAf8wCwYDVR0P\r\n"	  \
"BAQDAgEGMA0GCSqGSIb3DQEBCwUAA4IBAQAi+t5jBrMxFzoF76kyRd3riNDlWp0w\r\n"	  \
"NCewkohBkwBHsQfHzSnc6c504jdyzkEiD42UcI8asPsJcsYrQ+Uo6OBn049u49Wn\r\n"	  \
"zcSERVSVec1/TAPS/egFTU9QMWtPSAm8AEaQ6YYAuiwOLCcC+Cm/a3e3dWSRWt8o\r\n"	  \
"LqKX6CWTlmKWe182MhFPpZYxZQLGapti4R4mb5QusUbc6tXbkcX82GjDPTOuAw7b\r\n"	  \
"mWpzVd5xnlp7Vz+50u+YaAYUmCobg0hR/AuTrA4GDMlgzTnuZQhF6o8iVkypXOtS\r\n"	  \
"Ufz6X3tVVErVVc7UUfzSnupHj1M2h4rzlQ3oqHoAEnXcJmV4f/Pf/6FW\r\n"	  \
"-----END CERTIFICATE-----\r\n"
const char mqtt_test_cas_pem[] = MQTT_TEST_CA_CRT;
const size_t mqtt_test_cas_pem_len = sizeof(mqtt_test_cas_pem);

#define MQTT_TEST_CLI_CRT \
"-----BEGIN CERTIFICATE-----\r\n"	 \
"MIIC6jCCAdKgAwIBAgIBAjANBgkqhkiG9w0BAQsFADATMREwDwYDVQQDDAhNeVRl\r\n"	 \
"c3RDQTAeFw0xNzExMDIxMzQyNDhaFw0xOTExMDIxMzQyNDhaMC0xGjAYBgNVBAMT\r\n"	 \
"EU1hY0Jvb2stQWlyLmxvY2FsMQ8wDQYDVQQKEwZjbGllbnQwggEiMA0GCSqGSIb3\r\n"	 \
"DQEBAQUAA4IBDwAwggEKAoIBAQC8GptpL25hv1Qa3jCn4VLvDRH/SrHg9wXvqRkz\r\n"	 \
"HuiKMxYT30m4+kcaXv350CJrkV+8lR24wdN7DBVewpCUnyUBbzkLccy1LUzunZ3z\r\n"	 \
"nm37j6cautD3rlC9gsC9d0uJ745FLx5t/6f1jMk9rWxn+4iSGAnkWC3mVaQxP1zQ\r\n"	 \
"q8GI97uob9HNb0OH6ygHJAcKOWB+85a29LIMa1uo/lT3hMr8sBg2vX+1F/gTusmW\r\n"	 \
"xVoQc9XJxBCs995qsH0UkZIuOY0XZp9/qFfcZv2QmslG8DojIIHKcujzu8bItE2M\r\n"	 \
"OyL5NlWLvN6qg59hHzF4+D+T+8GkhhKWSC+xdY14eQ5fB4S5AgMBAAGjLzAtMAkG\r\n"	 \
"A1UdEwQCMAAwCwYDVR0PBAQDAgeAMBMGA1UdJQQMMAoGCCsGAQUFBwMCMA0GCSqG\r\n"	 \
"SIb3DQEBCwUAA4IBAQBLV4ZfhiKiFVnL/xO0MRGSKr3xd0LK64SW8Iw5DYkc0jNX\r\n"	 \
"sDrRbj2I/KJ/Rc4AeKT751L+C+KBzYpFgiLrxDmt/5pmgiFH51hPQtL7kRC0z2NY\r\n"	 \
"EY/P+u4IFVSo+b1hHYU7y+OMj6/Vvd4x0ETS4rHWI4mPDfGfvClEVLOktgRKrMU5\r\n"	 \
"9aTltF4U0FBUlYZTQBNBUFwBzj1+0lxK4EdhRmmWJ+uW9rgkQxpnUdbCPGvUKFRp\r\n"	 \
"3AbdHBAU9H2zVd2VZoJu6r7LMp6agxu0rYLgmamRAt+8rnDXvy7H1ZNdjT6fTbUO\r\n"	 \
"omVBMyJAc1+10gjpHw/EUD58t5/I5tZrnrANPgIs\r\n"	 \
"-----END CERTIFICATE-----\r\n"

const char mqtt_test_cli_pem[] = MQTT_TEST_CLI_CRT;
const size_t mqtt_test_cli_pem_len = sizeof(mqtt_test_cli_pem);

#define MQTT_TEST_CLI_KEY \
"-----BEGIN RSA PRIVATE KEY-----\r\n"	 \
"MIIEowIBAAKCAQEAvBqbaS9uYb9UGt4wp+FS7w0R/0qx4PcF76kZMx7oijMWE99J\r\n"	 \
"uPpHGl79+dAia5FfvJUduMHTewwVXsKQlJ8lAW85C3HMtS1M7p2d855t+4+nGrrQ\r\n"	 \
"965QvYLAvXdLie+ORS8ebf+n9YzJPa1sZ/uIkhgJ5Fgt5lWkMT9c0KvBiPe7qG/R\r\n"	 \
"zW9Dh+soByQHCjlgfvOWtvSyDGtbqP5U94TK/LAYNr1/tRf4E7rJlsVaEHPVycQQ\r\n"	 \
"rPfearB9FJGSLjmNF2aff6hX3Gb9kJrJRvA6IyCBynLo87vGyLRNjDsi+TZVi7ze\r\n"	 \
"qoOfYR8xePg/k/vBpIYSlkgvsXWNeHkOXweEuQIDAQABAoIBAHnFV7peRDzvGUlT\r\n"	 \
"cXgcvA2ZDn+QIVsbTzJ466FWbv+YVsCCmj0veHwv5oakIMQ2Fh4FAnqqr3dGuUbg\r\n"	 \
"+avc4p3tHKa2Aul+7ADE9I3TkCt8MZdyPPk6VXZ5gMCmy7X96MIM4Mwg5uBlRZmx\r\n"	 \
"/S3Lffvlp/G0y/ICmwpulG1Z4y4A5Vc0Qf7fBO03Ekl31oReARnB6ex7RnDHH1mW\r\n"	 \
"RyLWNqyu9BhUbFpIyFPWDSkBcajNIbQ6qVJfGLm5Y2xVhwdqbyvY8M06uuMKz/IR\r\n"	 \
"SYfdIpiC4PpQZQzzXMn/6LTKWcCe0T+dBcWTZHC3C2abrC7+5fwFobs2xoUaCwz0\r\n"	 \
"1CclogECgYEA6Jdv+2VSYIBLbS0VIe07JiZaQNd1QNg63MK/y7oqAKEzYvpWzJel\r\n"	 \
"owPdBU3GxZH6vUUF7sCABcjumEDazoqTtzHQBo0xYpJrjmAL0ANNGVvF09pJK2eq\r\n"	 \
"yotxJJAS5/lQNSgWOxGVc6qu6ZpgeIXVLIx816yq04h10yVgZ2Lm3+ECgYEAzwj5\r\n"	 \
"/UVpN/ak6PwZ+Tq/8qOYjY2ABylRmP+T0Rkqmwh2B5Sp9oXjkDQwWseY0Wybhd8F\r\n"	 \
"kO6BUCMUApnB3uU0baawVbDUSrt43SkkKV9m3pA35wA3pYw1a56QIEFr56npFYBS\r\n"	 \
"sn9yl/ZtNvnuwmrHWOq8HdwPJsWREyO61yknn9kCgYB1PdixpSo4AJOErePoHRfi\r\n"	 \
"rBR0eObez+Aj5Xsea3G+rYMkkkHskUhp+omPodvfPS1h+If8CEbAI7+5OX/R+uJo\r\n"	 \
"xpAwrT1Gjb3vn5R0vyU+8havKmoVmgTqYg2fO4x8KBz5HoLONZfbHR9cG3gjaHrD\r\n"	 \
"IPHRGXVmeXPDAiUtGBp+oQKBgQDDRIAkNPdMZUCczknhG1w3Cb20pKUAHCRt3YAZ\r\n"	 \
"U1cv6gcIl1rGvPko5VBGDsM/ouP8m6CwVYN5hdw1p7eG9z8/vFvMNn/EDJWuYkNN\r\n"	 \
"EkH/4J4ZLcdOSLOJ0X+2LH4Nfd/s+58D49i9IxtXItviWruyTZMnxooz01tFZgmv\r\n"	 \
"LY3F4QKBgDirafhlJqFK6sa8WesHpD5+lm3Opzi4Ua8fAGHy2oHN3WCEL74q691C\r\n"	 \
"fA0P2UrzYiF7dXf4fgK9eMMQsdWS4nKyCSqM6xE4EAhAHUTYzY3ApNjI3XFDIrKC\r\n"	 \
"oQefIOLum2UyWFuEoUtrEfc5fxktiQohCwuAvwC59EwhmsNlECA8\r\n"	 \
"-----END RSA PRIVATE KEY-----\r\n"

const char mqtt_test_cli_key[] = MQTT_TEST_CLI_KEY;
const size_t mqtt_test_cli_key_len = sizeof(mqtt_test_cli_key);

#endif
#endif

#ifdef WITH_CA_BI
#define MQTT_TEST_CA_CRT    \
"-----BEGIN CERTIFICATE-----\r\n"    \
"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\r\n"    \
"-----END CERTIFICATE-----\r\n"
const char mqtt_test_cas_pem[] = MQTT_TEST_CA_CRT;
const size_t mqtt_test_cas_pem_len = sizeof(mqtt_test_cas_pem);

#define MQTT_TEST_CLI_CRT \
"-----BEGIN CERTIFICATE-----\r\n"    \
"MIIG1zCCBL+gAwIBAgIISLnoJL0MGlAwDQYJKoZIhvcNAQELBQAwgZIxCzAJBgNV\r\n"    \
"BAYTAkNOMRIwEAYDVQQIEwlHdWFuZ0RvbmcxETAPBgNVBAcTCFNoZW5aaGVuMSUw\r\n"    \
"IwYDVQQKExxIdWF3ZWkgVGVjaG5vbG9naWVzIENvLiwgTHRkMSEwHwYDVQQLFBhP\r\n"    \
"U1MgJiBTZXJ2aWNlIFRvb2xzIERlcHQxEjAQBgNVBAMTCU9TUzMuMCBDQTAeFw0x\r\n"    \
"ODExMDgxNjAwMDBaFw0yMzExMDgxNjAwMDBaMIICTDELMAkGA1UEBhMCQ04xEjAQ\r\n"    \
"BgNVBAgTCUd1YW5nRG9uZzERMA8GA1UEBxMIU2hlblpoZW4xJTAjBgNVBAoTHEh1\r\n"    \
"YXdlaSBUZWNobm9sb2dpZXMgQ28uLCBMdGQxITAfBgNVBAsMGE9TUyAmIFNlcnZp\r\n"    \
"Y2UgVG9vbHMgRGVwdDFYMFYGA1UEAxNPY2IxZDMzNDMzZTI2NDhiMGEwZjQwYWQx\r\n"    \
"YTBjNWZmYmIuODk0ZDQ4ZjgtNDVjMC00YWM0LThkMTgtZjViOTgzZWY5MTU5IE9T\r\n"    \
"UzMuMCBDQTFGMEQGAykBARM9c3lzdGVtOm5vZGU6aHJuOmhlYzplZGdlOjg5NGQ0\r\n"    \
"OGY4LTQ1YzAtNGFjNC04ZDE4LWY1Yjk4M2VmOTE1OTFZMFcGAykBAgxQY2IxZDMz\r\n"    \
"NDMzZTI2NDhiMGEwZjQwYWQxYTBjNWZmYmI6Y2IxZDMzNDMzZTI2NDhiMGEwZjQw\r\n"    \
"YWQxYTBjNWZmYmI6b3BfY2ZlX2t1YmVsZXQxWDBWBgMpAQIMT2NiMWQzMzQzM2Uy\r\n"    \
"NjQ4YjBhMGY0MGFkMWEwYzVmZmJiOmNiMWQzMzQzM2UyNjQ4YjBhMGY0MGFkMWEw\r\n"    \
"YzVmZmJiOm9wX3Jlc3RyaWN0ZWQxSjBIBgMpAQMTQTI0Y2JjYzFhOTMxYzRlOWNi\r\n"    \
"MzVlMDA1YmZmMjRkN2ZhOjI0Y2JjYzFhOTMxYzRlOWNiMzVlMDA1YmZmMjRkN2Zh\r\n"    \
"MSkwJwYDKQEEEyBjYjFkMzM0MzNlMjY0OGIwYTBmNDBhZDFhMGM1ZmZiYjCCASIw\r\n"    \
"DQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKkbiSGVWJ1xNqpnhIRnscza4r94\r\n"    \
"rg7/BIfP+8Voj0w8/IcEjJbF7UiNmjZpOQhzdrViL8Fe1LyzylRHio7BwTi94lad\r\n"    \
"EWD58tzDR7y5kpjdOoU6j++GEBkBDk2it8qe4SKTyt8V7xhJ00lzeJ0oTnN4kGqM\r\n"    \
"/bVu9YTDiuE1gtN9UdRP2MfU4wEycFuCvbGFsKuHOENAmqg9pgMSGXrriQjWoydt\r\n"    \
"xz2OZhNQEXRUM6vh0YhNfVswDNu9IEMczU4kSoQxwJbzl+iMXHtGVMckTVAsLu7J\r\n"    \
"G7sRcFiRSG2YUHDOlUwGJH7oEs4tGDTuErS9g0rJtw6jNNsXpXIA63QMrZMCAwEA\r\n"    \
"AaN0MHIwDgYDVR0PAQH/BAQDAgKkMBMGA1UdJQQMMAoGCCsGAQUFBwMCMAwGA1Ud\r\n"    \
"EwEB/wQCMAAwHwYDVR0jBBgwFoAUhGSkEFXIWQXWwjYIS3nQnHzWqPYwHAYDVR0R\r\n"    \
"BBUwE4ILZWRnZS1kZXZpY2WHBAAAAAAwDQYJKoZIhvcNAQELBQADggIBAFPV86WP\r\n"    \
"1vj9cq9OKt3MO1VCbcmMmzxeD3Heraiedc6E/JhAvcjZliZxGBXeh2xRmk6iHlXx\r\n"    \
"s3VTFQ8QWqr/MtECb6mMKkL54uDDCBuPVC+eeQtNwzeHV7RJ2aN6H/WNE8blPI7C\r\n"    \
"/YArU/mvkBQ1+9p9O3TQVO7FrKMgJNrSwGthPvM5N0GU7NYzy9Fm1Qg2t7uwsfyv\r\n"    \
"FkMK67BrTYsLG/qF8ZXLW5hnUiljPgMWOAhkKplJFtz10kcSZoHl6xsAe0N48uPc\r\n"    \
"BWzQDDh+h6DWKxcwg1i6fw3d1ojY9lmGryKAguK44AzksnTylVMlDGw7i35k0SkZ\r\n"    \
"bXVuJhWkXlhwhV5iDc/duzUaS/C0Vnod0hGQBZDEHfe+EqY6EvurTwYg3PsBfPs2\r\n"    \
"VZgoHCB+asoW4UHYGILf17PNVmTpyjAE3tRS6iDXEy75ikauMJt4YVYwmebYZ6F8\r\n"    \
"oGqcWymgvw6ELwWnEf247/igOy9gVjCjYXvCPSyAsXqkqDlaBHkdWsHoaNNvE76Y\r\n"    \
"Xkcs93w8nQaAM+7T1h0R2jCYZA+Wym90rxyBTeZfF647ehUcNEKJqrEMLPMjXu0Z\r\n"    \
"OWiGAMj3fF2gyWShs390LfBhHp/i0lw+ivvrl6NqU5HXcuVEiJYx9LLb234iRl9Z\r\n"    \
"wxXflcIYZ2hdtapyOLUO+UFZRmqvCPOcxxmP\r\n"    \
"-----END CERTIFICATE-----\r\n"


const char mqtt_test_cli_pem[] = MQTT_TEST_CLI_CRT;
const size_t mqtt_test_cli_pem_len = sizeof(mqtt_test_cli_pem);

#define MQTT_TEST_CLI_KEY \
"-----BEGIN RSA PRIVATE KEY-----\r\n"    \
"MIIEpQIBAAKCAQEAqRuJIZVYnXE2qmeEhGexzNriv3iuDv8Eh8/7xWiPTDz8hwSM\r\n"    \
"lsXtSI2aNmk5CHN2tWIvwV7UvLPKVEeKjsHBOL3iVp0RYPny3MNHvLmSmN06hTqP\r\n"    \
"74YQGQEOTaK3yp7hIpPK3xXvGEnTSXN4nShOc3iQaoz9tW71hMOK4TWC031R1E/Y\r\n"    \
"x9TjATJwW4K9sYWwq4c4Q0CaqD2mAxIZeuuJCNajJ23HPY5mE1ARdFQzq+HRiE19\r\n"    \
"WzAM270gQxzNTiRKhDHAlvOX6Ixce0ZUxyRNUCwu7skbuxFwWJFIbZhQcM6VTAYk\r\n"    \
"fugSzi0YNO4StL2DSsm3DqM02xelcgDrdAytkwIDAQABAoIBAQCfo0HZ9S05O4Xa\r\n"    \
"aWx8vZLoCv2guOC/gVgaiWlomuMjmjLdlprNPj1Yj2wzzzMq2i0G35CrX8U9+g+X\r\n"    \
"bg3L4/ZuhTpkf1PHk00DwEk5TImqSrWSzYzRFWKe6BNDkMclvMRVKuWJoSAbcp3F\r\n"    \
"qf8DhOhCM19JZp2eqAeX9Cxnm2dME2mzu9Sv2qI7sasG0SQd+0c/b6uDwa/6esZd\r\n"    \
"ie0IjfT0pJ3rmoMGhHOA8GUkJ75D+QxNTVyN1/LCoXuJflZW5rKp/XPAtmqsjkw5\r\n"    \
"lvvYkULZgfShz04qygnbcNalg8z7J+S2VD5vjRARb+ZncUhnOl9Xv18W4y8/BR/V\r\n"    \
"WXBsTXfJAoGBAMrDSccJvgtsOcTrKE7mEqwx6Fan+r+kbKnlwMTrymXOjj4ypb3p\r\n"    \
"hcR6xvaAd8qnKKqUixhd15N8M0F+8HZhpRnXnv6+Xun7Q8s0Ol83HM7ohwVjyiW+\r\n"    \
"HOaRcsiV4if9aOHhJNzTkeksbA92rK2JylK+x5SHZPYjM2H1kzu05CVFAoGBANWC\r\n"    \
"Gz3CPxqGDfsnV+JVOTSv4aFNOhjAdvHPUSEbOgZnlyx13JY6Yo5hxVDPL+pmTVeJ\r\n"    \
"vk6MhPEUm7l87E5ciwBj4MUwUw4BTpKPjN+8rDfSRYfONEDMBgsC0Fm5BeGMQiLV\r\n"    \
"eyHsk7DWAOhWel3WYVvtoY14sn+9loiajSKz+Vj3AoGBALKvtWXlrQhuAN8MCcK2\r\n"    \
"OQBtHv63HGAlK+nx4pbn4L8lb+9aPPwHPu5u8MYtYuRBubHSJF593MxJqPXwQ0Ng\r\n"    \
"O9pxTdnbtNNork72oPZmIEorW0ohrfTi/J8o0Mn6ZIqvZO2itxSwkqa1kilygSyS\r\n"    \
"AK45GLNJOm07ij2q6Gy2tlQtAoGBAIMWpP8gOnpggEkJ+O2gAcWVj2Tn85mq4dzJ\r\n"    \
"uzgmbVPtWmYT32HlawmcgfeBnhu+wBHVIqE7qYwTaSFT0Aq0yytsJ18qsnPQ3Zsi\r\n"    \
"xA3C28JSOhqNwoteOP+dtKrB7Rh0c07L4BQOGwfh/SCpp+vhall051HjH0VK9Bff\r\n"    \
"PHRoJ9o1AoGAdUoUOsLxbj+4f28A/QXdnLlhclBt7y0rrk7r+h2nzov4grrkKdvl\r\n"    \
"e6XdSWUDfz64DFeqKyvD1s7t1K8d360YP3E2leVNzi4flmxBTy9lAWuZtswpvfWR\r\n"    \
"wbShRF9IrtNTP6y8TiOjCz8CeHfiUw9uTZ98k+3HkR7HZ36kfz8Seho=\r\n"    \
"-----END RSA PRIVATE KEY-----\r\n"


const char mqtt_test_cli_key[] = MQTT_TEST_CLI_KEY;
const size_t mqtt_test_cli_key_len = sizeof(mqtt_test_cli_key);

#endif


#define AGENT_TINY_DEMO_CLIENT_ID "liteos_test"
#define AGENT_TINY_DEMO_USERNAME NULL
#define AGENT_TINY_DEMO_PASSWORD NULL

#define AGENT_TINY_DEMO_PUB_TOPIC "/pub_test"
#define AGENT_TINY_DEMO_SUB_TOPIC "/helloworld"

#define AGENT_TINY_PROJECT_ID "cb1d33433e2648b0a0f40ad1a0c5ffbb"
#define AGENT_TINY_DEVICE_ID "894d48f8-45c0-4ac4-8d18-f5b983ef9159"

static void *g_phandle = NULL;
static atiny_device_info_t g_device_info;
static atiny_param_t g_atiny_params;

atiny_interest_uri_t g_interest_uris[ATINY_INTEREST_URI_MAX_NUM] =
{
    {
        .uri = AGENT_TINY_PROJECT_ID"/device/"AGENT_TINY_DEVICE_ID"/properties/result",
        .qos = CLOUD_QOS_MOST_ONCE,
        .cb = dev_message_cb
    },
    {
        .uri = AGENT_TINY_PROJECT_ID"/device/"AGENT_TINY_DEVICE_ID"/twins/result",
        .qos = CLOUD_QOS_MOST_ONCE,
        .cb = twins_message_cb
    },
    {
        .uri = AGENT_TINY_PROJECT_ID"/device/"AGENT_TINY_DEVICE_ID"/properties/todevice",
        .qos = CLOUD_QOS_MOST_ONCE,
        .cb = dev_message_update_cb
    },
    {
        .uri = AGENT_TINY_PROJECT_ID"/device/"AGENT_TINY_DEVICE_ID"/twins/expected",
        .qos = CLOUD_QOS_MOST_ONCE,
        .cb = twins_message_update_cb
    },
    {
        .uri = AGENT_TINY_PROJECT_ID"/device/"AGENT_TINY_DEVICE_ID"/messages/todevice/#",
        .qos = CLOUD_QOS_MOST_ONCE,
        .cb = dev_message_handle_cb
    },

};

atiny_dev_uri_t g_dev_uris[ATINY_DEV_URI_MAX_NUM] =
{
    {
        .uri = AGENT_TINY_PROJECT_ID"/device/"AGENT_TINY_DEVICE_ID"/properties",
        .qos = CLOUD_QOS_LEAST_ONCE,
        .payload_len = 0,
        .payload = ""
    },
    {
        .uri = AGENT_TINY_PROJECT_ID"/device/"AGENT_TINY_DEVICE_ID"/properties/tocloud",
        .qos = CLOUD_QOS_LEAST_ONCE,
        .payload_len = 13,
        .payload = "dev json data"
    },
    {
        .uri = AGENT_TINY_PROJECT_ID"/device/"AGENT_TINY_DEVICE_ID"/twins",
        .qos = CLOUD_QOS_MOST_ONCE,
        .payload_len = 0,
        .payload = ""
    },
};

void dev_message_cb(cloud_msg_t *msg)
{
    handle_data_t *handle;
    MQTTClient *client;

    handle = (handle_data_t *)g_phandle;
    client = &(handle->client);
    ATINY_LOG(LOG_DEBUG, "%.*s : %.*s", msg->uri_len, msg->uri, msg->payload_len,  (char *)msg->payload);
    //mqtt_topic_unsubscribe(client, msg->uri, msg->uri_len);
}

void twins_message_cb(cloud_msg_t *msg)
{
    handle_data_t *handle;
    MQTTClient *client;

    handle = (handle_data_t *)g_phandle;
    client = &(handle->client);

    ATINY_LOG(LOG_DEBUG, "%.*s : %.*s", msg->uri_len, msg->uri, msg->payload_len,  (char *)msg->payload);
    //mqtt_topic_unsubscribe(client, msg->uri, msg->uri_len);
}

void dev_message_update_cb(cloud_msg_t *msg)
{
    cJSON *root = NULL;

    ATINY_LOG(LOG_DEBUG, "%.*s : %.*s", msg->uri_len, msg->uri, msg->payload_len,  (char *)msg->payload);

    root = cJSON_Parse(msg->payload);
    printf("%s\n\n", cJSON_Print(root));
}

void twins_message_update_cb(cloud_msg_t *msg)
{

}

void dev_message_handle_cb(cloud_msg_t *msg)
{

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
	memcpy(device_info->dev_uris, g_dev_uris, sizeof(g_dev_uris));

    atiny_params = &g_atiny_params;
    atiny_params->server_ip = DEFAULT_SERVER_IPV4;
    atiny_params->server_port = DEFAULT_SERVER_PORT;
#if defined WITH_DTLS
    atiny_params->security_type = CLOUD_SECURITY_TYPE_PSK;
    atiny_params->u.psk.psk_id = (unsigned char *)AGENT_TINY_DEMO_PSK_ID;
    atiny_params->u.psk.psk_id_len = strlen(AGENT_TINY_DEMO_PSK_ID);
    atiny_params->u.psk.psk = g_demo_psk;
    atiny_params->u.psk.psk_len = AGENT_TINY_DEMO_PSK_LEN;
#if defined WITH_CA_UNI
    atiny_params->security_type = CLOUD_SECURITY_TYPE_CA_UNI;
    atiny_params->u.ca.ca_crt = MQTT_TEST_CA_CRT;
    atiny_params->u.ca.client_crt = NULL;
    atiny_params->u.ca.client_key = NULL;
#elif defined WITH_CA_BI
	atiny_params->security_type = CLOUD_SECURITY_TYPE_CA_BI;
	atiny_params->u.ca.ca_crt = MQTT_TEST_CA_CRT;
	atiny_params->u.ca.client_crt = MQTT_TEST_CLI_CRT;
	atiny_params->u.ca.client_key = MQTT_TEST_CLI_KEY;
#endif
#else
    atiny_params->security_type = CLOUD_SECURITY_TYPE_NONE;
#endif /* WITH_DTLS */

    if(ATINY_OK != atiny_init(atiny_params, &g_phandle))
    {
        return NULL;
    }

    //uwRet = creat_report_task();
    if(0 != uwRet)
    {
        return NULL;
    }

    (void)atiny_bind(device_info, g_phandle);
    return NULL;
}
