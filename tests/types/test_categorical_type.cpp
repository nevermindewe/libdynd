//
// Copyright (C) 2011-14 Mark Wiebe, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <iostream>
#include <sstream>
#include <stdexcept>
#include "inc_gtest.hpp"

#include <dynd/array.hpp>
#include <dynd/types/categorical_type.hpp>
#include <dynd/types/fixedstring_type.hpp>
#include <dynd/types/string_type.hpp>
#include <dynd/types/convert_type.hpp>
#include <dynd/array_range.hpp>

using namespace std;
using namespace dynd;

TEST(CategoricalDType, Create) {
    const char *a_vals[] = {"foo", "bar", "baz"};
    nd::array a = nd::empty(3, ndt::make_fixedstring(3, string_encoding_ascii));
    a.vals() = a_vals;

    ndt::type d;
    d = ndt::make_categorical(a);
    EXPECT_EQ(categorical_type_id, d.get_type_id());
    EXPECT_EQ(custom_kind, d.get_kind());
    EXPECT_EQ(1u, d.get_data_alignment());
    EXPECT_EQ(1u, d.get_data_size());
    EXPECT_FALSE(d.is_expression());
    EXPECT_EQ(ndt::make_type<uint8_t>(), d.p("storage_type").as<ndt::type>());
    EXPECT_EQ(a.get_dtype(), d.p("category_type").as<ndt::type>());

    // With <= 256 categories, storage is a uint8
    a = nd::range(256);
    d = ndt::make_categorical(a);
    EXPECT_EQ(1u, d.get_data_alignment());
    EXPECT_EQ(1u, d.get_data_size());
    EXPECT_EQ(ndt::make_type<uint8_t>(), d.p("storage_type").as<ndt::type>());
    EXPECT_EQ(ndt::make_type<int32_t>(), d.p("category_type").as<ndt::type>());

    // With <= 65536 categories, storage is a uint16
    a = nd::range(257);
    d = ndt::make_categorical(a);
    EXPECT_EQ(2u, d.get_data_alignment());
    EXPECT_EQ(2u, d.get_data_size());
    a = nd::range(65536);
    d = ndt::make_categorical(a);
    EXPECT_EQ(2u, d.get_data_alignment());
    EXPECT_EQ(2u, d.get_data_size());
    EXPECT_EQ(ndt::make_type<uint16_t>(), d.p("storage_type").as<ndt::type>());
    EXPECT_EQ(ndt::make_type<int32_t>(), d.p("category_type").as<ndt::type>());

    // Otherwise, storage is a uint32
    a = nd::range(65537);
    d = ndt::make_categorical(a);
    EXPECT_EQ(4u, d.get_data_alignment());
    EXPECT_EQ(4u, d.get_data_size());
    EXPECT_EQ(ndt::make_type<uint32_t>(), d.p("storage_type").as<ndt::type>());
    EXPECT_EQ(ndt::make_type<int32_t>(), d.p("category_type").as<ndt::type>());
}

TEST(CategoricalDType, Convert) {
    const char *a_vals[] = {"foo", "bar", "baz"};
    nd::array a = nd::empty(3, ndt::make_fixedstring(3, string_encoding_ascii));
    a.vals() = a_vals;

    ndt::type cd = ndt::make_categorical(a);
    ndt::type sd = ndt::make_string(string_encoding_utf_8);

    // String conversions report false, so that assignments encodings
    // get validated on assignment
    EXPECT_FALSE(is_lossless_assignment(sd, cd));
    EXPECT_FALSE(is_lossless_assignment(cd, sd));

    // This operation was crashing, hence the test
    ndt::type cvt = ndt::make_convert(sd, cd);
    EXPECT_EQ(cd, cvt.operand_type());
    EXPECT_EQ(sd, cvt.value_type());
}

TEST(CategoricalDType, Compare) {
    const char *a_vals[] = {"foo", "bar", "baz"};
    nd::array a = nd::empty(3, ndt::make_fixedstring(3, string_encoding_ascii));
    a.vals() = a_vals;

    const char *b_vals[] = {"foo", "bar"};
    nd::array b = nd::empty(2, ndt::make_fixedstring(3, string_encoding_ascii));
    b.vals() = b_vals;

    ndt::type da = ndt::make_categorical(a);
    ndt::type da2 = ndt::make_categorical(a);
    ndt::type db = ndt::make_categorical(b);

    EXPECT_EQ(da, da);
    EXPECT_EQ(da, da2);
    EXPECT_NE(da, db);

    nd::array i = nd::empty(3, ndt::make_type<int32_t>());
    i(0).vals() = 0;
    i(1).vals() = 10;
    i(2).vals() = 100;

    ndt::type di = ndt::make_categorical(i);
    EXPECT_FALSE(da == di);
}

TEST(CategoricalDType, Unique) {
    const char *a_vals[] = {"foo", "bar", "foo"};
    nd::array a = nd::empty(3, ndt::make_fixedstring(3, string_encoding_ascii));
    a.vals() = a_vals;

    EXPECT_THROW(ndt::make_categorical(a), std::runtime_error);

    int i_vals[] = {0, 10, 10};
    nd::array i = i_vals;

    EXPECT_THROW(ndt::make_categorical(i), std::runtime_error);
}

TEST(CategoricalDType, FactorFixedString) {
    const char *string_cats_vals[] = {"bar", "foo"};
    nd::array string_cats = nd::empty(2, ndt::make_fixedstring(3, string_encoding_ascii));
    string_cats.vals() = string_cats_vals;

    const char *a_vals[] = {"foo", "bar", "foo"};
    nd::array a = nd::empty(3, ndt::make_fixedstring(3, string_encoding_ascii));
    a.vals() = a_vals;

    ndt::type da = ndt::factor_categorical(a);
    EXPECT_EQ(ndt::make_categorical(string_cats), da);
}

TEST(CategoricalDType, FactorString) {
    const char *cats_vals[] = {"bar", "foo", "foot"};
    const char *a_vals[] = {"foo", "bar", "foot", "foo", "bar"};
    nd::array cats = cats_vals, a = a_vals;

    ndt::type da = ndt::factor_categorical(a);
    EXPECT_EQ(ndt::make_categorical(cats), da);
}

TEST(CategoricalDType, FactorStringLonger) {
    const char *cats_vals[] = {"a", "abcdefghijklmnopqrstuvwxyz", "bar", "foo", "foot", "z"};
    const char *a_vals[] = {"foo", "bar", "foot", "foo", "bar", "abcdefghijklmnopqrstuvwxyz",
                    "foot", "foo", "z", "a", "abcdefghijklmnopqrstuvwxyz"};
    ndt::type da = ndt::factor_categorical(a_vals);
    EXPECT_EQ(ndt::make_categorical(cats_vals), da);
}

TEST(CategoricalDType, FactorInt) {
    int int_cats_vals[] = {0, 10};
    nd::array int_cats = nd::empty(2, ndt::make_type<int32_t>());
    int_cats.vals() = int_cats_vals;

    int i_vals[] = {10, 10, 0};
    nd::array i = nd::empty(3, ndt::make_type<int32_t>());
    i.vals() = i_vals;

    ndt::type di = ndt::factor_categorical(i);
    EXPECT_EQ(ndt::make_categorical(int_cats), di);
}

TEST(CategoricalDType, Values) {
    const char *a_vals[] = {"foo", "bar", "baz"};
    nd::array a = nd::empty(3, ndt::make_fixedstring(3, string_encoding_ascii));
    a.vals() = a_vals;

    ndt::type dt = ndt::make_categorical(a);

    EXPECT_EQ(0u, static_cast<const categorical_type*>(dt.extended())->get_value_from_category(a(0)));
    EXPECT_EQ(1u, static_cast<const categorical_type*>(dt.extended())->get_value_from_category(a(1)));
    EXPECT_EQ(2u, static_cast<const categorical_type*>(dt.extended())->get_value_from_category(a(2)));
    EXPECT_EQ(0u, static_cast<const categorical_type*>(dt.extended())->get_value_from_category("foo"));
    EXPECT_EQ(1u, static_cast<const categorical_type*>(dt.extended())->get_value_from_category("bar"));
    EXPECT_EQ(2u, static_cast<const categorical_type*>(dt.extended())->get_value_from_category("baz"));
    EXPECT_THROW(static_cast<const categorical_type*>(dt.extended())->get_value_from_category("aaa"), std::runtime_error);
    EXPECT_THROW(static_cast<const categorical_type*>(dt.extended())->get_value_from_category("ddd"), std::runtime_error);
    EXPECT_THROW(static_cast<const categorical_type*>(dt.extended())->get_value_from_category("zzz"), std::runtime_error);
}

TEST(CategoricalDType, ValuesLonger) {
    const char *cats_vals[] = {"foo", "abcdefghijklmnopqrstuvwxyz", "z", "bar", "a", "foot"};
    const char *a_vals[] = {"foo", "z", "abcdefghijklmnopqrstuvwxyz",
                    "z", "bar", "a", "foot", "a", "abcdefghijklmnopqrstuvwxyz", "foo", "bar", "foo", "foot"};
    uint32_t a_uints[] = {0, 2, 1, 2, 3, 4, 5, 4, 1, 0, 3, 0, 5};
    int cats_count = sizeof(cats_vals) / sizeof(cats_vals[0]);
    int a_count = sizeof(a_uints) / sizeof(a_uints[0]);

    ndt::type dt = ndt::make_categorical(cats_vals);
    nd::array a = nd::array(a_vals).ucast(dt).eval();
    nd::array a_view = a.p("ints");

    // Check that the categories got the right values
    for (int i = 0; i < cats_count; ++i) {
        EXPECT_EQ((uint32_t)i, static_cast<const categorical_type*>(dt.extended())->get_value_from_category(cats_vals[i]));
    }
    // Check that everything in 'a' is right
    for (int i = 0; i < a_count; ++i) {
        EXPECT_EQ(a_vals[i], a(i).as<string>());
        EXPECT_EQ(a_uints[i], a_view(i).as<uint32_t>());
    }
}

TEST(CategoricalDType, AssignFixedString) {
    const char *cat_vals[] = {"foo", "bar", "baz"};
    nd::array cat = nd::empty(3, ndt::make_fixedstring(3, string_encoding_ascii));
    cat.vals() = cat_vals;

    ndt::type dt = ndt::make_categorical(cat);

    nd::array a = nd::empty(3, dt);
    a.val_assign(cat);
    EXPECT_EQ("foo", a(0).as<string>());
    EXPECT_EQ("bar", a(1).as<string>());
    EXPECT_EQ("baz", a(2).as<string>());
    a(0).vals() = cat(2);
    EXPECT_EQ("baz", a(0).as<string>());

    cat(0).vals() = "zzz";
    EXPECT_THROW(a(0).vals() = cat(0), std::runtime_error);

    nd::array tmp = nd::empty(3, cat.get_type().at(0));
    tmp.val_assign(a);
    EXPECT_EQ("baz", tmp(0).as<string>());
    EXPECT_EQ("bar", tmp(1).as<string>());
    EXPECT_EQ("baz", tmp(2).as<string>());
    tmp(0).vals() = a(1);
    EXPECT_EQ("bar", tmp(0).as<string>());
    tmp(0).vals() = "foo";
    EXPECT_EQ("foo", tmp(0).as<string>());
}

TEST(CategoricalDType, AssignInt) {
    int32_t cat_vals[] = {10, 100, 1000};
    nd::array cat = cat_vals;

    ndt::type dt = ndt::make_categorical(cat);

    nd::array a = nd::empty(3, dt);
    a.val_assign(cat);
    EXPECT_EQ(10, a(0).as<int32_t>());
    EXPECT_EQ(100, a(1).as<int32_t>());
    EXPECT_EQ(1000, a(2).as<int32_t>());
    a(0).vals() = cat(2);
    EXPECT_EQ(1000, a(0).as<int32_t>());

    // TODO implicit conversion?
    //a(0).vals() = string("bar");
    //cout << a << endl;

    nd::array tmp = nd::empty(3, cat.get_type().at(0));
    tmp.val_assign(a);
    EXPECT_EQ(1000, tmp(0).as<int32_t>());
    EXPECT_EQ(100, tmp(1).as<int32_t>());
    EXPECT_EQ(1000, tmp(2).as<int32_t>());
    tmp(0).vals() = a(1);
    EXPECT_EQ(100, tmp(0).as<int32_t>());

}

TEST(CategoricalDType, AssignRange) {
    const char *cat_vals[] = {"foo", "bar", "baz"};
    nd::array cat = nd::empty(3, ndt::make_fixedstring(3, string_encoding_ascii));
    cat.vals() = cat_vals;

    ndt::type dt = ndt::make_categorical(cat);

    nd::array a = nd::empty(9, dt);
    nd::array b = a(0 <= irange() < 3);
    b.val_assign(cat);
    nd::array c = a(3 <= irange() < 6 );
    c.val_assign(cat(0));
    nd::array d = a(6 <= irange().by(2) < 9 );
    d.val_assign(cat(1));
    a(7).vals() = cat(2);

    EXPECT_EQ("foo", a(0).as<string>());
    EXPECT_EQ("bar", a(1).as<string>());
    EXPECT_EQ("baz", a(2).as<string>());
    EXPECT_EQ("foo", a(3).as<string>());
    EXPECT_EQ("foo", a(4).as<string>());
    EXPECT_EQ("foo", a(5).as<string>());
    EXPECT_EQ("bar", a(6).as<string>());
    EXPECT_EQ("baz", a(7).as<string>());
    EXPECT_EQ("bar", a(8).as<string>());
}

TEST(CategoricalDType, CategoriesProperty) {
    const char *cats_vals[] = {"this", "is", "a", "test"};
    nd::array cats = cats_vals;
    ndt::type cd = ndt::make_categorical(cats_vals);
    EXPECT_TRUE(cats.equals_exact(cd.p("categories")));
}

TEST(CategoricalDType, AssignFromOther) {
    int cats_values[] = {3, 6, 100, 1000};
    ndt::type cd = ndt::make_categorical(cats_values);
    int16_t a_values[] = {6, 3, 100, 3, 1000, 100, 6, 1000};
    nd::array a = nd::array(a_values).ucast(cd);
    EXPECT_EQ(ndt::make_strided_dim(ndt::make_convert(cd, ndt::make_type<int16_t>())),
                    a.get_type());
    a = a.eval();
    EXPECT_EQ(ndt::make_strided_dim(cd), a.get_type());
    EXPECT_EQ(6,    a(0).as<int>());
    EXPECT_EQ(3,    a(1).as<int>());
    EXPECT_EQ(100,  a(2).as<int>());
    EXPECT_EQ(3,    a(3).as<int>());
    EXPECT_EQ(1000, a(4).as<int>());
    EXPECT_EQ(100,  a(5).as<int>());
    EXPECT_EQ(6,    a(6).as<int>());
    EXPECT_EQ(1000, a(7).as<int>());

    // Assignments from a few different input types
    a(3).vals() = "1000";
    EXPECT_EQ(1000, a(3).as<int>());
    a(4).vals() = 6.0;
    EXPECT_EQ(6, a(4).as<int>());
    a(5).vals() = (uint16_t)3;
    EXPECT_EQ(3, a(5).as<int>());
}

