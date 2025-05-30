// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/widestring.h"

#include <limits.h>

#include <algorithm>
#include <iterator>
#include <vector>

#include "build/build_config.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/containers/contains.h"
#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/utf16.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace fxcrt {

TEST(WideString, ElementAccess) {
  const WideString empty;
  pdfium::span<const wchar_t> empty_span = empty.span();
  pdfium::span<const wchar_t> empty_span_with_terminator =
      empty.span_with_terminator();
  EXPECT_EQ(0u, empty_span.size());
  ASSERT_EQ(1u, empty_span_with_terminator.size());
  EXPECT_EQ(L'\0', empty_span_with_terminator[0]);

  const WideString abc(L"abc");
  EXPECT_EQ(L'a', abc[0]);
  EXPECT_EQ(L'b', abc[1]);
  EXPECT_EQ(L'c', abc[2]);
  EXPECT_DEATH_IF_SUPPORTED({ abc[4]; }, "");

  pdfium::span<const wchar_t> abc_span = abc.span();
  EXPECT_EQ(3u, abc_span.size());
  EXPECT_EQ(0, UNSAFE_TODO(wmemcmp(abc_span.data(), L"abc", 3)));

  pdfium::span<const wchar_t> abc_span_with_terminator =
      abc.span_with_terminator();
  EXPECT_EQ(4u, abc_span_with_terminator.size());
  EXPECT_EQ(0,
            UNSAFE_TODO(wmemcmp(abc_span_with_terminator.data(), L"abc", 4)));

  WideString mutable_abc = abc;
  EXPECT_EQ(abc.c_str(), mutable_abc.c_str());
  EXPECT_EQ(L'a', mutable_abc[0]);
  EXPECT_EQ(L'b', mutable_abc[1]);
  EXPECT_EQ(L'c', mutable_abc[2]);
  EXPECT_EQ(abc.c_str(), mutable_abc.c_str());
  EXPECT_EQ(L"abc", abc);

  const wchar_t* c_str = abc.c_str();
  mutable_abc.SetAt(0, L'd');
  EXPECT_EQ(c_str, abc.c_str());
  EXPECT_NE(c_str, mutable_abc.c_str());
  EXPECT_EQ(L"abc", abc);
  EXPECT_EQ(L"dbc", mutable_abc);

  mutable_abc.SetAt(1, L'e');
  EXPECT_EQ(L"abc", abc);
  EXPECT_EQ(L"dec", mutable_abc);

  mutable_abc.SetAt(2, L'f');
  EXPECT_EQ(L"abc", abc);
  EXPECT_EQ(L"def", mutable_abc);
  EXPECT_DEATH_IF_SUPPORTED({ mutable_abc.SetAt(3, L'g'); }, "");
}

TEST(WideString, Construct) {
  {
    // Copy-construct.
    WideString string1(L"abc");
    WideString string2(string1);
    EXPECT_EQ(L"abc", string1);
    EXPECT_EQ(L"abc", string2);
    EXPECT_EQ(2, string1.ReferenceCountForTesting());
    EXPECT_EQ(2, string2.ReferenceCountForTesting());
  }
  {
    // Move-construct.
    WideString string1(L"abc");
    WideString string2(std::move(string1));
    EXPECT_TRUE(string1.IsEmpty());
    EXPECT_EQ(L"abc", string2);
    EXPECT_EQ(0, string1.ReferenceCountForTesting());
    EXPECT_EQ(1, string2.ReferenceCountForTesting());
  }
}

TEST(WideString, Assign) {
  {
    // Copy-assign.
    WideString string1;
    EXPECT_EQ(0, string1.ReferenceCountForTesting());
    {
      WideString string2(L"abc");
      EXPECT_EQ(1, string2.ReferenceCountForTesting());

      string1 = string2;
      EXPECT_EQ(2, string1.ReferenceCountForTesting());
      EXPECT_EQ(2, string2.ReferenceCountForTesting());
    }
    EXPECT_EQ(1, string1.ReferenceCountForTesting());
  }
  {
    // Move-assign.
    WideString string1;
    EXPECT_EQ(0, string1.ReferenceCountForTesting());
    {
      WideString string2(L"abc");
      EXPECT_EQ(1, string2.ReferenceCountForTesting());

      string1 = std::move(string2);
      EXPECT_EQ(L"abc", string1);
      EXPECT_TRUE(string2.IsEmpty());
      EXPECT_EQ(1, string1.ReferenceCountForTesting());
      EXPECT_EQ(0, string2.ReferenceCountForTesting());
    }
    EXPECT_EQ(1, string1.ReferenceCountForTesting());
  }
  {
    // From wchar_t*.
    WideString string1 = L"abc";
    EXPECT_EQ(L"abc", string1);
    string1 = nullptr;
    EXPECT_TRUE(string1.IsEmpty());
    string1 = L"def";
    EXPECT_EQ(L"def", string1);
    string1 = L"";
    EXPECT_TRUE(string1.IsEmpty());
  }
  {
    // From WideStringView.
    WideString string1(WideStringView(L"abc"));
    EXPECT_EQ(L"abc", string1);
    string1 = WideStringView(L"");
    EXPECT_TRUE(string1.IsEmpty());
    string1 = WideStringView(L"def");
    EXPECT_EQ(L"def", string1);
  }
}

TEST(WideString, OperatorLT) {
  WideString empty;
  WideString a(L"a");
  WideString ab(L"ab");
  WideString abc(L"\x0110qq");  // Comes before despite endianness.
  WideString def(L"\x1001qq");  // Comes after despite endianness.
  WideStringView v_empty;
  WideStringView v_a(L"a");
  WideStringView v_ab(L"ab");
  WideStringView v_abc(L"\x0110qq");
  WideStringView v_def(L"\x1001qq");
  const wchar_t* const c_null = nullptr;
  const wchar_t* const c_empty = L"";
  const wchar_t* const c_a = L"a";
  const wchar_t* const c_ab = L"ab";
  const wchar_t* const c_abc = L"\x0110qq";
  const wchar_t* const c_def = L"\x1001qq";

  EXPECT_FALSE(empty < empty);
  EXPECT_FALSE(a < a);
  EXPECT_FALSE(abc < abc);
  EXPECT_FALSE(def < def);
  EXPECT_FALSE(c_null < empty);
  EXPECT_FALSE(c_empty < empty);
  EXPECT_FALSE(c_a < a);
  EXPECT_FALSE(c_abc < abc);
  EXPECT_FALSE(c_def < def);
  EXPECT_FALSE(empty < c_null);
  EXPECT_FALSE(empty < c_empty);
  EXPECT_FALSE(a < c_a);
  EXPECT_FALSE(abc < c_abc);
  EXPECT_FALSE(def < c_def);
  EXPECT_FALSE(empty < v_empty);
  EXPECT_FALSE(a < v_a);
  EXPECT_FALSE(abc < v_abc);
  EXPECT_FALSE(def < v_def);

  EXPECT_TRUE(empty < a);
  EXPECT_FALSE(a < empty);
  EXPECT_TRUE(c_null < a);
  EXPECT_TRUE(c_empty < a);
  EXPECT_FALSE(c_a < empty);
  EXPECT_TRUE(empty < c_a);
  EXPECT_FALSE(a < c_null);
  EXPECT_FALSE(a < c_empty);
  EXPECT_TRUE(empty < v_a);
  EXPECT_FALSE(a < v_empty);

  EXPECT_TRUE(empty < abc);
  EXPECT_FALSE(abc < empty);
  EXPECT_TRUE(c_null < abc);
  EXPECT_TRUE(c_empty < abc);
  EXPECT_FALSE(c_abc < empty);
  EXPECT_TRUE(empty < c_abc);
  EXPECT_FALSE(abc < c_null);
  EXPECT_FALSE(abc < c_empty);
  EXPECT_TRUE(empty < v_abc);
  EXPECT_FALSE(abc < v_empty);

  EXPECT_TRUE(empty < def);
  EXPECT_FALSE(def < empty);
  EXPECT_TRUE(c_null < def);
  EXPECT_TRUE(c_empty < def);
  EXPECT_FALSE(c_def < empty);
  EXPECT_TRUE(empty < c_def);
  EXPECT_FALSE(def < c_null);
  EXPECT_FALSE(def < c_empty);
  EXPECT_TRUE(empty < v_def);
  EXPECT_FALSE(def < v_empty);

  EXPECT_TRUE(a < abc);
  EXPECT_FALSE(abc < a);
  EXPECT_TRUE(c_a < abc);
  EXPECT_FALSE(c_abc < a);
  EXPECT_TRUE(a < c_abc);
  EXPECT_FALSE(abc < c_a);
  EXPECT_TRUE(a < v_abc);
  EXPECT_FALSE(abc < v_a);

  EXPECT_TRUE(a < def);
  EXPECT_FALSE(def < a);
  EXPECT_TRUE(c_a < def);
  EXPECT_FALSE(c_def < a);
  EXPECT_TRUE(a < c_def);
  EXPECT_FALSE(def < c_a);
  EXPECT_TRUE(a < v_def);
  EXPECT_FALSE(def < v_a);

  EXPECT_TRUE(abc < def);
  EXPECT_FALSE(def < abc);
  EXPECT_TRUE(c_abc < def);
  EXPECT_FALSE(c_def < abc);
  EXPECT_TRUE(abc < c_def);
  EXPECT_FALSE(def < c_abc);
  EXPECT_TRUE(abc < v_def);
  EXPECT_FALSE(def < v_abc);

  EXPECT_TRUE(a < ab);
  EXPECT_TRUE(a < c_ab);
  EXPECT_TRUE(a < v_ab);
  EXPECT_TRUE(c_a < ab);
  EXPECT_TRUE(c_a < v_ab);
  EXPECT_TRUE(v_a < c_ab);
  EXPECT_TRUE(v_a < v_ab);
}

TEST(WideString, OperatorEQ) {
  WideString null_string;
  EXPECT_TRUE(null_string == null_string);

  WideString empty_string(L"");
  EXPECT_TRUE(empty_string == empty_string);
  EXPECT_TRUE(empty_string == null_string);
  EXPECT_TRUE(null_string == empty_string);

  WideString deleted_string(L"hello");
  deleted_string.Delete(0, 5);
  EXPECT_TRUE(deleted_string == deleted_string);
  EXPECT_TRUE(deleted_string == null_string);
  EXPECT_TRUE(deleted_string == empty_string);
  EXPECT_TRUE(null_string == deleted_string);
  EXPECT_TRUE(null_string == empty_string);

  WideString wide_string(L"hello");
  EXPECT_TRUE(wide_string == wide_string);
  EXPECT_FALSE(wide_string == null_string);
  EXPECT_FALSE(wide_string == empty_string);
  EXPECT_FALSE(wide_string == deleted_string);
  EXPECT_FALSE(null_string == wide_string);
  EXPECT_FALSE(empty_string == wide_string);
  EXPECT_FALSE(deleted_string == wide_string);

  WideString wide_string_same1(L"hello");
  EXPECT_TRUE(wide_string == wide_string_same1);
  EXPECT_TRUE(wide_string_same1 == wide_string);

  WideString wide_string_same2(wide_string);
  EXPECT_TRUE(wide_string == wide_string_same2);
  EXPECT_TRUE(wide_string_same2 == wide_string);

  WideString wide_string1(L"he");
  WideString wide_string2(L"hellp");
  WideString wide_string3(L"hellod");
  EXPECT_FALSE(wide_string == wide_string1);
  EXPECT_FALSE(wide_string == wide_string2);
  EXPECT_FALSE(wide_string == wide_string3);
  EXPECT_FALSE(wide_string1 == wide_string);
  EXPECT_FALSE(wide_string2 == wide_string);
  EXPECT_FALSE(wide_string3 == wide_string);

  WideStringView null_string_c;
  WideStringView empty_string_c(L"");
  EXPECT_TRUE(null_string == null_string_c);
  EXPECT_TRUE(null_string == empty_string_c);
  EXPECT_TRUE(empty_string == null_string_c);
  EXPECT_TRUE(empty_string == empty_string_c);
  EXPECT_TRUE(deleted_string == null_string_c);
  EXPECT_TRUE(deleted_string == empty_string_c);
  EXPECT_TRUE(null_string_c == null_string);
  EXPECT_TRUE(empty_string_c == null_string);
  EXPECT_TRUE(null_string_c == empty_string);
  EXPECT_TRUE(empty_string_c == empty_string);
  EXPECT_TRUE(null_string_c == deleted_string);
  EXPECT_TRUE(empty_string_c == deleted_string);

  WideStringView wide_string_c_same1(L"hello");
  EXPECT_TRUE(wide_string == wide_string_c_same1);
  EXPECT_TRUE(wide_string_c_same1 == wide_string);

  WideStringView wide_string_c1(L"he");
  WideStringView wide_string_c2(L"hellp");
  WideStringView wide_string_c3(L"hellod");
  EXPECT_FALSE(wide_string == wide_string_c1);
  EXPECT_FALSE(wide_string == wide_string_c2);
  EXPECT_FALSE(wide_string == wide_string_c3);
  EXPECT_FALSE(wide_string_c1 == wide_string);
  EXPECT_FALSE(wide_string_c2 == wide_string);
  EXPECT_FALSE(wide_string_c3 == wide_string);

  const wchar_t* const c_null_string = nullptr;
  const wchar_t* const c_empty_string = L"";
  EXPECT_TRUE(null_string == c_null_string);
  EXPECT_TRUE(null_string == c_empty_string);
  EXPECT_TRUE(empty_string == c_null_string);
  EXPECT_TRUE(empty_string == c_empty_string);
  EXPECT_TRUE(deleted_string == c_null_string);
  EXPECT_TRUE(deleted_string == c_empty_string);
  EXPECT_TRUE(c_null_string == null_string);
  EXPECT_TRUE(c_empty_string == null_string);
  EXPECT_TRUE(c_null_string == empty_string);
  EXPECT_TRUE(c_empty_string == empty_string);
  EXPECT_TRUE(c_null_string == deleted_string);
  EXPECT_TRUE(c_empty_string == deleted_string);

  const wchar_t* const c_string_same1 = L"hello";
  EXPECT_TRUE(wide_string == c_string_same1);
  EXPECT_TRUE(c_string_same1 == wide_string);

  const wchar_t* const c_string1 = L"he";
  const wchar_t* const c_string2 = L"hellp";
  const wchar_t* const c_string3 = L"hellod";
  EXPECT_FALSE(wide_string == c_string1);
  EXPECT_FALSE(wide_string == c_string2);
  EXPECT_FALSE(wide_string == c_string3);
  EXPECT_FALSE(c_string1 == wide_string);
  EXPECT_FALSE(c_string2 == wide_string);
  EXPECT_FALSE(c_string3 == wide_string);
}

TEST(WideString, OperatorNE) {
  WideString null_string;
  EXPECT_FALSE(null_string != null_string);

  WideString empty_string(L"");
  EXPECT_FALSE(empty_string != empty_string);
  EXPECT_FALSE(empty_string != null_string);
  EXPECT_FALSE(null_string != empty_string);

  WideString deleted_string(L"hello");
  deleted_string.Delete(0, 5);
  EXPECT_FALSE(deleted_string != deleted_string);
  EXPECT_FALSE(deleted_string != null_string);
  EXPECT_FALSE(deleted_string != empty_string);
  EXPECT_FALSE(null_string != deleted_string);
  EXPECT_FALSE(null_string != empty_string);

  WideString wide_string(L"hello");
  EXPECT_FALSE(wide_string != wide_string);
  EXPECT_TRUE(wide_string != null_string);
  EXPECT_TRUE(wide_string != empty_string);
  EXPECT_TRUE(wide_string != deleted_string);
  EXPECT_TRUE(null_string != wide_string);
  EXPECT_TRUE(empty_string != wide_string);
  EXPECT_TRUE(deleted_string != wide_string);

  WideString wide_string_same1(L"hello");
  EXPECT_FALSE(wide_string != wide_string_same1);
  EXPECT_FALSE(wide_string_same1 != wide_string);

  WideString wide_string_same2(wide_string);
  EXPECT_FALSE(wide_string != wide_string_same2);
  EXPECT_FALSE(wide_string_same2 != wide_string);

  WideString wide_string1(L"he");
  WideString wide_string2(L"hellp");
  WideString wide_string3(L"hellod");
  EXPECT_TRUE(wide_string != wide_string1);
  EXPECT_TRUE(wide_string != wide_string2);
  EXPECT_TRUE(wide_string != wide_string3);
  EXPECT_TRUE(wide_string1 != wide_string);
  EXPECT_TRUE(wide_string2 != wide_string);
  EXPECT_TRUE(wide_string3 != wide_string);

  WideStringView null_string_c;
  WideStringView empty_string_c(L"");
  EXPECT_FALSE(null_string != null_string_c);
  EXPECT_FALSE(null_string != empty_string_c);
  EXPECT_FALSE(empty_string != null_string_c);
  EXPECT_FALSE(empty_string != empty_string_c);
  EXPECT_FALSE(deleted_string != null_string_c);
  EXPECT_FALSE(deleted_string != empty_string_c);
  EXPECT_FALSE(null_string_c != null_string);
  EXPECT_FALSE(empty_string_c != null_string);
  EXPECT_FALSE(null_string_c != empty_string);
  EXPECT_FALSE(empty_string_c != empty_string);

  WideStringView wide_string_c_same1(L"hello");
  EXPECT_FALSE(wide_string != wide_string_c_same1);
  EXPECT_FALSE(wide_string_c_same1 != wide_string);

  WideStringView wide_string_c1(L"he");
  WideStringView wide_string_c2(L"hellp");
  WideStringView wide_string_c3(L"hellod");
  EXPECT_TRUE(wide_string != wide_string_c1);
  EXPECT_TRUE(wide_string != wide_string_c2);
  EXPECT_TRUE(wide_string != wide_string_c3);
  EXPECT_TRUE(wide_string_c1 != wide_string);
  EXPECT_TRUE(wide_string_c2 != wide_string);
  EXPECT_TRUE(wide_string_c3 != wide_string);

  const wchar_t* const c_null_string = nullptr;
  const wchar_t* const c_empty_string = L"";
  EXPECT_FALSE(null_string != c_null_string);
  EXPECT_FALSE(null_string != c_empty_string);
  EXPECT_FALSE(empty_string != c_null_string);
  EXPECT_FALSE(empty_string != c_empty_string);
  EXPECT_FALSE(deleted_string != c_null_string);
  EXPECT_FALSE(deleted_string != c_empty_string);
  EXPECT_FALSE(c_null_string != null_string);
  EXPECT_FALSE(c_empty_string != null_string);
  EXPECT_FALSE(c_null_string != empty_string);
  EXPECT_FALSE(c_empty_string != empty_string);
  EXPECT_FALSE(c_null_string != deleted_string);
  EXPECT_FALSE(c_empty_string != deleted_string);

  const wchar_t* const c_string_same1 = L"hello";
  EXPECT_FALSE(wide_string != c_string_same1);
  EXPECT_FALSE(c_string_same1 != wide_string);

  const wchar_t* const c_string1 = L"he";
  const wchar_t* const c_string2 = L"hellp";
  const wchar_t* const c_string3 = L"hellod";
  EXPECT_TRUE(wide_string != c_string1);
  EXPECT_TRUE(wide_string != c_string2);
  EXPECT_TRUE(wide_string != c_string3);
  EXPECT_TRUE(c_string1 != wide_string);
  EXPECT_TRUE(c_string2 != wide_string);
  EXPECT_TRUE(c_string3 != wide_string);
}

TEST(WideString, OperatorPlus) {
  EXPECT_EQ(L"I like dogs", L"I like " + WideString(L"dogs"));
  EXPECT_EQ(L"Dogs like me", WideString(L"Dogs") + L" like me");
  EXPECT_EQ(L"Oh no, error number 42",
            L"Oh no, error number " + WideString::Format(L"%d", 42));

  {
    // Make sure operator+= and Concat() increases string memory allocation
    // geometrically.
    int allocations = 0;
    WideString str(L"ABCDEFGHIJKLMN");
    const wchar_t* buffer = str.c_str();
    for (size_t i = 0; i < 10000; ++i) {
      str += L"!";
      const wchar_t* new_buffer = str.c_str();
      if (new_buffer != buffer) {
        buffer = new_buffer;
        ++allocations;
      }
    }
    EXPECT_LT(allocations, 25);
    EXPECT_GT(allocations, 10);
  }
}

TEST(WideString, ConcatInPlace) {
  WideString fred;
  fred.Concat(L"FRED", 4);
  EXPECT_EQ(L"FRED", fred);

  fred.Concat(L"DY", 2);
  EXPECT_EQ(L"FREDDY", fred);

  fred.Delete(3, 3);
  EXPECT_EQ(L"FRE", fred);

  fred.Concat(L"D", 1);
  EXPECT_EQ(L"FRED", fred);

  WideString copy = fred;
  fred.Concat(L"DY", 2);
  EXPECT_EQ(L"FREDDY", fred);
  EXPECT_EQ(L"FRED", copy);
}

TEST(WideString, Remove) {
  WideString freed(L"FREED");
  freed.Remove(L'E');
  EXPECT_EQ(L"FRD", freed);
  freed.Remove(L'F');
  EXPECT_EQ(L"RD", freed);
  freed.Remove(L'D');
  EXPECT_EQ(L"R", freed);
  freed.Remove(L'X');
  EXPECT_EQ(L"R", freed);
  freed.Remove(L'R');
  EXPECT_EQ(L"", freed);

  WideString empty;
  empty.Remove(L'X');
  EXPECT_EQ(L"", empty);
}

TEST(WideString, RemoveCopies) {
  WideString freed(L"FREED");
  const wchar_t* old_buffer = freed.c_str();

  // No change with single reference - no copy.
  freed.Remove(L'Q');
  EXPECT_EQ(L"FREED", freed);
  EXPECT_EQ(old_buffer, freed.c_str());

  // Change with single reference - no copy.
  freed.Remove(L'E');
  EXPECT_EQ(L"FRD", freed);
  EXPECT_EQ(old_buffer, freed.c_str());

  // No change with multiple references - no copy.
  WideString shared(freed);
  freed.Remove(L'Q');
  EXPECT_EQ(L"FRD", freed);
  EXPECT_EQ(old_buffer, freed.c_str());
  EXPECT_EQ(old_buffer, shared.c_str());

  // Change with multiple references -- must copy.
  freed.Remove(L'D');
  EXPECT_EQ(L"FR", freed);
  EXPECT_NE(old_buffer, freed.c_str());
  EXPECT_EQ(L"FRD", shared);
  EXPECT_EQ(old_buffer, shared.c_str());
}

TEST(WideString, Replace) {
  WideString empty;
  empty.Replace(L"", L"CLAMS");
  empty.Replace(L"xx", L"CLAMS");
  EXPECT_EQ(L"", empty);

  WideString fred(L"FRED");
  fred.Replace(L"", L"");
  EXPECT_EQ(L"FRED", fred);
  fred.Replace(L"", L"CLAMS");
  EXPECT_EQ(L"FRED", fred);
  fred.Replace(L"FR", L"BL");
  EXPECT_EQ(L"BLED", fred);
  fred.Replace(L"D", L"DDY");
  EXPECT_EQ(L"BLEDDY", fred);
  fred.Replace(L"LEDD", L"");
  EXPECT_EQ(L"BY", fred);
  fred.Replace(L"X", L"CLAMS");
  EXPECT_EQ(L"BY", fred);
  fred.Replace(L"BY", L"HI");
  EXPECT_EQ(L"HI", fred);
  fred.Replace(L"I", L"IHIHI");
  EXPECT_EQ(L"HIHIHI", fred);
  fred.Replace(L"HI", L"HO");
  EXPECT_EQ(L"HOHOHO", fred);
  fred.Replace(L"HO", L"");
  EXPECT_EQ(L"", fred);

  WideString five_xs(L"xxxxx");
  five_xs.Replace(L"xx", L"xxx");
  EXPECT_EQ(L"xxxxxxx", five_xs);

  WideString five_ys(L"yyyyy");
  five_ys.Replace(L"yy", L"y");
  EXPECT_EQ(L"yyy", five_ys);
}

TEST(WideString, Insert) {
  WideString fred(L"FRED");
  EXPECT_EQ(5u, fred.Insert(0, 'S'));
  EXPECT_EQ(L"SFRED", fred);
  EXPECT_EQ(6u, fred.Insert(1, 'T'));
  EXPECT_EQ(L"STFRED", fred);
  EXPECT_EQ(7u, fred.Insert(4, 'U'));
  EXPECT_EQ(L"STFRUED", fred);
  EXPECT_EQ(8u, fred.Insert(7, 'V'));
  EXPECT_EQ(L"STFRUEDV", fred);
  EXPECT_EQ(8u, fred.Insert(12, 'P'));
  EXPECT_EQ(L"STFRUEDV", fred);
  {
    WideString empty;
    EXPECT_EQ(1u, empty.Insert(0, 'X'));
    EXPECT_EQ(L"X", empty);
  }
  {
    WideString empty;
    EXPECT_EQ(0u, empty.Insert(5, 'X'));
    EXPECT_NE(L"X", empty);
  }
}

TEST(WideString, InsertAtFrontAndInsertAtBack) {
  {
    WideString empty;
    EXPECT_EQ(1u, empty.InsertAtFront('D'));
    EXPECT_EQ(L"D", empty);
    EXPECT_EQ(2u, empty.InsertAtFront('E'));
    EXPECT_EQ(L"ED", empty);
    EXPECT_EQ(3u, empty.InsertAtFront('R'));
    EXPECT_EQ(L"RED", empty);
    EXPECT_EQ(4u, empty.InsertAtFront('F'));
    EXPECT_EQ(L"FRED", empty);
  }
  {
    WideString empty;
    EXPECT_EQ(1u, empty.InsertAtBack('F'));
    EXPECT_EQ(L"F", empty);
    EXPECT_EQ(2u, empty.InsertAtBack('R'));
    EXPECT_EQ(L"FR", empty);
    EXPECT_EQ(3u, empty.InsertAtBack('E'));
    EXPECT_EQ(L"FRE", empty);
    EXPECT_EQ(4u, empty.InsertAtBack('D'));
    EXPECT_EQ(L"FRED", empty);
  }
  {
    WideString empty;
    EXPECT_EQ(1u, empty.InsertAtBack('E'));
    EXPECT_EQ(L"E", empty);
    EXPECT_EQ(2u, empty.InsertAtFront('R'));
    EXPECT_EQ(L"RE", empty);
    EXPECT_EQ(3u, empty.InsertAtBack('D'));
    EXPECT_EQ(L"RED", empty);
    EXPECT_EQ(4u, empty.InsertAtFront('F'));
    EXPECT_EQ(L"FRED", empty);
  }
}

TEST(WideString, Delete) {
  WideString fred(L"FRED");
  EXPECT_EQ(4u, fred.Delete(0, 0));
  EXPECT_EQ(L"FRED", fred);
  EXPECT_EQ(2u, fred.Delete(0, 2));
  EXPECT_EQ(L"ED", fred);
  EXPECT_EQ(1u, fred.Delete(1));
  EXPECT_EQ(L"E", fred);
  EXPECT_EQ(0u, fred.Delete(0));
  EXPECT_EQ(L"", fred);
  EXPECT_EQ(0u, fred.Delete(0));
  EXPECT_EQ(L"", fred);

  WideString empty;
  EXPECT_EQ(0u, empty.Delete(0));
  EXPECT_EQ(L"", empty);
  EXPECT_EQ(0u, empty.Delete(1));
  EXPECT_EQ(L"", empty);
}

TEST(WideString, OneArgSubstr) {
  WideString fred(L"FRED");
  EXPECT_EQ(L"FRED", fred.Substr(0));
  EXPECT_EQ(L"RED", fred.Substr(1));
  EXPECT_EQ(L"ED", fred.Substr(2));
  EXPECT_EQ(L"D", fred.Substr(3));
  EXPECT_EQ(L"", fred.Substr(4));

  WideString empty;
  EXPECT_EQ(L"", empty.Substr(0));
  EXPECT_EQ(L"", empty.Substr(1));
}

TEST(WideString, TwoArgSubstr) {
  WideString fred(L"FRED");
  EXPECT_EQ(L"", fred.Substr(0, 0));
  EXPECT_EQ(L"", fred.Substr(3, 0));
  EXPECT_EQ(L"FRED", fred.Substr(0, 4));
  EXPECT_EQ(L"RED", fred.Substr(1, 3));
  EXPECT_EQ(L"ED", fred.Substr(2, 2));
  EXPECT_EQ(L"D", fred.Substr(3, 1));
  EXPECT_EQ(L"F", fred.Substr(0, 1));
  EXPECT_EQ(L"R", fred.Substr(1, 1));
  EXPECT_EQ(L"E", fred.Substr(2, 1));
  EXPECT_EQ(L"D", fred.Substr(3, 1));
  EXPECT_EQ(L"FR", fred.Substr(0, 2));
  EXPECT_EQ(L"FRED", fred.Substr(0, 4));
  EXPECT_EQ(L"", fred.Substr(0, 10));

  EXPECT_EQ(L"", fred.Substr(1, 4));
  EXPECT_EQ(L"", fred.Substr(4, 1));

  WideString empty;
  EXPECT_EQ(L"", empty.Substr(0, 0));
}

TEST(WideString, First) {
  WideString fred(L"FRED");
  EXPECT_EQ(L"", fred.First(0));
  EXPECT_EQ(L"F", fred.First(1));
  EXPECT_EQ(L"FR", fred.First(2));
  EXPECT_EQ(L"FRE", fred.First(3));
  EXPECT_EQ(L"FRED", fred.First(4));

  EXPECT_EQ(L"", fred.First(5));

  WideString empty;
  EXPECT_EQ(L"", empty.First(0));
  EXPECT_EQ(L"", empty.First(1));
}

TEST(WideString, Last) {
  WideString fred(L"FRED");
  EXPECT_EQ(L"", fred.Last(0));
  EXPECT_EQ(L"D", fred.Last(1));
  EXPECT_EQ(L"ED", fred.Last(2));
  EXPECT_EQ(L"RED", fred.Last(3));
  EXPECT_EQ(L"FRED", fred.Last(4));

  EXPECT_EQ(L"", fred.Last(5));

  WideString empty;
  EXPECT_EQ(L"", empty.Last(0));
  EXPECT_EQ(L"", empty.Last(1));
}

TEST(WideString, Find) {
  WideString null_string;
  EXPECT_FALSE(null_string.Find(L'a').has_value());
  EXPECT_FALSE(null_string.Find(L'\0').has_value());

  WideString empty_string(L"");
  EXPECT_FALSE(empty_string.Find(L'a').has_value());
  EXPECT_FALSE(empty_string.Find(L'\0').has_value());

  WideString single_string(L"a");
  std::optional<size_t> result = single_string.Find(L'a');
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(0u, result.value());
  EXPECT_FALSE(single_string.Find(L'b').has_value());
  EXPECT_FALSE(single_string.Find(L'\0').has_value());

  WideString longer_string(L"abccc");
  result = longer_string.Find(L'a');
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(0u, result.value());
  result = longer_string.Find(L'c');
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(2u, result.value());
  result = longer_string.Find(L'c', 3);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(3u, result.value());
  EXPECT_FALSE(longer_string.Find(L'\0').has_value());

  result = longer_string.Find(L"ab");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(0u, result.value());
  result = longer_string.Find(L"ccc");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(2u, result.value());
  result = longer_string.Find(L"cc", 3);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(3u, result.value());
  EXPECT_FALSE(longer_string.Find(L"d").has_value());

  WideString hibyte_string(
      L"ab\xff8c"
      L"def");
  result = hibyte_string.Find(L'\xff8c');
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(2u, result.value());
}

TEST(WideString, ReverseFind) {
  WideString null_string;
  EXPECT_FALSE(null_string.ReverseFind(L'a').has_value());
  EXPECT_FALSE(null_string.ReverseFind(L'\0').has_value());

  WideString empty_string(L"");
  EXPECT_FALSE(empty_string.ReverseFind(L'a').has_value());
  EXPECT_FALSE(empty_string.ReverseFind(L'\0').has_value());

  WideString single_string(L"a");
  std::optional<size_t> result = single_string.ReverseFind(L'a');
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(0u, result.value());
  EXPECT_FALSE(single_string.ReverseFind(L'b').has_value());
  EXPECT_FALSE(single_string.ReverseFind(L'\0').has_value());

  WideString longer_string(L"abccc");
  result = longer_string.ReverseFind(L'a');
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(0u, result.value());
  result = longer_string.ReverseFind(L'c');
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(4u, result.value());
  EXPECT_FALSE(longer_string.ReverseFind(L'\0').has_value());

  WideString hibyte_string(
      L"ab\xff8c"
      L"def");
  result = hibyte_string.ReverseFind(L'\xff8c');
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(2u, result.value());
}

TEST(WideString, UpperLower) {
  WideString fred(L"F-Re.42D");
  fred.MakeLower();
  EXPECT_EQ(L"f-re.42d", fred);
  fred.MakeUpper();
  EXPECT_EQ(L"F-RE.42D", fred);

  WideString empty;
  empty.MakeLower();
  EXPECT_EQ(L"", empty);
  empty.MakeUpper();
  EXPECT_EQ(L"", empty);

  WideString empty_with_buffer(L"x");
  empty_with_buffer.Delete(0);

  WideString additional_empty_with_buffer_ref = empty_with_buffer;
  additional_empty_with_buffer_ref.MakeLower();
  EXPECT_EQ(L"", additional_empty_with_buffer_ref);

  additional_empty_with_buffer_ref = empty_with_buffer;
  additional_empty_with_buffer_ref.MakeUpper();
  EXPECT_EQ(L"", additional_empty_with_buffer_ref);
}

TEST(WideString, Trim) {
  WideString fred(L"  FRED  ");
  fred.TrimWhitespace();
  EXPECT_EQ(L"FRED", fred);
  fred.Trim(L'E');
  EXPECT_EQ(L"FRED", fred);
  fred.Trim(L'F');
  EXPECT_EQ(L"RED", fred);
  fred.Trim(L"ERP");
  EXPECT_EQ(L"D", fred);

  WideString blank(L"   ");
  blank.Trim(L"ERP");
  EXPECT_EQ(L"   ", blank);
  blank.Trim(L'E');
  EXPECT_EQ(L"   ", blank);
  blank.TrimWhitespace();
  EXPECT_EQ(L"", blank);

  WideString empty;
  empty.Trim(L"ERP");
  EXPECT_EQ(L"", empty);
  empty.Trim(L'E');
  EXPECT_EQ(L"", empty);
  empty.TrimWhitespace();
  EXPECT_EQ(L"", empty);

  WideString abc(L"  ABCCBA  ");
  abc.Trim(L"A");
  EXPECT_EQ(L"  ABCCBA  ", abc);
  abc.Trim(L" A");
  EXPECT_EQ(L"BCCB", abc);
}

TEST(WideString, TrimFront) {
  WideString fred(L"  FRED  ");
  fred.TrimWhitespaceFront();
  EXPECT_EQ(L"FRED  ", fred);
  fred.TrimFront(L'E');
  EXPECT_EQ(L"FRED  ", fred);
  fred.TrimFront(L'F');
  EXPECT_EQ(L"RED  ", fred);
  fred.TrimFront(L"ERP");
  EXPECT_EQ(L"D  ", fred);

  WideString blank(L"   ");
  blank.TrimFront(L"ERP");
  EXPECT_EQ(L"   ", blank);
  blank.TrimFront(L'E');
  EXPECT_EQ(L"   ", blank);
  blank.TrimWhitespaceFront();
  EXPECT_EQ(L"", blank);

  WideString empty;
  empty.TrimFront(L"ERP");
  EXPECT_EQ(L"", empty);
  empty.TrimFront(L'E');
  EXPECT_EQ(L"", empty);
  empty.TrimWhitespaceFront();
  EXPECT_EQ(L"", empty);
}

TEST(WideString, TrimFrontCopies) {
  {
    // With a single reference, no copy takes place.
    WideString fred(L"  FRED  ");
    const wchar_t* old_buffer = fred.c_str();
    fred.TrimWhitespaceFront();
    EXPECT_EQ(L"FRED  ", fred);
    EXPECT_EQ(old_buffer, fred.c_str());
  }
  {
    // With multiple references, we must copy.
    WideString fred(L"  FRED  ");
    WideString other_fred = fred;
    const wchar_t* old_buffer = fred.c_str();
    fred.TrimWhitespaceFront();
    EXPECT_EQ(L"FRED  ", fred);
    EXPECT_EQ(L"  FRED  ", other_fred);
    EXPECT_NE(old_buffer, fred.c_str());
  }
  {
    // With multiple references, but no modifications, no copy.
    WideString fred(L"FRED");
    WideString other_fred = fred;
    const wchar_t* old_buffer = fred.c_str();
    fred.TrimWhitespaceFront();
    EXPECT_EQ(L"FRED", fred);
    EXPECT_EQ(L"FRED", other_fred);
    EXPECT_EQ(old_buffer, fred.c_str());
  }
}

TEST(WideString, TrimBack) {
  WideString fred(L"  FRED  ");
  fred.TrimWhitespaceBack();
  EXPECT_EQ(L"  FRED", fred);
  fred.TrimBack(L'E');
  EXPECT_EQ(L"  FRED", fred);
  fred.TrimBack(L'D');
  EXPECT_EQ(L"  FRE", fred);
  fred.TrimBack(L"ERP");
  EXPECT_EQ(L"  F", fred);

  WideString blank(L"   ");
  blank.TrimBack(L"ERP");
  EXPECT_EQ(L"   ", blank);
  blank.TrimBack(L'E');
  EXPECT_EQ(L"   ", blank);
  blank.TrimWhitespaceBack();
  EXPECT_EQ(L"", blank);

  WideString empty;
  empty.TrimBack(L"ERP");
  EXPECT_EQ(L"", empty);
  empty.TrimBack(L'E');
  EXPECT_EQ(L"", empty);
  empty.TrimWhitespaceBack();
  EXPECT_EQ(L"", empty);
}

TEST(WideString, TrimBackCopies) {
  {
    // With a single reference, no copy takes place.
    WideString fred(L"  FRED  ");
    const wchar_t* old_buffer = fred.c_str();
    fred.TrimWhitespaceBack();
    EXPECT_EQ(L"  FRED", fred);
    EXPECT_EQ(old_buffer, fred.c_str());
  }
  {
    // With multiple references, we must copy.
    WideString fred(L"  FRED  ");
    WideString other_fred = fred;
    const wchar_t* old_buffer = fred.c_str();
    fred.TrimWhitespaceBack();
    EXPECT_EQ(L"  FRED", fred);
    EXPECT_EQ(L"  FRED  ", other_fred);
    EXPECT_NE(old_buffer, fred.c_str());
  }
  {
    // With multiple references, but no modifications, no copy.
    WideString fred(L"FRED");
    WideString other_fred = fred;
    const wchar_t* old_buffer = fred.c_str();
    fred.TrimWhitespaceBack();
    EXPECT_EQ(L"FRED", fred);
    EXPECT_EQ(L"FRED", other_fred);
    EXPECT_EQ(old_buffer, fred.c_str());
  }
}

TEST(WideString, Reserve) {
  {
    WideString str;
    str.Reserve(6);
    const wchar_t* old_buffer = str.c_str();
    str += L"ABCDEF";
    EXPECT_EQ(old_buffer, str.c_str());
    str += L"Blah Blah Blah Blah Blah Blah";
    EXPECT_NE(old_buffer, str.c_str());
  }
  {
    WideString str(L"A");
    str.Reserve(6);
    const wchar_t* old_buffer = str.c_str();
    str += L"BCDEF";
    EXPECT_EQ(old_buffer, str.c_str());
    str += L"Blah Blah Blah Blah Blah Blah";
    EXPECT_NE(old_buffer, str.c_str());
  }
}

TEST(WideString, GetBuffer) {
  WideString str1;
  {
    pdfium::span<wchar_t> buffer = str1.GetBuffer(12);
    // SAFETY: required for test.
    UNSAFE_BUFFERS(wcscpy(buffer.data(), L"clams"));
  }
  str1.ReleaseBuffer(str1.GetStringLength());
  EXPECT_EQ(L"clams", str1);

  WideString str2(L"cl");
  {
    pdfium::span<wchar_t> buffer = str2.GetBuffer(12);
    UNSAFE_TODO(wcscpy(buffer.data() + 2, L"ams"));
  }
  str2.ReleaseBuffer(str2.GetStringLength());
  EXPECT_EQ(L"clams", str2);
}

TEST(WideString, ReleaseBuffer) {
  {
    WideString str;
    str.Reserve(12);
    str += L"clams";
    const wchar_t* old_buffer = str.c_str();
    str.ReleaseBuffer(4);
    EXPECT_EQ(old_buffer, str.c_str());
    EXPECT_EQ(L"clam", str);
  }
  {
    WideString str(L"c");
    str.Reserve(12);
    str += L"lams";
    const wchar_t* old_buffer = str.c_str();
    str.ReleaseBuffer(4);
    EXPECT_EQ(old_buffer, str.c_str());
    EXPECT_EQ(L"clam", str);
  }
  {
    WideString str;
    str.Reserve(200);
    str += L"clams";
    const wchar_t* old_buffer = str.c_str();
    str.ReleaseBuffer(4);
    EXPECT_NE(old_buffer, str.c_str());
    EXPECT_EQ(L"clam", str);
  }
  {
    WideString str(L"c");
    str.Reserve(200);
    str += L"lams";
    const wchar_t* old_buffer = str.c_str();
    str.ReleaseBuffer(4);
    EXPECT_NE(old_buffer, str.c_str());
    EXPECT_EQ(L"clam", str);
  }
}

TEST(WideString, EmptyReverseIterator) {
  WideString empty;
  auto iter = empty.rbegin();
  EXPECT_TRUE(iter == empty.rend());
  EXPECT_FALSE(iter != empty.rend());
  EXPECT_FALSE(iter < empty.rend());
}

TEST(WideString, OneCharReverseIterator) {
  WideString one_str(L"a");
  auto iter = one_str.rbegin();
  EXPECT_FALSE(iter == one_str.rend());
  EXPECT_TRUE(iter != one_str.rend());
  EXPECT_TRUE(iter < one_str.rend());

  char ch = *iter++;
  EXPECT_EQ('a', ch);
  EXPECT_TRUE(iter == one_str.rend());
  EXPECT_FALSE(iter != one_str.rend());
  EXPECT_FALSE(iter < one_str.rend());
}

TEST(WideString, MultiCharReverseIterator) {
  WideString multi_str(L"abcd");
  auto iter = multi_str.rbegin();
  EXPECT_NE(iter, multi_str.rend());
  EXPECT_EQ(4, multi_str.rend() - iter);
  EXPECT_EQ(0, iter - multi_str.rbegin());

  char ch = *iter++;
  EXPECT_EQ('d', ch);
  EXPECT_EQ('c', *iter);
  EXPECT_NE(iter, multi_str.rend());
  EXPECT_EQ(3, multi_str.rend() - iter);
  EXPECT_EQ(1, iter - multi_str.rbegin());

  ch = *(++iter);
  EXPECT_EQ('b', ch);
  EXPECT_EQ('b', *iter);
  EXPECT_NE(iter, multi_str.rend());
  EXPECT_EQ(2, multi_str.rend() - iter);
  EXPECT_EQ(2, iter - multi_str.rbegin());

  ch = *iter++;
  EXPECT_EQ('b', ch);
  EXPECT_EQ('a', *iter);
  EXPECT_NE(iter, multi_str.rend());
  EXPECT_EQ(1, multi_str.rend() - iter);
  EXPECT_EQ(3, iter - multi_str.rbegin());

  ch = *iter++;
  EXPECT_EQ('a', ch);
  EXPECT_EQ(iter, multi_str.rend());
  EXPECT_EQ(0, multi_str.rend() - iter);
  EXPECT_EQ(4, iter - multi_str.rbegin());

  ch = *(--iter);
  EXPECT_EQ('a', ch);
  EXPECT_EQ('a', *iter);
  EXPECT_NE(iter, multi_str.rend());
  EXPECT_EQ(1, multi_str.rend() - iter);
  EXPECT_EQ(3, iter - multi_str.rbegin());

  ch = *iter--;
  EXPECT_EQ('a', ch);
  EXPECT_EQ('b', *iter);
  EXPECT_NE(iter, multi_str.rend());
  EXPECT_EQ(2, multi_str.rend() - iter);
  EXPECT_EQ(2, iter - multi_str.rbegin());

  ch = *iter--;
  EXPECT_EQ('b', ch);
  EXPECT_EQ('c', *iter);
  EXPECT_NE(iter, multi_str.rend());
  EXPECT_EQ(3, multi_str.rend() - iter);
  EXPECT_EQ(1, iter - multi_str.rbegin());

  ch = *(--iter);
  EXPECT_EQ('d', ch);
  EXPECT_EQ('d', *iter);
  EXPECT_EQ(iter, multi_str.rbegin());
  EXPECT_EQ(4, multi_str.rend() - iter);
  EXPECT_EQ(0, iter - multi_str.rbegin());
}

TEST(WideString, FromUTF8) {
  EXPECT_EQ(L"", WideString::FromUTF8(ByteStringView()));
  EXPECT_EQ(
      L"x"
      L"\u0080"
      L"\u00ff"
      L"\ud7ff"
      L"\ue000"
      L"\uff2c"
      L"\uffff"
      L"y",
      WideString::FromUTF8("x"
                           "\u0080"
                           "\u00ff"
                           "\ud7ff"
                           "\ue000"
                           "\uff2c"
                           "\uffff"
                           "y"));
}

TEST(WideString, FromUTF8Supplementary) {
  EXPECT_EQ(
      L"\U00010000"
      L"\U0001f3a8"
      L"\U0010ffff",
      WideString::FromUTF8("\U00010000"
                           "🎨"
                           "\U0010ffff"));
}

TEST(WideString, FromUTF8ErrorRecovery) {
  EXPECT_EQ(L"(A)", WideString::FromUTF8("(\xc2\x41)"))
      << "Invalid continuation";
  EXPECT_EQ(L"()", WideString::FromUTF8("(\xc2\xc2)"))
      << "Invalid continuation";
  EXPECT_EQ(L"()", WideString::FromUTF8("(\xc2\xff\x80)"))
      << "Invalid continuation";
  EXPECT_EQ(L"()", WideString::FromUTF8("(\x80\x80)")) << "Invalid leading";
  EXPECT_EQ(L"()", WideString::FromUTF8("(\xff\x80\x80)")) << "Invalid leading";
  EXPECT_EQ(L"()", WideString::FromUTF8("(\xf8\x80\x80\x80\x80)"))
      << "Invalid leading";
  EXPECT_EQ(L"()", WideString::FromUTF8("(\xf8\x88\x80\x80\x80)"))
      << "Invalid leading";
  EXPECT_EQ(L"()", WideString::FromUTF8("(\xf4\x90\x80\x80)"))
      << "Code point greater than U+10FFFF";
}

TEST(WideString, UTF8EncodeDecodeConsistency) {
  WideString wstr;
  wstr.Reserve(0x10000);
  for (char32_t w = 0; w < pdfium::kMinimumSupplementaryCodePoint; ++w) {
    if (pdfium::IsHighSurrogate(w) || pdfium::IsLowSurrogate(w)) {
      // Skip UTF-16 surrogates.
      continue;
    }
    wstr += static_cast<wchar_t>(w);
  }
  ASSERT_EQ(0xf800u, wstr.GetLength());

  ByteString bstr = FX_UTF8Encode(wstr.AsStringView());
  WideString wstr2 = WideString::FromUTF8(bstr.AsStringView());
  EXPECT_EQ(wstr, wstr2);
}

TEST(WideString, UTF8EncodeDecodeConsistencyUnpairedHighSurrogates) {
  WideString wstr;
  wstr.Reserve(0x400);
  for (wchar_t w = pdfium::kMinimumHighSurrogateCodeUnit;
       w <= pdfium::kMaximumHighSurrogateCodeUnit; ++w) {
    wstr += w;
  }
  ASSERT_EQ(0x400u, wstr.GetLength());

  ByteString bstr = FX_UTF8Encode(wstr.AsStringView());
  WideString wstr2 = WideString::FromUTF8(bstr.AsStringView());
  EXPECT_EQ(wstr, wstr2);
}

TEST(WideString, UTF8EncodeDecodeConsistencyUnpairedLowSurrogates) {
  WideString wstr;
  wstr.Reserve(0x400);
  for (wchar_t w = pdfium::kMinimumLowSurrogateCodeUnit;
       w <= pdfium::kMaximumLowSurrogateCodeUnit; ++w) {
    wstr += w;
  }
  ASSERT_EQ(0x400u, wstr.GetLength());

  ByteString bstr = FX_UTF8Encode(wstr.AsStringView());
  WideString wstr2 = WideString::FromUTF8(bstr.AsStringView());
  EXPECT_EQ(wstr, wstr2);
}

TEST(WideString, FromUTF16BE) {
  struct UTF16BEDecodeCase {
    ByteString in;
    WideString out;
  } const utf16be_decode_cases[] = {
      {"", L""},
      {UNSAFE_BUFFERS(ByteString("\0a\0b\0c", 6)), L"abc"},
      {UNSAFE_BUFFERS(ByteString("\0a\0b\0c\0\0\0d\0e\0f", 14)),
       UNSAFE_BUFFERS(WideString(L"abc\0def", 7))},
      {UNSAFE_BUFFERS(ByteString(" &", 2)), L"…"},
      {UNSAFE_BUFFERS(ByteString("\xD8\x3C\xDF\xA8", 4)), L"🎨"},
  };
  UNSAFE_TODO({
    for (size_t i = 0; i < std::size(utf16be_decode_cases); ++i) {
      EXPECT_EQ(
          WideString::FromUTF16BE(utf16be_decode_cases[i].in.unsigned_span()),
          utf16be_decode_cases[i].out)
          << " for case number " << i;
    }
  });
}

TEST(WideString, FromUTF16LE) {
  struct UTF16LEDecodeCase {
    ByteString in;
    WideString out;
  } const utf16le_decode_cases[] = {
      // SAFETY: not required, control sizes for test.
      {"", L""},
      {UNSAFE_BUFFERS(ByteString("a\0b\0c\0", 6)), L"abc"},
      {UNSAFE_BUFFERS(ByteString("a\0b\0c\0\0\0d\0e\0f\0", 14)),
       UNSAFE_BUFFERS(WideString(L"abc\0def", 7))},
      {UNSAFE_BUFFERS(ByteString("& ", 2)), L"…"},
      {UNSAFE_BUFFERS(ByteString("\x3C\xD8\xA8\xDF", 4)), L"🎨"},
  };
  UNSAFE_TODO({
    for (size_t i = 0; i < std::size(utf16le_decode_cases); ++i) {
      EXPECT_EQ(
          WideString::FromUTF16LE(utf16le_decode_cases[i].in.unsigned_span()),
          utf16le_decode_cases[i].out)
          << " for case number " << i;
    }
  });
}

TEST(WideString, ToUTF16LE) {
  struct UTF16LEEncodeCase {
    WideString ws;
    ByteString bs;
  } const utf16le_encode_cases[] = {
      {L"", UNSAFE_TODO(ByteString("\0\0", 2))},
      {L"abc", UNSAFE_TODO(ByteString("a\0b\0c\0\0\0", 8))},
      {L"abcdef", UNSAFE_TODO(ByteString("a\0b\0c\0d\0e\0f\0\0\0", 14))},
      {L"abc\0def", UNSAFE_TODO(ByteString("a\0b\0c\0\0\0", 8))},
      {L"\xaabb\xccdd", UNSAFE_TODO(ByteString("\xbb\xaa\xdd\xcc\0\0", 6))},
      {L"\x3132\x6162", UNSAFE_TODO(ByteString("\x32\x31\x62\x61\0\0", 6))},
      {L"🎨", UNSAFE_TODO(ByteString("\x3C\xD8\xA8\xDF\0\0", 6))},
  };
  UNSAFE_TODO({
    for (size_t i = 0; i < std::size(utf16le_encode_cases); ++i) {
      EXPECT_EQ(utf16le_encode_cases[i].bs,
                utf16le_encode_cases[i].ws.ToUTF16LE())
          << " for case number " << i;
    }
  });
}

TEST(WideString, ToUCS2LE) {
  struct UCS2LEEncodeCase {
    WideString ws;
    ByteString bs;
  } const ucs2le_encode_cases[] = {
      {L"", UNSAFE_TODO(ByteString("\0\0", 2))},
      {L"abc", UNSAFE_TODO(ByteString("a\0b\0c\0\0\0", 8))},
      {L"abcdef", UNSAFE_TODO(ByteString("a\0b\0c\0d\0e\0f\0\0\0", 14))},
      {L"abc\0def", UNSAFE_TODO(ByteString("a\0b\0c\0\0\0", 8))},
      {L"\xaabb\xccdd", UNSAFE_TODO(ByteString("\xbb\xaa\xdd\xcc\0\0", 6))},
      {L"\x3132\x6162", UNSAFE_TODO(ByteString("\x32\x31\x62\x61\0\0", 6))},
#if defined(WCHAR_T_IS_32_BIT)
      {L"🎨", UNSAFE_TODO(ByteString("\0\0", 2))},
#endif
  };
  UNSAFE_TODO({
    for (size_t i = 0; i < std::size(ucs2le_encode_cases); ++i) {
      EXPECT_EQ(ucs2le_encode_cases[i].bs, ucs2le_encode_cases[i].ws.ToUCS2LE())
          << " for case number " << i;
    }
  });
}

TEST(WideString, EncodeEntities) {
  EXPECT_EQ(WideString(L"Symbols &<>'\".").EncodeEntities(),
            L"Symbols &amp;&lt;&gt;&apos;&quot;.");
}

TEST(WideString, IsASCII) {
  EXPECT_TRUE(WideString(L"xy\u007fz").IsASCII());
  EXPECT_FALSE(WideString(L"xy\u0080z").IsASCII());
  EXPECT_FALSE(WideString(L"xy\u2041z").IsASCII());
}

TEST(WideString, EqualsASCII) {
  EXPECT_TRUE(WideString(L"").EqualsASCII(""));
  EXPECT_FALSE(WideString(L"A").EqualsASCII(""));
  EXPECT_FALSE(WideString(L"").EqualsASCII("A"));
  EXPECT_FALSE(WideString(L"A").EqualsASCII("B"));
  EXPECT_TRUE(WideString(L"ABC").EqualsASCII("ABC"));
  EXPECT_FALSE(WideString(L"ABC").EqualsASCII("AEC"));
  EXPECT_FALSE(WideString(L"\u00c1").EqualsASCII("\x41"));
  EXPECT_FALSE(WideString(L"\u0141").EqualsASCII("\x41"));
}

TEST(WideString, EqualsASCIINoCase) {
  EXPECT_TRUE(WideString(L"").EqualsASCIINoCase(""));
  EXPECT_FALSE(WideString(L"A").EqualsASCIINoCase("b"));
  EXPECT_TRUE(WideString(L"AbC").EqualsASCIINoCase("aBc"));
  EXPECT_FALSE(WideString(L"ABc").EqualsASCIINoCase("AeC"));
  EXPECT_FALSE(WideString(L"\u00c1").EqualsASCIINoCase("\x41"));
  EXPECT_FALSE(WideString(L"\u0141").EqualsASCIINoCase("\x41"));
}

TEST(WideString, ToASCII) {
  const char* kResult =
      "x"
      "\x02"
      "\x7f"
      "\x22"
      "\x0c"
      "y";
  EXPECT_EQ(kResult, WideString(L"x"
                                L"\u0082"
                                L"\u00ff"
                                L"\u0122"
                                L"\u208c"
                                L"y")
                         .ToASCII());
}

TEST(WideString, ToLatin1) {
  const char* kResult =
      "x"
      "\x82"
      "\xff"
      "\x22"
      "\x8c"
      "y";
  EXPECT_EQ(kResult, WideString(L"x"
                                L"\u0082"
                                L"\u00ff"
                                L"\u0122"
                                L"\u208c"
                                L"y")
                         .ToLatin1());
}

TEST(WideString, ToDefANSI) {
  EXPECT_EQ("", WideString().ToDefANSI());
#if BUILDFLAG(IS_WIN)
  const char* kResult =
      "x"
      "?"
      "\xff"
      "A"
      "?"
      "y";
#else
  const char* kResult =
      "x"
      "\x80"
      "\xff"
      "y";
#endif
  EXPECT_EQ(kResult, WideString(L"x"
                                L"\u0080"
                                L"\u00ff"
                                L"\u0100"
                                L"\u208c"
                                L"y")
                         .ToDefANSI());
}

TEST(WideString, FromASCII) {
  EXPECT_EQ(L"", WideString::FromASCII(ByteStringView()));
  const wchar_t* kResult =
      L"x"
      L"\u0002"
      L"\u007f"
      L"y";
  EXPECT_EQ(kResult, WideString::FromASCII("x"
                                           "\x82"
                                           "\xff"
                                           "y"));
}

TEST(WideString, FromLatin1) {
  EXPECT_EQ(L"", WideString::FromLatin1(ByteStringView()));
  const wchar_t* kResult =
      L"x"
      L"\u0082"
      L"\u00ff"
      L"y";
  EXPECT_EQ(kResult, WideString::FromLatin1("x"
                                            "\x82"
                                            "\xff"
                                            "y"));
}

TEST(WideString, FromDefANSI) {
  EXPECT_EQ(L"", WideString::FromDefANSI(ByteStringView()));
#if BUILDFLAG(IS_WIN)
  const wchar_t* kResult =
      L"x"
      L"\u20ac"
      L"\u00ff"
      L"y";
#else
  const wchar_t* kResult =
      L"x"
      L"\u0080"
      L"\u00ff"
      L"y";
#endif
  EXPECT_EQ(kResult, WideString::FromDefANSI("x"
                                             "\x80"
                                             "\xff"
                                             "y"));
}

TEST(WideStringView, ConstexprCtors) {
  static constexpr WideStringView null_string;
  static_assert(null_string.GetLength() == 0);
  static_assert(null_string.IsEmpty());

  static constexpr WideStringView copied_null_string(null_string);
  static_assert(copied_null_string.GetLength() == 0);
  static_assert(copied_null_string.IsEmpty());
}

TEST(WideStringView, FromVector) {
  std::vector<WideStringView::UnsignedType> null_vec;
  WideStringView null_string(null_vec);
  EXPECT_EQ(0u, null_string.GetLength());

  std::vector<WideStringView::UnsignedType> lower_a_vec(
      10, static_cast<WideStringView::UnsignedType>(L'a'));
  WideStringView lower_a_string(lower_a_vec);
  EXPECT_EQ(10u, lower_a_string.GetLength());
  EXPECT_EQ(L"aaaaaaaaaa", lower_a_string);

  std::vector<WideStringView::UnsignedType> cleared_vec;
  cleared_vec.push_back(42);
  cleared_vec.pop_back();
  WideStringView cleared_string(cleared_vec);
  EXPECT_EQ(0u, cleared_string.GetLength());
  EXPECT_FALSE(cleared_string.unterminated_unsigned_str());
}

TEST(WideStringView, ElementAccess) {
  WideStringView abc(L"abc");
  EXPECT_EQ(L'a', static_cast<wchar_t>(abc[0]));
  EXPECT_EQ(L'b', static_cast<wchar_t>(abc[1]));
  EXPECT_EQ(L'c', static_cast<wchar_t>(abc[2]));
  EXPECT_DEATH_IF_SUPPORTED({ abc[4]; }, "");
}

TEST(WideStringView, OperatorLT) {
  WideStringView empty;
  WideStringView a(L"a");
  WideStringView abc(L"\x0110qq");  // Comes InsertAtFront despite endianness.
  WideStringView def(L"\x1001qq");  // Comes InsertAtBack despite endianness.
  const wchar_t* const c_null = nullptr;
  const wchar_t* const c_empty = L"";
  const wchar_t* const c_a = L"a";
  const wchar_t* const c_abc = L"\x0110qq";
  const wchar_t* const c_def = L"\x1001qq";

  EXPECT_FALSE(empty < empty);
  EXPECT_FALSE(a < a);
  EXPECT_FALSE(abc < abc);
  EXPECT_FALSE(def < def);
  EXPECT_FALSE(c_null < empty);
  EXPECT_FALSE(c_empty < empty);
  EXPECT_FALSE(c_a < a);
  EXPECT_FALSE(c_abc < abc);
  EXPECT_FALSE(c_def < def);
  EXPECT_FALSE(empty < c_null);
  EXPECT_FALSE(empty < c_empty);
  EXPECT_FALSE(a < c_a);
  EXPECT_FALSE(abc < c_abc);
  EXPECT_FALSE(def < c_def);

  EXPECT_TRUE(empty < a);
  EXPECT_FALSE(a < empty);
  EXPECT_TRUE(empty < c_a);
  EXPECT_FALSE(a < c_null);
  EXPECT_FALSE(a < c_empty);

  EXPECT_TRUE(empty < abc);
  EXPECT_FALSE(abc < empty);
  EXPECT_TRUE(empty < c_abc);
  EXPECT_FALSE(abc < c_null);
  EXPECT_FALSE(abc < c_empty);

  EXPECT_TRUE(empty < def);
  EXPECT_FALSE(def < empty);
  EXPECT_TRUE(empty < c_def);
  EXPECT_FALSE(def < c_null);
  EXPECT_FALSE(def < c_empty);

  EXPECT_TRUE(a < abc);
  EXPECT_FALSE(abc < a);
  EXPECT_TRUE(a < c_abc);
  EXPECT_FALSE(abc < c_a);

  EXPECT_TRUE(a < def);
  EXPECT_FALSE(def < a);
  EXPECT_TRUE(a < c_def);
  EXPECT_FALSE(def < c_a);

  EXPECT_TRUE(abc < def);
  EXPECT_FALSE(def < abc);
  EXPECT_TRUE(abc < c_def);
  EXPECT_FALSE(def < c_abc);
}

TEST(WideStringView, OperatorEQ) {
  WideStringView wide_string_c(L"hello");
  EXPECT_TRUE(wide_string_c == wide_string_c);

  WideStringView wide_string_c_same1(L"hello");
  EXPECT_TRUE(wide_string_c == wide_string_c_same1);
  EXPECT_TRUE(wide_string_c_same1 == wide_string_c);

  WideStringView wide_string_c_same2(wide_string_c);
  EXPECT_TRUE(wide_string_c == wide_string_c_same2);
  EXPECT_TRUE(wide_string_c_same2 == wide_string_c);

  WideStringView wide_string_c1(L"he");
  WideStringView wide_string_c2(L"hellp");
  WideStringView wide_string_c3(L"hellod");
  EXPECT_FALSE(wide_string_c == wide_string_c1);
  EXPECT_FALSE(wide_string_c == wide_string_c2);
  EXPECT_FALSE(wide_string_c == wide_string_c3);
  EXPECT_FALSE(wide_string_c1 == wide_string_c);
  EXPECT_FALSE(wide_string_c2 == wide_string_c);
  EXPECT_FALSE(wide_string_c3 == wide_string_c);

  WideString wide_string_same1(L"hello");
  EXPECT_TRUE(wide_string_c == wide_string_same1);
  EXPECT_TRUE(wide_string_same1 == wide_string_c);

  WideString wide_string1(L"he");
  WideString wide_string2(L"hellp");
  WideString wide_string3(L"hellod");
  EXPECT_FALSE(wide_string_c == wide_string1);
  EXPECT_FALSE(wide_string_c == wide_string2);
  EXPECT_FALSE(wide_string_c == wide_string3);
  EXPECT_FALSE(wide_string1 == wide_string_c);
  EXPECT_FALSE(wide_string2 == wide_string_c);
  EXPECT_FALSE(wide_string3 == wide_string_c);

  const wchar_t* const c_string_same1 = L"hello";
  EXPECT_TRUE(wide_string_c == c_string_same1);
  EXPECT_TRUE(c_string_same1 == wide_string_c);

  const wchar_t* const c_string1 = L"he";
  const wchar_t* const c_string2 = L"hellp";
  const wchar_t* const c_string3 = L"hellod";
  EXPECT_FALSE(wide_string_c == c_string1);
  EXPECT_FALSE(wide_string_c == c_string2);
  EXPECT_FALSE(wide_string_c == c_string3);

  EXPECT_FALSE(c_string1 == wide_string_c);
  EXPECT_FALSE(c_string2 == wide_string_c);
  EXPECT_FALSE(c_string3 == wide_string_c);
}

TEST(WideStringView, OperatorNE) {
  WideStringView wide_string_c(L"hello");
  EXPECT_FALSE(wide_string_c != wide_string_c);

  WideStringView wide_string_c_same1(L"hello");
  EXPECT_FALSE(wide_string_c != wide_string_c_same1);
  EXPECT_FALSE(wide_string_c_same1 != wide_string_c);

  WideStringView wide_string_c_same2(wide_string_c);
  EXPECT_FALSE(wide_string_c != wide_string_c_same2);
  EXPECT_FALSE(wide_string_c_same2 != wide_string_c);

  WideStringView wide_string_c1(L"he");
  WideStringView wide_string_c2(L"hellp");
  WideStringView wide_string_c3(L"hellod");
  EXPECT_TRUE(wide_string_c != wide_string_c1);
  EXPECT_TRUE(wide_string_c != wide_string_c2);
  EXPECT_TRUE(wide_string_c != wide_string_c3);
  EXPECT_TRUE(wide_string_c1 != wide_string_c);
  EXPECT_TRUE(wide_string_c2 != wide_string_c);
  EXPECT_TRUE(wide_string_c3 != wide_string_c);

  WideString wide_string_same1(L"hello");
  EXPECT_FALSE(wide_string_c != wide_string_same1);
  EXPECT_FALSE(wide_string_same1 != wide_string_c);

  WideString wide_string1(L"he");
  WideString wide_string2(L"hellp");
  WideString wide_string3(L"hellod");
  EXPECT_TRUE(wide_string_c != wide_string1);
  EXPECT_TRUE(wide_string_c != wide_string2);
  EXPECT_TRUE(wide_string_c != wide_string3);
  EXPECT_TRUE(wide_string1 != wide_string_c);
  EXPECT_TRUE(wide_string2 != wide_string_c);
  EXPECT_TRUE(wide_string3 != wide_string_c);

  const wchar_t* const c_string_same1 = L"hello";
  EXPECT_FALSE(wide_string_c != c_string_same1);
  EXPECT_FALSE(c_string_same1 != wide_string_c);

  const wchar_t* const c_string1 = L"he";
  const wchar_t* const c_string2 = L"hellp";
  const wchar_t* const c_string3 = L"hellod";
  EXPECT_TRUE(wide_string_c != c_string1);
  EXPECT_TRUE(wide_string_c != c_string2);
  EXPECT_TRUE(wide_string_c != c_string3);

  EXPECT_TRUE(c_string1 != wide_string_c);
  EXPECT_TRUE(c_string2 != wide_string_c);
  EXPECT_TRUE(c_string3 != wide_string_c);
}

TEST(WideStringView, Find) {
  WideStringView null_string;
  EXPECT_FALSE(null_string.Find(L'a').has_value());
  EXPECT_FALSE(null_string.Find(L'\0').has_value());

  WideStringView empty_string(L"");
  EXPECT_FALSE(empty_string.Find(L'a').has_value());
  EXPECT_FALSE(empty_string.Find(L'\0').has_value());

  WideStringView single_string(L"a");
  std::optional<size_t> result = single_string.Find(L'a');
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(0u, result.value());
  EXPECT_FALSE(single_string.Find(L'b').has_value());
  EXPECT_FALSE(single_string.Find(L'\0').has_value());

  WideStringView longer_string(L"abccc");
  result = longer_string.Find(L'a');
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(0u, result.value());
  result = longer_string.Find(L'c');
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(2u, result.value());
  EXPECT_FALSE(longer_string.Find(L'd').has_value());
  EXPECT_FALSE(longer_string.Find(L'\0').has_value());

  WideStringView hibyte_string(
      L"ab\xFF8c"
      L"def");
  result = hibyte_string.Find(L'\xFF8c');
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(2u, result.value());
}

TEST(WideStringView, NullIterator) {
  WideStringView null_str;
  int32_t sum = 0;
  bool any_present = false;
  for (const auto& c : null_str) {
    sum += c;  // Avoid unused arg warnings.
    any_present = true;
  }
  EXPECT_FALSE(any_present);
  EXPECT_EQ(0, sum);
}

TEST(WideStringView, EmptyIterator) {
  WideStringView empty_str(L"");
  int32_t sum = 0;
  bool any_present = false;
  for (const auto& c : empty_str) {
    any_present = true;
    sum += c;  // Avoid unused arg warnings.
  }
  EXPECT_FALSE(any_present);
  EXPECT_EQ(0, sum);
}

TEST(WideStringView, OneCharIterator) {
  WideStringView one_str(L"a");
  int32_t sum = 0;
  bool any_present = false;
  for (const auto& c : one_str) {
    any_present = true;
    sum += c;  // Avoid unused arg warnings.
  }
  EXPECT_TRUE(any_present);
  EXPECT_EQ(static_cast<int32_t>(L'a'), sum);
}

TEST(WideStringView, MultiCharIterator) {
  WideStringView one_str(L"abc");
  int32_t sum = 0;
  bool any_present = false;
  for (const auto& c : one_str) {
    any_present = true;
    sum += c;  // Avoid unused arg warnings.
  }
  EXPECT_TRUE(any_present);
  EXPECT_EQ(static_cast<int32_t>(L'a' + L'b' + L'c'), sum);
}

TEST(WideStringView, EmptyReverseIterator) {
  WideStringView empty;
  auto iter = empty.rbegin();
  EXPECT_TRUE(iter == empty.rend());
  EXPECT_FALSE(iter != empty.rend());
  EXPECT_FALSE(iter < empty.rend());
}

TEST(WideStringView, OneCharReverseIterator) {
  WideStringView one_str(L"a");
  auto iter = one_str.rbegin();
  EXPECT_FALSE(iter == one_str.rend());
  EXPECT_TRUE(iter != one_str.rend());
  EXPECT_TRUE(iter < one_str.rend());

  char ch = *iter++;
  EXPECT_EQ('a', ch);
  EXPECT_TRUE(iter == one_str.rend());
  EXPECT_FALSE(iter != one_str.rend());
  EXPECT_FALSE(iter < one_str.rend());
}

TEST(WideStringView, MultiCharReverseIterator) {
  WideStringView multi_str(L"abcd");
  auto iter = multi_str.rbegin();
  EXPECT_FALSE(iter == multi_str.rend());

  char ch = *iter++;
  EXPECT_EQ('d', ch);
  EXPECT_EQ('c', *iter);
  EXPECT_FALSE(iter == multi_str.rend());

  ch = *(++iter);
  EXPECT_EQ('b', ch);
  EXPECT_EQ('b', *iter);
  EXPECT_FALSE(iter == multi_str.rend());

  ch = *iter++;
  EXPECT_EQ('b', ch);
  EXPECT_EQ('a', *iter);
  EXPECT_FALSE(iter == multi_str.rend());

  ch = *iter++;
  EXPECT_EQ('a', ch);
  EXPECT_TRUE(iter == multi_str.rend());

  ch = *(--iter);
  EXPECT_EQ('a', ch);
  EXPECT_EQ('a', *iter);
  EXPECT_FALSE(iter == multi_str.rend());

  ch = *iter--;
  EXPECT_EQ('a', ch);
  EXPECT_EQ('b', *iter);
  EXPECT_FALSE(iter == multi_str.rend());

  ch = *iter--;
  EXPECT_EQ('b', ch);
  EXPECT_EQ('c', *iter);
  EXPECT_FALSE(iter == multi_str.rend());

  ch = *(--iter);
  EXPECT_EQ('d', ch);
  EXPECT_EQ('d', *iter);
  EXPECT_TRUE(iter == multi_str.rbegin());
}

TEST(WideStringView, AnyAllNoneOf) {
  WideStringView str(L"aaaaaaaaaaaaaaaaab");
  EXPECT_FALSE(std::all_of(str.begin(), str.end(),
                           [](const wchar_t& c) { return c == L'a'; }));

  EXPECT_FALSE(std::none_of(str.begin(), str.end(),
                            [](const wchar_t& c) { return c == L'a'; }));

  EXPECT_TRUE(std::any_of(str.begin(), str.end(),
                          [](const wchar_t& c) { return c == L'a'; }));

  EXPECT_TRUE(pdfium::Contains(str, L'a'));
  EXPECT_TRUE(pdfium::Contains(str, L'b'));
  EXPECT_FALSE(pdfium::Contains(str, L'z'));
}

TEST(WideStringView, TrimmedRight) {
  WideStringView fred(L"FRED");
  EXPECT_EQ(L"FRED", fred.TrimmedRight(L'E'));
  EXPECT_EQ(L"FRE", fred.TrimmedRight(L'D'));
  WideStringView fredd(L"FREDD");
  EXPECT_EQ(L"FRE", fredd.TrimmedRight(L'D'));
}

TEST(WideString, FormatWidth) {
  EXPECT_EQ(L"    1", WideString::Format(L"%5d", 1));
  EXPECT_EQ(L"1", WideString::Format(L"%d", 1));
  EXPECT_EQ(L"    1", WideString::Format(L"%*d", 5, 1));
  EXPECT_EQ(L"1", WideString::Format(L"%-1d", 1));
  EXPECT_EQ(L"1", WideString::Format(L"%0d", 1));
  EXPECT_EQ(L"", WideString::Format(L"%1048576d", 1));
}

TEST(WideString, FormatPrecision) {
  EXPECT_EQ(L"1.12", WideString::Format(L"%.2f", 1.12345));
  EXPECT_EQ(L"1.123", WideString::Format(L"%.*f", 3, 1.12345));
  EXPECT_EQ(L"1.123450", WideString::Format(L"%f", 1.12345));
  EXPECT_EQ(L"1.123450", WideString::Format(L"%-1f", 1.12345));
  EXPECT_EQ(L"1.123450", WideString::Format(L"%0f", 1.12345));
  EXPECT_EQ(L"", WideString::Format(L"%.1048576f", 1.2));
}

TEST(WideString, FormatOutOfRangeChar) {
  EXPECT_NE(L"", WideString::Format(L"unsupported char '%c'", 0x00FF00FF));
}

TEST(WideString, FormatString) {
  // %ls and wide characters are the reliable combination across platforms.
  EXPECT_EQ(L"", WideString::Format(L"%ls", L""));
  EXPECT_EQ(L"", WideString::Format(L"%ls", WideString().c_str()));
  EXPECT_EQ(L"clams", WideString::Format(L"%ls", L"clams"));
  EXPECT_EQ(L"cla", WideString::Format(L"%.3ls", L"clams"));
  EXPECT_EQ(L"\u043e\u043f", WideString(L"\u043e\u043f"));

#if !BUILDFLAG(IS_APPLE)
  // See https://bugs.chromium.org/p/pdfium/issues/detail?id=1132
  EXPECT_EQ(L"\u043e\u043f", WideString::Format(L"\u043e\u043f"));
  EXPECT_EQ(L"\u043e\u043f", WideString::Format(L"%ls", L"\u043e\u043f"));
  EXPECT_EQ(L"\u043e", WideString::Format(L"%.1ls", L"\u043e\u043f"));
#endif
}

TEST(WideString, Empty) {
  WideString empty_str;
  EXPECT_TRUE(empty_str.IsEmpty());
  EXPECT_EQ(0u, empty_str.GetLength());

  const wchar_t* cstr = empty_str.c_str();
  EXPECT_TRUE(cstr);
  EXPECT_EQ(0u, UNSAFE_TODO(wcslen(cstr)));

  pdfium::span<const wchar_t> cspan = empty_str.span();
  EXPECT_TRUE(cspan.empty());
  EXPECT_FALSE(cspan.data());
}

TEST(WideString, InitializerList) {
  WideString many_str({L"clams", L" and ", L"oysters"});
  EXPECT_EQ(L"clams and oysters", many_str);
  many_str = {L"fish", L" and ", L"chips", L" and ", L"soda"};
  EXPECT_EQ(L"fish and chips and soda", many_str);
}

TEST(WideString, NullIterator) {
  WideString null_str;
  int32_t sum = 0;
  bool any_present = false;
  for (const auto& c : null_str) {
    sum += c;  // Avoid unused arg warnings.
    any_present = true;
  }
  EXPECT_FALSE(any_present);
  EXPECT_EQ(0, sum);
}

TEST(WideString, EmptyIterator) {
  WideString empty_str(L"");
  int32_t sum = 0;
  bool any_present = false;
  for (const auto& c : empty_str) {
    any_present = true;
    sum += c;  // Avoid unused arg warnings.
  }
  EXPECT_FALSE(any_present);
  EXPECT_EQ(0, sum);
}

TEST(WideString, OneCharIterator) {
  WideString one_str(L"a");
  int32_t sum = 0;
  bool any_present = false;
  for (const auto& c : one_str) {
    any_present = true;
    sum += c;  // Avoid unused arg warnings.
  }
  EXPECT_TRUE(any_present);
  EXPECT_EQ(static_cast<int32_t>(L'a'), sum);
}

TEST(WideString, MultiCharIterator) {
  WideString one_str(L"abc");
  int32_t sum = 0;
  bool any_present = false;
  for (const auto& c : one_str) {
    any_present = true;
    sum += c;  // Avoid unused arg warnings.
  }
  EXPECT_TRUE(any_present);
  EXPECT_EQ(static_cast<int32_t>(L'a' + L'b' + L'c'), sum);
}

TEST(WideString, StdBegin) {
  WideString one_str(L"abc");
  std::vector<wchar_t> vec(std::begin(one_str), std::end(one_str));
  ASSERT_EQ(3u, vec.size());
  EXPECT_EQ(L'a', vec[0]);
  EXPECT_EQ(L'b', vec[1]);
  EXPECT_EQ(L'c', vec[2]);
}

TEST(WideString, AnyAllNoneOf) {
  WideString str(L"aaaaaaaaaaaaaaaaab");
  EXPECT_FALSE(std::all_of(str.begin(), str.end(),
                           [](const wchar_t& c) { return c == L'a'; }));

  EXPECT_FALSE(std::none_of(str.begin(), str.end(),
                            [](const wchar_t& c) { return c == L'a'; }));

  EXPECT_TRUE(std::any_of(str.begin(), str.end(),
                          [](const wchar_t& c) { return c == L'a'; }));

  EXPECT_TRUE(pdfium::Contains(str, L'a'));
  EXPECT_TRUE(pdfium::Contains(str, L'b'));
  EXPECT_FALSE(pdfium::Contains(str, L'z'));
}

TEST(WideString, OStreamOverload) {
  std::ostringstream stream;

  // Basic case, empty string
  WideString str;
  stream << str;
  EXPECT_EQ("", stream.str());

  // Basic case, wide character
  str = L"\u20AC";
  stream << str;
  EXPECT_EQ("\u20AC", stream.str());

  // Basic case, non-empty string
  str = L"def";
  stream.str("");
  stream << "abc" << str << "ghi";
  EXPECT_EQ("abcdefghi", stream.str());

  // Changing the WideString does not change the stream it was written to.
  str = L"123";
  EXPECT_EQ("abcdefghi", stream.str());

  // Writing it again to the stream will use the latest value.
  stream.str("");
  stream << "abc" << str << "ghi";
  EXPECT_EQ("abc123ghi", stream.str());

  wchar_t stringWithNulls[]{'x', 'y', '\0', 'z'};

  // Writing a WideString with nulls and no specified length treats it as
  // a C-style null-terminated string.
  str = WideString(stringWithNulls);
  EXPECT_EQ(2u, str.GetLength());
  stream.str("");
  stream << str;
  EXPECT_EQ(2u, stream.tellp());

  // Writing a WideString with nulls but specifying its length treats it as
  // a C++-style string.
  // SAFETY: known fixed-length string.
  str = UNSAFE_BUFFERS(WideString(stringWithNulls, 4));
  EXPECT_EQ(4u, str.GetLength());
  stream.str("");
  stream << str;
  EXPECT_EQ(4u, stream.tellp());

  // << operators can be chained.
  WideString str1(L"abc");
  WideString str2(L"def");
  stream.str("");
  stream << str1 << str2;
  EXPECT_EQ("abcdef", stream.str());
}

TEST(WideString, WideOStreamOverload) {
  std::wostringstream stream;

  // Basic case, empty string
  WideString str;
  stream << str;
  EXPECT_EQ(L"", stream.str());

  // Basic case, wide character
  str = L"\u20AC";
  stream << str;
  EXPECT_EQ(L"\u20AC", stream.str());

  // Basic case, non-empty string
  str = L"def";
  stream.str(L"");
  stream << L"abc" << str << L"ghi";
  EXPECT_EQ(L"abcdefghi", stream.str());

  // Changing the WideString does not change the stream it was written to.
  str = L"123";
  EXPECT_EQ(L"abcdefghi", stream.str());

  // Writing it again to the stream will use the latest value.
  stream.str(L"");
  stream << L"abc" << str << L"ghi";
  EXPECT_EQ(L"abc123ghi", stream.str());

  wchar_t stringWithNulls[]{'x', 'y', '\0', 'z'};

  // Writing a WideString with nulls and no specified length treats it as
  // a C-style null-terminated string.
  str = WideString(stringWithNulls);
  EXPECT_EQ(2u, str.GetLength());
  stream.str(L"");
  stream << str;
  EXPECT_EQ(2u, stream.tellp());

  // Writing a WideString with nulls but specifying its length treats it as
  // a C++-style string.
  str = UNSAFE_BUFFERS(WideString(stringWithNulls, 4));
  EXPECT_EQ(4u, str.GetLength());
  stream.str(L"");
  stream << str;
  EXPECT_EQ(4u, stream.tellp());

  // << operators can be chained.
  WideString str1(L"abc");
  WideString str2(L"def");
  stream.str(L"");
  stream << str1 << str2;
  EXPECT_EQ(L"abcdef", stream.str());
}

TEST(WideStringView, OStreamOverload) {
  // Basic case, empty string
  {
    std::ostringstream stream;
    WideStringView str;
    stream << str;
    EXPECT_EQ("", stream.str());
  }

  // Basic case, non-empty string
  {
    std::ostringstream stream;
    WideStringView str(L"def");
    stream << "abc" << str << "ghi";
    EXPECT_EQ("abcdefghi", stream.str());
  }

  // Basic case, wide character
  {
    std::ostringstream stream;
    WideStringView str(L"\u20AC");
    stream << str;
    EXPECT_EQ("\u20AC", stream.str());
  }

  // Changing the WideStringView does not change the stream it was written to.
  {
    std::ostringstream stream;
    WideStringView str(L"abc");
    stream << str;
    str = L"123";
    EXPECT_EQ("abc", stream.str());
  }

  // Writing it again to the stream will use the latest value.
  {
    std::ostringstream stream;
    WideStringView str(L"abc");
    stream << str;
    stream.str("");
    str = L"123";
    stream << str;
    EXPECT_EQ("123", stream.str());
  }

  // Writing a WideStringView with nulls and no specified length treats it as
  // a C-style null-terminated string.
  {
    wchar_t stringWithNulls[]{'x', 'y', '\0', 'z'};
    std::ostringstream stream;
    WideStringView str(stringWithNulls);
    EXPECT_EQ(2u, str.GetLength());
    stream << str;
    EXPECT_EQ(2u, stream.tellp());
    str = L"";
  }

  // Writing a WideStringView with nulls but specifying its length treats it as
  // a C++-style string.
  {
    wchar_t stringWithNulls[]{'x', 'y', '\0', 'z'};
    // SAFETY: known array above.
    auto str = UNSAFE_BUFFERS(WideStringView(stringWithNulls, 4));
    std::ostringstream stream;
    EXPECT_EQ(4u, str.GetLength());
    stream << str;
    EXPECT_EQ(4u, stream.tellp());
    str = L"";
  }

  // << operators can be chained.
  {
    std::ostringstream stream;
    WideStringView str1(L"abc");
    WideStringView str2(L"def");
    stream << str1 << str2;
    EXPECT_EQ("abcdef", stream.str());
  }
}

TEST(WideStringView, WideOStreamOverload) {
  // Basic case, empty string
  {
    std::wostringstream stream;
    WideStringView str;
    stream << str;
    EXPECT_EQ(L"", stream.str());
  }

  // Basic case, non-empty string
  {
    std::wostringstream stream;
    WideStringView str(L"def");
    stream << "abc" << str << "ghi";
    EXPECT_EQ(L"abcdefghi", stream.str());
  }

  // Basic case, wide character
  {
    std::wostringstream stream;
    WideStringView str(L"\u20AC");
    stream << str;
    EXPECT_EQ(L"\u20AC", stream.str());
  }

  // Changing the WideStringView does not change the stream it was written to.
  {
    std::wostringstream stream;
    WideStringView str(L"abc");
    stream << str;
    str = L"123";
    EXPECT_EQ(L"abc", stream.str());
  }

  // Writing it again to the stream will use the latest value.
  {
    std::wostringstream stream;
    WideStringView str(L"abc");
    stream << str;
    stream.str(L"");
    str = L"123";
    stream << str;
    EXPECT_EQ(L"123", stream.str());
  }

  // Writing a WideStringView with nulls and no specified length treats it as
  // a C-style null-terminated string.
  {
    wchar_t stringWithNulls[]{'x', 'y', '\0', 'z'};
    std::wostringstream stream;
    WideStringView str(stringWithNulls);
    EXPECT_EQ(2u, str.GetLength());
    stream << str;
    EXPECT_EQ(2u, stream.tellp());
  }

  // Writing a WideStringView with nulls but specifying its length treats it as
  // a C++-style string.
  {
    wchar_t stringWithNulls[]{'x', 'y', '\0', 'z'};
    // SAFETY: known array above.
    auto str = UNSAFE_BUFFERS(WideStringView(stringWithNulls, 4));
    std::wostringstream stream;
    EXPECT_EQ(4u, str.GetLength());
    stream << str;
    EXPECT_EQ(4u, stream.tellp());
  }

  // << operators can be chained.
  {
    std::wostringstream stream;
    WideStringView str1(L"abc");
    WideStringView str2(L"def");
    stream << str1 << str2;
    EXPECT_EQ(L"abcdef", stream.str());
  }
}

TEST(WideString, FormatInteger) {
  // Base case of 0.
  EXPECT_EQ(L"0", WideString::FormatInteger(0));

  // Positive ordinary number.
  EXPECT_EQ(L"123456", WideString::FormatInteger(123456));

  // Negative ordinary number.
  EXPECT_EQ(L"-123456", WideString::FormatInteger(-123456));

  // int limits.
  EXPECT_EQ(L"2147483647", WideString::FormatInteger(INT_MAX));
  EXPECT_EQ(L"-2147483648", WideString::FormatInteger(INT_MIN));
}

TEST(WideString, FXHashCodeWide) {
  EXPECT_EQ(0u, FX_HashCode_GetW(L""));
  EXPECT_EQ(65u, FX_HashCode_GetW(L"A"));
  EXPECT_EQ(97u, FX_HashCode_GetLoweredW(L"A"));
  EXPECT_EQ(1313 * 65u + 66u, FX_HashCode_GetW(L"AB"));
  EXPECT_EQ(FX_HashCode_GetAsIfW("AB\xff"), FX_HashCode_GetW(L"AB\xff"));
  EXPECT_EQ(FX_HashCode_GetLoweredAsIfW("AB\xff"),
            FX_HashCode_GetLoweredW(L"AB\xff"));
}

}  // namespace fxcrt
