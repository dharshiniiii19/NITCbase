#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"

#include <iostream>
#include <cstring>

using namespace std;

int main()
{
    Disk disk_run;

    int currentBlock = ATTRCAT_BLOCK;
    bool updated = false;

    // Search all Attribute Catalog blocks
    while (currentBlock != -1)
    {
        RecBuffer attrCatBuffer(currentBlock);

        HeadInfo header;
        attrCatBuffer.getHeader(&header);

        for (int i = 0; i < header.numEntries; i++)
        {
            Attribute record[ATTRCAT_NO_ATTRS];

            attrCatBuffer.getRecord(record, i);

            if (strcmp(record[ATTRCAT_REL_NAME_INDEX].sVal, "Students") == 0 &&
                strcmp(record[ATTRCAT_ATTR_NAME_INDEX].sVal, "Class") == 0)
            {
                strcpy(record[ATTRCAT_ATTR_NAME_INDEX].sVal, "Batch");

                // Write the modified record back
                attrCatBuffer.setRecord(record, i);

                updated = true;
                break;
            }
        }

        if (updated)
            break;

        currentBlock = header.rblock;
    }

    if (updated)
        cout << "Schema updated successfully.\n\n";
    else
        cout << "Attribute not found.\n";


    return 0;
}
