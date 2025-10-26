#ifndef SMB_H
#define SMB_H

#include <time.h>

typedef struct
{
    struct timespec *timestamps;
    const char *group;
    const char *description;
    size_t runs_count;
} smb_benchmark_result;

typedef struct
{
    smb_benchmark_result **result_storage;
    size_t total_runs;
} smb_benchmark_options;

typedef struct
{
    size_t runs_count;
    double average;
    double std_dev;
    double median;
    double min;
    double max;
} smb_benchmark_report;

static const int SMB_DEFAULT_RUNS_COUNT = 1000;

static inline unsigned long smb_timespec_to_timestamp_ns(struct timespec *time);

static inline void smb_print_report(const smb_benchmark_report *restrict report);
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
        if (options.total_runs == 0) {                                                                                    \
            options.total_runs = SMB_DEFAULT_RUNS_COUNT;                                                                  \
        }                                                                                                                 \
        result.runs_count = options.total_runs;                                                                           \
        result.timestamps = malloc(sizeof(*result.timestamps) * options.total_runs * 2);                                  \
        for (register size_t i = 0; i < options.total_runs; i++) {                                                        \
            struct timespec time_before;                                                                                  \
            struct timespec time_after;                                                                                   \
            clock_gettime(CLOCK_REALTIME, &time_before);


#define SMB_BENCHMARK_END                                   \
    clock_gettime(CLOCK_REALTIME, &time_after);             \
    result.timestamps[i * 2] = time_before;                 \
    result.timestamps[i * 2 + 1] = time_after;              \
    }                                                       \
    if (options.result_storage != NULL) {                   \
        *options.result_storage = &result;                  \
        continue;                                           \
    }                                                       \
    smb_benchmark_report report = smb_eval_report(&result); \
    smb_print_report(&report);                              \
    free(result.timestamps);                                \
    }                                                       \
    while (0)

#ifdef SMB_IMPL

#include <math.h>
#include <stdlib.h>

static const long msins = 1000;
static const long nsinms = 1000000;
static const long nsinsec = 1000000000;

static inline void smb_print_report(const smb_benchmark_report *restrict report)
{
    printf("Mean    %40.1f\n", report->average);
    printf("Min     %40.1f\n", report->min);
    printf("Max     %40.1f\n", report->max);
    printf("Median  %40.1f\n", report->median);
    printf("std dev %40.1f\n", report->std_dev);
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
        .runs_count = result->runs_count, .average = mean, .std_dev = std_dev, .median = median, .min = min, .max = max
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
    return time->tv_sec * nsinsec + time->tv_nsec;
}


#endif// SMB_IMPL
#endif// SMB_H
