//
// Copyright (C) 2011-12, Dynamic NDArray Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/dtypes/struct_dtype.hpp>
#include <dynd/dtypes/dtype_alignment.hpp>
#include <dynd/shape_tools.hpp>
#include <dynd/exceptions.hpp>

using namespace std;
using namespace dynd;

dynd::struct_dtype::struct_dtype(const std::vector<dtype>& fields, const std::vector<std::string>& field_names)
    : m_fields(fields), m_field_names(field_names), m_metadata_offsets(fields.size())
{
    if (fields.size() != field_names.size()) {
        throw runtime_error("The field names for a struct dtypes must match the size of the field dtypes");
    }

    // Calculate the needed element alignment
    size_t metadata_offset = fields.size() * sizeof(size_t);
    m_alignment = 1;
    m_memory_management = pod_memory_management;
    for (size_t i = 0, i_end = fields.size(); i != i_end; ++i) {
        size_t field_alignment = fields[i].alignment();
        // Accumulate the biggest field alignment as the dtype alignment
        if (field_alignment > m_alignment) {
            m_alignment = field_alignment;
        }
        // Accumulate the correct memory management
        // TODO: Handle object, and object+blockref memory management types as well
        if (fields[i].get_memory_management() == blockref_memory_management) {
            m_memory_management = blockref_memory_management;
        }
        // Calculate the metadata offsets
        m_metadata_offsets[i] = metadata_offset;
        metadata_offset += m_fields[i].extended() ? m_fields[i].extended()->get_metadata_size() : 0;
    }
    m_metadata_size = metadata_offset;
}

size_t dynd::struct_dtype::get_default_element_size(int ndim, const intptr_t *shape) const
{
    // Default layout is to match the field order - could reorder the elements for more efficient packing
    size_t s = 0;
    for (size_t i = 0, i_end = m_fields.size(); i != i_end; ++i) {
        s = inc_to_alignment(s, m_fields[i].alignment());
        if (m_fields[i].extended()) {
            s += m_fields[i].extended()->get_default_element_size(ndim, shape);
        } else {
            s += m_fields[i].element_size();
        }
    }
    s = inc_to_alignment(s, m_alignment);
    return s;
}


void dynd::struct_dtype::print_element(std::ostream& o, const char *data, const char *metadata) const
{
    const size_t *offsets = reinterpret_cast<const size_t *>(metadata);
    o << "[";
    for (size_t i = 0, i_end = m_fields.size(); i != i_end; ++i) {
        m_fields[i].print_element(o, data + offsets[i], metadata + m_metadata_offsets[i]);
        if (i != i_end - 1) {
            o << ", ";
        }
    }
    o << "]";
}

void dynd::struct_dtype::print_dtype(std::ostream& o) const
{
    o << "struct<fields=(";
    for (size_t i = 0, i_end = m_fields.size(); i != i_end; ++i) {
        o << m_fields[i];
        if (i != i_end - 1) {
            o << ", ";
        }
    }
    o << ")>";
}

dtype dynd::struct_dtype::apply_linear_index(int nindices, const irange *indices, int current_i, const dtype& root_dt) const
{
    if (nindices == 0) {
        return dtype(this);
    } else {
        bool remove_dimension;
        intptr_t start_index, index_stride, dimension_size;
        apply_single_linear_index(*indices, m_fields.size(), current_i, &root_dt, remove_dimension, start_index, index_stride, dimension_size);
        if (remove_dimension) {
            return m_fields[start_index];
        } else {
            // Take the subset of the fixed fields in-place
            std::vector<dtype> fields(dimension_size);
            std::vector<std::string> field_names(dimension_size);

            for (intptr_t i = 0; i < dimension_size; ++i) {
                intptr_t idx = start_index + i * index_stride;
                fields[i] = m_fields[idx].apply_linear_index(nindices-1, indices+1, current_i+1, root_dt);
                field_names[i] = m_field_names[idx];
            }

            return dtype(new struct_dtype(fields, field_names));
        }
    }
}

void dynd::struct_dtype::get_shape(int i, std::vector<intptr_t>& out_shape) const
{
    // Ensure the output shape is big enough
    while (out_shape.size() <= (size_t)i) {
        out_shape.push_back(shape_signal_uninitialized);
    }

    // Adjust the current shape if necessary
    switch (out_shape[i]) {
        case shape_signal_uninitialized:
            out_shape[i] = m_fields.size();
            break;
        case shape_signal_varying:
            break;
        default:
            if ((size_t)out_shape[i] != m_fields.size()) {
                out_shape[i] = shape_signal_varying;
            }
            break;
    }

    // Process the later shape values
    for (size_t j = 0; j < m_fields.size(); ++j) {
        if (m_fields[j].extended()) {
            m_fields[j].extended()->get_shape(i+1, out_shape);
        }
    }
}

bool dynd::struct_dtype::is_lossless_assignment(const dtype& dst_dt, const dtype& src_dt) const
{
    if (dst_dt.extended() == this) {
        if (src_dt.extended() == this) {
            return true;
        } else if (src_dt.type_id() == struct_type_id) {
            return *dst_dt.extended() == *src_dt.extended();
        }
    }

    return false;
}

void dynd::struct_dtype::get_single_compare_kernel(single_compare_kernel_instance& DYND_UNUSED(out_kernel)) const
{
    throw runtime_error("struct_dtype::get_single_compare_kernel is unimplemented"); 
}

void dynd::struct_dtype::get_dtype_assignment_kernel(const dtype& DYND_UNUSED(dst_dt), const dtype& DYND_UNUSED(src_dt),
                assign_error_mode DYND_UNUSED(errmode),
                unary_specialization_kernel_instance& DYND_UNUSED(out_kernel)) const
{
    throw runtime_error("struct_dtype::get_dtype_assignment_kernel is unimplemented"); 
}

bool dynd::struct_dtype::operator==(const extended_dtype& rhs) const
{
    if (this == &rhs) {
        return true;
    } else if (rhs.type_id() != struct_type_id) {
        return false;
    } else {
        const struct_dtype *dt = static_cast<const struct_dtype*>(&rhs);
        return m_alignment == dt->m_alignment &&
                m_memory_management == dt->m_memory_management &&
                m_fields == dt->m_fields;
    }
}

size_t dynd::struct_dtype::get_metadata_size() const
{
    return m_metadata_size;
}

void dynd::struct_dtype::metadata_default_construct(char *metadata, int ndim, const intptr_t* shape) const
{
    // Validate that the shape is ok
    if (ndim > 0) {
        if (shape[0] >= 0 && shape[0] != m_fields.size()) {
            stringstream ss;
            ss << "Cannot construct dynd object of dtype " << dtype(this);
            ss << " with dimension size " << shape[0] << ", the size must be " << m_fields.size();
            throw runtime_error(ss.str());
        }
    }

    size_t *offsets = reinterpret_cast<size_t *>(metadata);
    size_t offs = 0;
    for (size_t i = 0; i < m_fields.size(); ++i) {
        const dtype& field_dt = m_fields[i];
        offs = inc_to_alignment(offs, field_dt.alignment());
        offsets[i] = offs;
        if (field_dt.extended()) {
            try {
                field_dt.extended()->metadata_default_construct(
                            metadata + m_metadata_offsets[i], ndim, shape);
            } catch(...) {
                // Since we're explicitly controlling the memory, need to manually do the cleanup too
                for (size_t j = 0; j < i; ++j) {
                    if (m_fields[j].extended()) {
                        m_fields[j].extended()->metadata_destruct(metadata + m_metadata_offsets[i]);
                    }
                }
                throw;
            }
            offs += m_fields[i].extended()->get_default_element_size(ndim, shape);
        } else {
            offs += m_fields[i].element_size();
        }
    }
}

void dynd::struct_dtype::metadata_destruct(char *metadata) const
{
    for (size_t i = 0; i < m_fields.size(); ++i) {
        const dtype& field_dt = m_fields[i];
        if (field_dt.extended()) {
            field_dt.extended()->metadata_destruct(metadata + m_metadata_offsets[i]);
        }
    }
}

void dynd::struct_dtype::metadata_debug_dump(const char *metadata, std::ostream& o, const std::string& indent) const
{
    const size_t *offsets = reinterpret_cast<const size_t *>(metadata);
    o << indent << " field offsets: ";
    for (size_t i = 0, i_end = m_fields.size(); i != i_end; ++i) {
        o << offsets[i];
        if (i != i_end - 1) {
            o << ", ";
        }
    }
    o << "\n";
}