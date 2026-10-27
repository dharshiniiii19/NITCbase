#include "BPlusTree.h"

#include <cstring>

inline bool operator == (RecId lhs, RecId rhs) {
	return (lhs.block == rhs.block && lhs.slot == rhs.slot);
}

inline bool operator != (RecId lhs, RecId rhs) {
	return (lhs.block != rhs.block || lhs.slot != rhs.slot);
}

inline bool operator == (IndexId lhs, IndexId rhs) {
	return (lhs.block == rhs.block && lhs.index == rhs.index);
}

inline bool operator != (IndexId lhs, IndexId rhs) {
	return (lhs.block != rhs.block || lhs.index != rhs.index);
}

RecId BPlusTree::bPlusSearch(int relId, char attrName[ATTR_SIZE],
                             Attribute attrVal, int op)
{
    /********** STEP 1: Load the bookmark and attribute info **********/

    // searchIndex = position (leaf block, entry no.) of the previous match.
    // {-1,-1} means "no previous search, start fresh".
    IndexId searchIndex;
    int ret = AttrCacheTable::getSearchIndex(relId, attrName, &searchIndex);
    if (ret != SUCCESS) return RecId{-1, -1};

    // attrCatEntry gives us the attribute's type and the root block of its B+ tree
    AttrCatEntry attrCatEntry;
    ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
    if (ret != SUCCESS) return RecId{-1, -1};

    int attrType = attrCatEntry.attrType;   // NUMBER or STRING, used for all comparisons

    // block = block we are currently looking at
    // index = entry number inside that block where scanning starts
    int block = -1, index = -1;

    /********** STEP 2: Decide where to start (first call vs resumed call) **********/

    if (searchIndex == IndexId{-1, -1})
    {
        // First call: start from the root of the B+ tree, first entry
        block = attrCatEntry.rootBlock;
        index = 0;

        // rootBlock == -1 means this attribute has no index
        if (block == -1) return RecId{-1, -1};
    }
    else
    {
        // Resumed call: we are already in a leaf. Continue from the entry
        // right after the last match.
        block = searchIndex.block;
        index = searchIndex.index + 1;

        // Read the leaf's header to know how many entries it has
        IndLeaf leaf(block);
        HeadInfo leafHead;
        leaf.getHeader(&leafHead);

        // If we've used up this leaf, move to the next leaf (right sibling)
        if (index >= leafHead.numEntries)
        {
            block = leafHead.rblock;
            index = 0;

            // No next leaf -> no more matches
            if (block == -1) return RecId{-1, -1};
        }
    }

    /********** STEP 3: Walk down internal nodes until we reach a leaf **********/
    // (Skipped automatically when resuming, because `block` is already a leaf.)

    while (StaticBuffer::getStaticBlockType(block) == IND_INTERNAL)
    {
        IndInternal internalBlk(block);
        HeadInfo intHead;
        internalBlk.getHeader(&intHead);   // gives numEntries of this internal node
        InternalEntry intEntry;

        if (op == NE || op == LT || op == LE)
        {
            // Matches start from the smallest keys (or NE needs all leaves),
            // so always go to the left child of the first entry.
            internalBlk.getEntry(&intEntry, 0);
            block = intEntry.lChild;
        }
        else   // EQ, GT, GE
        {
            bool entryFound = false;
            int i = 0;   // separate counter so `index` (used in the leaf scan) is not overwritten

            // Find the first key that is >= attrVal (EQ, GE) or > attrVal (GT)
            while (i < intHead.numEntries)
            {
                internalBlk.getEntry(&intEntry, i);
                int cmpVal = compareAttrs(intEntry.attrVal, attrVal, attrType);

                if ((op == EQ && cmpVal >= 0) ||
                    (op == GE && cmpVal >= 0) ||
                    (op == GT && cmpVal > 0))
                {
                    entryFound = true;
                    break;
                }
                i++;
            }

            if (entryFound)
                block = intEntry.lChild;   // the first matching key could be in its left subtree
            else
                block = intEntry.rChild;   // every key is too small -> go right of the last entry
        }
    }

    // `block` is now a leaf index block

    /********** STEP 4: Scan the leaves for the first matching entry **********/

    while (block != -1)
    {
        IndLeaf leafBlk(block);
        HeadInfo leafHead;
        leafBlk.getHeader(&leafHead);
        Index leafEntry;

        // Check entries one by one, starting at `index`
        while (index < leafHead.numEntries)
        {
            leafBlk.getEntry(&leafEntry, index);
            int cmpVal = compareAttrs(leafEntry.attrVal, attrVal, attrType);

            if ((op == EQ && cmpVal == 0) ||
                (op == LE && cmpVal <= 0) ||
                (op == GE && cmpVal >= 0) ||
                (op == LT && cmpVal < 0)  ||
                (op == GT && cmpVal > 0)  ||
                (op == NE && cmpVal != 0))
            {
                // Match found: save the position as the new bookmark
                searchIndex = IndexId{block, index};
                AttrCacheTable::setSearchIndex(relId, attrName, &searchIndex);

                // Return the location (block, slot) of the actual record
                return RecId{leafEntry.block, leafEntry.slot};
            }
            else if ((op == EQ || op == LE || op == LT) && cmpVal > 0)
            {
                // Keys are sorted ascending, so once a key is bigger than attrVal,
                // no later entry can satisfy EQ, LE or LT -> stop.
                return RecId{-1, -1};
            }

            ++index;   // try the next entry
        }

        // Finished this leaf with no match.
        // Only NE has to continue; for other ops, later leaves can't match.
        if (op != NE) break;

        // NE: move to the next leaf and start at its first entry
        block = leafHead.rblock;
        index = 0;
    }

    // Nothing (more) found
    return RecId{-1, -1};
}