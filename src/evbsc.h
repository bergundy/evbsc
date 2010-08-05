/**
 * =====================================================================================
 * @file   evbsc.h
 * @brief  header file for evbsc - libev implementation of the beanstalk client library
 * @date   07/05/2010 06:50:22 PM
 * @author Roey Berman, (royb@walla.net.il), Walla!
 * @todo   change EVBSC_ENQ_CMD to normal function and make the callback api easier
 * =====================================================================================
 */
#ifndef EVBSC_H
#define EVBSC_H 

#ifdef __cplusplus
    extern "C" {
#endif

#include <stdbool.h>
#include <ev.h>
#include <beanstalkclient.h>

#define BSCIFY(evc) ( (bsc *)evc )
#define EVBSCIFY(c) ( (evbsc *)c )

#define evbsc_new_w_defaults(loop, host, port, def_tube, onerror, errorstr)     \
    ( evbsc_new( (loop), (host), (port), (def_tube), (onerror),                 \
        BSC_DEFAULT_BUFFER_SIZE,                                                \
        BSC_DEFAULT_VECTOR_SIZE,                                                \
        BSC_DEFAULT_VECTOR_MIN,                                                 \
        (errorstr) ) )

struct _evbsc {
    bsc             bsclient;
    ev_io           rw;
    ev_io           ww;
    struct ev_loop *loop;
};

typedef struct _evbsc evbsc;

evbsc *evbsc_new( struct ev_loop *loop, const char *host, const char *port,
                  const char *default_tube, error_callback_p_t onerror,
                  size_t buf_len, size_t vec_len, size_t vec_min, char **errorstr );

void evbsc_free(evbsc *client);
bool evbsc_connect(evbsc *client, char **errorstr);
void evbsc_disconnect(evbsc *client);
bool evbsc_reconnect(evbsc *client, char **errorstr);

#ifdef __cplusplus
    }
#endif
#endif /* EVBSC_H */
