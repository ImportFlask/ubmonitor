#include <signal.h>
#include <stdlib.h>

#include "includes/ubus_methods.h"

void handle_sig(int signo);

int main(void) {
    struct sigaction sa;
    sigset_t sigset;
    sigemptyset(&sigset);

    sa.sa_handler = handle_sig;
    sa.sa_mask = sigset;
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGPIPE, &sa, NULL);

    int ubus_rc = initialize_ubus();
    if (ubus_rc != 0) {
        ubus_methods_cleanup();
        fprintf(stderr, "UBMonitor exited with %d\n", ubus_rc);
        exit(ubus_rc);
    }
    return 0;
}

void handle_sig(int signo) {
    ubus_methods_cleanup();
    exit(0);
}