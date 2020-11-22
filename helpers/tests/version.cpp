#include "helpers/tests.h"

#include "helpers/version.h"

TEST(helpers_version, emptyVersion) {
    Version v;
    EXPECT_EQ(v.major, 0);
    EXPECT_EQ(v.minor, 0);
    EXPECT_EQ(v.build, 0);
    Version v2{""};
    EXPECT_EQ(v2.major, 0);
    EXPECT_EQ(v2.minor, 0);
    EXPECT_EQ(v2.build, 0);
}

TEST(helpers_version, constructor) {
    Version v{1,2,3};
    EXPECT_EQ(v.major, 1);
    EXPECT_EQ(v.minor, 2);
    EXPECT_EQ(v.build, 3);
}

TEST(helpers_version, fromString) {
    Version v{"1.2.3"};
    EXPECT_EQ(v.major, 1);
    EXPECT_EQ(v.minor, 2);
    EXPECT_EQ(v.build, 3);
    v = Version{"2.4"};
    EXPECT_EQ(v.major, 2);
    EXPECT_EQ(v.minor, 4);
    EXPECT_EQ(v.build, 0);
    v = Version{"3"};
    EXPECT_EQ(v.major, 3);
    EXPECT_EQ(v.minor, 0);
    EXPECT_EQ(v.build, 0);
}

TEST(helpers_version, comparsion) {
    EXPECT(Version{"1.0.0"} < Version{"2.0.0"});
    EXPECT(Version{"1.0.0"} < Version{"1.1.0"});
    EXPECT(Version{"1.0.0"} < Version{"1.0.1"});
    EXPECT(Version{"1.2.0"} < Version{"2.1.0"});
    EXPECT(Version{"1.0.0"} <= Version{"2.0.0"});
    EXPECT(Version{"1.0.0"} <= Version{"1.1.0"});
    EXPECT(Version{"1.0.0"} <= Version{"1.0.1"});
    EXPECT(Version{"1.2.0"} <= Version{"2.1.0"});
    EXPECT(! (Version{"1.2.0"} < Version{"1.2.0"}));
    EXPECT(Version{"1.2.0"} <= Version{"1.2.0"});
    EXPECT(Version{"1.2.0"} >= Version{"1.2.0"});
    EXPECT(! (Version{"1.2.0"} > Version{"1.2.0"}));
    EXPECT(Version{"2.0.0"} > Version{"1.0.0"});
    EXPECT(Version{"1.1.0"} > Version{"1.0.0"});
    EXPECT(Version{"1.0.1"} > Version{"1.0.0"});
    EXPECT(Version{"1.2.3"} > Version{"1.1.10"});
    EXPECT(Version{"2.0.0"} >= Version{"1.0.0"});
    EXPECT(Version{"1.1.0"} >= Version{"1.0.0"});
    EXPECT(Version{"1.0.1"} >= Version{"1.0.0"});
    EXPECT(Version{"1.2.3"} >= Version{"1.1.10"});
}
