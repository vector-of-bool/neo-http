#include <neo/http/parse/chunked.hpp>

#include <neo/iostream_io.hpp>
#include <neo/pathological_buffer_range.hpp>
#include <neo/string_io.hpp>
#include <neo/test_concept.hpp>

#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>

NEO_TEST_CONCEPT(neo::buffer_source<neo::http::chunked_buffers<neo::proto_buffer_source>>);

namespace fs = std::filesystem;

namespace {
const auto THIS_FILE = fs::canonical(fs::path(__FILE__));
const auto ROOT      = (THIS_FILE / "../../../../..").lexically_normal();
const auto DATA_DIR  = ROOT / "data";
}  // namespace

using namespace neo::literals;

TEST_CASE("Parse a basic chunk head") {
    auto buf = "4\r\nTest"_buf;
    auto chk = neo::http::chunk_head::parse(buf);
    CHECK(chk.chunk_size == 4);
    CHECK(chk.ext_buf.empty());
    CHECK(std::string_view(chk.parse_tail) == "Test");
}

TEST_CASE("Pull data from a chunk decoder") {
    auto buf = neo::const_buffer(
        "4\r\n"
        "Text\r\n"
        "0\r\n\r\n");
    neo::http::chunked_buffers chunks{neo::buffers_consumer{buf}};
    CHECK_FALSE(chunks.done());
    neo::const_buffer text = chunks.next(4);
    CHECK(text.size() == 4);
    CHECK(std::string_view(text) == "Text");
    chunks.consume(1);
    CHECK(std::string_view(chunks.next(4)) == "ext");
    chunks.consume(3);
    CHECK(std::string_view(chunks.next(4)) == "");
    CHECK(chunks.done());
}

TEST_CASE("Pull from a buffer range, worst case") {
    auto buf = neo::const_buffer(
        "4\r\n"
        "Text\r\n"
        "0\r\n\r\n");
    neo::pathological_buffer_range rng{buf};
    neo::http::chunked_buffers     chunks{neo::buffers_consumer{rng}};
    neo::const_buffer              text = chunks.next(4);
    CHECK(text.size() == 1);
    CHECK(std::string_view(text) == "T");
    chunks.consume(1);
    CHECK(std::string_view(chunks.next(4)) == "e");
    chunks.consume(1);
    CHECK(std::string_view(chunks.next(4)) == "x");
    chunks.consume(1);
    CHECK(std::string_view(chunks.next(4)) == "t");
    chunks.consume(1);
    CHECK(std::string_view(chunks.next(4)) == "");
}

TEST_CASE("Read all of Shakespeare") {
    std::ifstream         infile{DATA_DIR / "shakespeare.chunked.txt", std::ios::binary};
    neo::iostream_io      file_in{infile};
    neo::string_dynbuf_io str;
    neo::buffer_copy(str, file_in);

    neo::http::chunked_buffers chunk_io{
        neo::buffers_consumer{neo::const_buffer{neo::as_buffer(str.read_area_view())}}};
    neo::string_dynbuf_io str2;
    neo::buffer_copy(str2, chunk_io);
    CHECK(str2.available() == 5'447'532);
}
