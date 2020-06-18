/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init(void) {
	vm_anon_init();
	vm_file_init();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init();
#endif
	register_inspect_intr();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
	page_get_type(struct page* page) {
	int ty = VM_TYPE(page->operations->type);
	switch (ty) {
	case VM_UNINIT:
		return VM_TYPE(page->uninit.type);
	default:
		return ty;
	}
}

/* Helpers */
static struct frame* vm_get_victim(void);
static bool vm_do_claim_page(struct page* page);
static struct frame* vm_evict_frame(void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer(enum vm_type type, void* upage, bool writable,
	vm_initializer* init, void* aux) {

	ASSERT(VM_TYPE(type) != VM_UNINIT)
		struct supplemental_page_table* spt = &thread_current()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page(spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */

		 /* TODO: Insert the page into the spt. */

		struct page *pg;
		pg = malloc(sizeof(struct page));

		pg->va = upage;
		pg->writable = writable;

		if (type == VM_ANON) {
			uninit_new(pg, upage, init, type, aux, anon_initializer);
		}
		else if (type == VM_FILE) {
			uninit_new(pg, upage, init, type, aux, file_map_initializer);
		}
		//spt_insert_page(spt, &pg);
		//return true;
		if (vm_do_claim_page(pg)) return true;
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page*
	spt_find_page(struct supplemental_page_table* spt, void* va) {
	/* TODO: Fill this function. */

	struct list_elem *e;

	for (e = list_begin(&spt->page_list); e != list_end(&spt->page_list); e = e->next) {
		struct page* pg;
		pg = list_entry(e, struct page, pg_e);
		if (pg->va == pg_round_down(va)) {
			return pg;
		}
	}
	return NULL;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page(struct supplemental_page_table* spt,
	struct page* page) {
	//printf("insert!\n");
	/* TODO: Fill this function. */
	struct page* pg = spt_find_page(spt, page->va);
	if (pg == NULL) {
		list_push_back(&spt->page_list, &page->pg_e);
		//printf("inserted pg addr : %p\n", page->va);
		return true;
	}
	return false;
}

void
spt_remove_page(struct supplemental_page_table* spt, struct page* page) {
	vm_dealloc_page(page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame*
vm_get_victim(void) {
	struct frame* victim = NULL;
	/* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame*
vm_evict_frame(void) {
	struct frame* victim UNUSED = vm_get_victim();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
struct frame*
vm_get_frame(void) {

	struct frame* frame = NULL;
	/* TODO: Fill this function. */
	
	frame = malloc(sizeof(struct frame));
	uint64_t kpage = palloc_get_page(PAL_USER);
	frame->kva = kpage;
	frame->page = NULL;
	//printf("in vm_get_frame frame kva : %p\n", frame->kva);

	ASSERT(frame != NULL);
	ASSERT(frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth(void* addr) {
	struct page *pg;
	pg = malloc(sizeof(struct page));
	pg->va = pg_round_down(addr);
	pg->writable = true;
	struct frame* frame = vm_get_frame();
	void *kpage = frame->kva;
	frame->page = pg;
	pg->frame = frame;
	struct supplemental_page_table* spt = &thread_current()->spt;
	spt_insert_page(spt, pg);
	
	pml4_set_page(thread_current()->pml4, pg->va, kpage, pg->writable);
	//install_page(pg->va, kpage, pg->writable);
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp(struct page* page UNUSED) {
}


bool
vm_try_handle_fault(struct intr_frame* f, void* addr,
	bool user UNUSED, bool write UNUSED, bool not_present) {
	struct supplemental_page_table* spt = &thread_current()->spt;
	struct page* page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	
	if(!not_present) exit(-1);
	page = spt_find_page(spt, addr);

	if(page == NULL){
		if(!(is_user_vaddr(addr) && (uint64_t)f->rsp - (uint64_t)addr <= 64)) exit(-1);
		vm_stack_growth(addr);
		return true;
	}
	
	return vm_do_claim_page(page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page(struct page* page) {
	//destroy(page);
	free(page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page(void* va) {
	struct page* page = NULL;
	/* TODO: Fill this function */
	page = spt_find_page(&thread_current()->spt, pg_round_down(va));
	if(page == NULL){
		NOT_REACHED();
	}
	bool t = vm_do_claim_page(page);

	return t;
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page(struct page* page) {
	struct frame* frame = vm_get_frame();
	
	/* Set links */
	frame->page = page;
	page->frame = frame;
	
	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	struct supplemental_page_table* spt = &thread_current()->spt;
	spt_insert_page(spt, page);
	bool a = swap_in(page, frame->kva);
	return a;
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init(struct supplemental_page_table* spt) {
	//printf("init!\n");
	//list_init(&spt->page_list);
	spt->i = 0;
}

static bool writable_update(uint64_t* pte, void* va, void* aux){
	struct thread* parent = (struct thread*) aux;
	struct supplemental_page_table* spt = &parent->spt;
	int index = spt->i;
	int inner_index = 0;
	struct list_elem *e;
	//printf("target : %d\n", index);
	//printf("size : %d\n", list_size(&spt->page_list));
	for (e = list_begin(&spt->page_list); e != list_end(&spt->page_list); e = e->next) {
		//printf("current : %d\n", inner_index);
		struct page* pg;
		pg = list_entry(e, struct page, pg_e);
		if(inner_index == index){
			pg->writable = is_writable(pte);
			//printf("writable : %d\n", pg->writable);
			spt->i = index + 1;
			//printf("found!\n");
			return true;
		}
		inner_index++;
	}
	//printf("Not found..\n");
	return false;
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy(struct thread* cur, struct thread* parent) {
	struct supplemental_page_table* src = &parent->spt;
	struct supplemental_page_table* dst = &cur->spt;
	bool result = true;
	struct list_elem *e;
	//printf("parent size : %d\n", list_size(&src->page_list));
	int i = 1;
	//if(!pml4_for_each(parent->pml4, writable_update, parent)) PANIC("writable failed");
	pml4_for_each(parent->pml4, writable_update, parent);
	dst->i = 0;
	
	for (e = list_begin(&src->page_list); e != list_end(&src->page_list); e = e->next) {
		struct page* pg;
		pg = list_entry(e, struct page, pg_e);
		void* parent_page = pg->frame->kva;
		//printf("\n%dth iteration\n", i);
		//printf("parent_kva : %p\n", parent_page);
		struct page *pg2;
		pg2 = malloc(sizeof(struct page));
		
		//pg2->writable = pg->writable;
		//uninit_new(pg2, pg->va, pg->uninit.init, pg->uninit.type, pg->uninit.aux, pg->uninit.page_initializer);
		//uint64_t error_page = palloc_get_page(PAL_USER);
		//printf("repeated_kva : %p\n", error_page);
		
		memcpy(pg2, pg, sizeof(struct page));
		
		//vm_do_claim_page(pg2);
		struct frame* frame = vm_get_frame();
		frame->page = pg2;
		pg2->frame = frame;
		
		void* child_page = pg2->frame->kva;
		//printf("child_kva : %p\n", child_page);
		memcpy(child_page, parent_page, PGSIZE);
		ASSERT(pg->va == pg2->va);
		ASSERT(pg->writable == pg2->writable);
		ASSERT(memcmp(child_page, parent_page, PGSIZE)==0);
		//printf("child_kva : %p\n", child_page);
		
		spt_insert_page(dst, pg2);
		//printf("%p %p %p %d\n", thread_current()->pml4, pg2->va, child_page, pg2->writable);
		pml4_set_page(thread_current()->pml4, pg2->va, child_page, pg2->writable);
		//printf("%dth iteration done\n", i++);
	}
	return result;
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill(struct supplemental_page_table* spt) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	 //if(spt->page_list.head.next = NULL) return;
	 struct list_elem *e;
	 while(!list_empty(&spt->page_list)){
	 	struct page *pg = list_entry (list_pop_front (&spt->page_list), struct page, pg_e);
	 	vm_dealloc_page(pg);
	 }
}




















