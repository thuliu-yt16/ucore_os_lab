#ifndef __KERN_MM_BUDDY_PMM_H__
#define  __KERN_MM_BUDDY_PMM_H__

#include <pmm.h>

extern const struct pmm_manager buddy_pmm_manager;
struct buddy{
    uint32_t size;
    uint32_t longest[1];
};
typedef struct buddy buddy_t;

#endif /* ! __KERN_MM_BUDDY_PMM_H__ */
