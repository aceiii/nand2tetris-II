#include <string>
#include <string_view>
#include <print>
#include <catch2/catch_test_macros.hpp>

#include "../src/hdl/test_parser.hpp"


using namespace hdl::test;

TEST_CASE("TestParser parses empty string") {
  auto result = TestParser::Parse("");

  INFO("error: " << result.error());
  REQUIRE(result.has_value());
  REQUIRE(result->empty());
}

TEST_CASE("TestParser ignores comments") {
  auto result = TestParser::Parse("// this is a comment");

  INFO("error: " << result.error());
  REQUIRE(result.has_value());
  REQUIRE(result->empty());
}

TEST_CASE("TestParser fails on invalid contents") {
  auto result = TestParser::Parse("hello, world");

  INFO("error: " << result.error());
  REQUIRE(!result.has_value());
}

TEST_CASE("TestParser parses eval command") {
  auto result = TestParser::Parse("eval;");

  INFO("error: " << result.error());
  REQUIRE(result.has_value());
  REQUIRE(result->size() == 1);
  REQUIRE(result->at(0).type == CommandType::Eval);
}

TEST_CASE("TestParser fails parsing command without delimiter") {
  auto result = TestParser::Parse("eval");

  INFO("error: " << result.error());
  REQUIRE(!result.has_value());
}

TEST_CASE("TestParser parses output command") {
  auto result = TestParser::Parse("output;");

  INFO("error: " << result.error());
  REQUIRE(result.has_value());
  REQUIRE(result->size() == 1);
  REQUIRE(result->at(0).type == CommandType::Output);
}

TEST_CASE("TestParser parses multiple commands in group") {
  auto result = TestParser::Parse("eval, output;");

  INFO("error: " << result.error());
  REQUIRE(result.has_value());
  REQUIRE(result->size() == 2);
  REQUIRE(result->at(0).type == CommandType::Eval);
  REQUIRE(result->at(1).type == CommandType::Output);
}

TEST_CASE("TestParser parses multiple commands ungrouped") {
  auto result = TestParser::Parse("eval;output;");

  INFO("error: " << result.error());
  REQUIRE(result.has_value());
  REQUIRE(result->size() == 2);
  REQUIRE(result->at(0).type == CommandType::Eval);
  REQUIRE(result->at(1).type == CommandType::Output);
}

TEST_CASE("TestParser parses commands with leading whitespace") {
  auto result = TestParser::Parse("     eval;output;");

  INFO("error: " << result.error());
  REQUIRE(result.has_value());
  REQUIRE(result->size() == 2);
  REQUIRE(result->at(0).type == CommandType::Eval);
  REQUIRE(result->at(1).type == CommandType::Output);
}

TEST_CASE("TestParser parses commands with trailing whitespace") {
  auto result = TestParser::Parse("eval  ;output ;  ");

  INFO("error: " << result.error());
  REQUIRE(result.has_value());
  REQUIRE(result->size() == 2);
  REQUIRE(result->at(0).type == CommandType::Eval);
  REQUIRE(result->at(1).type == CommandType::Output);
}

TEST_CASE("TestParser parses command and ignores trailing comment") {
  auto result = TestParser::Parse("eval ;  // a trailing comment  ");

  REQUIRE(result.has_value());
  REQUIRE(result->size() == 1);
  REQUIRE(result->at(0).type == CommandType::Eval);
}

TEST_CASE("TestParser parses commands and ignores all comments") {
  auto result = TestParser::Parse("eval  ;  // trailing comment; output ;  // trailing comment");
  INFO("error: " << result.error());
  REQUIRE(result.has_value());
  REQUIRE(result->size() == 1);
  REQUIRE(result->at(0).type == CommandType::Eval);
}

TEST_CASE("TestParser parses multiline commands and ignores all comments") {
  std::string_view contents = R"(eval  ;  // trailing comment;
  output ;  // trailing comment)";

  auto result = TestParser::Parse(contents);

  INFO("error: " << result.error());
  REQUIRE(result.has_value());
  REQUIRE(result->size() == 1);
  REQUIRE(result->at(0).type == CommandType::Eval);
}
