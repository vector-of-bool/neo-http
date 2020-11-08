#include <neo/http/headers.hpp>

#include <catch2/catch.hpp>

TEST_CASE("Create a simple headers container") {
    neo::http::headers hds;
    hds.add("Content-Type", "bunch-o-bytes");
    CHECK(hds["Content-Type"].value == "bunch-o-bytes");
    CHECK(hds["Content-type"].value == "bunch-o-bytes");
    CHECK(hds["content-type"].value == "bunch-o-bytes");
    CHECK(hds["CONTENT-TYPE"].value == "bunch-o-bytes");
    CHECK(hds["CoNtEnT-TyPe"].value == "bunch-o-bytes");

    CHECK_FALSE(hds.find("Transport-Encoding"));
}
