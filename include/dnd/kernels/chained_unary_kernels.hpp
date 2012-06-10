//
// Copyright (C) 2012 Continuum Analytics
// All rights reserved.
//
#ifndef _DND__CHAINED_UNARY_KERNELS_HPP_
#define _DND__CHAINED_UNARY_KERNELS_HPP_

#include <vector>
#include <deque>

#include <dnd/kernels/chained_unary_kernels.hpp>
#include <dnd/buffer_storage.hpp>

namespace dnd {

/**
 * This is a unary kernel function + auxiliary data for chaining two
 * kernel functions together, using a single intermediate buffer.
 *
 * Example usage:
 *   void produce_kernel(intptr_t dst_fixedstride, intptr_t src_fixedstride, kernel_instance<unary_operation_t>& out_kernel)
 *   {
 *       // Set the kernel function
 *       out_kernel.kernel = &chained_2_unary_kernel;
 *       // Allocate the auxiliary data for the kernel
 *       make_auxiliary_data<chained_2_unary_kernel_auxdata>(out_kernel.auxdata);
 *       // Get a reference to the auxiliary data just allocated
 *       chained_2_unary_kernel_auxdata &auxdata = out_kernel.auxdata.get<chained_2_unary_kernel_auxdata>();
 *       // Allocate the buffering memory
 *       auxdata.buf.allocate(intermediate_element_size);
 *       // Get the two kernels in the chain
 *       produce_first_kernel(auxdata.buf.element_size(), src_fixedstride, auxdata.kernels[0]);
 *       produce_second_kernel(dst_fixedstride, auxdata.buf.element_size(), auxdata.kernels[1]);
 *   }
 */
void chained_2_unary_kernel(char *dst, intptr_t dst_stride, const char *src, intptr_t src_stride,
                        intptr_t count, const AuxDataBase *auxdata);
struct chained_2_unary_kernel_auxdata {
    kernel_instance<unary_operation_t> kernels[2];
    buffer_storage buf;
};

void chained_unary_kernel(char *dst, intptr_t dst_stride, const char *src, intptr_t src_stride,
                        intptr_t count, const AuxDataBase *auxdata);
struct chained_unary_kernel_auxdata {
    // We use raw heap-allocated arrays for performance.
    // If we were using C++11, would use unique_ptr<T[]>.
    kernel_instance<unary_operation_t>* m_kernels;
    buffer_storage* m_bufs;
    int m_buf_count;

    chained_unary_kernel_auxdata() {
        m_kernels = 0;
        m_bufs = 0;
    }

    ~chained_unary_kernel_auxdata() {
        delete[] m_kernels;
        delete[] m_bufs;
    }

    void init(int buf_count) {
        m_kernels = new kernel_instance<unary_operation_t>[buf_count + 1];
        try {
            m_bufs = new buffer_storage[buf_count];
        } catch(const std::exception&) {
            delete[] m_kernels;
            m_kernels = 0;
            throw;
        }
        m_buf_count = buf_count;
    }
};

/**
 * Given a size-N deque of kernel instances and a size-(N-1) vector
 * of the intermediate element sizes, creates a kernel which chains
 * them all together through intermediate buffers.
 *
 * The deque is used instead of vector because kernel_instance's shouldn't
 * be copied unless you want an expensive copy operation, and we can't rely
 * on C++11 move semantics for this library.
 *
 * For efficiency, the kernels are swapped out of the deque instead of copied,
 * so the deque 'kernels' no longer contains them on exit.
 */
void make_chained_unary_kernel(std::deque<kernel_instance<unary_operation_t> >& kernels,
                    std::deque<intptr_t>& element_sizes, kernel_instance<unary_operation_t>& out_kernel);

/**
 * This uses push_front calls on the output kernels and element_sizes
 * deques to create a chain of kernels which can transform the dtype's
 * storage_dtype values into its value_dtype values. It assumes
 * contiguous arrays are used for the intermediate buffers.
 *
 * This function assumes 'dt' is an expression_kind dtype, the
 * caller must verify this before calling.
 */
void push_front_dtype_storage_to_value_kernels(const dnd::dtype& dt,
                    intptr_t dst_fixedstride, intptr_t src_fixedstride,
                    std::deque<kernel_instance<unary_operation_t> >& out_kernels,
                    std::deque<intptr_t>& out_element_sizes);

/**
 * This uses push_back calls on the output kernels and element_sizes
 * deques to create a chain of kernels which can transform the dtype's
 * value_dtype values into its storage_dtype values. It assumes
 * contiguous arrays are used for the intermediate buffers.
 *
 * This function assumes 'dt' is an expression_kind dtype, the
 * caller must verify this before calling.
 */
void push_back_dtype_value_to_storage_kernels(const dnd::dtype& dt,
                    intptr_t dst_fixedstride, intptr_t src_fixedstride,
                    std::deque<kernel_instance<unary_operation_t> >& out_kernels,
                    std::deque<intptr_t>& out_element_sizes);

} // namespace dnd

#endif // _DND__CHAINED_UNARY_KERNELS_HPP_