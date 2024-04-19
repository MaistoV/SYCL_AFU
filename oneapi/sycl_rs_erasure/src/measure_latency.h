#ifndef __MEASURE_LATENCY_H__
#define __MEASURE_LATENCY_H__

// TODO: export this
// Start measure with chrono
#define MEASURE_LATENCY_START(start)			start = std::chrono::steady_clock::now();
// End measure, return double
#define MEASURE_LATENCY_END(start, time_sec) 	end = std::chrono::steady_clock::now(); \
												time_sec = std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();
// Print time_sec on fd_latency
#define MEASURE_LATENCY_FPRINTF(fd_latency, time_sec) fprintf(fd_latency, "%0.10f\n", time_sec);
// Combine simpler macros
#define MEASURE_LATENCY_END_AND_PRINT(start, time_sec, fd_latency) \
    MEASURE_LATENCY_END(start,time_sec); \
    MEASURE_LATENCY_FPRINTF(fd_latency, time_sec);

#endif // __MEASURE_LATENCY_H__
