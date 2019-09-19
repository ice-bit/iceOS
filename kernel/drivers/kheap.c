#include "kheap.h"

// Extern variables are declared in the linker script
extern uint32_t end;
uint32_t placement_addr = (uint32_t)&end;
extern page_directory_t *kernel_directory; // FIXME:
heap_t *kheap = 0;

/* The following function is a simple method to find the smallest hole that
 * fit user space request, since we will do this process many times, it's a good
 * idea to wrap it in a function */
static int32_t find_smallest_hole(uint32_t size, uint8_t page_align, heap_t *heap) {
    uint32_t i = 0;
    while(i < heap->index.size) {
        header_t *header = (header_t*)lookup_ordered_list(i, &heap->index);
        if(page_align > 0) {
            uint32_t loc = (uint32_t)header;
            int32_t offset = 0;
            if((loc+sizeof(header_t)) & 0xFFFFF000 != 0) // Page aligned memory
                offset = 0x1000 - (loc+sizeof(header_t))%0x1000;
            int32_t hole_size = (int32_t)header->size - offset;
            if(hole_size >= (int32_t)size)
                break;
        } else if(header->size >= size)
            break;
        i++;
    }
    // Return something according to the iterator
    if(i == heap->index.size)
        return -1; // Nothing found
    else
        return i;
}

static int8_t header_t_less_than(void *a, void *b) {
    return (((header_t*)a)->size < ((header_t*)b)->size)?1:0;
}

heap_t *create_heap(uint32_t start, uint32_t end, uint32_t max, uint8_t supervisor, uint8_t readonly) {
    heap_t *heap = (heap_t*)kmalloc(sizeof(heap_t)); //TODO: implement kmalloc

    ASSERT(start%0x1000 == 0);
    ASSERT(end%0x1000 == 0);

    // Init heap's index
    heap->index = place_ordered_list((void*)start, HEAP_INDEX_SIZE, &header_t_less_than);

    // Shift start address to the corrent position, where we can put on data
    start += sizeof(type_t)*HEAP_INDEX_SIZE;

    // Check if start address is page-aligned
    if(start & 0xFFFFF000 != 0) {
        start &= 0xFFFFF000;
        start += 0x1000;
    }

    // Fill the heap structure with start, end and max addresses
    heap->start_address = start;
    heap->end_address = end;
    heap->max_address = max;
    heap->supervisor = supervisor;
    heap->readonly = readonly;

    // Let's create one large hole in the new index
    header_t *hole = (header_t*)start;
    hole->size = end-start;
    hole->magic = HEAP_MAGIC;
    hole->is_hole = 1;
    insert_ordered_list((void*)hole, &heap->index);

    return heap;
}

static void expand(uint32_t new_size, heap_t *heap) {
    // Before anything else let's check that new size is greater than older one
    ASSERT(new_size > heap->end_address - heap->start_address);
    if(new_size&0xFFFFF000 != 0) {
        new_size &= 0xFFFFF000;
        new_size += 0x1000;
    }
    // Check if new size is not greater than maximum size
    ASSERT(heap->start_address+new_size <= heap->max_address);

    uint32_t old_size = heap->end_address-heap->start_address;
    uint32_t i = old_size;
    while(i < new_size) {
        alloc_frame(get_page(heap->start_address+i, 1, kernel_directory),
        (heap->supervisor)?1:0, (heap->readonly)?0:1);
    }
}

static uint32_t contract(uint32_t new_size, heap_t *heap) {
    // This function will be literally the opposite of the previous one
    ASSERT(new_size < heap->end_address-heap->start_address);

    if(new_size&0x1000) {
        new_size &= 0x1000;
        new_size += 0x1000;
    }

    if(new_size < HEAP_MIN_SIZE)
        new_size = HEAP_MIN_SIZE;
    uint32_t old_size = heap->end_address-heap->start_address;
    uint32_t i = old_size - 0x1000;
    while(new_size < i) {
        free_frame(get_page(heap->start_address+i, 0, kernel_directory));
        i -= 0x1000;
    }
    heap->end_address = heap->start_address + new_size;
    return new_size;
}

void *alloc(uint32_t size, uint8_t page_align, heap_t *heap) {
    uint32_t new_size = size + sizeof(header_t) + sizeof(footer_t);
    int32_t i = find_smallest_hole(new_size, page_align, heap);
    // Error checking for "no hole available"
    if(i == -1) {
        // Save previous data
        uint32_t old_len = heap->end_address - heap->start_address;
        uint32_t old_end_addr = heap->end_address;

        // Allocate more space
        expand(old_len+new_size, heap);
        uint32_t new_len = heap->end_address-heap->start_address;

        i = 0; // Endmost header in location
        uint32_t idx = -1; // Endmost header's index
        uint32_t value = 0x0; // Endmost header's value
        while(i < heap->index.size) {
            uint32_t tmp = (uint32_t)lookup_ordered_list(i, &heap->index);
            if(tmp > value) {
                value = tmp;
                idx = i;
            }
            i++;
        }
        // In case we did not find any headers, we need to add one
        if(idx == -1) {
            header_t *head = (header_t*)old_end_addr;
            head->magic = HEAP_MAGIC;
            head->size = new_len - old_len;
            head->is_hole = 1;
            footer_t *foot = (footer_t*) (old_end_addr + head->size - sizeof(footer_t));
            foot->magic = HEAP_MAGIC;
            foot->header = head;
            insert_ordered_list((void*)head, &heap->index);
        } else {
            header_t *head = lookup_ordered_list(idx, &heap->index);
            head->size += new_len - old_len;
            // Rewrite the footer
            footer_t *foot = (footer_t*) ((uint32_t)head + head->size - sizeof(footer_t));
            foot->header = head;
            foot->magic = HEAP_MAGIC;
        }
        // Now that we have enough space, use recursing to call this function again
        return alloc(size, page_align, heap);
    }
    
    header_t *origin_hole_header = (header_t*)lookup_ordered_list(i, &heap->index);
    uint32_t origin_hole_p = (uint32_t)origin_hole_header;
    uint32_t origin_hole_s = origin_hole_header->size;
    // Now check if we should split the hole into two parts
    if(origin_hole_s-new_size < sizeof(header_t)+sizeof(header_t)) {
        size += origin_hole_s-new_size;
        new_size = origin_hole_s;
    }

    // Now check if we need page-aligned data
    if(page_align && origin_hole_p&0xFFFFF000) {
        uint32_t new_location = origin_hole_p + 0x1000 - (origin_hole_p&0xFFF) - sizeof(header_t);
        header_t *hole_header = (header_t*)origin_hole_p;
        hole_header -= 0x1000 - (origin_hole_p&0xFFF) - sizeof(header_t);
        hole_header->magic = HEAP_MAGIC;
        hole_header->is_hole = 1;
        footer_t *hole_footer = (footer_t*) ((uint32_t)new_location - sizeof(footer_t));
        hole_footer->magic = HEAP_MAGIC;
        hole_footer->header = hole_header;
        origin_hole_p = new_location;
        origin_hole_s = origin_hole_s - hole_header->size;
    } else // Otherwise delete this hole from the index since we don't need it anymore
        remove_ordered_list(i, &heap->index);
    
    // Since we're creating a new hole at the old hole's address we can reuse the old hole
    header_t *block_header = (header_t*)origin_hole_p;
    block_header->magic = HEAP_MAGIC;
    block_header->is_hole = 0;
    block_header->size = new_size;
    // Now overwrite original footer
    footer_t *block_footer = (footer_t*)(origin_hole_p + sizeof(header_t) + size);
    block_footer->magic = HEAP_MAGIC;
    block_footer->header = block_header;

    // If the new block have positive size, then write a new hole after new block
    if(origin_hole_s - new_size > 0) {
        header_t *hole_head = (header_t*)(origin_hole_p * sizeof(header_t) + size + sizeof(footer_t));
        hole_head->magic = HEAP_MAGIC;
        hole_head->is_hole = 1;
        hole_head->size = origin_hole_s - (new_size);
        footer_t *hole_foot = (footer_t*)((uint32_t)hole_head + origin_hole_s - new_size - sizeof(footer_t));
        if((uint32_t)hole_foot < heap->end_address) {
            hole_foot->magic = HEAP_MAGIC;
            hole_foot->header = hole_head;
        }
        insert_ordered_list((void*)hole_head, &heap->index);
    }
    // Finally, return the new hole
    return (void*)(uint32_t)block_header+sizeof(header_t));
}