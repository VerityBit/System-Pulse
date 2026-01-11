#include "controller.h"
#include "proc_parser.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

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

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [--format=json]\n", prog);
}

int main(int argc, char **argv) {
    CpuTimes prev = {0};
    CpuTimes curr = {0};
    MemInfo mem = {0};
    double usage = 0.0;
    int have_prev = 0;
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

    for (;;) {
        usleep(250000);

        if (read_cpu_times(&curr) != 0) {
            fprintf(stderr, "Warning: failed to read /proc/stat; skipping sample\n");
            continue;
        }

        if (!have_prev) {
            prev = curr;
            have_prev = 1;
            continue;
        }

        usage = cpu_usage_percent(&prev, &curr);

        if (output_json) {
            int mem_ok = (read_mem_info(&mem) == 0);
            printf("{\"cpu_usage_percent\":%.2f,", usage);
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
    }
}
