//
// Copyright (C) 2011-12, Dynamic NDArray Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/ndobject.hpp>

using namespace std;
using namespace dynd;

dynd::ndobject::ndobject()
    : m_memblock()
{
}

void dynd::ndobject::swap(ndobject& rhs)
{
    m_memblock.swap(rhs.m_memblock);
}

template<class T>
inline typename enable_if<is_dtype_scalar<T>::value, memory_block_ptr>::type
make_immutable_builtin_scalar_ndobject(const T& value)
{
    char *data_ptr = NULL;
    memory_block_ptr result = make_ndobject_memory_block(0, sizeof(T), scalar_align_of<T>::value, &data_ptr);
    *reinterpret_cast<T *>(data_ptr) = value;
    ndobject_preamble *ndo = reinterpret_cast<ndobject_preamble *>(result.get());
    ndo->m_dtype = reinterpret_cast<extended_dtype *>(type_id_of<T>::value);
    ndo->m_data_pointer = data_ptr;
    ndo->m_data_reference = NULL;
    ndo->m_flags = read_access_flag | immutable_access_flag;
    return result;
}

// Constructors from C++ scalars
dynd::ndobject::ndobject(dynd_bool value)
    : m_memblock(make_immutable_builtin_scalar_ndobject(value))
{
}
dynd::ndobject::ndobject(bool value)
    : m_memblock(make_immutable_builtin_scalar_ndobject(dynd_bool(value)))
{
}
dynd::ndobject::ndobject(signed char value)
    : m_memblock(make_immutable_builtin_scalar_ndobject(value))
{
}
dynd::ndobject::ndobject(short value)
    : m_memblock(make_immutable_builtin_scalar_ndobject(value))
{
}
dynd::ndobject::ndobject(int value)
    : m_memblock(make_immutable_builtin_scalar_ndobject(value))
{
}
dynd::ndobject::ndobject(long value)
    : m_memblock(make_immutable_builtin_scalar_ndobject(value))
{
}
dynd::ndobject::ndobject(long long value)
    : m_memblock(make_immutable_builtin_scalar_ndobject(value))
{
}
dynd::ndobject::ndobject(unsigned char value)
    : m_memblock(make_immutable_builtin_scalar_ndobject(value))
{
}
dynd::ndobject::ndobject(unsigned short value)
    : m_memblock(make_immutable_builtin_scalar_ndobject(value))
{
}
dynd::ndobject::ndobject(unsigned int value)
    : m_memblock(make_immutable_builtin_scalar_ndobject(value))
{
}
dynd::ndobject::ndobject(unsigned long value)
    : m_memblock(make_immutable_builtin_scalar_ndobject(value))
{
}
dynd::ndobject::ndobject(unsigned long long value)
    : m_memblock(make_immutable_builtin_scalar_ndobject(value))
{
}
dynd::ndobject::ndobject(float value)
    : m_memblock(make_immutable_builtin_scalar_ndobject(value))
{
}
dynd::ndobject::ndobject(double value)
    : m_memblock(make_immutable_builtin_scalar_ndobject(value))
{
}
dynd::ndobject::ndobject(std::complex<float> value)
    : m_memblock(make_immutable_builtin_scalar_ndobject(value))
{
}
dynd::ndobject::ndobject(std::complex<double> value)
    : m_memblock(make_immutable_builtin_scalar_ndobject(value))
{
}

std::ostream& dynd::operator<<(std::ostream& o, const ndobject& rhs)
{
    if (rhs.get_ndo() != NULL) {
        if (rhs.get_ndo()->is_builtin_dtype()) {
            print_builtin_scalar(rhs.get_ndo()->get_builtin_type_id(), o, rhs.get_ndo()->m_data_pointer);
        } else {
            rhs.get_ndo()->m_dtype->print_element(o, rhs.get_ndo()->m_data_pointer, rhs.get_ndo_meta());
        }
    } else {
        o << "<null>";
    }
    return o;
}