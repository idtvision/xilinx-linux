
/*
 * xsm.h -- definitions for the networked camera module
 *
 */

/*
 * Macros to help debugging
 */

#undef PDEBUG             /* undef it, just in case */
#ifdef XSM_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk("xsm: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#  error
#endif

#undef PDEBUGG
#define PDEBUGG(fmt, args...) /* nothing: it's a placeholder */


/* These are the flags in the statusword */
#define XSM_RX_INTR 0x0001
#define XSM_TX_INTR 0x0002

/* Default timeout period */
#define XSM_TIMEOUT 5   /* In jiffies */
