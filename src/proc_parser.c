#include "proc_parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *find_line_prefix(const char *text, const char *prefix) {
    size_t prefix_len = 0;
    const char *p = text;

    if (!text || !prefix) {
        return NULL;
    }

    prefix_len = strlen(prefix);
    while (*p) {
        if (strncmp(p, prefix, prefix_len) == 0) {
            return p;
        }
        while (*p && *p != '\n') {
            p++;
        }
        if (*p == '\n') {
            p++;
        }
    }

    return NULL;
}

int parse_proc_stat_cpu_line(const char *text, CpuTimes *out) {
    const char *line = NULL;

    if (!text || !out) {
        return -1;
    }

    line = find_line_prefix(text, "cpu ");
    if (!line) {
        return -1;
    }

    if (sscanf(line,
               "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
               &out->user,
               &out->nice,
               &out->system,
               &out->idle,
               &out->iowait,
               &out->irq,
               &out->softirq,
               &out->steal) != 8) {
        return -1;
    }

    return 0;
}

static int parse_kb_value(const char *line, const char *label, unsigned long long *out) {
    size_t label_len = 0;
    const char *p = NULL;
    unsigned long long value = 0;

    if (!line || !label || !out) {
        return 0;
    }

    label_len = strlen(label);
    if (strncmp(line, label, label_len) != 0) {
        return 0;
    }

    p = line + label_len;
    while (*p && (*p == ':' || isspace((unsigned char)*p))) {
        p++;
    }

    if (sscanf(p, "%llu kB", &value) == 1) {
        *out = value;
        return 1;
    }

    return 0;
}

int parse_proc_meminfo(const char *text, MemInfo *out) {
    const char *p = text;
    int have_total = 0;

    if (!text || !out) {
        return -1;
    }

    memset(out, 0, sizeof(*out));

    while (*p) {
        const char *line_start = p;
        while (*p && *p != '\n') {
            p++;
        }

        if (parse_kb_value(line_start, "MemTotal", &out->mem_total_kb)) {
            have_total = 1;
        } else if (parse_kb_value(line_start, "MemFree", &out->mem_free_kb)) {
            continue;
        } else if (parse_kb_value(line_start, "MemAvailable", &out->mem_available_kb)) {
            continue;
        } else if (parse_kb_value(line_start, "Buffers", &out->buffers_kb)) {
            continue;
        } else if (parse_kb_value(line_start, "Cached", &out->cached_kb)) {
            continue;
        }

        if (*p == '\n') {
            p++;
        }
    }

    return have_total ? 0 : -1;
}

int parse_net_dev_line(const char *line, NetDevStats *out) {
    const char *colon = NULL;
    const char *name_start = NULL;
    const char *name_end = NULL;
    const char *p = NULL;
    unsigned long long rx_bytes = 0;
    unsigned long long tx_bytes = 0;

    if (!line || !out) {
        return -1;
    }

    colon = strchr(line, ':');
    if (!colon) {
        return 0;
    }

    name_start = line;
    while (*name_start && isspace((unsigned char)*name_start)) {
        name_start++;
    }

    name_end = colon;
    while (name_end > name_start && isspace((unsigned char)name_end[-1])) {
        name_end--;
    }

    if (name_end <= name_start || (size_t)(name_end - name_start) >= sizeof(out->name)) {
        return -1;
    }

    memcpy(out->name, name_start, (size_t)(name_end - name_start));
    out->name[name_end - name_start] = '\0';

    p = colon + 1;
    for (int field = 0; field < 16; field++) {
        char *endptr = NULL;
        unsigned long long value = 0;

        while (*p && isspace((unsigned char)*p)) {
            p++;
        }

        if (!*p) {
            return -1;
        }

        value = strtoull(p, &endptr, 10);
        if (endptr == p) {
            return -1;
        }

        if (field == 0) {
            rx_bytes = value;
        } else if (field == 8) {
            tx_bytes = value;
        }

        p = endptr;
    }

    out->rx_bytes = rx_bytes;
    out->tx_bytes = tx_bytes;
    return 1;
}

double cpu_usage_percent(const CpuTimes *prev, const CpuTimes *curr) {
    unsigned long long prev_idle = 0;
    unsigned long long curr_idle = 0;
    unsigned long long prev_total = 0;
    unsigned long long curr_total = 0;
    unsigned long long total_delta = 0;
    unsigned long long idle_delta = 0;

    if (!prev || !curr) {
        return 0.0;
    }

    prev_idle = prev->idle + prev->iowait;
    curr_idle = curr->idle + curr->iowait;

    prev_total = prev->user + prev->nice + prev->system + prev->idle +
                 prev->iowait + prev->irq + prev->softirq + prev->steal;
    curr_total = curr->user + curr->nice + curr->system + curr->idle +
                 curr->iowait + curr->irq + curr->softirq + curr->steal;

    total_delta = curr_total - prev_total;
    idle_delta = curr_idle - prev_idle;

    if (total_delta == 0) {
        return 0.0;
    }

    return ((double)(total_delta - idle_delta) * 100.0) / (double)total_delta;
}
