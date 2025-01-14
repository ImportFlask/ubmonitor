#include "../includes/helpers.h"

char* get_uptime() {
    FILE* fp = fopen("/proc/uptime", "r");
    if (fp == NULL) {
        syslog(LOG_WARNING, "Failed to open /proc/uptime");
        return strdup("unable to open /proc/uptime");
    }

    double uptime = -1.0;
    if (fscanf(fp, "%lf", &uptime) != 1) {
        fclose(fp);
        syslog(LOG_WARNING, "Failed to read uptime from /proc/uptime");
        return strdup("failed to read uptime}");
    }
    fclose(fp);

    char* result = (char*)malloc(32);
    if (result == NULL) {
        syslog(LOG_CRIT, "Memory allocation failed in get_uptime");
        return strdup("memory allocation failed");
    }
    
    snprintf(result, 32, "%d", (int)uptime);
    return result;
}

unsigned get_timestamp() {
    return (unsigned)time(NULL);
}

cpu_info* get_cpu_info() {
    FILE* fp = fopen("/proc/cpuinfo", "r");
    if (fp == NULL) {
        syslog(LOG_WARNING, "Failed to open /proc/cpuinfo");
        return NULL;
    }

    cpu_info* cpu = (cpu_info*) malloc(sizeof(cpu_info));
    if (cpu == NULL) {
        fclose(fp);
        syslog(LOG_WARNING, "Failed allocate memory for cpu_info struct!");
        return NULL;
    }

    cpu->cpus_active = 0;
    for (int i = 0; i < MAX_CPUS; i++)
        cpu->cpus[i] = NULL;

    char buffer[512];
    _cpu_info c_cpu;
    unsigned cpu_index = 0;
    int processed_ids[MAX_CPUS];
    for (int i = 0; i < MAX_CPUS; i++)
        processed_ids[i] = -1;

    while(fgets(buffer, sizeof(buffer), fp)) {
        if (strncmp(buffer, "processor", 9) == 0) {
            if (cpu_index > 0 && cpu_index <= MAX_CPUS) {
                cpu->cpus[cpu_index - 1] = (_cpu_info*) malloc(sizeof(_cpu_info));
                memcpy(cpu->cpus[cpu_index - 1], &c_cpu, sizeof(_cpu_info));
            }
            memset(&c_cpu, 0, sizeof(_cpu_info));
        }
        
        if (strncmp(buffer, "vendor_id", 9) == 0) {
            if (c_cpu.vendor != NULL)
                free(c_cpu.vendor);
            c_cpu.vendor = strdup(strchr(buffer, ':') + 2);
            c_cpu.vendor[strcspn(c_cpu.vendor, "\n")] = '\0';
        }

        if (strncmp(buffer, "model name", 10) == 0) {
            if (c_cpu.model != NULL)
                free(c_cpu.model);
            c_cpu.model = strdup(strchr(buffer, ':') + 2);
            c_cpu.model[strcspn(c_cpu.model, "\n")] = '\0';
        }

        if (strncmp(buffer, "cpu cores", 9) == 0)
            sscanf(strchr(buffer, ':') + 2, "%u", &c_cpu.cores);

        if (strncmp(buffer, "cache size", 10) == 0)
            sscanf(strchr(buffer, ':') + 2, "%u", &c_cpu.cache_size);

        if (strncmp(buffer, "cpu MHz", 7) == 0)
            sscanf(strchr(buffer, ':') + 2, "%lf", &c_cpu.cpu_mhz);
        
        if (strncmp(buffer, "cache_alignment", 15) == 0)
            sscanf(strchr(buffer, ':') + 2, "%u", &c_cpu.cache_align);

        if (strncmp(buffer, "address sizes", 13) == 0) {
            if (c_cpu.address_sizes != NULL)
                free(c_cpu.address_sizes);
            c_cpu.address_sizes = strdup(strchr(buffer, ':') + 2);
            c_cpu.address_sizes[strcspn(c_cpu.address_sizes, "\n")] = '\0';
        }

        if (strncmp(buffer, "physical id", 11) == 0) {
            sscanf(strchr(buffer, ':') + 2, "%u", &c_cpu.physical_id);
            bool processed = false;
            for (int i = 0; i < MAX_CPUS; i++) {
                if (processed_ids[i] == c_cpu.physical_id) {
                    processed = true;
                    break;
                }
            }

            if (processed) {
                while((fgets(buffer, sizeof(buffer), fp)) && strncmp(buffer, "processor", 9) != 0);
                continue;
            } else {
                if (cpu_index < MAX_CPUS) {
                    processed_ids[cpu_index] = c_cpu.physical_id;
                    cpu_index++;
                    cpu->cpus_active = cpu_index;
                } else {
                    while((fgets(buffer, sizeof(buffer), fp)) && strncmp(buffer, "processor", 9) != 0);
                    continue;
                }
            }
        }
    }
    fclose(fp);

    free(c_cpu.vendor);
    free(c_cpu.model);
    free(c_cpu.address_sizes);
    return cpu;
}

char* get_current_user() {
    FILE* fp;
    char buffer[32+1];

    fp = popen("who | awk '{print $1}' | sort -u", "r");
    if (fp == NULL) {
        syslog(LOG_ERR, "Failed to get the current user");
        return "unknown";
    }

    fgets(buffer, sizeof(buffer), fp);
    buffer[strcspn(buffer, "\r\n")] = 0;

    pclose(fp);
    return strdup(buffer);
}

memory_info* get_mem_info() {
    FILE* fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        syslog(LOG_WARNING, "Failed to open /proc/meminfo");
        return NULL;
    }

    memory_info* memory = (memory_info*) malloc(sizeof(memory_info));
    if (memory == NULL) {
        fclose(fp);
        syslog(LOG_WARNING, "Failed allocate memory for memory_info struct!");
        return NULL;
    }

    memory->memory_total = 0;
    memory->memory_free = 0;
    memory->memory_available = 0;
    memory->memory_cached = 0;
    memory->swap_memory = (swap_info*) malloc(sizeof(swap_info));
    if (memory->swap_memory != NULL) {
        memory->swap_memory->swap_total = 0;
        memory->swap_memory->swap_cached = 0;
        memory->swap_memory->swap_free = 0;
    } 

    char buffer[256];
    while(fgets(buffer, sizeof(buffer), fp)) {
        if (strncmp(buffer, "MemTotal", 8) == 0) {
            sscanf(strchr(buffer, ':') + 2, "%u", &memory->memory_total);
        }

        if (strncmp(buffer, "MemFree", 7) == 0) {
            sscanf(strchr(buffer, ':') + 2, "%u", &memory->memory_free);
        }

        if (strncmp(buffer, "MemAvailable", 12) == 0) {
            sscanf(strchr(buffer, ':') + 2, "%u", &memory->memory_available);
        }

        if (strncmp(buffer, "Cached", 6) == 0) {
            sscanf(strchr(buffer, ':') + 2, "%u", &memory->memory_cached);
        }

        if (strncmp(buffer, "SwapTotal", 9) == 0) {
            if (memory->swap_memory != NULL)
                sscanf(strchr(buffer, ':') + 2, "%u", &memory->swap_memory->swap_total);
        }

        if (strncmp(buffer, "SwapCached", 10) == 0) {
            if (memory->swap_memory != NULL)
                sscanf(strchr(buffer, ':') + 2, "%u", &memory->swap_memory->swap_cached);
        }

        if (strncmp(buffer, "SwapFree", 8) == 0) {
            if (memory->swap_memory != NULL)
                sscanf(strchr(buffer, ':') + 2, "%u", &memory->swap_memory->swap_free);
        }
    }
    fclose(fp);
    return memory;
}

network_info* get_net_info() {
    struct ifaddrs *addrs, *tmp;
    if (getifaddrs(&addrs) == -1) {
        syslog(LOG_ERR, "Failed to obtain network interfaces.");
        return NULL;
    }

    network_info* net_info = (network_info*) malloc(sizeof(network_info));
    if (net_info == NULL) {
        syslog(LOG_ERR, "Failed to allocate memory for network_info struct!");
        freeifaddrs(addrs);
        return NULL;
    }

    for (int i = 0; i < MAX_NETINT; i++)
        net_info->interfaces[i] = NULL;

    net_info->interface_count = 0;
    tmp = addrs;

    while (tmp) {

        if (net_info->interface_count >= MAX_NETINT)
            break;

        if (tmp->ifa_addr->sa_family != AF_PACKET) {
            tmp = tmp->ifa_next;
            continue;
        }

        _network* c_net = net_info->interfaces[net_info->interface_count];
        c_net = (_network*) malloc(sizeof(_network));
        if (c_net == NULL) {
            syslog(LOG_ERR, "Failed to allocate memory for _network struct!");
            return NULL;
        }
        
        c_net->ni_name = strdup(tmp->ifa_name);
        c_net->ni_flags = tmp->ifa_flags;

        net_info->interfaces[net_info->interface_count] = c_net;
        net_info->interface_count++;
        tmp = tmp->ifa_next;
    }
    freeifaddrs(addrs);
    return net_info;
}

void blank_sysinf_init(system_info** s) {
    if (s == NULL)
        return;
    
    if ((*s) != NULL) {
        syslog(LOG_ERR, "Blank initialization could not be performed. The pointer is not NULL!");
        return;
    }

    (*s) = (system_info*) malloc(sizeof(system_info));
    if ((*s) == NULL) {
        syslog(LOG_CRIT, "Failed to allocate memory for the system_info object!");
        return;
    }

    (*s)->cpu = NULL;
    (*s)->memory = NULL;
    (*s)->network = NULL;
    (*s)->current_user = NULL;
}

system_info* get_system_info_obj() {
    system_info* sys_info = (system_info*) malloc(sizeof(system_info));
    if (sys_info == NULL) {
        syslog(LOG_WARNING, "Failed allocate memory for system_info struct!");
        return NULL;
    }

    sys_info->cpu = get_cpu_info();
    sys_info->memory = get_mem_info();
    sys_info->network = get_net_info();
    sys_info->current_user = get_current_user();
    return sys_info;
}

void get_system_info(system_info** s) {
    if (s == NULL) {
        syslog(LOG_ERR, "get_system_info method received a null pointer!");
        return;
    }

    if (*s == NULL) {
        *s = get_system_info_obj();
        return;
    }

    (*s)->cpu = get_cpu_info();
    (*s)->memory = get_mem_info();
    (*s)->network = get_net_info();
    (*s)->current_user = get_current_user();
}

process* pid_lookup(int pid) {
    FILE* fp = NULL;
    char exec_buffer[32];
    sprintf(exec_buffer, "cat /proc/%d/stat 2>/dev/null", pid);
    fp = popen(exec_buffer, "r");
    if (fp == NULL) {
        syslog(LOG_ERR, "Failed to open stream for pid lookup!");
        return NULL;
    }
    
    int ppid;
    char state;
    char proc_name[256];
    if (fscanf(fp, "%*d (%[^)]) %c %d", proc_name, &state, &ppid) != 3) {
        pclose(fp);
        syslog(LOG_ERR, "Failed to read stat file!");
        return NULL;
    }
    pclose(fp);

    process* proc = (process*) malloc(sizeof(process));
    if (proc == NULL) {
        syslog(LOG_ERR, "Failed to allocate memory for process struct!");
        return NULL;
    }

    proc->pid = pid;
    proc->ppid = ppid;
    proc->state = state;
    strcpy(proc->process_name, proc_name);
    return proc;
}

char* send_signal(int pid, int signal_id) {
    FILE* fp = NULL;
    char exec_buffer[32];
    sprintf(exec_buffer, "/bin/kill -%d %d 2>&1", signal_id, pid);
    fp = popen(exec_buffer, "r");
    if (fp == NULL)
        return strdup("failed to create a pipe");

    char buffer[256] = {0};
    while (fgets(buffer, sizeof(buffer), fp) != NULL);
    
    pclose(fp);
    if (strlen(buffer) == 0)
        return NULL;

    if (strstr(buffer, "No such process") != NULL)
        return strdup("pid is not reserved");

    return strdup(buffer);
}

void cpuinf_cleanup(cpu_info** c) {
    if (*c == NULL)
        return;
    
    for (int i = 0; i < MAX_CPUS; i++) {
        _cpu_info* cpu_data = (*c)->cpus[i];
        if (cpu_data != NULL) {
            if (cpu_data->model != NULL)
                free(cpu_data->model);
            if (cpu_data->vendor != NULL) 
                free(cpu_data->vendor);
            if (cpu_data->address_sizes != NULL) 
                free(cpu_data->address_sizes);
            free(cpu_data);
        }
    }

    free(*c);
    *c = NULL;
}

void meminf_cleanup(memory_info** m) {
    if (*m == NULL)
        return;
    
    if ((*m)->swap_memory != NULL) {
        free((*m)->swap_memory);
        (*m)->swap_memory = NULL;
    }

    free(*m);
    *m = NULL;
}

void netinf_cleanup(network_info** n) {
    if (*n == NULL)
        return;

    network_info* netinf = *n;
    for (int i = 0; i < netinf->interface_count; i++) {
        _network* network = netinf->interfaces[i];
        if (network == NULL) continue;
        if (network->ni_name) free(network->ni_name);
        free(netinf->interfaces[i]);
        netinf->interfaces[i] = NULL;
    }

    free(netinf);
    (*n) = NULL;
}

void sysinf_cleanup(system_info** s) {
    if (s == NULL || *s == NULL)
        return;
    
    cpuinf_cleanup(&((*s)->cpu));
    meminf_cleanup(&((*s)->memory));
    netinf_cleanup(&((*s)->network));
    free((*s)->current_user);
    free(*s);
    *s = NULL;
}