#ifndef UBUS_METHODS_H
#define UBUS_METHODS_H

#include <libubus.h>
#include <libubox/blobmsg.h>
#include <libubox/blobmsg_json.h>

#include "defs.h"
#include "helpers.h"

enum { PROC_ID, SIGNAL_ID, __PSIG_MAX };
enum { __PLOOKUP_MAX = 1 };

extern struct blob_buf b;
extern struct ubus_context* ctx;
extern system_info* info;

int initialize_ubus();
void ubus_methods_cleanup();

int get_info(struct ubus_context *ctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method,
        struct blob_attr *msg);

int get_cpu(struct ubus_context *ctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method,
        struct blob_attr *msg);

int get_memory(struct ubus_context *ctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method,
        struct blob_attr *msg);

int get_network(struct ubus_context *ctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method,
        struct blob_attr *msg);

int ub_send_signal(struct ubus_context *ctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method,
        struct blob_attr *msg);

int ub_pid_lookup(struct ubus_context *ctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method,
        struct blob_attr *msg);

#endif // UBUS_METHODS_H