//
// Copyright (C) 2011-14 Mark Wiebe, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <stdexcept>
#include <vector>
#include <cstdlib>
#include <algorithm>

#include <dynd/memblock/zeroinit_memory_block.hpp>

using namespace std;
using namespace dynd;

namespace {
    struct zeroinit_memory_block {
        /** Every memory block object needs this at the front */
        memory_block_data m_mbd;
        intptr_t m_total_allocated_capacity;
        /** The malloc'd memory */
        vector<char *> m_memory_handles;
        /** The current malloc'd memory being doled out */
        char *m_memory_begin, *m_memory_current, *m_memory_end;

        /**
         * Allocates some new memory from which to dole out
         * more. Adds it to the memory handles vector.
         */
        void append_memory(intptr_t capacity_bytes)
        {
            m_memory_handles.push_back(NULL);
            m_memory_begin = reinterpret_cast<char *>(malloc(capacity_bytes));
            m_memory_handles.back() = m_memory_begin;
            if (m_memory_begin == NULL) {
                m_memory_handles.pop_back();
                throw bad_alloc();
            }
            m_memory_current = m_memory_begin;
            m_memory_end = m_memory_current + capacity_bytes;
            m_total_allocated_capacity += capacity_bytes;
        }

        zeroinit_memory_block(intptr_t initial_capacity_bytes)
            : m_mbd(1, zeroinit_memory_block_type), m_total_allocated_capacity(0),
                    m_memory_handles()
        {
            append_memory(initial_capacity_bytes);
        }

        ~zeroinit_memory_block()
        {
            for (size_t i = 0, i_end = m_memory_handles.size(); i != i_end; ++i) {
                free(m_memory_handles[i]);
            }
        }
    };
} // anonymous namespace

memory_block_ptr dynd::make_zeroinit_memory_block(intptr_t initial_capacity_bytes)
{
    zeroinit_memory_block *pmb = new zeroinit_memory_block(initial_capacity_bytes);
    return memory_block_ptr(reinterpret_cast<memory_block_data *>(pmb), false);
}

namespace dynd { namespace detail {

void free_zeroinit_memory_block(memory_block_data *memblock)
{
    zeroinit_memory_block *emb = reinterpret_cast<zeroinit_memory_block *>(memblock);
    delete emb;
}

static void allocate(memory_block_data *self, intptr_t size_bytes, intptr_t alignment, char **out_begin, char **out_end)
{
//    cout << "allocating " << size_bytes << " of memory with alignment " << alignment << endl;
    // Allocate new POD memory of the requested size and alignment
    zeroinit_memory_block *emb = reinterpret_cast<zeroinit_memory_block *>(self);
    char *begin = reinterpret_cast<char *>(
                    (reinterpret_cast<uintptr_t>(emb->m_memory_current) + alignment - 1) & ~(alignment - 1));
    char *end = begin + size_bytes;
    if (end > emb->m_memory_end) {
        emb->m_total_allocated_capacity -= emb->m_memory_end - emb->m_memory_current;
        // Allocate memory to double the amount used so far, or the requested size, whichever is larger
        // NOTE: We're assuming malloc produces memory which has good enough alignment for anything
        emb->append_memory(max(emb->m_total_allocated_capacity, size_bytes));
        begin = emb->m_memory_begin;
        end = begin + size_bytes;
    }

    // Indicate where to allocate the next memory
    emb->m_memory_current = end;

    // Zero-initialize the memory
    memset(begin, 0, end - begin);

    // Return the allocated memory
    *out_begin = begin;
    *out_end = end;
//    cout << "allocated at address " << (void *)begin << endl;
}

static void resize(memory_block_data *self, intptr_t size_bytes, char **inout_begin, char **inout_end)
{
    // Resizes previously allocated POD memory to the requested size
    zeroinit_memory_block *emb = reinterpret_cast<zeroinit_memory_block *>(self);
//    cout << "resizing memory " << (void *)*inout_begin << " / " << (void *)*inout_end << " from size " << (*inout_end - *inout_begin) << " to " << size_bytes << endl;
//    cout << "memory state before " << (void *)emb->m_memory_begin << " / " << (void *)emb->m_memory_current << " / " << (void *)emb->m_memory_end << endl;
    if (*inout_end != emb->m_memory_current) {
        // Simple sanity check
        throw runtime_error("zeroinit_memory_block resize must be called only using the most recently allocated memory");
    }
    char *end = *inout_begin + size_bytes;
    if (end <= emb->m_memory_end) {
        // If it fits, just adjust the current allocation point
        emb->m_memory_current = end;
        // Zero-initialize any newly allocated memory
        if (end > *inout_end) {
            memset(*inout_end, 0, end - *inout_end);
        }
        *inout_end = end;
    } else {
        // If it doesn't fit, need to copy to newly malloc'd memory
		char *old_current = *inout_begin, *old_end = *inout_end;
        intptr_t old_size_bytes = *inout_end - *inout_begin;
        // Allocate memory to double the amount used so far, or the requested size, whichever is larger
        // NOTE: We're assuming malloc produces memory which has good enough alignment for anything
        emb->append_memory(max(emb->m_total_allocated_capacity, size_bytes));
        memcpy(emb->m_memory_begin, *inout_begin, old_size_bytes);
        end = emb->m_memory_begin + size_bytes;
        emb->m_memory_current = end;
        // Zero-initialize the newly allocated memory
        memset(emb->m_memory_begin + old_size_bytes, 0, size_bytes - old_size_bytes);

        *inout_begin = emb->m_memory_begin;
        *inout_end = end;
        emb->m_total_allocated_capacity -= old_end - old_current;
    }
//    cout << "memory state after " << (void *)emb->m_memory_begin << " / " << (void *)emb->m_memory_current << " / " << (void *)emb->m_memory_end << endl;
}

static void finalize(memory_block_data *self)
{
    // Finalizes POD memory so there are no more allocations
    zeroinit_memory_block *emb = reinterpret_cast<zeroinit_memory_block *>(self);
    
    if (emb->m_memory_current < emb->m_memory_end) {
        emb->m_total_allocated_capacity -= emb->m_memory_end - emb->m_memory_current;
    }
    emb->m_memory_begin = NULL;
    emb->m_memory_current = NULL;
    emb->m_memory_end = NULL;
}

static void reset(memory_block_data *self)
{
    // Resets the POD memory so it can reuse it from the start
    zeroinit_memory_block *emb = reinterpret_cast<zeroinit_memory_block *>(self);
   
    if (emb->m_memory_handles.size() > 1) {
        // If there are more than one allocated memory chunks,
        // throw them all away except the last
        for (size_t i = 0, i_end = emb->m_memory_handles.size() - 1; i != i_end; ++i) {
            free(emb->m_memory_handles[i]);
        }
        emb->m_memory_handles.front() = emb->m_memory_handles.back();
        emb->m_memory_handles.resize(1);
    }

    // Reset to use the whole chunk
    emb->m_memory_current = emb->m_memory_begin;
    emb->m_total_allocated_capacity = emb->m_memory_end - emb->m_memory_begin;
}

memory_block_pod_allocator_api zeroinit_memory_block_allocator_api = {
    &allocate,
    &resize,
    &finalize,
    &reset
};

}} // namespace dynd::detail

void dynd::zeroinit_memory_block_debug_print(const memory_block_data *memblock, std::ostream& o, const std::string& indent)
{
    const zeroinit_memory_block *emb = reinterpret_cast<const zeroinit_memory_block *>(memblock);
    if (emb->m_memory_begin != NULL) {
        o << indent << " allocated: " << emb->m_total_allocated_capacity << "\n";
    } else {
        o << indent << " finalized: " << emb->m_total_allocated_capacity << "\n";
    } 
}
