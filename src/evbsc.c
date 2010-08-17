/**
 * =====================================================================================
 * @file     evbsc.c
 * @brief  
 * @date     07/05/2010 07:01:09 PM
 * @author   Roey Berman, (roey.berman@gmail.com)
 * @version  1.0
 *
 * Copyright (c) 2010, Roey Berman, (roeyb.berman@gmail.com)
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by Roey Berman.
 * 4. Neither the name of Roey Berman nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY ROEY BERMAN ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ROEY BERMAN BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
static void after_connect(bsc *self);
static void before_diconnect(bsc *self);

static int buffer_fill_cb(bsc *bsclient)
{
    evbsc *client = (evbsc *)bsclient;
    ev_io_start((client)->loop, &((client)->ww));
    return 1;
}

evbsc *evbsc_new(struct ev_loop *loop, const char *host, const char *port,
                 const char *default_tube, error_callback_p_t onerror,
                 size_t buf_len, size_t vec_len, size_t vec_min, char *errorstr)
{
    bsc   *bsclient = NULL;
    evbsc *client   = NULL;

    if ( (bsclient = bsc_new(host, port, default_tube, onerror, buf_len, vec_len, vec_min, errorstr)) == NULL )
        return NULL;

    if ( ( client = (evbsc *)realloc(bsclient, sizeof(evbsc)) ) == NULL ) {
        bsc_free(bsclient);
        strcpy(errorstr, "out of memory");
        return NULL;
    }

    BSCIFY(client)->buffer_fill_cb    = buffer_fill_cb;
    BSCIFY(client)->post_connect_cb   = after_connect;
    BSCIFY(client)->pre_disconnect_cb = before_diconnect;
    client->loop                      = loop;

    after_connect(BSCIFY(client));

    return client;
}

void evbsc_free(evbsc *client)
{
    bsc_free(BSCIFY(client));
}

static void after_connect(bsc *self)
{
    evbsc *client = EVBSCIFY(self);
    ev_io_init( &(client->rw), read_ready,  client->bsclient.fd, EV_READ  );
    ev_io_init( &(client->ww), write_ready, client->bsclient.fd, EV_WRITE );
    client->rw.data = client;
    client->ww.data = client;
    ev_io_start(client->loop, &(client->rw));

    if (strcmp(self->default_tube, BSC_DEFAULT_TUBE) != 0) 
        ev_io_start((client)->loop, &((client)->ww));

    if (!AQ_EMPTY(client->bsclient.outq))
        ev_io_start(client->loop, &(client->ww));
}

static void before_diconnect(bsc *self)
{
    evbsc *client = EVBSCIFY(self);
    ev_io_stop(client->loop, &(client->rw));
    ev_io_stop(client->loop, &(client->ww));
}

static void write_ready(EV_P_ ev_io *w, int revents)
{
    evbsc *client = (evbsc *)w->data;
    bsc_write(&(client->bsclient));
    if (AQ_EMPTY(client->bsclient.outq))
        ev_io_stop(loop, &(client->ww));

    return;
}

static void read_ready(EV_P_ ev_io *w, int revents)
{
    evbsc *client = (evbsc *)w->data;
    bsc_read(&(client->bsclient));
}

#ifdef __cplusplus
    }
#endif
