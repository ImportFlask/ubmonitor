#include "../includes/ubus_methods.h"

struct blob_buf b;
struct ubus_context* ctx;
system_info* info = NULL;

static const struct blobmsg_policy signal_policy[] = {
    [PROC_ID] = { .name = "pid", .type = BLOBMSG_TYPE_INT32 },
    [SIGNAL_ID] = { .name = "sig_id", .type = BLOBMSG_TYPE_INT32 },
};

static const struct blobmsg_policy pid_lookup_policy[] = {
    [PROC_ID] = { .name = "pid", .type = BLOBMSG_TYPE_INT32 },
};

static const struct ubus_method ubm_methods[] = {
    UBUS_METHOD_NOARG("info", get_info),
    UBUS_METHOD_NOARG("cpu", get_cpu),
    UBUS_METHOD_NOARG("mem", get_memory),
    UBUS_METHOD_NOARG("net", get_network),
    UBUS_METHOD("signal", ub_send_signal, signal_policy),
    UBUS_METHOD("lookup", ub_pid_lookup, pid_lookup_policy),
};

static struct ubus_object_type ubm_object_type = 
    UBUS_OBJECT_TYPE("ubm", ubm_methods);

static struct ubus_object ubm_object = {
    .name = "ubm",
    .type = &ubm_object_type,
    .methods = ubm_methods,
    .n_methods = ARRAY_SIZE(ubm_methods)
};

int initialize_ubus() {
    uloop_init();
    blank_sysinf_init(&info);

    ctx = ubus_connect(NULL);
    if (!ctx) {
        syslog(LOG_CRIT, "Failed to initialize UBus!");
        return -1;
    }
    ubus_add_uloop(ctx);

    int rc = ubus_add_object(ctx, &ubm_object);
    if (rc) {
        syslog(LOG_CRIT, "Failed to add UBus object!");
        ubus_free(ctx);
        uloop_done();
        return -2;
    }

    uloop_run();

    ubus_free(ctx);
    uloop_done();
    return 0;
}

void ubus_methods_cleanup() {
    blob_buf_free(&b);
    sysinf_cleanup(&info);
    if (ctx) {
        ubus_free(ctx);
        uloop_done();
    }
}

int get_info(struct ubus_context *ctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method,
        struct blob_attr *msg) 
        {
            if (info != NULL) {
                if (!PRESERVE_CPU_DATA)
                    cpuinf_cleanup(&(info->cpu));
                meminf_cleanup(&(info->memory));
                netinf_cleanup(&(info->network));
                free(info->current_user);
            }

            get_system_info(&info);
            if (info == NULL){
                syslog(LOG_CRIT, "Failed to parse any information at all!");
                return -1;
            }

            blob_buf_init(&b, 0);

            void *cookie, *cookie2, *cookie3;
            if (info->cpu != NULL) {
                cookie = blobmsg_open_table(&b, "cpu");
                blobmsg_add_u32(&b, "cpu_count", info->cpu->cpus_active);
                cookie2 = blobmsg_open_array(&b, "cpus");
                for (int i = 0; i < MAX_CPUS; i++) {
                    _cpu_info* c_cpu = info->cpu->cpus[i];
                    if (c_cpu != NULL) {
                        cookie3 = blobmsg_open_table(&b, NULL);
                        blobmsg_add_string(&b, "vendor_id", c_cpu->vendor);
                        blobmsg_add_string(&b, "model_name", c_cpu->model);
                        blobmsg_add_u32(&b, "cores", c_cpu->cores);
                        blobmsg_add_u32(&b, "cache_size", c_cpu->cache_size);
                        blobmsg_add_u32(&b, "cache_align", c_cpu->cache_align);
                        blobmsg_add_double(&b, "cpu_mhz", c_cpu->cpu_mhz);
                        blobmsg_add_string(&b, "address_sizes", c_cpu->address_sizes);
                        blobmsg_close_table(&b, cookie3);
                    }
                }
                blobmsg_close_array(&b, cookie2);
                blobmsg_close_table(&b, cookie);
            } else {
                blobmsg_add_string(&b, "cpu_msg", "failed to obtain");
            }

            if (info->memory != NULL) {
                cookie = blobmsg_open_table(&b, "memory");
                memory_info* mem = info->memory;
                blobmsg_add_u32(&b, "memory_total", mem->memory_total);
                blobmsg_add_u32(&b, "memory_free", mem->memory_free);
                blobmsg_add_u32(&b, "memory_available", mem->memory_available);
                blobmsg_add_u32(&b, "memory_cached", mem->memory_cached);

                cookie2 = blobmsg_open_table(&b, "memory_swap");
                swap_info* swap = mem->swap_memory;
                blobmsg_add_u32(&b, "m_swap_total", swap->swap_total);
                blobmsg_add_u32(&b, "m_swap_free", swap->swap_free);
                blobmsg_add_u32(&b, "m_swap_cached", swap->swap_cached);
                blobmsg_close_table(&b, cookie2);
                blobmsg_close_table(&b, cookie);
            } else {
                blobmsg_add_string(&b, "memory_msg", "failed to obtain");
            }

            if (info->network != NULL) {
                cookie = blobmsg_open_table(&b, "network");
                network_info* interfaces = info->network;
                blobmsg_add_u32(&b, "interface_count", interfaces->interface_count);
                cookie2 = blobmsg_open_array(&b, "interfaces");
                for (int i = 0; i < interfaces->interface_count; i++) {
                    cookie3 = blobmsg_open_table(&b, NULL);
                    _network* interface = interfaces->interfaces[i];
                    blobmsg_add_string(&b, "name", interface->ni_name);
                    blobmsg_add_u32(&b, "flags", interface->ni_flags);
                    blobmsg_close_table(&b, cookie3);
                }
                blobmsg_close_array(&b, cookie2);
                blobmsg_close_table(&b, cookie);
            } else {
                blobmsg_add_string(&b, "network_msg", "failed to obtain");
            }

            blobmsg_add_string(&b, "current_user", info->current_user);

            char* uptime = get_uptime();
            blobmsg_add_string(&b, "uptime", uptime);
            blobmsg_add_u32(&b, "requested", get_timestamp());

            free(uptime);
            ubus_send_reply(ctx, req, b.head);
            return 0;
        }

int get_cpu(struct ubus_context *ctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method,
        struct blob_attr *msg)
        {
            if (info != NULL) {
                if (!(bool)PRESERVE_CPU_DATA) {
                    cpuinf_cleanup(&(info->cpu));
                    info->cpu = get_cpu_info();
                } else {
                    if (info->cpu == NULL)
                        info->cpu = get_cpu_info();
                }
            } else {
                get_system_info(&info);
            }

            blob_buf_init(&b, 0);

            void *cookie, *cookie2, *cookie3;
            if (info->cpu != NULL) {
                cookie = blobmsg_open_table(&b, "cpu");
                blobmsg_add_u32(&b, "cpu_count", info->cpu->cpus_active);
                cookie2 = blobmsg_open_array(&b, "cpus");
                for (int i = 0; i < MAX_CPUS; i++) {
                    _cpu_info* c_cpu = info->cpu->cpus[i];
                    if (c_cpu != NULL) {
                        cookie3 = blobmsg_open_table(&b, NULL);
                        blobmsg_add_string(&b, "vendor_id", c_cpu->vendor);
                        blobmsg_add_string(&b, "model_name", c_cpu->model);
                        blobmsg_add_u32(&b, "cores", c_cpu->cores);
                        blobmsg_add_u32(&b, "cache_size", c_cpu->cache_size);
                        blobmsg_add_u32(&b, "cache_align", c_cpu->cache_align);
                        blobmsg_add_double(&b, "cpu_mhz", c_cpu->cpu_mhz);
                        blobmsg_add_string(&b, "address_sizes", c_cpu->address_sizes);
                        blobmsg_close_table(&b, cookie3);
                    }
                }
                blobmsg_close_array(&b, cookie2);
                blobmsg_close_table(&b, cookie);
            } else {
                blobmsg_add_string(&b, "cpu_msg", "failed to obtain");
            }
            blobmsg_add_u32(&b, "requested", get_timestamp());
            ubus_send_reply(ctx, req, b.head);
            return 0;
        }

int get_memory(struct ubus_context *ctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method,
        struct blob_attr *msg)
        {
            if (info != NULL)
                meminf_cleanup(&(info->memory));

            info->memory = get_mem_info();
            if (info == NULL){
                syslog(LOG_CRIT, "Failed to parse any memory information at all!");
                return -1;
            }

            blob_buf_init(&b, 0);

            void *cookie, *cookie2;
            if (info->memory != NULL) {
                cookie = blobmsg_open_table(&b, "memory");
                memory_info* mem = info->memory;
                blobmsg_add_u32(&b, "memory_total", mem->memory_total);
                blobmsg_add_u32(&b, "memory_free", mem->memory_free);
                blobmsg_add_u32(&b, "memory_available", mem->memory_available);
                blobmsg_add_u32(&b, "memory_cached", mem->memory_cached);

                cookie2 = blobmsg_open_table(&b, "memory_swap");
                swap_info* swap = mem->swap_memory;
                blobmsg_add_u32(&b, "m_swap_total", swap->swap_total);
                blobmsg_add_u32(&b, "m_swap_free", swap->swap_free);
                blobmsg_add_u32(&b, "m_swap_cached", swap->swap_cached);
                blobmsg_close_table(&b, cookie2);
                blobmsg_close_table(&b, cookie);
            } else {
                blobmsg_add_string(&b, "memory_msg", "failed to obtain");
            }
            blobmsg_add_u32(&b, "requested", get_timestamp());
            ubus_send_reply(ctx, req, b.head);
            return 0;
        }

int get_network(struct ubus_context *ctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method,
        struct blob_attr *msg)
        {
            if (info != NULL)
                netinf_cleanup(&(info->network));

            info->network = get_net_info();
            blob_buf_init(&b, 0);

            void *cookie, *cookie2;
            if (info->network != NULL) {
                network_info* interfaces = info->network;
                blobmsg_add_u32(&b, "interface_count", interfaces->interface_count);
                cookie = blobmsg_open_array(&b, "interfaces");
                for (int i = 0; i < interfaces->interface_count; i++) {
                    cookie2 = blobmsg_open_table(&b, NULL);
                    _network* interface = interfaces->interfaces[i];
                    blobmsg_add_string(&b, "name", interface->ni_name);
                    blobmsg_add_u32(&b, "flags", interface->ni_flags);
                    blobmsg_close_table(&b, cookie2);
                }
                blobmsg_close_array(&b, cookie);
            } else {
                blobmsg_add_string(&b, "network_msg", "failed to obtain");
            }
            blobmsg_add_u32(&b, "requested", get_timestamp());
            ubus_send_reply(ctx, req, b.head);
            return 0;
        }

int ub_send_signal(struct ubus_context *ctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method,
        struct blob_attr *msg) 
        {
            struct blob_attr* tb[__PSIG_MAX];
            blobmsg_parse(signal_policy, ARRAY_SIZE(signal_policy), tb, blob_data(msg), blob_len(msg));

            blob_buf_init(&b, 0);
            if (tb[PROC_ID] && tb[SIGNAL_ID]) {
                char* resp = send_signal(
                    (int)blobmsg_get_u32(tb[PROC_ID]),
                    (int)blobmsg_get_u32(tb[SIGNAL_ID])
                );
                blobmsg_add_string(&b, "response", resp == NULL ? "signal sent" : resp);
            } else {
                blobmsg_add_string(&b, "error", "failed to parse provided fields");
            }

            blobmsg_add_u32(&b, "requested", get_timestamp());
            ubus_send_reply(ctx, req, b.head);
            return 0;
        }

int ub_pid_lookup(struct ubus_context *ctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method,
        struct blob_attr *msg)
        {
            struct blob_attr* tb[__PLOOKUP_MAX];
            blobmsg_parse(pid_lookup_policy, ARRAY_SIZE(pid_lookup_policy), tb, blob_data(msg), blob_len(msg));

            blob_buf_init(&b, 0);
            if (tb[PROC_ID]) {
                process* proc = pid_lookup(blobmsg_get_u32(tb[PROC_ID]));
                if (proc != NULL) {
                    blobmsg_add_string(&b, "process_name", proc->process_name);
                    blobmsg_add_u32(&b, "pid", proc->pid);
                    blobmsg_add_u32(&b, "ppid", proc->ppid);
                    switch (proc->state) {
                        case 'R':
                            blobmsg_add_string(&b, "state", "running");
                            break;
                        case 'D':
                            blobmsg_add_string(&b, "state", "uninterruptible sleep");
                            break;
                        case 'S':
                            blobmsg_add_string(&b, "state", "interruptible sleep");
                            break;
                        case 'T':
                            blobmsg_add_string(&b, "state", "stopped");
                            break;
                        case 'Z':
                            blobmsg_add_string(&b, "state", "zombie");
                            break;
                        default:
                            blobmsg_add_string(&b, "state", "unknown");
                            break;
                    }
                    free(proc);
                } else {
                    blobmsg_add_string(&b, "error", "failed to lookup");
                }
            }
            blobmsg_add_u32(&b, "requested", get_timestamp());
            ubus_send_reply(ctx, req, b.head);
            return 0;
        }