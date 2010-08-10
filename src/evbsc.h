/**
 * =====================================================================================
 * @file     evbsc.h
 * @brief    header file for evbsc - libev implementation of the beanstalk client library
 * @date     07/05/2010 06:50:22 PM
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

evbsc *evbsc_new(struct ev_loop *loop, const char *host, const char *port,
                 const char *default_tube, error_callback_p_t onerror,
                 size_t buf_len, size_t vec_len, size_t vec_min, char *errorstr);

void evbsc_free(evbsc *client);

#ifdef __cplusplus
    }
#endif
#endif /* EVBSC_H */
