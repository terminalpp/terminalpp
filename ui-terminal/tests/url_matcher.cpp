#include "helpers/tests.h"

#include "../url_matcher.h"

using namespace ui;

TEST(url_matcher, simple) {
    EXPECT(UrlMatcher::IsValid("http://terminalpp.com"));
    EXPECT(UrlMatcher::IsValid("https://terminalpp.com"));
}

TEST(url_matcher, withPort) {
    EXPECT(UrlMatcher::IsValid("http://terminalpp.com:80"));
    EXPECT(UrlMatcher::IsValid("https://terminalpp.com:80"));
}

TEST(url_matcher, withAddress) {
    EXPECT(UrlMatcher::IsValid("http://terminalpp.com/"));
    EXPECT(UrlMatcher::IsValid("https://terminalpp.com/~term"));
    EXPECT(UrlMatcher::IsValid("https://terminalpp.com/~term/foo/bar/"));
}

TEST(url_matcher, withArguments) {
    EXPECT(UrlMatcher::IsValid("http://terminalpp.com?foo=bar"));
    EXPECT(UrlMatcher::IsValid("https://terminalpp.com/~term?foo=bar"));
    EXPECT(UrlMatcher::IsValid("http://terminalpp.com/?foo=bar"));
    EXPECT(UrlMatcher::IsValid("https://terminalpp.com/~term/?foo=bar"));
    EXPECT(UrlMatcher::IsValid("https://terminalpp.com/~term/?foo=bar.xy.3&q=7"));
}
