#include "helpers/tests.h"

#include "helpers/string.h"

TEST(string, trim) {
    EXPECT_EQ(Trim("foobar"), "foobar");
    EXPECT_EQ(Trim("  foobar   "), "foobar");
    EXPECT_EQ(Trim(" \t\r\nfoobar \t\r\n"),  "foobar");
    EXPECT_EQ(Trim(" \t\r\nfoo bar \t\r\n"), "foo bar");
}

TEST(string, trimEmpty) {
    EXPECT_EQ(Trim(""), "");
    EXPECT_EQ(Trim(" "), "");
    EXPECT_EQ(Trim(" \t\r\n "), "");
}

TEST(string, trimRight) {
    EXPECT_EQ(TrimRight("foobar"), "foobar");
    EXPECT_EQ(TrimRight("  foobar   "), "  foobar");
    EXPECT_EQ(TrimRight(" \t\r\nfoobar \t\r\n"), " \t\r\nfoobar");
    EXPECT_EQ(TrimRight(" \t\r\nfoo bar \t\r\n"), " \t\r\nfoo bar");
}

TEST(string, trimRightEmpty) {
    EXPECT_EQ(TrimRight(""), "");
    EXPECT_EQ(TrimRight(" "), "");
    EXPECT_EQ(TrimRight(" \t\r\n "), "");
}