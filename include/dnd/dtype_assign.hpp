//
// Copyright (C) 2011 Mark Wiebe (mwwiebe@gmail.com)
// All rights reserved.
//
// This is unreleased proprietary software.
//
#ifndef _DTYPE_ASSIGN_HPP_
#define _DTYPE_ASSIGN_HPP_

#include <dnd/dtype.hpp>

namespace dnd {

// Assign one element where src and dst have the same dtype
void dtype_assign(void *dst, const void *src, dtype dt);
// Assign one element where src and dst have different dtypes
void dtype_assign(void *dst, const void *src, dtype dst_dt, dtype src_dt);

} // namespace dnd

#endif//_DTYPE_ASSIGN_HPP_