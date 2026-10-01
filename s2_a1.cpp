#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
//#include "FrontendInterface/FrontendInterface.h"
#include <iostream>
#include <cstring>
int main(int argc,char *argv[]){
     Disk disk_run;
 RecBuffer relCatBuffer(RELCAT_BLOCK);
 HeadInfo relCatHeader;
 HeadInfo  attrCatHeader;
  relCatBuffer.getHeader(&relCatHeader);
  printf("Number of Relation Catalog entries = %d\n",
       relCatHeader.numEntries);
  for(int i=0;i<relCatHeader.numEntries;i++){
     Attribute relCatRecord[RELCAT_NO_ATTRS]; // will store the record from the relation catalog

    relCatBuffer.getRecord(relCatRecord, i);

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);
     int currentBlock = ATTRCAT_BLOCK;
   while(currentBlock != -1){
     RecBuffer attrCatBuffer(currentBlock);
     attrCatBuffer.getHeader(&attrCatHeader);
    for(int j=0;j<attrCatHeader.numEntries;j++){
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBuffer.getRecord(attrCatRecord, j);
         if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal,relCatRecord[RELCAT_REL_NAME_INDEX].sVal) == 0){
             const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
              printf("  %s: %s\n",attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal , attrType);
         }
    }
    currentBlock = attrCatHeader.rblock;
   }     
      printf("\n");
  }
  return 0;
}
