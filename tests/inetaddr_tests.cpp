//
// Created by Marco Massenzio on 4/6/15.
//

#include <gtest/gtest.h>
#include <iostream>

#include "inetaddr.hpp"

class InetAddrTest : public ::testing::Test
{

};

TEST(InetAddrTest, IsValid)
{
    ASSERT_TRUE(InetAddress::is_valid("10.10.1.2"));
    ASSERT_TRUE(InetAddress::is_valid("192.168.1.1"));
}

TEST(InetAddrTest, CanParse) {
    auto address = InetAddress("10.10.2.15", 5050);
    EXPECT_EQ("10.10.2.15:5050", address.to_string());
    EXPECT_EQ("1.2.3.4:5", InetAddress("1.2.3.4", 5).to_string());
}

TEST(InetAddrTest, CantParseMalformed)
{
    ASSERT_FALSE(InetAddress::is_valid("10.32.0.0/16"));
    // Not enough octets (note the missing fourth in the initializer)
    EXPECT_EQ("0.0.0.0:5050", InetAddress("100.111.122", 5050).to_string());
    EXPECT_EQ("192.0.0.99", InetAddress("192.999.999.99").to_string());
}

TEST(InetAddrTest, CanInitOctects)
{
    short unsigned int add[] = {10, 10, 0, 101};
    auto address = InetAddress(add, 9999);
    ASSERT_EQ("10.10.0.101:9999", address.to_string());
    short unsigned int add2[] = {174, 254, 0, 22};
    ASSERT_EQ("174.254.0.22", InetAddress(add2, 0).to_string());
}


int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
