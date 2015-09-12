typedef struct slab_descriptor{
  void *start;
  u32 refcount;
  void *next;
  void *prev;
} slab_descriptor_t;

typedef struct block_descriptor{
  void *start;
  slab_descriptor_t *slab_desc;  
  void *prev;
  void *next;
} block_descriptor_t;

typedef struct kheap{
  u32 flags; //0:(1=Small) 1-31:RESERVED
  u32 block_size;
  u32 slab_size;
  u32 blocks_per_slab;
  u32 max_size;
  u32 alignment;
  void *(*constructor)(void *);
  void *(*destructor)(void *);
  void *start;
  void *end;
  block_descriptor_t *first_free;
  block_descriptor_t *last_free;
  slab_descriptor_t *first_free_slab;
  slab_descriptor_t *last_free_slab;
  u32 slabs_per_bchunk;
  u32 bchunks_per_schunk;
  u32 bchunk_metadata_size;
  u32 schunk_metadata_size;
  u32 effective_block_size;
  u32 bchunk_size;
  u32 schunk_size;
} kheap_t;

typedef struct block_info{
  block_descriptor_t *block_desc;
  slab_descriptor_t *slab_desc;
  void *schunk_start;
  void *bchunk_start;
} block_info_t;

#define PAGE_SIZE 4096
#define SLABS_PER_SCHUNK PAGE_SIZE/sizeof(slab_descriptor_t)
#define BLOCKS_PER_BCHUNK PAGE_SIZE/sizeof(block_descriptor_t)

void slab_debug_print(slab_descriptor_t *s);
void block_debug_print(block_descriptor_t *b);
void kheap_debug_print(kheap_t *heap, u32 detail);
void print_info(block_info_t *info);

inline u32 get_schunk_index(kheap_t *heap, u32 info_start){
  return (info_start - (u32)(heap->start))/(heap->schunk_size);
}
inline u32 get_schunk_start(kheap_t *heap, u32 info_start){
  return get_schunk_index(heap, info_start) * heap->schunk_size + (u32)(heap->start);
}
inline u32 get_bchunks_start(kheap_t *heap, u32 info_start){
  return get_schunk_start(heap, info_start) + heap->schunk_metadata_size;
}

void get_block_info(kheap_t *heap, void *block_start, block_info_t *info){
   //prints("\nheap->start = ");print_hex_32(heap->start);
   //prints("\nblock_start = ");print_hex_32(block_start);prints("\n");
   //print_hex_32(get_schunk_index(heap, block_start));prints(" ");print_hex_32(get_schunk_start(heap, block_start));prints(" ");print_hex_32(get_bchunks_start(heap, block_start));prints("\n");
  u32 u32_block_start = (u32)(block_start);
  u32 schunk_index = (u32_block_start - (u32)(heap->start))/(heap->schunk_size);
  //prints("info = ");print_hex_32((u32)info);
  info->schunk_start = (void *)(schunk_index * (heap->schunk_size) + (u32)(heap->start));
  
  u32 bchunks_start = (u32)(info->schunk_start) + heap->schunk_metadata_size;//align_by(SLABS_PER_SCHUNK * sizeof(slab_descriptor_t), PAGE_SIZE);
  //prints("\nbchunks_start: ");print_hex_32(bchunks_start);
  u32 bchunk_index = (u32_block_start - bchunks_start)/(heap->bchunk_size);
  info->bchunk_start = (void *)(bchunk_index * (heap->bchunk_size) + bchunks_start);

  u32 blocks_start = (u32)(info->bchunk_start) + heap->bchunk_metadata_size;//align_by(BLOCKS_PER_BCHUNK*sizeof(block_descriptor_t), PAGE_SIZE);
  u32 block_index = (u32_block_start - blocks_start)/(heap->block_size);
  info->block_desc = (block_descriptor_t *)(info->bchunk_start + block_index*sizeof(block_descriptor_t));

  u32 slab_index = bchunk_index * (heap->slabs_per_bchunk) + (heap->slab_size) * ((u32_block_start - blocks_start)/(heap->slab_size));
  info->slab_desc = (slab_descriptor_t *)(info->schunk_start + sizeof(slab_descriptor_t)*slab_index);
   //print_info(info);
  return;
}

u32 align_by(u32 size, u32 block_size){ return size + (block_size == 0 ? 0 : (size % block_size != 0 ? block_size : 0) - (size % block_size)); }


//======//
//EXPAND//
//======//

u32 expand(kheap_t *heap){
  void *new_end = heap->end + heap->slab_size;
  void *new_slab_start = heap->end;//This will always be the start of the new slab, as the bchunk/shunk descriptors will be allocated when we get to the end of the current chunk
  //If expanding would put us over a schunk boundary, add another page of slab descriptors:
  if((new_end - heap->start)/(heap->schunk_size) > (heap->end - heap->start)/(heap->schunk_size)){
    new_end += heap->schunk_metadata_size + heap->bchunk_metadata_size;
     //prints("\nlk\n");
  }
  else if(((u32)new_end - get_bchunks_start(heap, (u32)new_end - heap->block_size))/(heap->bchunk_size) > ((u32)(heap->end) - get_bchunks_start(heap, (u32)(heap->end) - heap->block_size))/(heap->bchunk_size)){
     //prints("jk\n");
    new_end += heap->bchunk_metadata_size;
  }
   //prints("BBBBB!\n");
   //prints("new end: ");print_hex_32(new_end);
   //prints("\nheap->end");print_hex_32(heap->end);
   //prints("\nheap->start");print_hex_32(heap->start);
   //prints("\n");
  //if(alloc_pages_for(heap->end, new_end, 1, 1) != 0) return -1;
  alloc_pages_for(heap->end, (u32)new_end - (u32)(heap->end), 1, 1, kernel_node->pagedir);//MAKE PAGING.C THINGS RETURN 0 ON SUCCESS TO USE LINE ABOVE INSTEAD OF THIS ONE!
  
   //kheap_debug_print(heap, 8);

  heap->end = new_end;

   //kheap_debug_print(heap, 8);

  //FILL OUT BLOCK AND SLAB DESCRIPTOR STARTS: 


  block_info_t info;
  get_block_info(heap, new_slab_start, &info);
   //print_info(&info);
  //Fill out struct for new slab: 
  info.slab_desc->start = new_slab_start;
  info.slab_desc->refcount = 0;
  //Update slab freelist linkage (new slab is entirely free, so goes on the freelist): 
  info.slab_desc->prev = heap->last_free_slab;
  info.slab_desc->next = NULL;
  if(heap->last_free_slab != NULL) heap->last_free_slab->next = info.slab_desc;
  heap->last_free_slab = info.slab_desc;
  //Fill out the info for the newly allocated block descriptors, including freelist linkage: 
  u32 i = 1;
  block_descriptor_t *b = info.block_desc;
  if(heap->first_free == NULL) heap->first_free = b;
  b->start = new_slab_start;
  b->next = b+1;
  b->prev = heap->last_free;
  b->slab_desc = info.slab_desc;
  //prints("\nASLDJA: ");print_hex_32(new_slab_start);print_hex_32(b);
  b = b+1;
  while(i < heap->blocks_per_slab - 1){
    b->start = new_slab_start + i*(heap->block_size);
    //Relying on pointer arithmetic to handle 1 replaced with sizeof(block_descirptor_t): 
    b->next = b+1;
    b->prev = b-1;
    b->slab_desc = info.slab_desc;
    b = b+1;
    i++;
  }
  b->next = NULL;
  b->prev = b-1;//ASSUMES MORE THAN ONE BLOCK PER SLAB!
  heap->last_free = b;
  //kheap_debug_print(heap, 8);
  return 0;
}

//========//
//CONTRACT//
//========//

u32 contract(kheap_t *heap){
  if(heap->first_free_slab == NULL) return -1;
  free_pages(heap->first_free_slab->start, heap->slab_size, kernel_node->pagedir);
  heap->first_free_slab = heap->first_free_slab->next;
}

//======//
//CREATE//
//======//

kheap_t *kheap_init(kheap_t *heap, u32 b_size, u32 s_size, u32 m_size, u32 align, void * (*constr)(void *), void *(*destr)(void *), void *location){
  heap->block_size = b_size;
  heap->slab_size = s_size;
  heap->blocks_per_slab = s_size / b_size;
  heap->max_size = m_size;
  heap->alignment = align;
  heap->constructor = constr;
  heap->destructor = destr;
  heap->start = location;
  heap->slabs_per_bchunk = BLOCKS_PER_BCHUNK / ((heap->slab_size)/(heap->block_size));
  heap->bchunks_per_schunk = SLABS_PER_SCHUNK / heap->slabs_per_bchunk;
  heap->bchunk_metadata_size = align_by(BLOCKS_PER_BCHUNK*sizeof(block_descriptor_t), PAGE_SIZE);
  heap->schunk_metadata_size = align_by(SLABS_PER_SCHUNK*sizeof(slab_descriptor_t), PAGE_SIZE);
  heap->end = location + heap->bchunk_metadata_size + heap->schunk_metadata_size;
  heap->effective_block_size = align_by(heap->block_size, heap->alignment);
  heap->bchunk_size = heap->slabs_per_bchunk*(heap->slab_size) + heap->bchunk_metadata_size;
  heap->schunk_size = SLABS_PER_SCHUNK*(heap->slab_size) + (heap->bchunk_metadata_size)*(heap->bchunks_per_schunk) + (heap->schunk_metadata_size);
  heap->first_free = NULL;
  heap->last_free = NULL;
  heap->first_free_slab = NULL;
  heap->last_free_slab = NULL;
  alloc_pages_for(heap->start, (u32)(heap->end) - (u32)(heap->start), 1, 1, kernel_node->pagedir);
  if(expand(heap) == 0) return heap;
  return NULL;
}

//=====//
//ALLOC//
//=====//

void *kheap_alloc(kheap_t *heap){
  //Expand if needed
  if(heap->first_free == NULL) if(expand(heap) != 0) return NULL;

  //Update linkage
  //prints("A");
  block_descriptor_t *new_block = heap->first_free;
  //prints("new_block: ");print_addr(new_block);
  block_debug_print(new_block);
  heap->first_free = heap->first_free->next;
  heap->first_free->prev = NULL;
  new_block->next = NULL;
  new_block->prev = NULL;
  //prints("A");

  block_info_t info;
  get_block_info(heap, new_block->start, &info);

  //Update refcount
  info.slab_desc->refcount++;
  //prints("A");

  //If that slab was formerly on the freelist, remove it.  
  if(info.slab_desc->refcount == 1){
    if(info.slab_desc->next == NULL) heap->last_free_slab = info.slab_desc->prev;
    else ((slab_descriptor_t *)(info.slab_desc->next))->prev = (slab_descriptor_t *)(info.slab_desc->prev);
    if(info.slab_desc->prev == NULL) heap->first_free_slab = info.slab_desc->next;
    else ((slab_descriptor_t *)(info.slab_desc->prev))->next = (slab_descriptor_t *)(info.slab_desc->next);
  }

  //Return:
  return new_block->start;
}


//====//
//FREE//
//====//

void kheap_free(kheap_t *heap, void *block_start){
  //Update linkage
  block_info_t info;
  get_block_info(heap, block_start, &info);
  block_descriptor_t *to_free = info.block_desc;
  to_free->next = NULL;//Possibly unnecessary, as we should set all occupied blocks' next/prev to NULL in allocation
  to_free->prev = heap->last_free;
  heap->last_free = to_free;

  //Update refcount
  info.slab_desc->refcount--;

  //If this makes this slab entirely free, put it on the freelist: 
  if(info.slab_desc->refcount == 0){
    info.slab_desc->next = NULL;
    info.slab_desc->prev = heap->last_free_slab;
    heap->last_free_slab = info.slab_desc;
  }

  //Reclaim if wanted
  //if((get_slab_descriptor(to_free))->refcount == 0) contract(to_free);

  return;
}

//===================//
//AUXILIARY FUNCTIONS//
//===================//

/*block_descriptor_t *get_block_descriptor(void *block_start){
  u32 info_start = (u32)(block_start), schunk_index, schunk_start, bchunks_start, bchunk_index, bchunk_start, blocks_start;
  get_location_info(heap, info_start, &schunk_index, &schunk_start, &bchunks_start, &bchunk_index, &bchunk_start, &blocks_start);

  u32 block_index = (info_start - blocks_start)/(heap->block_size);

  return ((block_descriptor_t *)(bchunk_start))[block_index];
}

slab_descriptor_t *get_slab_descriptor(block_descriptor_t *block_desc){
  u32 info_start = (u32)(block_desc), schunk_index, schunk_start, bchunks_start, bchunk_index, bchunk_start, blocks_start;
  get_location_info(heap, info_start, &schunk_index, &schunk_start, &bchunks_start, &bchunk_index, &bchunk_start, &blocks_start);

  u32 block_index = (info_start - bchunk_start)/sizeof(block_descriptor_t);
  u32 block_start = block_index + blocks_start * (heap->block_size);

  u32 slab_index_within_bchunk = (block_start - blocks_start)/(heap->slab_size)
    u32 slab_index_within_schunk = bchunk_index * slabs_per_bchunk + slab_index_within_bchunk;

  return ((slab_descriptor_t *)(schunk_start))[slab_index_within_schunk];
}


void *get_slab_start(slab_descriptor_t *slab_desc){
  u32 info_start = (u32)(slab_desc);
  u32 bchunks_start = get_bchunks_start(heap, info_start);
  u32 schunk_start = get_schunk_start(heap, info_start);
  u32 slab_index = ((info_start - schunk_start) / sizeof(slab_descriptor_t));
  u32 bchunk_index = slab_index / (heap->slabs_per_bchunk);
  u32 slab_index_within_bchunk = slab_index % (heap->slabs_per_bchunk);
  return (void *)(bchunks_start + (heap->bchunk_size)*bchunk_index + heap->bchunk_metadata_size + (heap->slab_size)*slab_index_within_bchunk);
}

void *get_block_start(block_descriptor_t *block_desc){
  u32 info_start = (u32)(block_desc), schunk_index, schunk_start, bchunks_start, bchunk_index, bchunk_start, blocks_start;
  get_location_info(heap, info_start, &schunk_index, &schunk_start, &bchunks_start, &bchunk_index, &bchunk_start, &blocks_start);

  u32 block_index = (info_start - bchunk_start)/sizeof(block_descriptor_t);

  return (void*)(blocks_start + block_index*(heap->block_size));
}

inline u32 get_schunk_index(kheap_t *heap, u32 info_start){
  return (info_start - heap->start)/(heap->schunk_size);
}
inline u32 get_schunk_start(kheap_t *heap, u32 info_start){
  return get_schunk_index(heap, info_start) * heap->schunk_size + heap->start;
}
inline u32 get_bchunks_start(kheap_t *heap, u32 info_start){
  return get_schunk_start(heap, info_start) + heap->schunk_metadata_size;
}
inline u32 get_bchunk_index(kheap_t *heap, u32 info_start){
  return (info_start - get_bchunks_start(heap, info_start))/(heap->bchunk_size);
}
inline u32 get_bchunk_start(kheap_t *heap, u32 info_start){
  return get_bchunk_index(heap, info_start) * (heap->bchunk_size) + get_bchunks_start;
}
inline u32 get_blocks_start(kheap_t *heap, u32 info_start){
  return bchunk_start + heap->bchunk_metadata_size;
}

void get_location_info(kheap_t *heap, u32 info_start, u32 *schunk_index, u32 *schunk_start, u32 *bchunks_start, u32 *bchunk_index, u32 *bchunk_start, u32 *blocks_start){
  *schunk_index = (info_start - heap->start)/heap->schunk_size;
  *schunk_start = *schunk_index * heap->schunk_size + heap->start;

  *bchunks_start = *schunk_start + align_by(SLABS_PER_SCHUNK * sizeof(slab_descriptor_t), PAGE_SIZE);
  *bchunk_index = (info_start - *bchunks_start)/(heap->bchunk_size);
  *bchunk_start = *bchunk_index * (heap->bchunk_size) + *bchunks_start;

  *blocks_start = *bchunk_start + align_by(BLOCKS_PER_BCHUNK*sizeof(block_descriptor_t), PAGE_SIZE);
  return;
}*/
