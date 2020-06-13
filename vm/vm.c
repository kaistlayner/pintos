/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "threads/vaddr.h"

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)
	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		/*
		if (!vm_claim_page (upage)) NOT_REACHED();
		struct page *pg = spt_find_page(spt, upage);
		pg->writable = writable;
		//pg->operations->type = type;
		if (type == VM_ANON){
			uninit_new(pg, upage, init, type, aux, anon_initializer);
			PANIC("HERE2!");
			if(!swap_in(pg, upage)) NOT_REACHED();
		}
		else if (type == VM_FILE) uninit_new(pg, upage, init, type, aux, file_map_initializer);
		//bool success2 = init(pg, type, pg->frame->kva);

		/* TODO: Insert the page into the spt. */
		//if (!spt_insert_page (spt , pg)) NOT_REACHED();)*/
		
		struct page pg;
		
		pg.va = upage;
		pg.writable = writable;
		spt_insert_page(spt, &pg);
		
		if (type == VM_ANON){
			uninit_new(&pg, upage, init, type, aux, anon_initializer);
		}
		else if (type == VM_FILE) {
			uninit_new(&pg, upage, init, type, aux, file_map_initializer);
		}
		if(vm_do_claim_page(&pg)) return true;
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt , void *va ) {
	/* TODO: Fill this function. */
	struct page *pg;
	struct list_elem *e;	
	
	for (e = list_begin (&spt->page_list); e != list_end (&spt->page_list); e = e->next){
		pg = list_entry(e, struct page, pg_e);
		if(pg->va == pg_round_down (va)) return pg;
	}
	return NULL;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt ,
		struct page *page ) {
	/* TODO: Fill this function. */
	struct page *pg = spt_find_page(spt, page->va);
	if (pg == NULL){
		list_push_back(&spt->page_list, &page->pg_e);
		return true;
	}
	return false;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	//frame = palloc_get_page(PAL_USER);
	//frame->kva = frame;
	//frame->page = NULL;
	
	frame = palloc_get_page(PAL_USER);
	frame->kva = frame;
	
	//frame->kva = frame;
	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}


bool
vm_try_handle_fault (struct intr_frame *f, void *addr,
		bool user UNUSED, bool write, bool not_present UNUSED) {
		
	struct supplemental_page_table *spt = &thread_current ()->spt;
	//printf("%x\n", addr);
	//struct page *page = NULL;
	//page = spt_find_page(spt, addr);
	//page->va = addr;
	
	
	//return vm_do_claim_page (page);
	if(!vm_claim_page(addr)) NOT_REACHED();
	//do_iret(f);
	PANIC("COME?");
	return true;
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
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va) {
	struct page *page = NULL;
	/* TODO: Fill this function */
	page = spt_find_page(&thread_current()->spt, pg_round_down(va));
	
	/*if (page == NULL) PANIC("NO PAGE");
	else if (page->frame == NULL) PANIC("NO FRAME");*/
	bool t = vm_do_claim_page (page);
	PANIC("THERE IS A GOOD PAGE!");
	
	
	return t;
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();
	
	/* Set links */
	frame->page = page;
	//PANIC("HERE!");
	page->frame = frame;
	
	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	/*struct thread *t = thread_current();
	struct supplemental_page_table *spt = &t->spt;
	spt_insert_page (spt , page);*/
	
	bool a = swap_in (page, frame->kva);
	return a;
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt) {
	list_init(&spt->page_list);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}
