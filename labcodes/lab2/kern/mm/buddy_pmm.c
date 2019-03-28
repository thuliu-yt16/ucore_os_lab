#include <pmm.h>
#include <list.h>
#include <string.h>
#include <buddy_pmm.h>
buddy_t* self;
struct Page* buddy_base;

#define lc(a) (((a) << 1) + 1)
#define rc(a) (((a) + 1) << 1)
#define fa(a) (((a) - 1) >> 1)
#define off2page(i) (buddy_base + (i))
#define max(a, b) ((a) > (b) ? (a) : (b)) 
uint32_t buddy_nr_free_page;

static void
buddy_init(void) {
    buddy_nr_free_page = 0;
}

static bool is_power_of_2(uint32_t n){
    return (n & (n - 1)) == 0;
}

static uint32_t smallest_power_of_2(uint32_t n){
    n -= 1;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}

static uint32_t largest_power_of_2(uint32_t n){
    return is_power_of_2(n) ? n : (smallest_power_of_2(n) >> 1);
}

static void
buddy_init_memmap(struct Page *base, size_t n) {
    assert(n > 0);
    // divide first page
    struct Page* p = base;
    for (; p != base + n; p ++) {
        assert(PageReserved(p));
        p -> flags = p -> property = 0;
        set_page_ref(p, 0);
    }
    cprintf("start addr: %x\n", page2pa(base));
    cprintf("end addr: %x\n", page2pa(base + n));
    cprintf("init size: %x\n", n);
    uint32_t page_size_for_buddy = sizeof(uint32_t) * (2 * n + 1) / PGSIZE + 1;
    uint32_t usable_page_size = n - page_size_for_buddy;
    cprintf("usable size: %x\n", usable_page_size);
    uint32_t size = largest_power_of_2(usable_page_size);
    cprintf("alloc size: %x\n", size);

    buddy_nr_free_page = size;

    self = (buddy_t*)KADDR(page2pa(base));
    buddy_base = base + page_size_for_buddy;
    cprintf("addr: %x\n", page2pa(base + size));
    cprintf("size: %d\n", size);
    cprintf("self: %x\n", self);
    self -> size = size;
    buddy_base -> property = size;
    SetPageProperty(buddy_base);

    uint32_t node_size = size * 2, i;
    for (i = 0; i < 2 * size - 1; i ++){
        if(is_power_of_2(i + 1))
            node_size /= 2;
        self -> longest[i] = node_size;
    }
}

static struct Page *
buddy_alloc_pages(size_t n)
{
    assert(n > 0);
    if(self == NULL) return NULL;
    if(n > self -> longest[0]) return NULL;
    uint32_t size = smallest_power_of_2(n);
    uint32_t index = 0, node_size, offset;
    for(node_size = self -> size; node_size != size; node_size >>= 1){
        if(self -> longest[lc(index)] >= size){
            if(self -> longest[rc(index)] > self -> longest[lc(index)]){
                index = rc(index);
            }
            else{
                index = lc(index);
            }
        }
        else index = rc(index);
    }
    self -> longest[index] = 0;
    offset = (index + 1) * node_size - self -> size;
    struct Page* page = buddy_base + offset;
    while(index){
        index = fa(index);
        self -> longest[index] = max(self -> longest[lc(index)], self -> longest[rc(index)]);
    }
    page -> property = node_size;
    SetPageProperty(page + node_size);
    ClearPageProperty(page);
    return page;
}

static void
buddy_free_pages(struct Page *base, size_t n)
{
    assert(n > 0);
    assert(is_power_of_2(n));
    struct Page *p = base;
    for (; p != base + n; p++)
    {
        // assert(!PageReserved(p) && !PageProperty(p));
        p -> flags = 0;
        set_page_ref(p, 0);
    }
    cprintf("0000\n");
    base -> property = n;
    SetPageProperty(base);

    uint32_t node_size = n, offset = base - buddy_base;
    uint32_t index = (offset + self -> size) / node_size - 1;
    cprintf("index: %d\n", index);
    cprintf("1111\n");
    self -> longest[index] = node_size;
    cprintf("2222\n");
    while(index){
        cprintf("!!!!\n");
        index = fa(index);
        node_size *= 2;
        uint32_t left_longest = self->longest[lc(index)];
        uint32_t right_longest = self->longest[rc(index)];
        cprintf("!!!!\n");

        if (left_longest + right_longest == node_size){
            self->longest[index] = node_size;
            uint32_t left_offset = (lc(index) + 1) * left_longest - self -> size;
            uint32_t right_offset = left_offset + left_longest;
            ClearPageProperty(buddy_base + right_offset);
            (buddy_base + left_offset) -> property = node_size;
        }
        else
            self->longest[index] = max(left_longest, right_longest);
    }
    cprintf("end free\n");
}

static size_t 
buddy_nr_free_pages(void) 
{ 
    return buddy_nr_free_page;
}

// static void
// basic_check(void)
// {
//     struct Page *p0, *p1, *p2;
//     p0 = p1 = p2 = NULL;
//     assert((p0 = alloc_page()) != NULL);
//     assert((p1 = alloc_page()) != NULL);
//     assert((p2 = alloc_page()) != NULL);

//     assert(p0 != p1 && p0 != p2 && p1 != p2);
//     assert(page_ref(p0) == 0 && page_ref(p1) == 0 && page_ref(p2) == 0);

//     assert(page2pa(p0) < npage * PGSIZE);
//     assert(page2pa(p1) < npage * PGSIZE);
//     assert(page2pa(p2) < npage * PGSIZE);

//     list_entry_t free_list_store = free_list;
//     list_init(&free_list);
//     assert(list_empty(&free_list));

//     unsigned int nr_free_store = nr_free;
//     nr_free = 0;

//     assert(alloc_page() == NULL);

//     free_page(p0);
//     free_page(p1);
//     free_page(p2);
//     assert(nr_free == 3);

//     assert((p0 = alloc_page()) != NULL);
//     assert((p1 = alloc_page()) != NULL);
//     assert((p2 = alloc_page()) != NULL);

//     assert(alloc_page() == NULL);

//     free_page(p0);
//     assert(!list_empty(&free_list));

//     struct Page *p;
//     assert((p = alloc_page()) == p0);
//     assert(alloc_page() == NULL);

//     assert(nr_free == 0);
//     free_list = free_list_store;
//     nr_free = nr_free_store;

//     free_page(p);
//     free_page(p1);
//     free_page(p2);
// }

static void
buddy_check(void)
{
    struct Page *p0 = alloc_pages(8192);
    struct Page *p1 = alloc_pages(2048);
    struct Page *p2 = alloc_pages(2048);
    struct Page *p3 = alloc_pages(256);
    struct Page *p4 = alloc_pages(2048);
    struct Page *p5 = alloc_pages(256);
    assert(alloc_pages(1532) == NULL);
    assert(alloc_pages(1077) == NULL);
    struct Page *p6 = alloc_pages(512);
    struct Page *p7 = alloc_pages(1024);
    free_pages(p5, 256);
    struct Page *p9 = alloc_pages(256);
    free_pages(p7, 1024);
    cprintf("check buddy system success!\n");
    assert(p2 == p1 + (2048));
    assert(p6 == p1 + (4608));
    assert(p9 == p1 + (4352));
    assert(p4 == p1 + (6144));
    assert(p3 == p1 + (4096));
    assert(p0 == p1 + (-8192));
    assert(nr_free_pages() == 1024);
    struct Page *p11 = alloc_pages(512);
    struct Page *p12 = alloc_pages(256);
    struct Page *p13 = alloc_pages(64);
    struct Page *p14 = alloc_pages(64);
    struct Page *p15 = alloc_pages(64);
    struct Page *p16 = alloc_pages(32);
    struct Page *p17 = alloc_pages(16);
    struct Page *p18 = alloc_pages(2);
    assert(alloc_pages(13) == NULL);
    assert(alloc_pages(12) == NULL);
    struct Page *p19 = alloc_pages(4);
    free_pages(p1, 2048);
    free_pages(p2, 2048);
    free_pages(p6, 512);
    free_pages(p9, 256);
    free_pages(p4, 2048);
    free_pages(p3, 256);
    free_pages(p0, 8192);
    free_pages(p11, 512);
    free_pages(p12, 256);
    free_pages(p13, 64);
    free_pages(p14, 64);
    free_pages(p15, 64);
    free_pages(p16, 32);
    free_pages(p17, 16);
    free_pages(p18, 2);
    free_pages(p19, 4);
    assert(nr_free_pages() == 16384);
}

const struct pmm_manager buddy_pmm_manager = {
    .name = "buddy_pmm_manager",
    .init = buddy_init,
    .init_memmap = buddy_init_memmap,
    .alloc_pages = buddy_alloc_pages,
    .free_pages = buddy_free_pages,
    .nr_free_pages = buddy_nr_free_pages,
    .check = buddy_check,
};