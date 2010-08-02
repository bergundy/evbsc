/**
 * =====================================================================================
 * @file   evbsc.c
 * @brief  
 * @date   07/05/2010 07:01:09 PM
 * @author Roey Berman, (royb@walla.net.il), Walla!
 * =====================================================================================
 */

#ifdef __cplusplus
    extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "evbsc.h"

static void write_ready(EV_P_ ev_io *w, int revents);
static void read_ready(EV_P_ ev_io *w, int revents);
static void evbsc_after_connect(evbsc *client);

static int buffer_fill_cb(bsc *bsclient)
{
    evbsc *client = (evbsc *)bsclient->data;
    ev_io_start((client)->loop, &((client)->ww));
    return 1;
}

evbsc *evbsc_new( struct ev_loop *loop, const char *host, const char *port,
                  const char *default_tube, error_callback_p_t onerror,
                  size_t buf_len, size_t vec_len, size_t vec_min, char **errorstr )
{
    evbsc *client   = NULL;
    bsc   *bsclient = NULL;

    if ( ( bsclient = bsc_new(host, port, default_tube, onerror, buf_len, vec_len, vec_min, errorstr) ) == NULL )
        return NULL;

    if ( ( client = (evbsc *)malloc(sizeof(evbsc)) ) == NULL ) {
        bsc_free(bsclient);
        return NULL;
    }

    bsclient->buffer_fill_cb = buffer_fill_cb;
    bsclient->data   = client;
    client->loop     = loop;
    client->bsclient = bsclient;
    evbsc_after_connect(client);
    if (strcmp(default_tube, BSC_DEFAULT_TUBE) != 0) 
        ev_io_start((client)->loop, &((client)->ww));
    return client;
}

void evbsc_free(evbsc *client)
{
    bsc_free(client->bsclient);
    free(client);
}

bool evbsc_connect(evbsc *client, char **errorstr)
{
    if (!bsc_connect(client->bsclient, errorstr))
        return false;

    evbsc_after_connect(client);
    return true;
}

static void evbsc_after_connect(evbsc *client)
{
    ev_io_init( &(client->rw), read_ready,  client->bsclient->fd, EV_READ  );
    ev_io_init( &(client->ww), write_ready, client->bsclient->fd, EV_WRITE );
    client->rw.data = client;
    client->ww.data = client;
    ev_io_start(client->loop, &(client->rw));

    if (!AQUEUE_EMPTY(client->bsclient->outq))
        ev_io_start(client->loop, &(client->ww));
}

void evbsc_disconnect(evbsc *client)
{
    ev_io_stop(client->loop, &(client->rw));
    ev_io_stop(client->loop, &(client->ww));
    bsc_disconnect(client->bsclient);
}

bool evbsc_reconnect(evbsc *client, char **errorstr)
{
    evbsc_disconnect(client);
    return evbsc_connect(client, errorstr);
}

static void write_ready(EV_P_ ev_io *w, int revents)
{
    evbsc *client = (evbsc *)w->data;
    bsc_write(client->bsclient);
    if (AQUEUE_EMPTY(client->bsclient->outq))
        ev_io_stop(loop, &(client->ww));

    return;
}

static void read_ready(EV_P_ ev_io *w, int revents)
{
    evbsc *client = (evbsc *)w->data;
    bsc_read(client->bsclient);
}

#ifdef __cplusplus
    }
#endif
