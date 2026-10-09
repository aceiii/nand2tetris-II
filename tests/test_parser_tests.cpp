#include <string>
#include <string_view>
#include <print>
#include <catch2/catch_test_macros.hpp>

#include "../src/hdl/test_parser.hpp"


using namespace hdl::test;

TEST_CASE("TestParser parses empty string") {
  auto result = TestParser::Parse("");

  REQUIRE(result.has_value());
  REQUIRE(result->empty());
}

TEST_CASE("TestParser ignores comments") {
  auto result = TestParser::Parse("// this is a comment");

  REQUIRE(result.has_value());
  REQUIRE(result->empty());
}

TEST_CASE("TestParser fails on invalid contents") {
  auto result = TestParser::Parse("hello, world");
  REQUIRE(!result.has_value());
}

TEST_CASE("TestParser parses eval command") {
  auto result = TestParser::Parse("eval;");
  REQUIRE(result.has_value());
  REQUIRE(result->size() == 1);
  REQUIRE(result->at(0).type == CommandType::Eval);
}

TEST_CASE("TestParser parses output command") {
  auto result = TestParser::Parse("output;");
  REQUIRE(result.has_value());
  REQUIRE(result->size() == 1);
  REQUIRE(result->at(0).type == CommandType::Output);
}

TEST_CASE("TestParser parses multiple commands") {
  auto result = TestParser::Parse("eval, output;");
  REQUIRE(result.has_value());
  REQUIRE(result->size() == 2);
  REQUIRE(result->at(0).type == CommandType::Eval);
  REQUIRE(result->at(1).type == CommandType::Output);
}
