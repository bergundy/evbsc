/**
 * =====================================================================================
 * @file   bsc_reconnect_handler.c
 * @brief  
 * @date   09/19/2010 05:37:25 PM
 * @author Roey Berman, (royb@walla.net.il), Walla!
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <evbsc.h>

char spawn_cmd[200], kill_cmd[200], *host = "localhost", *port = "16666", errorstr[BSC_ERRSTR_LEN];
static bsc_error_t bsc_error;
static int reconnect_attempts = 5, finished = 0, put_finished;

void put_cb(bsc *client, struct bsc_put_info *info)
{
    if (info->response.code == BSC_PUT_RES_INSERTED)
        printf("put_client: put successful\n");
    else
        printf("put_client: put failed\n");
}

void spawn_put_client(struct ev_loop *loop)
{
    evbsc *put_client;
    put_client = evbsc_new(loop, host, port, "baba", NULL, 16, 12, 4, errorstr);
    if(!put_client) {
        fprintf(stderr, "bsc_new: %s", errorstr);
        exit(EXIT_FAILURE);
    }

    bsc_error = bsc_put(BSCIFY(put_client), put_cb, NULL, 1, 0, 10, strlen("baba"), "baba", false);

    if (bsc_error != BSC_ERROR_NONE) {
        fprintf(stderr, "bsc_put failed (%d)", bsc_error );
        exit(EXIT_FAILURE);
    }
}

void ignore_cb(bsc *client, struct bsc_ignore_info *info)
{
    system(kill_cmd);
}

void reserve_cb(bsc *client, struct bsc_reserve_info *info)
{
    printf("%s\n", "got reserve cb");
    ev_unloop(EVBSCIFY(client)->loop, EVUNLOOP_ONE);
}

static void reconnect(bsc *client, bsc_error_t error)
{
    char errorstr[BSC_ERRSTR_LEN];
    int i;

    system(spawn_cmd);
    spawn_put_client(EVBSCIFY(client)->loop);

    switch (error) {
        case BSC_ERROR_INTERNAL:
            fprintf(stderr, "critical error: recieved BSC_ERROR_INTERNAL, quitting\n");
            break;
        case BSC_ERROR_MEMORY:
            fprintf(stderr, "critical error: recieved BSC_ERROR_MEMORY, quitting\n");
            break;
        case BSC_ERROR_SOCKET:
            fprintf(stderr, "error: recieved BSC_ERROR_SOCKET, attempting to reconnect ...\n");
            for (i = 0; i < reconnect_attempts; ++i) {
                if ( bsc_reconnect(client, errorstr) ) {
                    printf("reconnect successful\n");
                    return;
                }
                else
                    fprintf(stderr, "error: reconnect attempt %d/%d - %s", i+1, reconnect_attempts, errorstr);
            }
            fprintf(stderr, "critical error: maxed out reconnect attempts, quitting\n");
            break;
        default:
            fprintf(stderr, "critical error: got unknown error (%d)\n", error);
    }
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    evbsc *client;
    struct ev_loop *loop = ev_default_loop(0);

    sprintf(spawn_cmd, "beanstalkd -p %s -d", port);
    sprintf(kill_cmd, "ps -ef|grep beanstalkd |grep '%s'| gawk '!/grep/ {print $2}'|xargs kill", port);

    system(kill_cmd);
    system(spawn_cmd);

    client = evbsc_new(loop, host, port, "baba", reconnect, 16, 12, 4, errorstr);
    if(!client) {
        fprintf(stderr, "bsc_new: %s", errorstr);
        return EXIT_FAILURE;
    }

    bsc_error = bsc_ignore(BSCIFY(client), ignore_cb, NULL, "default");
    if (bsc_error != BSC_ERROR_NONE)
        fprintf(stderr, "bsc_ignore failed (%d)", bsc_error );

    bsc_error = bsc_reserve(BSCIFY(client), reserve_cb, NULL, BSC_RESERVE_NO_TIMEOUT);
    if (bsc_error != BSC_ERROR_NONE)
        fprintf(stderr, "bsc_reserve failed (%d)", bsc_error );

    ev_loop(loop, 0);

    // cleanup
    system(kill_cmd);
    evbsc_free(client);
}
