#include "doctest.h"

import SimpleEngine.Config;


TEST_SUITE("SimpleEngine.Config")
{
TEST_CASE("not exist config file")
{
    const se::config::ParseResult v = se::config::Config::ReadConfig(u8"Test.t1oml");

    CHECK(!v.has_value());
}
}
