#ifndef HELPERS_H
#define HELPERS_H

#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdbool.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <libubox/blobmsg.h>
#include <libubox/blobmsg_json.h>

/* Maximum amount of CPUs to index */
#define MAX_CPUS            2
/* Maximum amount of network interfaces to index */
#define MAX_NETINT          4

/**
 * @typedef _cpu_info
 * @property {char*} vendor - The vendor of the CPU.
 * @property {char*} model - The model of the CPU.
 * @property {unsigned} cores - The number of cores.
 * @property {unsigned} cache_size - The size of the CPU cache.
 * @property {unsigned} cache_align - The cache alignment.
 * @property {double} cpu_mhz - The clock speed of the CPU in MHz.
 * @property {char*} address_sizes - The address sizes supported by the CPU.
 * @property {unsigned} physical_id - The physical ID of the CPU.
 */
typedef struct _cpu_info {
    char* vendor;
    char* model;
    unsigned cores;
    unsigned cache_size;
    unsigned cache_align;
    double cpu_mhz;
    char* address_sizes;
    unsigned physical_id;
} _cpu_info;

/**
 * @typedef cpu_info
 * @property {unsigned} cpus_active - The number of active CPUs.
 * @property {_cpu_info*} cpus[MAX_CPUS] - Array of pointers to _cpu_info structures.
 */
typedef struct cpu_info {
    unsigned cpus_active;
    _cpu_info* cpus[MAX_CPUS];
} cpu_info;

/**
 * @typedef swap_info
 * @property {unsigned} swap_total - The total swap memory.
 * @property {unsigned} swap_free - The free swap memory.
 * @property {unsigned} swap_cached - The cached swap memory.
 */
typedef struct swap_info {
    unsigned swap_total;
    unsigned swap_free;
    unsigned swap_cached;
} swap_info;

/**
 * @typedef memory_info
 * @property {unsigned} memory_total - The total physical memory.
 * @property {unsigned} memory_free - The free physical memory.
 * @property {unsigned} memory_available - The available memory.
 * @property {unsigned} memory_cached - The cached memory.
 * @property {swap_info*} swap_memory - Pointer to swap_info structure.
 */
typedef struct memory_info {
    unsigned memory_total;
    unsigned memory_free;
    unsigned memory_available;
    unsigned memory_cached;
    swap_info* swap_memory;
} memory_info;

/**
 * @typedef _network
 * @property {char*} ni_name - The name of the network interface.
 * @property {int} ni_flags - The flags associated with the network interface.
 */
typedef struct _network {
    char* ni_name;
    int ni_flags;
} _network;

/**
 * @typedef network_info
 * @property {unsigned} interface_count - The number of network interfaces.
 * @property {_network*} interfaces[MAX_NETINT] - Array of pointers to _network structures.
 */
typedef struct network_info {
    unsigned interface_count;
    _network* interfaces[MAX_NETINT];
} network_info;

/**
 * @typedef system_info
 * @property {cpu_info*} cpu - Pointer to cpu_info structure.
 * @property {memory_info*} memory - Pointer to memory_info structure.
 * @property {network_info*} network - Pointer to network_info structure.
 * @property {int} uptime - The system uptime.
 * @property {unsigned} requested - The requested data.
 * @property {char*} current_user - The name of the current user.
 */
typedef struct system_info {
    cpu_info* cpu;
    memory_info* memory;
    network_info* network;
    char* current_user;
} system_info;

/**
 * @typedef process
 * @property {char[256]} process_name - The name of the process.
 * @property {char} state - The state of the process.
 * @property {unsigned} pid - The process ID.
 * @property {unsigned} ppid - The parent process ID.
 */
typedef struct process {
    char process_name[256];
    char state;
    unsigned pid;
    unsigned ppid;
} process;

/**
 * @brief Fetches the current uptime.
 * @return the current uptime or error message
 */
char* get_uptime();

/**
 * @brief Fetches the current timestamp.
 * @return the current timestamp.
 */
unsigned get_timestamp();

/**
 * @brief Fetches information about active CPUs.
 * @return a pointer to the `cpu_info` structure or `NULL`.
 * @note the user is responsible for freeing the object.
 */
cpu_info* get_cpu_info();

/**
 * @brief Fetches information about the memory.
 * @return a pointer to the `memory_info` structure or `NULL`.
 * @note the user is responsible for freeing the object.
 */
memory_info* get_mem_info();

/**
 * @brief Fetches information about network interfaces available.
 * @return a pointer to the `network_info` structure or `NULL`.
 * @note the user is responsible for freeing the object.
 */
network_info* get_net_info();

/**
 * @brief Fetches the name of the current user.
 * @return a pointer to the current user's name string.
 * @note this function uses `strdup`, therefore the user is responsible for freeing the string after use.
 */
char* get_current_user();

/**
 * @brief Performs a blank initialization of the `system_info` structure.
 * @param s double pointer to the system_info structure.
 */
void blank_sysinf_init(system_info** s);

/**
 * @brief Fetches all the essential information about the system.
 * @return a pointer to the `system_info` structure or `NULL`.
 * @note the user is responsible for freeing the object.
 */
system_info* get_system_info_obj();

/**
 * @brief Fetches all the essential information about the system.
 * @param s double pointer to the `system_info` structure.
 * @note the user has to provide a double pointer to the `system_info` structure.
 */
void get_system_info(system_info** s);

/**
 * @brief Fetches information about a specific process.
 * @param pid the process ID to look up.
 * @return a pointer to the newly created `process` structure or `NULL`.
 */
process* pid_lookup(int pid);

/**
 * @brief Sends a signal to a specific process.
 * @param pid process ID.
 * @param signal_id ID of the signal.
 * @return a string status message or `NULL`.
 * @note the user is responsible for freeing the string.
 */
char* send_signal(int pid, int signal_id);

/**
 * @brief Cleans up the whole `cpu_info` object, rendering it unusable.
 * @param c double pointer to the `cpu_info` structure.
 */
void cpuinf_cleanup(cpu_info** c);

/**
 * @brief Cleans up the `memory_info` object.
 * @param m double pointer to the `memory_info` structure.
 */
void meminf_cleanup(memory_info** m);

/**
 * @brief Cleans up the `network_info` object.
 * @param n double pointer to the `network_info` structure.
 */
void netinf_cleanup(network_info** n);

/**
 * @brief Cleans up the whole `system_info` object.
 * @param s double pointer to the `system_info` structure.
 */
void sysinf_cleanup(system_info** s);

#endif // HELPERS_H
