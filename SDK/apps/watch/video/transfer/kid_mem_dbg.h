#ifndef KID_MEM_DBG_H
#define KID_MEM_DBG_H

/**
 * @brief  OOM triage helper for the KWT video/audio/transfer pipeline.
 *
 *         KWT_MEM_STATS(label) prints a labeled marker through the caller's
 *         log_info (so it carries that file's LOG_TAG / LOG_TAG_CONST) and then
 *         dumps the runtime heap via the SDK's mem_stats().  The heap is a
 *         single ~871KB SRAM arena (MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN), so
 *         watching the "free" figure at each marker shows where the heap runs
 *         out during a connect/disconnect/reconnect cycle.
 *
 *         mem_stats() is declared in system/malloc.h but is internalized by the
 *         SDK LTO build; the local extern matches the existing call site in
 *         kid_wt_video.c.  The macro expands to nothing when KWT_MEM_DEBUG=0
 *         (define it to 0 for a release build).
 */

extern void mem_stats(void);

#ifndef KWT_MEM_DEBUG
#define KWT_MEM_DEBUG   1
#endif

#if KWT_MEM_DEBUG
#define KWT_MEM_STATS(label) \
    do { \
        log_info("[MEM] %s\n", (label)); \
        mem_stats(); \
    } while (0)
#else
#define KWT_MEM_STATS(label)  do { } while (0)
#endif

#endif /* KID_MEM_DBG_H */
