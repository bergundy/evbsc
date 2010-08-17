/**
 * =====================================================================================
 * @file   check_evbsc.c
 * @brief  test suite for evbsc library
 * @date   06/13/2010 05:58:28 PM
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

#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "evbsc.h"

const char *host = "localhost", *port = BSC_DEFAULT_PORT;
char *exp_data;
static bsc_error_t bsc_error;

/*****************************************************************************************************************/ 
/*                                                      test 1                                                   */
/*****************************************************************************************************************/ 
void put_cb(bsc *client, struct bsc_put_info *info);
void reserve_cb(bsc *client, struct bsc_reserve_info *info);
void delete_cb(bsc *client, struct bsc_delete_info *info);

void onerror(bsc *client, bsc_error_t error)
{
    fail("evbsc got error callback");
}

void put_cb(bsc *client, struct bsc_put_info *info)
{
    fail_if(info->response.code != BSC_PUT_RES_INSERTED, "put_cb: info->code != BSC_PUT_RES_INSERTED");
}

void reserve_cb(bsc *client, struct bsc_reserve_info *info)
{
    fail_if(info->response.code != BSC_RESERVE_RES_RESERVED,
        "bsp_reserve: response.code != BSC_RESERVE_RES_RESERVED");
    fail_if(info->response.bytes != strlen(exp_data),
        "bsp_reserve: response.bytes != exp_bytes");
    fail_if(strcmp((const char *)info->response.data, exp_data) != 0,
        "bsp_reserve: got invalid data");

    bsc_error = bsc_delete(client, delete_cb, NULL, info->response.id);
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_delete failed (%d)", bsc_error);
}

void delete_cb(bsc *client, struct bsc_delete_info *info)
{
    evbsc *evclient = (evbsc *)client;

    fail_if( info->response.code != BSC_DELETE_RES_DELETED,
        "bsp_delete: response.code != BSC_DELETE_RES_DELETED");

    if (info->user_data) {
        exp_data = (char *)"bababuba12341234";
        bsc_error = bsc_put(client, put_cb, client, 1, 0, 10, strlen(exp_data), exp_data, false);
        fail_if(bsc_error != BSC_ERROR_NONE, "bsc_put failed (%d)", bsc_error);
        bsc_error = bsc_reserve(client, reserve_cb, NULL, -1);
        fail_if(bsc_error != BSC_ERROR_NONE, "bsc_reserve failed (%d)", bsc_error);
    }
    else
        ev_unloop(evclient->loop, EVUNLOOP_ONE);
}

void use_cb(bsc *client, struct bsc_use_info *info)
{
    fail_if( info->response.code != BSC_USE_RES_USING,
        "bsp_use: response.code != BSC_USE_RES_USING");
    fail_if( strcmp(info->response.tube, info->request.tube),
        "bsp_use: response.tube != info->request.tube");
}

void watch_cb(bsc *client, struct bsc_watch_info *info)
{
    fail_if( info->response.code != BSC_RES_WATCHING,
        "bsp_watch: response.code != BSC_RES_WATCHING");
    fail_if( client->watched_tubes_count != info->response.count,
        "bsp_watch: watched_tubes_count != response.count" );
    fail_if( strcmp(client->watched_tubes->name, BSC_DEFAULT_TUBE) );
    fail_if( strcmp(client->watched_tubes->next->name, info->request.tube) );
}

void ignore_cb(bsc *client, struct bsc_ignore_info *info)
{
    fail_if( info->response.code != BSC_RES_WATCHING,
        "bsp_ignore: response.code != BSC_RES_WATCHING");
    fail_if( client->watched_tubes_count != info->response.count,
        "bsp_ignore: watched_tubes_count != response.count" );
    fail_if( strcmp(client->watched_tubes->name, "test") );
}

START_TEST(test_evbsc_small_vec) {
    char  errstr[BSC_ERRSTR_LEN];
    evbsc *evclient;
    struct ev_loop *loop = ev_default_loop(0);
    evclient = evbsc_new(loop, host, port, BSC_DEFAULT_TUBE, onerror, 16, 12, 4, errstr);
    fail_if(evclient == NULL, "evbsc_new: %s", errstr);
    bsc   *client = BSCIFY(evclient);

    bsc_error = bsc_use(client, use_cb, NULL, "test");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_use failed (%d)", bsc_error );

    bsc_error = bsc_watch(client, watch_cb, NULL, "test");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_watch failed (%d)", bsc_error );

    bsc_error = bsc_watch(client, watch_cb, NULL, "test");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_watch failed (%d)", bsc_error );

    bsc_error = bsc_ignore(client, ignore_cb, NULL, "default"); 
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_ignore failed (%d)", bsc_error );

    bsc_error = bsc_ignore(client, ignore_cb, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_ignore failed (%d)", bsc_error );

    bsc_error = bsc_ignore(client, ignore_cb, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_ignore failed (%d)", bsc_error );

    bsc_error = bsc_ignore(client, ignore_cb, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_ignore failed (%d)", bsc_error );

    bsc_error = bsc_ignore(client, ignore_cb, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_ignore failed (%d)", bsc_error );

    exp_data = (char *)"baba";
    bsc_error = bsc_put(client, put_cb, NULL, 1, 0, 10, strlen(exp_data), exp_data, false);
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_put failed (%d)", bsc_error);

    bsc_error = bsc_reserve(client, reserve_cb, NULL, -1);
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_reserve failed (%d)", bsc_error);

    ev_loop(loop, 0);
    bsc_free(client);
}
END_TEST

/*****************************************************************************************************************/ 
/*                                                  end of tests                                                 */
/*****************************************************************************************************************/ 

Suite *local_suite(void)
{
    Suite *s  = suite_create(__FILE__);
    TCase *tc = tcase_create("evbsc");

    tcase_add_test(tc, test_evbsc_small_vec);

    suite_add_tcase(s, tc);
    return s;
}

int main() {
    SRunner *sr;
    Suite *s;
    int failed;

    s = local_suite();
    sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);

    failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
