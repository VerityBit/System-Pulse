#include "controller.h"
#include "proc_parser.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct NetTotals {
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
} NetTotals;

static int read_cpu_times(CpuTimes *out) {
    FileBuffer buf = {0};
    int rc = 0;

    rc = read_file_to_buffer("/proc/stat", &buf);
    if (rc != 0) {
        return -1;
    }

    rc = parse_proc_stat_cpu_line(buf.data, out);
    free_file_buffer(&buf);

    return rc;
}

static int read_mem_info(MemInfo *out) {
    FileBuffer buf = {0};
    int rc = 0;

    rc = read_file_to_buffer("/proc/meminfo", &buf);
    if (rc != 0) {
        return -1;
    }

    rc = parse_proc_meminfo(buf.data, out);
    free_file_buffer(&buf);

    return rc;
}

static int read_net_totals(NetTotals *out) {
    FileBuffer buf = {0};
    const char *p = NULL;
    int rc = 0;
    int have_data = 0;

    if (!out) {
        return -1;
    }

    out->rx_bytes = 0;
    out->tx_bytes = 0;

    rc = read_file_to_buffer("/proc/net/dev", &buf);
    if (rc != 0) {
        return -1;
    }

    p = buf.data;
    while (*p) {
        const char *line_start = p;
        size_t line_len = 0;
        char line[512];
        NetDevStats stats = {0};

        while (*p && *p != '\n') {
            p++;
        }
        line_len = (size_t)(p - line_start);

        if (line_len > 0) {
            if (line_len >= sizeof(line)) {
                free_file_buffer(&buf);
                return -1;
            }
            memcpy(line, line_start, line_len);
            line[line_len] = '\0';

            rc = parse_net_dev_line(line, &stats);
            if (rc < 0) {
                free_file_buffer(&buf);
                return -1;
            }
            if (rc > 0) {
                out->rx_bytes += stats.rx_bytes;
                out->tx_bytes += stats.tx_bytes;
                have_data = 1;
            }
        }

        if (*p == '\n') {
            p++;
        }
    }

    free_file_buffer(&buf);
    return have_data ? 0 : -1;
}

static double timespec_seconds(const struct timespec *ts) {
    if (!ts) {
        return 0.0;
    }
    return (double)ts->tv_sec + ((double)ts->tv_nsec / 1000000000.0);
}

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [--format=json]\n", prog);
}

int main(int argc, char **argv) {
    CpuTimes prev = {0};
    CpuTimes curr = {0};
    MemInfo mem = {0};
    NetTotals prev_net = {0};
    NetTotals curr_net = {0};
    double usage = 0.0;
    double rx_bps = 0.0;
    double tx_bps = 0.0;
    struct timespec prev_ts = {0};
    struct timespec curr_ts = {0};
    int have_prev = 0;
    int have_prev_net = 0;
    int output_json = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--format=json") == 0) {
            output_json = 1;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            print_usage(argv[0]);
            return 2;
        }
    }

    if (read_cpu_times(&prev) == 0) {
        have_prev = 1;
    } else {
        fprintf(stderr, "Warning: failed to read /proc/stat; skipping sample\n");
    }

    if (read_net_totals(&prev_net) == 0 && clock_gettime(CLOCK_MONOTONIC, &prev_ts) == 0) {
        have_prev_net = 1;
    } else {
        fprintf(stderr, "Warning: failed to read /proc/net/dev; skipping sample\n");
    }

    for (;;) {
        usleep(250000);

        if (read_cpu_times(&curr) != 0) {
            fprintf(stderr, "Warning: failed to read /proc/stat; skipping sample\n");
            continue;
        }

        rx_bps = 0.0;
        tx_bps = 0.0;
        int net_ok = 0;
        if (read_net_totals(&curr_net) == 0 && clock_gettime(CLOCK_MONOTONIC, &curr_ts) == 0) {
            net_ok = 1;
            if (have_prev_net) {
                double delta = timespec_seconds(&curr_ts) - timespec_seconds(&prev_ts);
                if (delta > 0.0) {
                    rx_bps = (double)(curr_net.rx_bytes - prev_net.rx_bytes) / delta;
                    tx_bps = (double)(curr_net.tx_bytes - prev_net.tx_bytes) / delta;
                }
            }
        } else {
            fprintf(stderr, "Warning: failed to read /proc/net/dev; skipping sample\n");
        }

        if (!have_prev) {
            prev = curr;
            have_prev = 1;
            if (net_ok) {
                prev_net = curr_net;
                prev_ts = curr_ts;
                have_prev_net = 1;
            }
            continue;
        }

        usage = cpu_usage_percent(&prev, &curr);

        if (output_json) {
            int mem_ok = (read_mem_info(&mem) == 0);
            printf("{\"cpu_usage_percent\":%.2f,", usage);
            printf("\"net\":{\"rx_bps\":%.2f,\"tx_bps\":%.2f},", rx_bps, tx_bps);
            if (mem_ok) {
                printf("\"mem\":{\"total_kb\":%llu,\"free_kb\":%llu,\"available_kb\":%llu,\"buffers_kb\":%llu,\"cached_kb\":%llu}}\n",
                       mem.mem_total_kb,
                       mem.mem_free_kb,
                       mem.mem_available_kb,
                       mem.buffers_kb,
                       mem.cached_kb);
            } else {
                printf("\"mem\":null}\n");
                fprintf(stderr, "Warning: failed to read /proc/meminfo; skipping sample\n");
            }
        } else {
            printf("\033[2J\033[H");
            printf("CPU usage: %.2f%%\n", usage);
            printf("Net RX: %.2f Bps\n", rx_bps);
            printf("Net TX: %.2f Bps\n", tx_bps);

            if (read_mem_info(&mem) == 0) {
                printf("MemTotal: %llu kB\n", mem.mem_total_kb);
                printf("MemFree: %llu kB\n", mem.mem_free_kb);
                printf("MemAvailable: %llu kB\n", mem.mem_available_kb);
                printf("Buffers: %llu kB\n", mem.buffers_kb);
                printf("Cached: %llu kB\n", mem.cached_kb);
            } else {
                fprintf(stderr, "Warning: failed to read /proc/meminfo; skipping sample\n");
            }
        }

        fflush(stdout);
        prev = curr;
        if (net_ok) {
            prev_net = curr_net;
            prev_ts = curr_ts;
            have_prev_net = 1;
        }
    }
}
