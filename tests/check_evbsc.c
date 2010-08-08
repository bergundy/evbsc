/**
 * =====================================================================================
 * @file   check_evbsc.c
 * @brief  test suite for evbsc library
 * @date   06/13/2010 05:58:28 PM
 * @author Roey Berman, (royb@walla.net.il), Walla!
 * =====================================================================================
 */

#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "evbsc.h"

const char *host = "localhost", *port = BSP_DEFAULT_PORT;
char *exp_data;
static bsc_error_t bsc_error;

/*****************************************************************************************************************/ 
/*                                                      test 1                                                   */
/*****************************************************************************************************************/ 

void small_vec_cb(bsc *client, cbq_node *node, void *data, size_t len);
void put_cb(bsc *client, struct bsc_put_info *info);
void reserve_cb(bsc *client, struct bsc_reserve_info *info);
void delete_cb(bsc *client, struct bsc_delete_info *info);

void onerror(bsc *client, bsc_error_t error)
{
    fail("evbsc got error callback");
}

void put_cb(bsc *client, struct bsc_put_info *info)
{
    fail_if(info->response.code != BSP_PUT_RES_INSERTED, "put_cb: info->code != BSP_PUT_RES_INSERTED");
}

void reserve_cb(bsc *client, struct bsc_reserve_info *info)
{
    fail_if(info->response.code != BSP_RESERVE_RES_RESERVED,
        "bsp_reserve: response.code != BSP_RESERVE_RES_RESERVED");
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

    fail_if( info->response.code != BSP_DELETE_RES_DELETED,
        "bsp_delete: response.code != BSP_DELETE_RES_DELETED");

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
    fail_if( info->response.code != BSP_USE_RES_USING,
        "bsp_use: response.code != BSP_USE_RES_USING");
    fail_if( strcmp(info->response.tube, info->request.tube),
        "bsp_use: response.tube != info->request.tube");
}

void watch_cb(bsc *client, struct bsc_watch_info *info)
{
    fail_if( info->response.code != BSP_RES_WATCHING,
        "bsp_watch: response.code != BSP_RES_WATCHING");
    fail_if( client->watched_tubes_count != info->response.count,
        "bsp_watch: watched_tubes_count != response.count" );
    fail_if( strcmp(client->watched_tubes->name, BSC_DEFAULT_TUBE) );
    fail_if( strcmp(client->watched_tubes->next->name, info->request.tube) );
}

void ignore_cb(bsc *client, struct bsc_ignore_info *info)
{
    fail_if( info->response.code != BSP_RES_WATCHING,
        "bsp_ignore: response.code != BSP_RES_WATCHING");
    fail_if( client->watched_tubes_count != info->response.count,
        "bsp_ignore: watched_tubes_count != response.count" );
    fail_if( strcmp(client->watched_tubes->name, "test") );
}

START_TEST(test_evbsc_small_vec) {
    char  *errstr = NULL;
    evbsc *evclient;
    struct ev_loop *loop = ev_default_loop(0);
    evclient = evbsc_new(loop, host, port, BSC_DEFAULT_TUBE, onerror, 16, 12, 4, &errstr);
    fail_if( evclient == NULL, "evbsc_new: %s", errstr);
    bsc   *client = &(evclient->bsclient);

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
/*                                                      test 2                                                   */
/*****************************************************************************************************************/ 

void ignore_cb2(bsc *client, struct bsc_ignore_info *info)
{
    evbsc *evclient = (evbsc *)client;

    fail_if( info->response.code != BSP_IGNORE_RES_NOT_IGNORED,
        "bsc_ignore: response.code != BSP_IGNORE_RES_NOT_IGNORED");
    fail_if( strcmp(client->watched_tubes->name, BSC_DEFAULT_TUBE) );
    fail_if( client->watched_tubes->next != NULL);

    if (info->user_data != NULL)
        ev_unloop(evclient->loop, EVUNLOOP_ONE);
}


START_TEST(test_evbsc_defaults) {
    evbsc *evclient;
    char  *errstr = NULL;
    struct ev_loop *loop = ev_default_loop(0);
    evclient = evbsc_new_w_defaults(loop, host, port, BSC_DEFAULT_TUBE, onerror, &errstr);
    fail_if( evclient == NULL, "evbsc_new: %s", errstr);
    bsc   *client = &(evclient->bsclient);

    bsc_error = bsc_ignore(client, ignore_cb2, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_ignore failed (%d)", bsc_error );

    bsc_error = bsc_ignore(client, ignore_cb2, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_ignore failed (%d)", bsc_error );

    bsc_error = bsc_ignore(client, ignore_cb2, client, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_ignore failed (%d)", bsc_error );

    ev_loop(loop, 0);

    bsc_free(client);
}
END_TEST

/*****************************************************************************************************************/ 
/*                                                      test 3                                                   */
/*****************************************************************************************************************/ 
const char *reconnect_test_port = "16666";
static char spawn_cmd[200];
static char kill_cmd[200];

void reconnect_test_ignore_cb(bsc *client, struct bsc_ignore_info *info)
{
    system(kill_cmd);
}

static void reconnect(bsc *client, bsc_error_t error)
{
    char *errorstr;
    system(spawn_cmd);
    evbsc *evclient = (evbsc *)client;

    if (error == BSC_ERROR_INTERNAL) {
        fail("critical error: recieved BSC_ERROR_INTERNAL, quitting\n");
    }
    else if (error == BSC_ERROR_SOCKET) {
        if ( bsc_reconnect(client, &errorstr) ) {
            fail_if( client->outq->used != 9, 
                "after reconnect: nodes_used : %d/%d", client->outq->used, 9);

            ev_unloop(evclient->loop, EVUNLOOP_ONE);
            return;
        }
    }
    fail("critical error: maxed out reconnect attempts, quitting\n");
}

START_TEST(test_evbsc_reconnect) {
    evbsc *evclient;
    char *errstr = NULL;
    struct ev_loop *loop = ev_default_loop(0);
    sprintf(spawn_cmd, "beanstalkd -p %s -d", reconnect_test_port);
    sprintf(kill_cmd, "ps -ef|grep beanstalkd |grep '%s'| gawk '!/grep/ {print $2}'|xargs kill", reconnect_test_port);
    system(spawn_cmd);
    evclient = evbsc_new( loop, host, reconnect_test_port, "baba", reconnect, 16, 12, 4, &errstr);
    fail_if( evclient == NULL, "evbsc_new: %s", errstr);
    bsc *client = BSCIFY(evclient);

    bsc_error = bsc_ignore(client, reconnect_test_ignore_cb, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_ignore failed (%d)", bsc_error );
    bsc_error = bsc_reserve(client, NULL, NULL, BSC_RESERVE_NO_TIMEOUT);
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_reserve failed (%d)", bsc_error);
    bsc_error = bsc_reserve(client, NULL, NULL, BSC_RESERVE_NO_TIMEOUT);
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_reserve failed (%d)", bsc_error);
    bsc_error = bsc_reserve(client, NULL, NULL, BSC_RESERVE_NO_TIMEOUT);
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_reserve failed (%d)", bsc_error);
    bsc_error = bsc_put(client, NULL, NULL, 1, 0, 10, strlen(kill_cmd), kill_cmd, false);
    fail_if(bsc_error, "bsc_put failed (%d)", bsc_error);
    bsc_error = bsc_put(client, NULL, NULL, 1, 0, 10, strlen(kill_cmd), kill_cmd, false);
    fail_if(bsc_error, "bsc_put failed (%d)", bsc_error);

    ev_loop(loop, 0);
    system(kill_cmd);
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
    tcase_add_test(tc, test_evbsc_defaults);
    tcase_add_test(tc, test_evbsc_reconnect);

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
