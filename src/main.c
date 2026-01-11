#include "controller.h"
#include "proc_parser.h"

#include <stdio.h>
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

int main(void) {
    CpuTimes prev = {0};
    CpuTimes curr = {0};
    MemInfo mem = {0};
    double usage = 0.0;

    if (read_cpu_times(&prev) != 0) {
        fprintf(stderr, "failed to read /proc/stat\n");
        return 1;
    }

    for (;;) {
        usleep(250000);

        if (read_cpu_times(&curr) != 0) {
            fprintf(stderr, "failed to read /proc/stat\n");
            return 1;
        }

        usage = cpu_usage_percent(&prev, &curr);

        printf("\033[2J\033[H");
        printf("CPU usage: %.2f%%\n", usage);

        if (read_mem_info(&mem) == 0) {
            printf("MemTotal: %llu kB\n", mem.mem_total_kb);
            printf("MemFree: %llu kB\n", mem.mem_free_kb);
            printf("MemAvailable: %llu kB\n", mem.mem_available_kb);
            printf("Buffers: %llu kB\n", mem.buffers_kb);
            printf("Cached: %llu kB\n", mem.cached_kb);
        } else {
            fprintf(stderr, "failed to read /proc/meminfo\n");
        }

        fflush(stdout);
        prev = curr;
    }
}
