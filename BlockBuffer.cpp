#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>
BlockBuffer::BlockBuffer(int blockNum){
    this->blockNum=blockNum;
}
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}
 unsigned char buffer[BLOCK_SIZE];

 int BlockBuffer::getHeader(struct HeadInfo *head) {

    unsigned char buffer[BLOCK_SIZE];

    // Read the block into buffer
    Disk::readBlock(buffer, this->blockNum);

    // Copy header fields
    memcpy(&head->blockType, buffer + 0, 4);
    memcpy(&head->pblock,    buffer + 4, 4);
    memcpy(&head->lblock,    buffer + 8, 4);
    memcpy(&head->rblock,    buffer + 12, 4);
    memcpy(&head->numEntries,buffer + 16, 4);
    memcpy(&head->numAttrs,  buffer + 20, 4);
    memcpy(&head->numSlots,  buffer + 24, 4);

    return SUCCESS;
}
int RecBuffer::getRecord(union Attribute *rec, int slotNum) {

    HeadInfo head;

    // Read header
    getHeader(&head);

    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;

    unsigned char buffer[BLOCK_SIZE];

    // Read the block
    Disk::readBlock(buffer, this->blockNum);

    // Calculate record size
    int recordSize = attrCount * ATTR_SIZE;

    // Slot map size = number of slots (1 byte per slot)
    int slotMapSize = slotCount;

    // Address of required record
    unsigned char *slotPointer =
        buffer + HEADER_SIZE + slotMapSize + (recordSize * slotNum);

    // Copy record
    memcpy(rec, slotPointer, recordSize);

    return SUCCESS;
}
// write the record from the argument pointer into slotNum of this block
int RecBuffer::setRecord(union Attribute *rec, int slotNum) {
  struct HeadInfo head;

  // 1. Fetch header info to calculate record size and total slots
  this->getHeader(&head);

  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;

  // Validation: ensure slotNum is within valid bounds
  if (slotNum < 0 || slotNum >= slotCount) {
    return E_OUTOFBOUND;
  }

  // 2. Read the full block from disk into a local buffer
  unsigned char buffer[BLOCK_SIZE];
  Disk::readBlock(buffer, this->blockNum);

  // 3. Calculate offset: HEADER_SIZE + slotMapSize + (recordSize * slotNum)
  int recordSize = attrCount * ATTR_SIZE;
  int slotMapSize = slotCount;

  unsigned char *slotPointer = buffer + HEADER_SIZE + slotMapSize + (recordSize * slotNum);

  // 4. Copy updated record bytes into buffer
  memcpy(slotPointer, rec, recordSize);

  // 5. Write the updated block back to disk
  Disk::writeBlock(buffer, this->blockNum);

  return SUCCESS;
}
