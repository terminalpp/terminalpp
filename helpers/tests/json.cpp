#include "../json.h"
#include "../tests.h"

TEST(helpers_json, empty) {
    JSON json;
    EXPECT(json.empty());
}

TEST(helpers_json, null) {
    JSON json;
    EXPECT(json.isNull());
    EXPECT(json.kind() == JSON::Kind::Null);
    JSON j2(nullptr);
    EXPECT(j2.isNull());
    EXPECT(j2.kind() == JSON::Kind::Null);
    JSON j3(1);
    EXPECT(! j3.isNull());
    j3 = nullptr;
    EXPECT(j3.isNull());
}

TEST(helpers_json, integer) {
    JSON j(1);
    EXPECT(j.kind() == JSON::Kind::Integer);
    int i = j;
    EXPECT(i == 1);
    j = 67;
    i = j;
    EXPECT(i == 67);
}

TEST(helpers_json, boolean) {
    JSON j{true};
    EXPECT(j.kind() == JSON::Kind::Boolean);
    bool i = j;
    EXPECT(i == true);
    j = false;
    i = j;
    EXPECT(i == false);
}
