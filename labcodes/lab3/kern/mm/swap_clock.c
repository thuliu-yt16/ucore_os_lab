#include <defs.h>
#include <x86.h>
#include <stdio.h>
#include <string.h>
#include <swap.h>
#include <swap_clock.h>
#include <list.h>

list_entry_t pra_list_head;
list_entry_t* p;
static int
_clock_init_mm(struct mm_struct *mm)
{
    list_init(&pra_list_head);
    mm->sm_priv = &pra_list_head;
    p = &pra_list_head;
    return 0;
}
/*
 * (3)_fifo_map_swappable: According FIFO PRA, we should link the most recent arrival page at the back of pra_list_head qeueue
 */
static int
_clock_map_swappable(struct mm_struct *mm, uintptr_t addr, struct Page *page, int swap_in)
{
    list_entry_t *head = (list_entry_t *)mm -> sm_priv;
    list_entry_t *entry = &(page -> pra_page_link);

    assert(entry != NULL && head != NULL);
    //record the page access situlation
    /*LAB3 EXERCISE 2: 2016011358*/
    //(1)link the most recent arrival page at the back of the pra_list_head queue.
    // list_add_before(head, entry);
    list_add(p, entry);
    p = list_next(p);
    return 0;
}
/*
 *  (4)_fifo_swap_out_victim: According FIFO PRA, we should unlink the  earliest arrival page in front of pra_list_head qeueue,
 *                            then assign the value of *ptr_page to the addr of this page.
 */
static int
_clock_swap_out_victim(struct mm_struct *mm, struct Page **ptr_page, int in_tick)
{
    list_entry_t *head = (list_entry_t *)mm -> sm_priv;
    assert(head != NULL);
    assert(in_tick == 0);
    assert(!list_empty(head));
    /* Select the victim */
    /*LAB3 EXERCISE 2: 2016011358*/
    //(1)  unlink the  earliest arrival page in front of pra_list_head qeueue
    struct Page* cur_page;
    int loop = 0;
    while(++ loop){
        if(p == head) p = list_next(p);
        cur_page = le2page(p, pra_page_link);
        pte_t* pte = get_pte(mm -> pgdir, cur_page -> pra_vaddr, 0);
        assert(pte != NULL);
        cprintf("loop: %d\n", loop);
        cprintf("addr: 0x%4x\n", cur_page -> pra_vaddr);
        cprintf("pte: 0x%3x\n", *pte & (PTE_D | PTE_A));
        if((*pte & (PTE_D | PTE_A)) == 0){
            break;
        }
        else {
            if((*pte & PTE_D) != 0){
                *pte &= ~PTE_D; // reset change bit
            }
            else{
                *pte &= ~PTE_A;
            }
        }
        p = list_next(p);
    }
    p = list_prev(p);
    list_del(list_next(p));
    //(2)  assign the value of *ptr_page to the addr of this page
    *ptr_page = cur_page;
    return 0;
}

static void clear_da(uintptr_t la){
    pte_t* pte = get_pte(boot_pgdir, la, 0);
    assert(pte != NULL);
    cprintf("clear addr: 0x%4x\n", la);
    cprintf("clear pte: 0x%3x\n", *pte & (PTE_D | PTE_A));
    *pte &= ~ (PTE_D | PTE_A);
}


static void print_da(uintptr_t la){
    pte_t* pte = get_pte(boot_pgdir, la, 0);
    assert(pte != NULL);
    cprintf("print addr: 0x%4x\n", la);
    cprintf("print pte: 0x%3x\n", *pte & (PTE_D | PTE_A));
}

static int
_clock_check_swap(void)
{
    cprintf("boot_pgdir: %x\n", boot_pgdir);
    unsigned char temp;
    cprintf("read Virt Page c in clock_check_swap\n");
    temp = *(unsigned char *)0x3000;
    assert(pgfault_num == 4);
    cprintf("write Virt Page a in clock_check_swap\n");
    *(unsigned char *)0x1000 = 0x0a;
    assert(pgfault_num == 4);
    cprintf("read Virt Page d in clock_check_swap\n");
    temp = *(unsigned char *)0x4000;
    assert(pgfault_num == 4);
    cprintf("write Virt Page b in clock_check_swap\n");
    *(unsigned char *)0x2000 = 0x0b;
    assert(pgfault_num == 4);

    cprintf("read Virt Page e in clock_check_swap\n");
    temp = *(unsigned char *)0x5000;
    assert(pgfault_num == 5);

    print_da(0x1000);
    print_da(0x2000);
    print_da(0x3000);
    print_da(0x4000);

    cprintf("read Virt Page b in clock_check_swap\n");
    temp = *(unsigned char *)0x2000;
    assert(pgfault_num == 5);

    cprintf("write Virt Page a in clock_check_swap\n");
    *(unsigned char *)0x1000 = 0x0a;
    assert(pgfault_num == 5);

    cprintf("read Virt Page b in clock_check_swap\n");
    temp = *(unsigned char *)0x2000;
    assert(pgfault_num == 5);

    cprintf("read Virt Page c in clock_check_swap\n");
    temp = *(unsigned char *)0x3000;
    assert(pgfault_num == 6);

    cprintf("read Virt Page d in clock_check_swap\n");
    temp = *(unsigned char *)0x4000;
    assert(pgfault_num == 7);

    return 0;
}

static int
_clock_init(void)
{
    return 0;
}

static int
_clock_set_unswappable(struct mm_struct *mm, uintptr_t addr)
{
    return 0;
}

static int
_clock_tick_event(struct mm_struct *mm)
{
    return 0;
}

struct swap_manager swap_manager_clock =
    {
        .name = "clock swap manager",
        .init = &_clock_init,
        .init_mm = &_clock_init_mm,
        .tick_event = &_clock_tick_event,
        .map_swappable = &_clock_map_swappable,
        .set_unswappable = &_clock_set_unswappable,
        .swap_out_victim = &_clock_swap_out_victim,
        .check_swap = &_clock_check_swap,
};
