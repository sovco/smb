#ifndef SMB_H
#define SMB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>

typedef struct
{
    double average;
    double std_dev;
    double median;
    double min;
    double max;
} smb_benchmark_report;

typedef struct
{
    struct timespec *timestamps;
    const char *group;
    const char *description;
    size_t runs_count;
    smb_benchmark_report report;
} smb_benchmark_result;

typedef struct
{
    smb_benchmark_result **result_storage;
    size_t total_runs;
    smb_benchmark_report report;
} smb_benchmark_options;


static const size_t SMB_DEFAULT_RUNS_COUNT = 1000;
static const size_t SMB_MINIMAL_RUNS_COUNT = 10;

static inline unsigned long smb_timespec_to_timestamp_ns(struct timespec *time);

static inline void smb_print_report(const smb_benchmark_result *restrict result);
static inline double smb_average_duration_calc_ns(const double *durations, const size_t count);
static inline void smb_durations_calc_ns(const smb_benchmark_result *result, double *ret);
static inline double smb_std_dev_calc_ns(const double *durations, const double mean, const size_t count);
static inline void smb_sort_durations(double *durations, const size_t count);
static inline double smb_determine_median_ns(const double *sorted_durations, const size_t count);
static inline smb_benchmark_report smb_eval_report(const smb_benchmark_result *result);

#define SMB_BENCHMARK_BEGIN(group_id, description_str, ...)                                                               \
    do {                                                                                                                  \
        static smb_benchmark_result result = { .group = #group_id, .description = #description_str, .timestamps = NULL }; \
        smb_benchmark_options options;                                                                                    \
        if (sizeof((char[]){ #__VA_ARGS__ }) == 0) {                                                                      \
            options = (smb_benchmark_options){ 0 };                                                                       \
        } else {                                                                                                          \
            options = (smb_benchmark_options){ __VA_ARGS__ };                                                             \
        }                                                                                                                 \
        if (options.total_runs < SMB_MINIMAL_RUNS_COUNT) {                                                                \
            options.total_runs = SMB_DEFAULT_RUNS_COUNT;                                                                  \
        }                                                                                                                 \
        result.runs_count = options.total_runs;                                                                           \
        result.timestamps = malloc(sizeof(*result.timestamps) * options.total_runs * 2);                                  \
        for (register size_t __i = 0; __i < options.total_runs; __i++) {                                                  \
            struct timespec time_before;                                                                                  \
            struct timespec time_after;                                                                                   \
            clock_gettime(CLOCK_REALTIME, &time_before);


#define SMB_BENCHMARK_END                        \
    clock_gettime(CLOCK_REALTIME, &time_after);  \
    result.timestamps[__i * 2] = time_before;    \
    result.timestamps[__i * 2 + 1] = time_after; \
    }                                            \
    result.report = smb_eval_report(&result);    \
    if (options.result_storage != NULL) {        \
        *options.result_storage = &result;       \
        continue;                                \
    }                                            \
    smb_print_report(&result);                   \
    free(result.timestamps);                     \
    }                                            \
    while (0)

#ifdef __cplusplus
}// extern "C"
#endif

#ifdef SMB_IMPL

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define SMB_RESET_ALL "\x1b[0m"
#define SMB_COLOR_RED "\x1b[31m"
#define SMB_COLOR_GREEN "\x1b[32m"

static const long smb_ns_in_sec = 1000000000;


static inline int __smb_count_digits_d(double val)
{
    double base = 10;
    int count = 0;
    if (val == 0) return 1;
    while (val / base > 0) {
        count++;
        base *= 10;
    }
    return count + 1;
}

static inline int __smb_count_digits_ul(size_t val)
{
    size_t base = 10;
    int count = 0;
    if (val == 0) return 1;
    while (val / base > 0) {
        count++;
        base *= 10;
    }
    return count + 1;
}

static inline void __smb_print_duration_entry(const char *desc, size_t lenght, double value)
{
    static const char *numeric_format = "%.1f";
    const int numeric_len = __smb_count_digits_d(value) + 2;
    char report_entry_contents[lenght + 1];
    char numeric_buf[numeric_len + 1];
    sprintf(&numeric_buf[0], numeric_format, value);
    memset(&report_entry_contents[0], (int)' ', lenght + 1);
    memcpy(&report_entry_contents[0], desc, strlen(desc));
    report_entry_contents[lenght + 1] = '\0';
    report_entry_contents[10] = ' ';
    report_entry_contents[11] = '|';
    report_entry_contents[lenght - 4] = ' ';
    report_entry_contents[lenght - 3] = 'n';
    report_entry_contents[lenght - 2] = 's';
    report_entry_contents[lenght - 1] = ' ';
    report_entry_contents[lenght] = '|';
    memcpy(&report_entry_contents[lenght - 4 - strlen(&numeric_buf[0])], &numeric_buf[0], strlen(&numeric_buf[0]));
    printf(SMB_COLOR_GREEN "%s\n" SMB_RESET_ALL, &report_entry_contents[0]);
}

static inline void __smb_print_count_entry(const char *desc, size_t lenght, size_t count)
{
    static const char *numeric_format = "%.ld";
    const int numeric_len = __smb_count_digits_ul(count) + 2;
    char report_entry_contents[lenght + 1];
    char numeric_buf[numeric_len + 1];
    sprintf(&numeric_buf[0], numeric_format, count);
    memset(&report_entry_contents[0], (int)' ', lenght + 1);
    memcpy(&report_entry_contents[0], desc, strlen(desc));
    report_entry_contents[lenght + 1] = '\0';
    report_entry_contents[10] = ' ';
    report_entry_contents[11] = '|';
    report_entry_contents[lenght - 4] = ' ';
    report_entry_contents[lenght - 3] = ' ';
    report_entry_contents[lenght - 2] = ' ';
    report_entry_contents[lenght - 1] = ' ';
    report_entry_contents[lenght] = '|';
    memcpy(&report_entry_contents[lenght - 4 - strlen(&numeric_buf[0])], &numeric_buf[0], strlen(&numeric_buf[0]));
    printf(SMB_COLOR_GREEN "%s\n" SMB_RESET_ALL, &report_entry_contents[0]);
}

static inline void smb_print_report(const smb_benchmark_result *restrict result)
{
    static const size_t report_minimal_width = 32;
    const size_t title_width = strlen(result->group) + strlen(result->description) + 1 /* the point separator */;
    const size_t report_width = report_minimal_width > title_width ? report_minimal_width : title_width;
    char spacer[report_width + 1];
    memset(&spacer[0], '-', report_width + 1);
    spacer[report_width + 1] = '\0';
    puts(SMB_COLOR_GREEN "SMB Benchmark report:" SMB_RESET_ALL);
    printf(SMB_COLOR_GREEN "%s\n" SMB_RESET_ALL, &spacer[0]);
    printf(SMB_COLOR_GREEN "%s.%s\n\n" SMB_RESET_ALL, result->group, result->description);
    __smb_print_count_entry("Runs Count", report_width, result->runs_count);
    __smb_print_duration_entry("Mean", report_width, result->report.average);
    __smb_print_duration_entry("Min", report_width, result->report.min);
    __smb_print_duration_entry("Max", report_width, result->report.max);
    __smb_print_duration_entry("Median", report_width, result->report.median);
    __smb_print_duration_entry("std dev", report_width, result->report.std_dev);
}

static inline smb_benchmark_report smb_eval_report(const smb_benchmark_result *result)
{
    double mean = 0;
    double std_dev = 0;
    double min = 0;
    double max = 0;
    double median = 0;
    double *durations = malloc(sizeof(*durations) * result->runs_count);

    smb_durations_calc_ns(result, durations);
    mean = smb_average_duration_calc_ns(durations, result->runs_count);
    std_dev = smb_std_dev_calc_ns(durations, mean, result->runs_count);
    smb_sort_durations(durations, result->runs_count);
    min = durations[0];
    max = durations[result->runs_count - 1];
    median = smb_determine_median_ns(durations, result->runs_count);
    free(durations);
    return (smb_benchmark_report){
        .average = mean, .std_dev = std_dev, .median = median, .min = min, .max = max
    };
}

static inline void smb_durations_calc_ns(const smb_benchmark_result *result, double *ret)
{
    for (register size_t i = 0; i < result->runs_count; i++) {
        ret[i] = (double)smb_timespec_to_timestamp_ns(&result->timestamps[i * 2 + 1]) - smb_timespec_to_timestamp_ns(&result->timestamps[i * 2]);
    }
}

static inline double smb_average_duration_calc_ns(const double *durations, const size_t count)
{
    double sum = 0;
    for (register size_t i = 0; i < count; i++) {
        sum += durations[i];
    }
    return sum / (double)count;
}

static inline double smb_std_dev_calc_ns(const double *durations, const double mean, const size_t count)
{
    double sum = 0;
    for (register size_t i = 0; i < count; i++) {
        sum += pow(durations[i] - mean, 2);
    }
    return sqrt(sum / (double)count);
}

static inline int __smb_sort_compare(const void *a, const void *b)
{
    double arg1 = *(const double *)a;
    double arg2 = *(const double *)b;

    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

static inline void smb_sort_durations(double *durations, const size_t count)
{
    qsort(durations, count, sizeof(*durations), __smb_sort_compare);
}

static inline double smb_determine_median_ns(const double *sorted_durations, const size_t count)
{
    if (count % 2 > 0) {
        return sorted_durations[((count - 1) / 2)];
    }
    return (sorted_durations[(count / 2) - 1] + sorted_durations[(count / 2)]) / 2;
}

static inline unsigned long smb_timespec_to_timestamp_ns(struct timespec *time)
{
    return time->tv_sec * smb_ns_in_sec + time->tv_nsec;
}

#ifdef __cplusplus
}// extern "C"
#endif

#endif// SMB_IMPL
#endif// SMB_H
