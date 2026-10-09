#include "doctest.h"

#include "Core/Base/Json.h"
#include "Core/Base/ValueConvert.h"

using namespace hg;

TEST_SUITE("Value")
{
    TEST_CASE("object keeps insertion order and replaces existing keys")
    {
        ValueObject object;
        object.set("b", Value(1));
        object.set("a", Value(2));
        object.set("b", Value(3));

        REQUIRE(object.size() == 2);
        CHECK(object.begin()->key == "b");
        CHECK(object.get("b").asInt() == 3);
        CHECK(object.get("missing").isNull());
        CHECK(object.erase("a"));
        CHECK_FALSE(object.contains("a"));
    }

    TEST_CASE("numbers compare across int and double")
    {
        CHECK(Value(2) == Value(2.0));
        CHECK_FALSE(Value(2) == Value(2.5));
        CHECK(Value(2.5).asInt() == 3);
        CHECK(Value(7).asFloat() == doctest::Approx(7.0f));
    }

    TEST_CASE("object equality ignores member order")
    {
        ValueObject left{{"x", Value(1)}, {"y", Value(2)}};
        ValueObject right{{"y", Value(2)}, {"x", Value(1)}};
        CHECK(left == right);
    }

    TEST_CASE("vector and color conversion")
    {
        CHECK(readVec2(writeVec2({3.5f, -2.0f})) == Vec2{3.5f, -2.0f});
        CHECK(readColor(Value("#FF8000")) == Color{255, 128, 0, 255});
        CHECK(readColor(Value(ValueArray{Value(1), Value(2), Value(3)})) == Color{1, 2, 3, 255});
        CHECK_FALSE(readVec2(Value("text")).has_value());
    }
}

TEST_SUITE("Json")
{
    TEST_CASE("round trip preserves structure")
    {
        const char* text = R"({"name": "door", "position": [10, 20.5], "nested": {"flag": true, "list": [{"a": 1}]}, "none": null})";
        auto parsed = parseJson(text);
        REQUIRE(parsed.ok());

        auto reparsed = parseJson(writeJson(parsed.value()));
        REQUIRE(reparsed.ok());
        CHECK(reparsed.value() == parsed.value());
    }

    TEST_CASE("comments and trailing commas are accepted")
    {
        auto parsed = parseJson("{\n// comment\n\"a\": [1, 2,],\n}");
        REQUIRE(parsed.ok());
        CHECK(parsed.value().asObject().get("a").asArray().size() == 2);
    }

    TEST_CASE("errors report line and column")
    {
        auto parsed = parseJson("{\n\"a\": 1,\n\"b\": }");
        REQUIRE_FALSE(parsed.ok());
        CHECK(parsed.error().find("line 3") != std::string::npos);
    }

    TEST_CASE("writer keeps scalar arrays on one line and shortens floats")
    {
        ValueObject object;
        object.set("position", writeVec2({0.3f, 1920.0f}));
        const std::string text = writeJson(Value(object));
        CHECK(text.find("[0.3, 1920]") != std::string::npos);
    }

    TEST_CASE("writer escapes strings")
    {
        const std::string text = writeJsonCompact(Value("quote \" and \\ slash"));
        auto parsed = parseJson(text);
        REQUIRE(parsed.ok());
        CHECK(parsed.value().asString() == "quote \" and \\ slash");
    }
}
