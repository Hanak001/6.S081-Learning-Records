// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"
#define NBUCKETS 13
struct {
  struct buf buckets[NBUCKETS];
  struct spinlock buclocks[NBUCKETS];
  char lockname[NBUCKETS][20];
  struct buf buf[NBUF];
  struct spinlock lock;
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;
} bcache;

void
binit(void)
{
  struct buf *b;
  for(int i=0;i<NBUCKETS;i++){
    snprintf(bcache.lockname[i],20,"bcache.bucket-%d",i);
    initlock(&bcache.buclocks[i], bcache.lockname[i]);
  } 
  

  // Create linked list of buffers
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;
  // for(b = bcache.buf; b < bcache.buf+NBUF; b++){
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   initsleeplock(&b->lock, "buffer");
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }
  for(b=bcache.buf;b<bcache.buf+NBUF;b++){
    int i=(b-bcache.buf)%NBUCKETS;
    acquire(&bcache.buclocks[i]);
    b->next=bcache.buckets[i].next;
    bcache.buckets[i].next=b;
    release(&bcache.buclocks[i]);
    initsleeplock(&b->lock,"buffer");

  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b,*cur=0,*pre=0;
  uint i,bucketno,sno;
  bucketno=blockno%NBUCKETS;
  acquire(&bcache.buclocks[bucketno]);
  for(b=bcache.buckets[bucketno].next;b!=0;b=b->next){
    if(b->dev==dev&&b->blockno==blockno){
      b->refcnt++;
      b->timestamp=ticks;
      release(&bcache.buclocks[bucketno]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.buclocks[bucketno]);
  acquire(&bcache.lock);
  for(i=0;i<NBUCKETS;i++){
    sno=(bucketno+i)%NBUCKETS;
    acquire(&bcache.buclocks[sno]);
    if(sno==bucketno){
      for(b=bcache.buckets[bucketno].next;b!=0;b=b->next){
        if(b->dev==dev&&b->blockno==blockno){
          b->refcnt++;
          release(&bcache.buclocks[bucketno]);
          acquiresleep(&b->lock);
          return b;
        }
      }
    }
    for(b=&bcache.buckets[sno];b->next!=0;b=b->next){
      if(b->next->refcnt==0&&(!cur||b->next->timestamp<cur->timestamp)){
        pre=b;
        cur=b->next;
      }
    }
    if(cur){
      cur->dev = dev;
      cur->blockno = blockno;
      cur->valid = 0;
      cur->refcnt = 1;
      pre->next=cur->next;
      cur->timestamp=ticks;
      release(&bcache.buclocks[sno]);

      acquire(&bcache.buclocks[bucketno]);
      cur->next=bcache.buckets[bucketno].next;
      bcache.buckets[bucketno].next=cur;
      release(&bcache.buclocks[bucketno]);


      release(&bcache.lock);
      acquiresleep(&cur->lock);
      return cur;
    }
    release(&bcache.buclocks[sno]);
  }
  panic("bget: no buffers");
  // acquire(&bcache.lock);

  // // Is the block already cached?
  // for(b = bcache.head.next; b != &bcache.head; b = b->next){
  //   if(b->dev == dev && b->blockno == blockno){
  //     b->refcnt++;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }

  // // Not cached.
  // // Recycle the least recently used (LRU) unused buffer.
  // for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
  //   if(b->refcnt == 0) {
  //     b->dev = dev;
  //     b->blockno = blockno;
  //     b->valid = 0;
  //     b->refcnt = 1;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }
  // panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  uint bucketno;
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);


  bucketno=b->blockno%NBUCKETS;
  acquire(&bcache.buclocks[bucketno]);
  b->refcnt--;
  if(b->refcnt==0){
    b->timestamp=ticks;
  }
  release(&bcache.buclocks[bucketno]);
  // acquire(&bcache.lock);
  // b->refcnt--;
  // if (b->refcnt == 0) {
  //   // no one is waiting for it.
  //   b->next->prev = b->prev;
  //   b->prev->next = b->next;
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }
  
  // release(&bcache.lock);
}

void
bpin(struct buf *b) {
  int bucketno=b->blockno%NBUCKETS;
  acquire(&bcache.buclocks[bucketno]);
  b->refcnt++;
  release(&bcache.buclocks[bucketno]);
}

void
bunpin(struct buf *b) {
  int bucketno=b->blockno%NBUCKETS;
  acquire(&bcache.buclocks[bucketno]);
  b->refcnt--;
  release(&bcache.buclocks[bucketno]);
}


