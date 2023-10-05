#include "helpers/tests.h"

#include "../url_matcher.h"

using namespace ui;

TEST(url_matcher, invalidUrls) {
    EXPECT(!UrlMatcher::IsValid("http:foobar"));
    EXPECT(!UrlMatcher::IsValid("http:/"));
    EXPECT(!UrlMatcher::IsValid("http://"));
    EXPECT(!UrlMatcher::IsValid("http://@"));
    EXPECT(!UrlMatcher::IsValid("http:///"));
}

TEST(url_matcher, simple) {
    EXPECT(UrlMatcher::IsValid("http://terminalpp.com"));
    EXPECT(UrlMatcher::IsValid("https://terminalpp.com"));
}

TEST(url_matcher, withPort) {
    EXPECT(UrlMatcher::IsValid("http://terminalpp.com:80"));
    EXPECT(UrlMatcher::IsValid("https://terminalpp.com:80"));
    EXPECT(UrlMatcher::IsValid("https://terminalpp.com/foo/bar:80"));
    EXPECT(UrlMatcher::IsValid("https://terminalpp.com?foo=bar:80"));
    EXPECT(UrlMatcher::IsValid("https://terminalpp.com?foo=bar&baz=7:80"));
    EXPECT(UrlMatcher::IsValid("https://terminalpp.com?foo=bar&baz=:80"));
    EXPECT(UrlMatcher::IsValid("https://terminalpp.com/hello?foo=bar&a=b:80"));
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
    EXPECT(UrlMatcher::IsValid("https://terminalpp.com/~term/?foo=bar.xy.3&q="));
}

TEST(url_matcher, noTLD) {
    EXPECT(UrlMatcher::IsValid("http://terminalpp"));
    EXPECT(UrlMatcher::IsValid("https://terminalpp/~term"));
    EXPECT(UrlMatcher::IsValid("https://terminalpp/~term/foo/bar/"));
    EXPECT(UrlMatcher::IsValid("http://terminalpp?foo=bar"));
    EXPECT(UrlMatcher::IsValid("https://terminalpp/~term?foo=bar"));
    EXPECT(UrlMatcher::IsValid("http://terminalpp/?foo=bar"));
    EXPECT(UrlMatcher::IsValid("https://terminalpp/~term/?foo=bar"));
    EXPECT(UrlMatcher::IsValid("https://terminalpp/~term/?foo=bar.xy.3&q=7"));
}

TEST(url_matcher, IP) {
    EXPECT(UrlMatcher::IsValid("http://10.20.30.40"));
    EXPECT(UrlMatcher::IsValid("http://10.20.30.40/foo"));
    EXPECT(UrlMatcher::IsValid("http://10.20.30.40/"));
    EXPECT(UrlMatcher::IsValid("http://10.20.30.40?foo=bar"));
    EXPECT(UrlMatcher::IsValid("http://10.20.30.40/foo?foo=bar&q=7"));
}
