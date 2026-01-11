#include "proc_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_run = 0;
static int tests_failed = 0;

static void assert_true(int condition, const char *msg) {
    tests_run++;
    if (!condition) {
        tests_failed++;
        fprintf(stderr, "FAIL: %s\n", msg);
    }
}

static void assert_double_close(double actual, double expected, double tol, const char *msg) {
    double diff = actual > expected ? actual - expected : expected - actual;
    tests_run++;
    if (diff > tol) {
        tests_failed++;
        fprintf(stderr, "FAIL: %s (got %.6f, expected %.6f)\n", msg, actual, expected);
    }
}

static void test_parse_proc_stat_cpu_line_valid(void) {
    const char *text =
        "cpu  100 200 300 400 50 60 70 80 90\n"
        "cpu0 1 1 1 1 1 1 1 1\n";
    CpuTimes out = {0};

    assert_true(parse_proc_stat_cpu_line(text, &out) == 0, "parse_proc_stat_cpu_line valid input");
    assert_true(out.user == 100, "cpu user parsed");
    assert_true(out.nice == 200, "cpu nice parsed");
    assert_true(out.system == 300, "cpu system parsed");
    assert_true(out.idle == 400, "cpu idle parsed");
    assert_true(out.iowait == 50, "cpu iowait parsed");
    assert_true(out.irq == 60, "cpu irq parsed");
    assert_true(out.softirq == 70, "cpu softirq parsed");
    assert_true(out.steal == 80, "cpu steal parsed");
}

static void test_parse_proc_stat_cpu_line_missing_prefix(void) {
    const char *text = "cpu0 1 2 3 4 5 6 7 8\n";
    CpuTimes out = {0};

    assert_true(parse_proc_stat_cpu_line(text, &out) != 0, "parse_proc_stat_cpu_line missing cpu line");
}

static void test_parse_proc_meminfo_valid(void) {
    const char *text =
        "MemTotal:       16384256 kB\n"
        "MemFree:        1234 kB\n"
        "MemAvailable:   5678 kB\n"
        "Buffers:        4321 kB\n"
        "Cached:         8765 kB\n";
    MemInfo out = {0};

    assert_true(parse_proc_meminfo(text, &out) == 0, "parse_proc_meminfo valid input");
    assert_true(out.mem_total_kb == 16384256ULL, "mem total parsed");
    assert_true(out.mem_free_kb == 1234ULL, "mem free parsed");
    assert_true(out.mem_available_kb == 5678ULL, "mem available parsed");
    assert_true(out.buffers_kb == 4321ULL, "buffers parsed");
    assert_true(out.cached_kb == 8765ULL, "cached parsed");
}

static void test_parse_proc_meminfo_missing_total(void) {
    const char *text = "MemFree: 123 kB\n";
    MemInfo out = {0};

    assert_true(parse_proc_meminfo(text, &out) != 0, "parse_proc_meminfo missing MemTotal");
}

static void test_cpu_usage_percent_basic(void) {
    CpuTimes prev = {100, 0, 100, 100, 0, 0, 0, 0};
    CpuTimes curr = {200, 0, 200, 200, 0, 0, 0, 0};
    double usage = cpu_usage_percent(&prev, &curr);

    assert_double_close(usage, 66.6667, 0.01, "cpu usage percent basic");
}

static void test_cpu_usage_percent_zero_delta(void) {
    CpuTimes prev = {1, 2, 3, 4, 5, 6, 7, 8};
    CpuTimes curr = prev;
    double usage = cpu_usage_percent(&prev, &curr);

    assert_double_close(usage, 0.0, 0.0001, "cpu usage percent zero delta");
}

int main(void) {
    test_parse_proc_stat_cpu_line_valid();
    test_parse_proc_stat_cpu_line_missing_prefix();
    test_parse_proc_meminfo_valid();
    test_parse_proc_meminfo_missing_total();
    test_cpu_usage_percent_basic();
    test_cpu_usage_percent_zero_delta();

    if (tests_failed != 0) {
        fprintf(stderr, "\n%d/%d tests failed\n", tests_failed, tests_run);
        return 1;
    }

    printf("%d tests passed\n", tests_run);
    return 0;
}
