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

evbsc *client;
char *host = "localhost", *port = BSP_DEFAULT_PORT;
char *reconnect_test_port = "16666";
int counter = 0;
char *exp_data;
static char spawn_cmd[200];
static char kill_cmd[200];

void onerror(bsc *client, bsc_error_t error)
{
    fail("evbsc got error callback");
}

void small_vec_cb(bsc *client, queue_node *node, void *data, size_t len)
{
    evbsc *evclient = (evbsc *)client->data;
    struct ev_loop *loop = evclient->loop;
    bsp_response_t res;
    static uint32_t exp_id;
    static uint32_t res_id, bytes;
    switch (counter) {
        case 0:
            fail_if(strcmp(data, "USING test\r\n") != 0, "use cmd res");
            break;
        case 1:
            fail_if(strcmp(data, "WATCHING 2\r\n") != 0, "watch cmd res");
            break;
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            fail_if(strcmp(data, "WATCHING 1\r\n") != 0, "watch cmd res");
            break;
        case 7:
        case 11:
            res = bsp_get_put_res(data, &exp_id);
            fail_if(res != BSP_PUT_RES_INSERTED, "bsp_get_put_res != BSP_PUT_RES_INSERTED");
            break;
        case 8:
        case 12:
            res = bsp_get_reserve_res(data, &res_id, &bytes);
            if (res != BSP_RESERVE_RES_RESERVED || bytes != strlen(exp_data))
                fail("bsp_get_reserve_res != BSP_RESERVE_RES_RESERVED || bytes != exp_bytes");
            node->bytes_expected = bytes;
            break;
        case 9:
        case 13:
            fail_if(strcmp(data, exp_data) != 0, "got invalid data from reserve");
            EVBSC_ENQ_CMD(delete, evclient, small_vec_cb, NULL, cmd_error, res_id );
            break;
        case 10:
        case 14:
            res = bsp_get_delete_res(data);
            if ( res != BSP_DELETE_RES_DELETED )
                fail("bsp_get_delete_res != BSP_DELETE_RES_DELETED");
            else if (counter == 10) {
                exp_data = "bababuba12341234";
                EVBSC_ENQ_CMD(put,     evclient, small_vec_cb, NULL, cmd_error, 1, 0, 10, strlen(exp_data), exp_data);
                EVBSC_ENQ_CMD(reserve, evclient, small_vec_cb, NULL, cmd_error );
                break;
            }
        default:
            ev_unloop(EV_A_ EVUNLOOP_ALL);
    }
    ++counter;
    fail_if( len != strlen(data), "got invalid len argument" );

    return;

cmd_error:
    fail("error enqueuing command");
    ev_unloop(EV_A_ EVUNLOOP_ALL);
}

START_TEST(test_evbsc_small_vec) {
    struct ev_loop *loop = ev_default_loop(0);
    char *errstr = NULL;
    client = evbsc_new( loop, host, port, onerror, 11, 12, 4, &errstr);
    fail_if( client == NULL, "evbsc_new: %s", errstr);
    exp_data = "baba";

    EVBSC_ENQ_CMD(use,     client, small_vec_cb, NULL, cmd_error, "test");
    EVBSC_ENQ_CMD(watch,   client, small_vec_cb, NULL, cmd_error, "test");
    EVBSC_ENQ_CMD(ignore,  client, small_vec_cb, NULL, cmd_error, "default");
    EVBSC_ENQ_CMD(ignore,  client, small_vec_cb, NULL, cmd_error, "default");
    EVBSC_ENQ_CMD(ignore,  client, small_vec_cb, NULL, cmd_error, "default");
    EVBSC_ENQ_CMD(ignore,  client, small_vec_cb, NULL, cmd_error, "default");
    EVBSC_ENQ_CMD(ignore,  client, small_vec_cb, NULL, cmd_error, "default");
    EVBSC_ENQ_CMD(ignore,  client, NULL, NULL, cmd_error, "default");
    EVBSC_ENQ_CMD(put,     client, small_vec_cb, NULL, cmd_error, 1, 0, 10, strlen(exp_data), exp_data);
    EVBSC_ENQ_CMD(reserve, client, small_vec_cb, NULL, cmd_error );

    ev_loop(loop, 0);
    return;

cmd_error:
    evbsc_free(client);
    fail("cmd_error");
}
END_TEST

void fin_cb(bsc *bsclient, queue_node *node, void *data, size_t len)
{
    evbsc *evclient = (evbsc *)bsclient->data;
    fail_unless(strcmp(node->cb_data, exp_data) == 0, "got bad data");
    ev_unloop(evclient->loop, EVUNLOOP_ALL);
}

START_TEST(test_evbsc_defaults) {
    struct ev_loop *loop = ev_default_loop(0);
    char *errstr = NULL;
    exp_data = "baba";
    client = evbsc_new_w_defaults( loop, host, port, onerror, &errstr);
    fail_if( client == NULL, "evbsc_new: %s", errstr);

    EVBSC_ENQ_CMD(ignore,  client, NULL, NULL, cmd_error, "default");
    EVBSC_ENQ_CMD(ignore,  client, NULL, NULL, cmd_error, "default");
    EVBSC_ENQ_CMD(ignore,  client, NULL, NULL, cmd_error, "default");
    EVBSC_ENQ_CMD(ignore,  client, fin_cb, exp_data, cmd_error, "default");

    ev_loop(loop, 0);
    return;

cmd_error:
    evbsc_free(client);
    fail("cmd_error");
}
END_TEST

void reconnect_test_cb(bsc *client, queue_node *node, void *data, size_t len)
{
    system(kill_cmd);
}

static void reconnect(bsc *bsclient, bsc_error_t error)
{
    int i;
    char *errorstr;
    evbsc *evclient      = (evbsc *)bsclient->data;
    struct ev_loop *loop = evclient->loop;

    system(spawn_cmd);

    if (error == BSC_ERROR_INTERNAL) {
        fail("recieved BSC_ERROR_INTERNAL, quitting\n");
    }
    else if (error == BSC_ERROR_SOCKET) {
        EVBSC_ENQ_CMD(put, evclient, NULL, NULL, cmd_error, 1, 0, 10, strlen(kill_cmd), kill_cmd);
        if ( evbsc_reconnect(evclient, &errorstr) ) {
            fail_if( IOQ_NODES_USED(bsclient->outq) != 4, 
                "after reconnect: IOQ_NODES_USED : %d/%d", IOQ_NODES_USED(bsclient->outq), 4);

            ev_unloop(loop, EVUNLOOP_ALL);
            return;
        }
    }
    ev_unloop(loop, EVUNLOOP_ALL);
    fail("critical error: maxed out reconnect attempts, quitting\n");
    return;

cmd_error:
    evbsc_free(evclient);
    ev_unloop(loop, EVUNLOOP_ALL);
    fail("cmd_error");
}

START_TEST(test_evbsc_reconnect) {
    struct ev_loop *loop = ev_default_loop(0);
    char *errstr = NULL;
    sprintf(spawn_cmd, "beanstalkd -p %s -d", reconnect_test_port);
    sprintf(kill_cmd, "ps -ef|grep beanstalkd |grep '%s'| gawk '!/grep/ {print $2}'|xargs kill", reconnect_test_port);
    system(spawn_cmd);
    client = evbsc_new( loop, host, reconnect_test_port, reconnect, 5, 12, 4, &errstr);
    fail_if( client == NULL, "evbsc_new: %s", errstr);

    EVBSC_ENQ_CMD(ignore,  client, reconnect_test_cb, NULL, cmd_error, "default");
    EVBSC_ENQ_CMD(reserve, client, reconnect_test_cb, "1", cmd_error);
    EVBSC_ENQ_CMD(reserve, client, reconnect_test_cb, "1", cmd_error);
    EVBSC_ENQ_CMD(reserve, client, reconnect_test_cb, "1", cmd_error);

    ev_loop(loop, 0);
    evbsc_free(client);
    system(kill_cmd);
    return;

cmd_error:
    evbsc_free(client);
    fail("cmd_error");
}
END_TEST

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
