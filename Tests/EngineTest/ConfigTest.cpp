#include "doctest.h"

import SimpleEngine.Config;


TEST_SUITE("SimpleEngine.Config")
{
TEST_CASE("not exist config file")
{
    se::config::Config config;
    const se::config::ParseResult v = config.ReadConfig("Test.t1oml");

    CHECK(!v.has_value());
}
}
