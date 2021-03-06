//
// Copyright (C) 2011-14 Mark Wiebe, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cmath>

#include "inc_gtest.hpp"

#include <dynd/array.hpp>
#include <dynd/func/rolling_arrfunc.hpp>
#include <dynd/kernels/reduction_kernels.hpp>
#include <dynd/types/arrfunc_type.hpp>
#include <dynd/func/lift_reduction_arrfunc.hpp>
#include <dynd/func/call_callable.hpp>

using namespace std;
using namespace dynd;

TEST(Rolling, BuiltinSum_Kernel) {
    nd::arrfunc sum_1d =
        kernels::make_builtin_sum1d_arrfunc(float64_type_id);
    nd::arrfunc rolling_sum = make_rolling_arrfunc(sum_1d, 4);

    double adata[] = {1, 3, 7, 2, 9, 4, -5, 100, 2, -20, 3, 9, 18};
    nd::array a = adata;
    nd::array b = rolling_sum(a);
    EXPECT_EQ(ndt::type("strided * real"), b.get_type());
    for (int i = 0; i < 3; ++i) {
        EXPECT_TRUE(DYND_ISNAN(b(i).as<double>()));
    }
    for (int i = 3, i_end = (int)b.get_dim_size(); i < i_end; ++i) {
        double s = 0;
        for (int j = i-3; j <= i; ++j) {
            s += adata[j];
        }
        EXPECT_EQ(s, b(i).as<double>());
    }
}

TEST(Rolling, BuiltinMean_Kernel) {
    nd::arrfunc mean_1d =
        kernels::make_builtin_mean1d_arrfunc(float64_type_id, 0);
    nd::arrfunc rolling_sum = make_rolling_arrfunc(mean_1d, 4);

    double adata[] = {1, 3, 7, 2, 9, 4, -5, 100, 2, -20, 3, 9, 18};
    nd::array a = adata;
    nd::array b = rolling_sum(a);
    EXPECT_EQ(ndt::type("strided * real"), b.get_type());
    for (int i = 0; i < 3; ++i) {
        EXPECT_TRUE(DYND_ISNAN(b(i).as<double>()));
    }
    for (int i = 3, i_end = (int)b.get_dim_size(); i < i_end; ++i) {
        double s = 0;
        for (int j = i-3; j <= i; ++j) {
            s += adata[j];
        }
        EXPECT_EQ(s / 4, b(i).as<double>());
    }
}
