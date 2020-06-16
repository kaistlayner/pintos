/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "threads/vaddr.h"

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
	/* TODO: Fill this function. */
	struct page* pg = spt_find_page(spt, page->va);
	if (pg == NULL) {
		list_push_back(&spt->page_list, &page->pg_e);
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
vm_stack_growth(void* addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp(struct page* page UNUSED) {
}

/*
bool
vm_try_handle_fault(struct intr_frame* f, void* addr,
	bool user, bool write, bool not_present) {

	struct supplemental_page_table* spt = &thread_current()->spt;
	//printf("%x\n", addr);
	//struct page *page = NULL;
	//page = spt_find_page(spt, addr);
	//page->va = addr;
	//if(!write) PANIC("READ ONLY");
	if (!not_present) PANIC("ALREADY PRESENT");

	//return vm_do_claim_page (page);
	if (!vm_claim_page(addr)) NOT_REACHED();
	//do_iret(f);
	PANIC("COME?");
	return true;
}*/
/* Return true on success */

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
		PANIC("TO DO");
		vm_stack_growth(addr);
		return;
	}
	
	return vm_do_claim_page(page);
}

/* Return true on success */
/* TODO: Validate the fault */
/* TODO: Your code goes here */
/*
bool
vm_try_handle_fault (struct intr_frame *f, void *addr,
		bool user UNUSED, bool write, bool not_present UNUSED) {


	if(!vm_claim_page(addr)){
		return false;
	}
	return true;
	struct page *page = NULL;
	page->va = addr;
	return vm_do_claim_page (page);

	//return vm_claim_page(addr);

	//struct supplemental_page_table *spt = &thread_current ()->spt;
	//struct page *page = spt_find_page(spt, addr);
	//page->writable = write;

	//do_iret(f);

}*/

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page(struct page* page) {
	destroy(page);
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
	list_init(&spt->page_list);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy(struct supplemental_page_table* dst,
	struct supplemental_page_table* src) {
	
	
	bool result = true;
	struct list_elem *e;
	printf("parent size : %d\n", list_size(&src->page_list));
	int i = 1;
	for (e = list_begin(&src->page_list); e != list_end(&src->page_list); e = e->next) {
		struct page* pg;
		pg = list_entry(e, struct page, pg_e);
		void* parent_page = pg->frame->kva;
		printf("\n%dth iteration\n", i);
		printf("parent_kva : %p\n", parent_page);
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
		printf("child_kva : %p\n", child_page);
		memcpy(child_page, parent_page, PGSIZE);
		//printf("child_kva : %p\n", child_page);
		
		pml4_set_page(thread_current()->pml4, pg2->va, child_page, pg2->writable);
		printf("%dth iteration done\n", i++);
	}
	return result;
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill(struct supplemental_page_table* spt) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	 /*
	 struct list_elem *e;
	 while(!list_empty(&spt->page_list)){
	 	struct page *pg = list_entry (list_pop_front (&spt->page_list), struct page, pg_e);
	 	void *kpage = pg->frame->kva;
	 	palloc_free_page(kpage);
	 	vm_dealloc_page(pg);
	 }*/
}




















