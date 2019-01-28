#include "mm.h"

#define ALIGNMENT 16
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

#define OVERHEAD (sizeof(block_header) * 2)
#define HDRP(bp) ((char *)(bp) - sizeof(block_header))
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - OVERHEAD)

#define GET(p) (*(size_t *)(p))
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_SIZE(p) (GET(p) & ~0xF)
#define PUT(p, val) (*(size_t *)(p) = (val))
#define PACK(size, alloc) ((size) | (alloc))

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE((char *)(bp)-OVERHEAD))

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

typedef size_t block_header;

typedef struct list_node
{
  struct list_node *next;
  struct list_node *prev;
} list_node;

list_node *free_list;

void mm_init(void *heap, size_t heap_size)
{
  // prolog
  PUT(heap, PACK(OVERHEAD, 1));
  PUT(heap + sizeof(block_header), PACK(OVERHEAD, 1));

  // payload
  void *bp = heap + 3 * sizeof(block_header);
  bp += sizeof(block_header); // align to 16
  PUT(HDRP(bp), PACK(heap_size - 4 * sizeof(block_header), 0));
  PUT(FTRP(bp), PACK(heap_size - 4 * sizeof(block_header), 0));

  free_list = (list_node *)bp;
  free_list->prev = NULL;
  free_list->next = NULL;

  // terminate block
  bp = heap + heap_size;
  PUT(HDRP(heap + heap_size), PACK(0, 1));
}

static void set_allocated(void *bp, size_t size)
{
  PUT(HDRP(bp), PACK(size, 1));
  PUT(FTRP(bp), PACK(size, 1));
}

static void set_unallocated(void *bp, size_t size)
{
  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
}

void add_to_front_list(list_node **head, list_node *new_node)
{
  new_node->next = *head;
  new_node->prev = NULL;
  if (*head != NULL)
  {
    (*head)->prev = new_node;
  }
  *head = new_node;
}

void replace_in_list(list_node **head, list_node *old_node, list_node *new_node)
{
  *new_node = *old_node;

  if (old_node->next != NULL)
  {
    old_node->next->prev = new_node;
  }
  if (old_node->prev != NULL)
  {
    old_node->prev->next = new_node;
  }

  if (*head == old_node)
  {
    *head = new_node;
  }
}

void delete_from_list(list_node **head, list_node *deleted_node)
{
  if (*head == deleted_node)
  {
    *head = deleted_node->next;
  }

  if (deleted_node->next != NULL)
  {
    deleted_node->next->prev = deleted_node->prev;
  }
  if (deleted_node->prev != NULL)
  {
    deleted_node->prev->next = deleted_node->next;
  }
}

void *mm_malloc(size_t size)
{
  size_t real_size = ALIGN(MAX(size, sizeof(list_node)) + OVERHEAD);
  size_t available_size;

  list_node *p = free_list;
  while (1)
  {
    if (p == NULL)
    {
      // run out of memory
      return NULL;
    }

    available_size = GET_SIZE(HDRP(p));

    if (available_size >= real_size)
    {
      break;
    }

    p = p->next;
  }

  void *bp = (void *)p;

  if (available_size - real_size < sizeof(list_node) + OVERHEAD)
  {
    real_size = available_size;
    set_allocated(bp, real_size);

    // full, delete node from free list
    delete_from_list(&free_list, p);
  }
  else
  {
    set_allocated(bp, real_size);
    void *next_bp = NEXT_BLKP(bp);
    set_unallocated(next_bp, available_size - real_size);
    list_node *new_node = (list_node *)next_bp;

    // still have space, set current node to next free block
    replace_in_list(&free_list, p, new_node);
  }
  return bp;
}

void mm_free(void *bp)
{
  char prev_allocated = GET_ALLOC(HDRP(PREV_BLKP(bp)));
  char next_allocated = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  if (prev_allocated && next_allocated)
  {
    set_unallocated(bp, size);
    list_node *new_node = (list_node *)bp;

    // add to free list
    add_to_front_list(&free_list, new_node);
  }
  else if (prev_allocated && !next_allocated)
  {
    size_t next_block_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
    list_node *next_block_node = (list_node *)NEXT_BLKP(bp);
    set_unallocated(bp, size + next_block_size);
    list_node *current_block_node = (list_node *)bp;

    // remove next block from free list and put current block instead
    replace_in_list(&free_list, next_block_node, current_block_node);
  }
  else if (!prev_allocated && next_allocated)
  {
    size_t prev_block_size = GET_SIZE(HDRP(PREV_BLKP(bp)));
    set_unallocated(PREV_BLKP(bp), size + prev_block_size);

    // do nothing to free list
  }
  else
  {
    size_t next_block_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
    size_t prev_block_size = GET_SIZE(HDRP(PREV_BLKP(bp)));
    list_node *next_block_node = (list_node *)NEXT_BLKP(bp);
    set_unallocated(PREV_BLKP(bp), size + prev_block_size + next_block_size);

    // remove next block from free list
    delete_from_list(&free_list, next_block_node);
  }
}
