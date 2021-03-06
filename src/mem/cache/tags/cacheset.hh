/*
 * Copyright (c) 2013 ARM Limited
 * All rights reserved.
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2009 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Lisa Hsu
 */

/**
 * @file
 * Declaration of an associative set
 */

#ifndef __CACHESET_HH__
#define __CACHESET_HH__

#include <cassert>
#include <map>
#include "mem/cache/blk.hh" // base class

/**
 * An associative set of cache blocks.
 */
template <class Blktype>
class CacheSet
{
  public:
    /** The associativity of this set. */
    int assoc;

    /** Cache blocks in this set, maintained in LRU order 0 = MRU. */
    Blktype **blks;

    // Fangfei modified
    /** define a search key class for LNreg and RMT_ID */
    class CamKey{
    public:
        int rmtid;
        int P_bit;
        int lnreg;
        CamKey(int _rmtid, int _P_bit, int _lnreg):rmtid(_rmtid), P_bit(_P_bit), lnreg(_lnreg){}
        bool operator<(const CamKey& key) const
        {
             if (rmtid == key.rmtid) {
                 if (P_bit == key.P_bit){
                     return lnreg < key.lnreg;
                  } else
                     return P_bit < key.P_bit;
              } else
                   return rmtid < key.rmtid;
         }
    };

    /** hash table for highly associative search */
     /** Hash table type mapping lnreg to cache block pointers. */
    typedef typename std::map<CamKey, Blktype*> map_t;
    /** Iterator into the address hash table. */
    typedef typename map_t::const_iterator lnregIterator;

    /** The lnreg hash table. */
     map_t lnregMap;


    /**
     * Find a block matching the tag in this set.
     * @param way_id The id of the way that matches the tag.
     * @param tag The Tag to find.
     * @return Pointer to the block if found. Set way_id to assoc if none found
     */
    Blktype* findBlk(Addr tag, int& way_id) const ;
    Blktype* findBlk(Addr tag) const ;

    Blktype* findBlk(int rmtid, int P_bit, int lnreg, Addr tag) const;
    Blktype* mapLookup(int rmtid, int P_bit, int lnreg) const;
    bool inMap(int rmtid, int P_bit, int lnreg) const;

    CamKey setKey(int rmtid, int P_bit, int lnreg);

    /**
     * Move the given block to the head of the list.
     * @param blk The block to move.
     */
    void moveToHead(Blktype *blk);

    /**
     * Move the given block to the tail of the list.
     * @param blk The block to move
     */
    void moveToTail(Blktype *blk);

};

template <class Blktype>
Blktype*
CacheSet<Blktype>::findBlk(Addr tag, int& way_id) const
{
    /**
     * Way_id returns the id of the way that matches the block
     * If no block is found way_id is set to assoc.
     */
    way_id = assoc;
    for (int i = 0; i < assoc; ++i) {
        if (blks[i]->tag == tag && blks[i]->isValid()) {
            way_id = i;
            return blks[i];
        }
    }
    return NULL;
}

template <class Blktype>
Blktype*
CacheSet<Blktype>::findBlk(Addr tag) const
{
    int ignored_way_id;
    return findBlk(tag, ignored_way_id);
}

template <class Blktype>
Blktype*
CacheSet<Blktype>::findBlk(int rmtid, int P_bit, int lnreg, Addr tag) const
{
    Blktype * blk = mapLookup(rmtid, P_bit, lnreg);
    if (blk && blk->isValid() && (blk->tag == tag)) {
           return blk;
     }
    return 0;
}

template <class Blktype>
bool
CacheSet<Blktype>::inMap(int rmtid, int P_bit, int lnreg) const
{
   CacheBlk * blk = mapLookup(rmtid, P_bit, lnreg);
   if (blk) {
         return true;
    }
   return false;
}

template <class Blktype>
Blktype*
CacheSet<Blktype>::mapLookup(int rmtid, int P_bit, int lnreg) const
{
    CamKey key(rmtid, P_bit, lnreg);
    lnregIterator iter = lnregMap.find(key);
    if (iter != lnregMap.end()) {
        return (*iter).second;
    }
    return NULL;
}

template <class Blktype>
typename CacheSet<Blktype>::CamKey
CacheSet<Blktype>::setKey(int rmtid, int P_bit, int lnreg)
{
   CamKey key(rmtid, P_bit, lnreg);
   return key;
}


template <class Blktype>
void
CacheSet<Blktype>::moveToHead(Blktype *blk)
{
    // nothing to do if blk is already head
    if (blks[0] == blk)
        return;

    // write 'next' block into blks[i], moving up from MRU toward LRU
    // until we overwrite the block we moved to head.

    // start by setting up to write 'blk' into blks[0]
    int i = 0;
    Blktype *next = blk;

    do {
        assert(i < assoc);
        // swap blks[i] and next
        Blktype *tmp = blks[i];
        blks[i] = next;
        next = tmp;
        ++i;
    } while (next != blk);
}

template <class Blktype>
void
CacheSet<Blktype>::moveToTail(Blktype *blk)
{
    // nothing to do if blk is already tail
    if (blks[assoc - 1] == blk)
        return;

    // write 'next' block into blks[i], moving from LRU to MRU
    // until we overwrite the block we moved to tail.

    // start by setting up to write 'blk' into tail
    int i = assoc - 1;
    Blktype *next = blk;

    do {
        assert(i >= 0);
        // swap blks[i] and next
        Blktype *tmp = blks[i];
        blks[i] = next;
        next = tmp;
        --i;
    } while (next != blk);
}

#endif
