#include <stdio.h>
#include <stdlib.h>
#include <sysrepo.h>
#include <signal.h>
#include <libyang/libyang.h>

volatile int exit_application = 0;

static void
sigint_handler(int signum)
{
    (void)signum;

    exit_application = 1;
}

int main() {
    sr_conn_ctx_t *connection = NULL;
    sr_session_ctx_t *session = NULL;
    const char *data_xml;
    char *state[] = {"LOCKED", "HOLDOVER", "FREERUN"};
    int i = 0;

    int rc = SR_ERR_OK;

    /* Connect to sysrepo */
    if (sr_connect(SR_CONN_DEFAULT, &connection) != SR_ERR_OK) {
        fprintf(stderr, "Failed to connect to sysrepo.\n");
        return EXIT_FAILURE;
    }

    /* Start a session */
    if (sr_session_start(connection, SR_DS_OPERATIONAL, &session) != SR_ERR_OK) {
        fprintf(stderr, "Failed to start sysrepo session.\n");
        sr_disconnect(connection);
        return EXIT_FAILURE;
    }

    sr_log_stderr(SR_LL_DBG);
    /* Set the operational data for schema mounts */
state_change:
    i = (i+1) % 3;
    rc = sr_set_item_str(session, "/o-ran-sync-state-change-noti-test:sync-noti-test/sync-status/sync-state", state[i], NULL, SR_EDIT_DEFAULT);
    if (rc != SR_ERR_OK) goto cleanup;

    /* Commit the changes */
    if (sr_apply_changes(session, 0) != SR_ERR_OK) {
        fprintf(stderr, "Failed to apply schema-mount configuration.\n");
    } else {
        printf("Schema mount configuration applied successfully.\n");
    }


    printf("\n\n ========== WAITING AS OWNER OF SET DATA ==========\n\n");

    /* loop until ctrl-c is pressed / SIGINT is received */
    signal(SIGINT, sigint_handler);
    signal(SIGPIPE, SIG_IGN);
    while (!exit_application) {

        const struct ly_ctx *ctx = sr_acquire_context(connection);
        struct lyd_node *notif = NULL;
        LY_ERR lyrc;
            
        lyrc = lyd_new_path(NULL, ctx,
                            "/o-ran-sync-state-change-noti-test:synchronization-state-change",
                            NULL, 0, &notif);
        if (lyrc != LY_SUCCESS) {
            fprintf(stderr, "Failed to create notification root.\n");
            goto cleanup;
        }
        
        lyrc = lyd_new_path(notif, ctx,
                            "/o-ran-sync-state-change-noti-test:synchronization-state-change/sync-state",
                            state[i], 0, NULL);
        if (lyrc != LY_SUCCESS) {
            fprintf(stderr, "Failed to add sync-state to notification.\n");
            lyd_free_all(notif);
            goto cleanup;
        }
        
        if (sr_notif_send_tree(session, notif, 0, 0) == SR_ERR_OK) {
            printf("Notification sent: sync-state = %s\n", state[i]);
        } else {
            fprintf(stderr, "Failed to send notification.\n");
        }
        
        lyd_free_all(notif);

        sleep(10);
        goto state_change;
    }

    printf("Application exit requested, exiting.\n");

    /* Cleanup */
cleanup:
    printf("Entered cleanup\n");
    sr_session_stop(session);
    sr_disconnect(connection);

    return EXIT_SUCCESS;
}
