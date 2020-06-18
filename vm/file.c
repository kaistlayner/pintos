/* file.c: Implementation of memory mapped file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
/* An open file. */

static bool file_map_swap_in (struct page *page, void *kva);
static bool file_map_swap_out (struct page *page);
static void file_map_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_map_swap_in,
	.swap_out = file_map_swap_out,
	.destroy = file_map_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file mapped page */
bool
file_map_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
}

/* Swap in the page by read contents from the file. */
static bool
file_map_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_map_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file mapped page. PAGE will be freed by the caller. */
static void
file_map_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
		struct supplemental_page_table* spt = &thread_current()->spt;
		struct page *pg;
		if(pg = spt_find_page(spt, addr) != NULL) {
			return NULL;
		}
		file_seek(file, offset);
		int left = length;
		while(left > 0){
			pg = malloc(sizeof(struct page));
			struct frame* frame = vm_get_frame();
			frame->page = pg;
			pg->frame = frame;
			file_map_initializer(pg, VM_FILE, frame->kva);
			pg->writable = writable;
			pg->va = addr;
	
			spt_insert_page(spt, pg);
			int read_bytes = left > PGSIZE ? PGSIZE : left;
			int readed = file_read(file, pg->frame->kva, read_bytes);
			memset(pg->frame->kva + readed, 0, PGSIZE - readed);
			left = left - PGSIZE;
			if (readed != read_bytes) break;
		}
		
		
		
// here
		pml4_set_page(thread_current()->pml4, pg->va, pg->frame->kva, pg->writable);
		return addr;
}

/* Do the munmap */
void
do_munmap (void *addr) {
	
}
