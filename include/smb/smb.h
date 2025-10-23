#ifndef SMB_H
#define SMB_H

#include <time.h>

static inline long smb_timestamp_ms();
static inline long smb_timespec_to_timestamp_ms(struct timespec *time);
static inline unsigned long smb_timespec_to_timestamp_ns(struct timespec *time);
static inline void smb_abs_timeout(struct timespec *restrict time, long offset_ms);
static inline struct timespec smb_time_now();

static const int SMB_DEFAULT_RUNS_COUNT = 1000;

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

// static inline void smb_print_result(const smb_benchmark_result *restrict result);
static inline unsigned long smb_average_duration_calc_ns(const smb_benchmark_result *result);

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


#define SMB_BENCHMARK_END                                                             \
    clock_gettime(CLOCK_REALTIME, &time_after);                                       \
    result.timestamps[i] = time_before;                                               \
    result.timestamps[i + 1] = time_after;                                            \
    }                                                                                 \
    if (options.result_storage != NULL) {                                             \
        *options.result_storage = &result;                                            \
        continue;                                                                     \
    }                                                                                 \
    printf("average execution time %ld ns\n", smb_average_duration_calc_ns(&result)); \
    free(result.timestamps);                                                          \
    }                                                                                 \
    while (0)

#ifdef SMB_IMPL

static const long msins = 1000;
static const long nsinms = 1000000;
static const long nsinsec = 1000000000;

static inline unsigned long smb_average_duration_calc_ns(const smb_benchmark_result *result)
{
    unsigned long sum = 0;
    for (register size_t i = 0; i < result->runs_count; i++) {
        sum += smb_timespec_to_timestamp_ns(&result->timestamps[i + 1]) - smb_timespec_to_timestamp_ns(&result->timestamps[i]);
    }
    return sum / (unsigned long)result->runs_count;
}

static inline long smb_timespec_to_timestamp_ms(struct timespec *time)
{
    return time->tv_sec * msins + time->tv_nsec / nsinms;
}

static inline unsigned long smb_timespec_to_timestamp_ns(struct timespec *time)
{
    return time->tv_sec * nsinsec + time->tv_nsec;
}

static inline long smb_timestamp_ms()
{
    struct timespec time = smb_time_now();
    return smb_timespec_to_timestamp_ms(&time);
}

static inline void smb_abs_timeout(struct timespec *restrict time, long offset_ms)
{
    time->tv_nsec += offset_ms * nsinms;
    time->tv_sec += time->tv_nsec / nsinsec;
    time->tv_nsec -= (time->tv_nsec / nsinsec) * nsinsec;
}

static inline struct timespec smb_time_now()
{
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    return time;
}

#endif// SMB_IMPL
#endif// SMB_H
