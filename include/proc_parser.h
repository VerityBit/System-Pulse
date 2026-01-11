#ifndef SYSTEM_PULSE_PROC_PARSER_H
#define SYSTEM_PULSE_PROC_PARSER_H

#include <stddef.h>

typedef struct CpuTimes {
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;
} CpuTimes;

typedef struct MemInfo {
    unsigned long long mem_total_kb;
    unsigned long long mem_free_kb;
    unsigned long long mem_available_kb;
    unsigned long long buffers_kb;
    unsigned long long cached_kb;
} MemInfo;

typedef struct NetDevStats {
    char name[32];
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
} NetDevStats;

int parse_proc_stat_cpu_line(const char *text, CpuTimes *out);
int parse_proc_meminfo(const char *text, MemInfo *out);
int parse_net_dev_line(const char *line, NetDevStats *out);

double cpu_usage_percent(const CpuTimes *prev, const CpuTimes *curr);

#endif
