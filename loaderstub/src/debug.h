#ifndef __DEBUG_H
#define __DEBUG_H

#ifdef DEBUG
void debug_init (void);
void dprintf (const char *fmt, ...) __attribute__((format(printf, 1, 2)));
#else
#define debug_init() ((void)0)
#define dprintf(f,...) ((void)0)
#endif

#endif /* __DEBUG_H */
