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

#if 1//def WITH_DTLS
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/platform.h"
#include "mbedtls/x509_crt.h"
#include "dtls_interface.h"
#endif
#if defined(WITH_LINUX)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#endif

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

static int linux_mqtt_read(void *ctx, unsigned char *buffer, int len, int timeout_ms)
{
    int fd;
    struct timeval interval = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
    if (interval.tv_sec < 0 || (interval.tv_sec == 0 && interval.tv_usec <= 0))
    {
        interval.tv_sec = 0;
        interval.tv_usec = 100;
    }
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

static int linux_mqtt_tls_read(mbedtls_ssl_context *ssl, unsigned char *buffer, int len, int timeout_ms)
{
    int ret;

    if(NULL == ssl || NULL == buffer)
    {
        ATINY_LOG(LOG_FATAL, "invalid params.");
        return -1;
    }

    mbedtls_ssl_conf_read_timeout(ssl->conf, timeout_ms);

    ret = mbedtls_ssl_read(ssl, buffer, len);
    if(ret == MBEDTLS_ERR_SSL_WANT_READ
            || ret == MBEDTLS_ERR_SSL_WANT_WRITE
            || ret == MBEDTLS_ERR_SSL_TIMEOUT)
        ret = 0;

    return ret;
}


int linux_read(Network *n, unsigned char *buffer, int len, int timeout_ms)
{

    int ret = -1;

    if(NULL == n || NULL == buffer)
    {
        ATINY_LOG(LOG_FATAL, "invalid params.");
        return -1;
    }

    switch(n->proto)
    {
    case MQTT_PROTO_NONE :
        ret = linux_mqtt_read(n->ctx, buffer, len, timeout_ms);
        break;
    case MQTT_PROTO_TLS_PSK:
    case MQTT_PROTO_TLS_CA:
        ret = linux_mqtt_tls_read(n->ctx, buffer, len, timeout_ms);
        break;
    default :
        ATINY_LOG(LOG_WARNING, "unknow proto : %d", n->proto);
        break;
    }

    return ret;



}


static int linux_mqtt_write(void *ctx, unsigned char *buffer, int len, int timeout_ms)
{
    struct timeval tv;
    int fd;
    fd = ((mqtt_context_t *)ctx)->fd;


    tv.tv_sec = 0;  /* 30 Secs Timeout */
    tv.tv_usec = timeout_ms * 1000;  // Not init'ing this can cause strange errors

    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(struct timeval));
    int    rc = write(fd, buffer, len);
    return rc;
}

int linux_write(Network *n, unsigned char *buffer, int len, int timeout_ms)
{
    int ret = -1;

    if(NULL == n || NULL == buffer)
    {
        ATINY_LOG(LOG_FATAL, "invalid params.");
        return -1;
    }

    switch(n->proto)
    {
    case MQTT_PROTO_NONE :
        ret = linux_mqtt_write(n->ctx, buffer, len, timeout_ms);
        break;
    case MQTT_PROTO_TLS_PSK:
    case MQTT_PROTO_TLS_CA:
        ret = dtls_write(n->ctx, buffer, len);
        break;
    default :
        ATINY_LOG(LOG_WARNING, "unknow proto : %d", n->proto);
        break;
    }

    return ret;
}

static void *atiny_calloc(size_t n, size_t size)
{
    void *p = atiny_malloc(n * size);
    if(p)
    {
        memset(p, 0, n * size);
    }

    return p;
}

static void my_debug( void *ctx, int level,
                      const char *file, int line,
                      const char *str )
{
    const char *p, *basename;

    /* Extract basename from file */
    for( p = basename = file; *p != '\0'; p++ )
        if( *p == '/' || *p == '\\' )
            basename = p + 1;

    mbedtls_fprintf( (FILE *) ctx, "%s:%04d: |%d| %s", basename, line, level, str );
    fflush(  (FILE *) ctx  );
}

int mbedtls_net_set_block( mbedtls_net_context *ctx )
{
#if ( defined(_WIN32) || defined(_WIN32_WCE) ) && !defined(EFIX64) && \
  !defined(EFI32)
  u_long n = 0;
  return( ioctlsocket( ctx->fd, FIONBIO, &n ) );
#else
  return( fcntl( ctx->fd, F_SETFL, fcntl( ctx->fd, F_GETFL, 0) & ~O_NONBLOCK ) );
#endif
}

int mbedtls_net_set_nonblock( mbedtls_net_context *ctx )
{
#if ( defined(_WIN32) || defined(_WIN32_WCE) ) && !defined(EFIX64) && \
  !defined(EFI32)
  u_long n = 1;
  return( ioctlsocket( ctx->fd, FIONBIO, &n ) );
#else
  return( fcntl( ctx->fd, F_SETFL, fcntl( ctx->fd, F_GETFL, 0) | O_NONBLOCK ) );
#endif
}

#if defined(_MSC_VER)
#define MSVC_INT_CAST   (int)
#else
#define MSVC_INT_CAST
#endif

int mbedtls_mqtt_connect( mbedtls_net_context *ctx, const char *host,
                        const char *port, int proto )
{
  int ret;
  struct addrinfo hints, *addr_list, *cur;

  /* Do name resolution with both IPv6 and IPv4 */
  memset( &hints, 0, sizeof( hints ) );
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = proto == MBEDTLS_NET_PROTO_UDP ? SOCK_DGRAM : SOCK_STREAM;
  hints.ai_protocol = proto == MBEDTLS_NET_PROTO_UDP ? IPPROTO_UDP : IPPROTO_TCP;

  if( getaddrinfo( host, port, &hints, &addr_list ) != 0 )
      return( MBEDTLS_ERR_NET_UNKNOWN_HOST );

  /* Try the sockaddrs until a connection succeeds */
  ret = MBEDTLS_ERR_NET_UNKNOWN_HOST;
  for( cur = addr_list; cur != NULL; cur = cur->ai_next )
  {
      ctx->fd = (int) socket( cur->ai_family, cur->ai_socktype,
                              cur->ai_protocol );
      if( ctx->fd < 0 )
      {
          ret = MBEDTLS_ERR_NET_SOCKET_FAILED;
          continue;
      }

      if( connect( ctx->fd, cur->ai_addr, MSVC_INT_CAST cur->ai_addrlen ) == 0 )
      {
          ret = 0;
          break;
      }

      close( ctx->fd );
      ret = MBEDTLS_ERR_NET_CONNECT_FAILED;
  }

  freeaddrinfo( addr_list );

  return( ret );
}


void NetworkInit(Network *n)
{
    memset(n, 0x0, sizeof(Network));
    n->proto = MQTT_PROTO_MAX;
    n->mqttread = linux_read;
    n->mqttwrite = linux_write;
}

static int los_mqtt_connect(Network *n, char *addr, int port)
{
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



#ifdef WITH_CA
static int los_mqtt_tls_ca_connect(Network *n, char *addr, int port)
{
    int ret;
	unsigned char buf[MBEDTLS_SSL_MAX_CONTENT_LEN + 1];
    mbedtls_net_context *server_fd;
    mbedtls_ssl_context *ssl;
    mbedtls_ssl_config *conf;
    mbedtls_entropy_context *entropy;
    mbedtls_ctr_drbg_context *ctr_drbg;
    mbedtls_x509_crt *cacert;
    mbedtls_x509_crt *clicert;
    mbedtls_pk_context *pkey;
    mbedtls_timing_delay_context *timer;

    char port_str[16] = {0};
    const char *pers = "mqtt_client";

    if(NULL == n || NULL == addr)
    {
        ATINY_LOG(LOG_FATAL, "invalid params.");
        return -1;
    }

    (void)mbedtls_platform_set_calloc_free(atiny_calloc, atiny_free);
    (void)mbedtls_platform_set_snprintf(atiny_snprintf);
    (void)mbedtls_platform_set_printf(atiny_printf);

    ssl       = mbedtls_calloc(1, sizeof(mbedtls_ssl_context));
    conf      = mbedtls_calloc(1, sizeof(mbedtls_ssl_config));
    entropy   = mbedtls_calloc(1, sizeof(mbedtls_entropy_context));
    ctr_drbg  = mbedtls_calloc(1, sizeof(mbedtls_ctr_drbg_context));
    server_fd = mbedtls_calloc(1, sizeof(mbedtls_net_context));
    cacert    = mbedtls_calloc(1, sizeof(mbedtls_x509_crt));
	clicert   = mbedtls_calloc(1, sizeof(mbedtls_x509_crt));
	pkey      = mbedtls_calloc(1, sizeof(mbedtls_pk_context));
    timer     = mbedtls_calloc(1, sizeof(mbedtls_timing_delay_context));

    if (NULL == ssl || NULL == conf || entropy == NULL ||
            NULL == ctr_drbg || NULL == server_fd ||
            NULL == cacert || NULL == timer)
    {
        goto exit;
    }

#ifdef MBEDTLS_DEBUG_C
    mbedtls_debug_set_threshold(100);
#endif
    /*
     * 0. Initialize the RNG and the session data
     */
    mbedtls_net_init(server_fd);
    mbedtls_ssl_init(ssl);
    mbedtls_ssl_config_init(conf);
    mbedtls_x509_crt_init(cacert);
    mbedtls_ctr_drbg_init(ctr_drbg);

    mbedtls_entropy_init(entropy);
    if( ( ret = mbedtls_ctr_drbg_seed(ctr_drbg, mbedtls_entropy_func, entropy,
                                      (const unsigned char *) pers,
                                      strlen( pers ) ) ) != 0 )
    {
        ATINY_LOG(LOG_ERR, "failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
        goto exit;
    }


    /*
     * 1. Initialize certificates
     */
	/*
	 * 1.1. Load the trusted CA
	 */
    ATINY_LOG(LOG_DEBUG, "  . Loading the CA root certificate ...");

    extern const char    mqtt_test_cas_pem[];
    extern const size_t mqtt_test_cas_pem_len;

    ret = mbedtls_x509_crt_parse(cacert, \
                                 (const unsigned char *) mqtt_test_cas_pem, \
                                 mqtt_test_cas_pem_len );

    if( ret < 0 )
    {
        ATINY_LOG(LOG_DEBUG,  \
                    " failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n",
                    -ret );
        goto exit;
    }

    /*
     * 1.2. Load own certificate and private key
     *
     * (can be skipped if client authentication is not required)
     */
	ATINY_LOG(LOG_DEBUG, "	. Loading the client cert. and key ...");

	extern const char	 mqtt_test_cli_pem[];
	extern const size_t mqtt_test_cli_pem_len;

	extern const char	 mqtt_test_cli_key[];
	extern const size_t mqtt_test_cli_key_len;

	ret = mbedtls_x509_crt_parse(clicert, \
								 (const unsigned char *) mqtt_test_cli_pem, \
								 mqtt_test_cli_pem_len );

	if( ret < 0 )
	{
		ATINY_LOG(LOG_DEBUG,  \
					" failed\n	!  mbedtls_x509_crt_parse returned -0x%x\n\n",
					-ret );
		goto exit;
	}

	ret = mbedtls_pk_parse_key( pkey, (const unsigned char *) mqtt_test_cli_key,
			mqtt_test_cli_key_len, NULL, 0 );
	if( ret < 0 )
	{
		ATINY_LOG(LOG_DEBUG,  \
					" failed\n	!  mbedtls_pk_parse_key returned -0x%x\n\n",
					-ret );
		goto exit;
	}


    (void)atiny_snprintf(port_str, sizeof(port_str) - 1, "%d", port);
    if( ( ret = mbedtls_mqtt_connect(server_fd, addr, port_str, MBEDTLS_NET_PROTO_TCP) ) != 0 )
    {
        ATINY_LOG(LOG_ERR, "mbedtls_net_connect failed.");
        goto exit;
    }
    ATINY_LOG(LOG_DEBUG, "mbedtls_net_connect success");
    n->ctx = (void *)ssl;

    ret = mbedtls_net_set_block( server_fd );
    if( ret != 0 )
    {
        ATINY_LOG(LOG_ERR, " failed\n  ! net_set_(non)block() returned -0x%x", -ret );
        goto exit;
    }

    /*
     * 2. Setup stuff
     */
    ATINY_LOG(LOG_DEBUG, "  . Setting up the DTLS structure..." );
    if( ( ret = mbedtls_ssl_config_defaults(conf,
                                            MBEDTLS_SSL_IS_CLIENT,
                                            MBEDTLS_SSL_TRANSPORT_STREAM,
                                            MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )
    {
        ATINY_LOG(LOG_ERR, " failed\n  ! mbedtls_ssl_config_defaults returned -0x%x", -ret );
        goto exit;
    }

    mbedtls_ssl_conf_authmode( conf, MBEDTLS_SSL_VERIFY_REQUIRED );
    mbedtls_ssl_conf_ca_chain( conf, cacert, NULL );
    mbedtls_ssl_conf_rng( conf, mbedtls_ctr_drbg_random, ctr_drbg );
    mbedtls_ssl_conf_dbg( conf, my_debug, stdout );

    mbedtls_ssl_conf_read_timeout( conf, 1000 );

	if( ( ret = mbedtls_ssl_conf_own_cert( conf, clicert, pkey ) ) != 0 )
	{
		ATINY_LOG(LOG_ERR, " failed\n	! mbedtls_ssl_conf_own_cert returned %d\n\n", ret );
		goto exit;
	}


    if( ( ret = mbedtls_ssl_setup( ssl, conf ) ) != 0 )
    {
        ATINY_LOG(LOG_ERR, " failed\n  ! mbedtls_ssl_setup returned -0x%x", -ret );
        goto exit;
    }

	extern const char server_name[];
    if( ( ret = mbedtls_ssl_set_hostname( ssl, server_name ) ) != 0 )
    {
        ATINY_LOG(LOG_ERR,
                        " failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n",
                        ret );

        goto exit;
    }

    mbedtls_ssl_set_bio( ssl, server_fd, mbedtls_net_send, mbedtls_net_recv,
                         mbedtls_net_recv_timeout);

#if 0
    mbedtls_ssl_set_timer_cb(ssl,
                          timer,
                          mbedtls_timing_set_delay,
                          mbedtls_timing_get_delay );
#endif
    /*
     * 3. Handshake
     */
    ATINY_LOG(LOG_DEBUG,"  . Performing the SSL/TLS handshake...");
    while( ( ret = mbedtls_ssl_handshake( ssl ) ) != 0 )
    {
        if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE
                && ret != MBEDTLS_ERR_SSL_TIMEOUT)
        {
            ATINY_LOG(LOG_ERR, " failed\n  ! mbedtls_ssl_handshake returned -0x%x", -ret );
            goto exit;
        }
    }

    /*
     * 4. Verify the server certificate
     */
    ATINY_LOG(LOG_DEBUG, " . Verifying peer X.509 certificate...");
    if( ( ret = mbedtls_ssl_get_verify_result( ssl ) ) != 0 )
    {
        char vrfy_buf[512];

        ATINY_LOG(LOG_ERR, " failed\n" );

        mbedtls_x509_crt_verify_info(vrfy_buf, \
                                     sizeof( vrfy_buf ), \
                                     "    ! ", \
                                     ret );

        ATINY_LOG(LOG_ERR, "%s\n", vrfy_buf );
    }
    else
        ATINY_LOG(LOG_INFO, "ok\n");

    if( mbedtls_ssl_get_peer_cert( ssl ) != NULL )
    {
        mbedtls_x509_crt_info( (char *) buf, sizeof( buf ) - 1, "      ",
                       mbedtls_ssl_get_peer_cert( ssl ) );
    }

	ATINY_LOG(LOG_DEBUG, "  . Performing renegotiation..." );
	while( ( ret = mbedtls_ssl_renegotiate( ssl ) ) != 0 )
	{
		if( ret != MBEDTLS_ERR_SSL_WANT_READ &&
			ret != MBEDTLS_ERR_SSL_WANT_WRITE )
		{
			ATINY_LOG(LOG_DEBUG, " failed\n	! mbedtls_ssl_renegotiate returned %d\n\n", ret );
			goto exit;
		}
	}
	ATINY_LOG(LOG_DEBUG, " ok\n" );


    return 0;
exit:
    if (conf)
    {
        mbedtls_ssl_config_free(conf);
        mbedtls_free(conf);
    }

    if (ctr_drbg)
    {
        mbedtls_ctr_drbg_free(ctr_drbg);
        mbedtls_free(ctr_drbg);
    }

    if (entropy)
    {
        mbedtls_entropy_free(entropy);
        mbedtls_free(entropy);
    }

    if (ssl)
    {
        mbedtls_ssl_free(ssl);
        mbedtls_free(ssl);
    }
    if(server_fd)
    {
        mbedtls_free(server_fd);
    }
    if(cacert)
    {
        mbedtls_free(cacert);
    }
    if(timer)
    {
        mbedtls_free(timer);
    }

    n->ctx = (void *)NULL;
    return -1;
}
#endif


int NetworkConnect(Network *n, char *addr, int port)
{
    int ret = -1;

    if(NULL == n || NULL == addr)
    {
        ATINY_LOG(LOG_FATAL, "invalid params.\n");
        return -1;
    }

    switch(n->proto)
    {
    case MQTT_PROTO_NONE :
        ret = los_mqtt_connect(n, addr, port);
        break;
    case MQTT_PROTO_TLS_PSK:
        //ret = los_mqtt_tls_connect(n, addr, port);
        break;
#ifdef WITH_CA
    case MQTT_PROTO_TLS_CA:
        ret = los_mqtt_tls_ca_connect(n, addr, port);
        break;
#endif
    default :
        ATINY_LOG(LOG_WARNING, "unknow proto : %d\n", n->proto);
        break;
    }

    return ret;
}



void NetworkDisconnect(Network *n)
{
    int fd;
    void *ctx;
    ctx = n->ctx;
    fd = ((mqtt_context_t *)ctx)->fd;
    close(fd);
}
