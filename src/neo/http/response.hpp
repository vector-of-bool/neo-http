#pragma once

#include "./headers.hpp"
#include "./parse/chunked.hpp"
#include <neo/http/parse/header.hpp>
#include <neo/http/parse/response.hpp>
#include <neo/http/version.hpp>

#include <neo/buffer_source.hpp>
#include <neo/string_io.hpp>
#include <neo/transform_io.hpp>
#include <neo/ufmt.hpp>

#include <map>

namespace neo::http {

struct simple_response {
    int           status = 0;
    std::string   status_message;
    http::version version;

    http::headers headers;

    std::size_t head_byte_size = 0;
};

template <typename ResponseType, buffer_input In>
ResponseType read_response_head(In&& in_) {
    neo::string_dynbuf_io strbuf;

    auto&&       in = ensure_buffer_source(in_);
    ResponseType ret;

    // Copy data from the source until we find the ending CRLFCRLF sequence
    const std::string_view END_SEQ = "\r\n\r\n";
    while (true) {
        auto prev_avail = strbuf.available();
        auto n_copied   = buffer_copy(strbuf, in.next(1024));
        if (n_copied == 0) {
            throw std::runtime_error("Didn't find terminal CRLF+CRLF for HTTP response-head?");
        }
        if (auto end_pos = strbuf.read_area_view().find(END_SEQ);
            end_pos != std::string_view::npos) {
            // Consume from the input only the amount to get past the CRLFCRLF
            auto head_end = end_pos + END_SEQ.size();
            auto fin_size = head_end - prev_avail;
            ret.head_byte_size += fin_size;
            in.consume(fin_size);
            break;
        }
        // Didn't find it yet. Keep looking.
        ret.head_byte_size += n_copied;
        in.consume(n_copied);
        if (strbuf.available() > 1024 * 1024) {
            throw std::runtime_error(
                "Didn't find terminal CRLF+CRLF within first 1MB of HTTP response stream. Is this "
                "an actual HTTP response?");
        }
    }

    auto head = response_head::parse(const_buffer(strbuf.read_area_view()));

    ret.version        = head.start_line.http_version;
    ret.status         = head.start_line.status;
    ret.status_message = std::string(head.start_line.phrase_view);

    for (auto header : head.headers.iter_headers()) {
        ret.headers.add(header.key_view, header.value_view);
    }

    return ret;
}

// clang-format off
template <buffer_output Out, buffer_input In, typename TransformerFactory>
std::size_t read_response(Out&& out_, In&& in_, TransformerFactory&& tr_factory)
    requires requires(std::string_view te) {
        { tr_factory(te, ensure_buffer_source(in_)) } -> buffer_source;
    }
{
    // clang-format on
    auto&& in  = ensure_buffer_source(in_);
    auto&& out = ensure_buffer_sink(out_);

    auto head = read_response_head<simple_response>(in);
    auto clen = head.headers.find(standard_headers::content_length);
    auto te   = head.headers.find(standard_headers::transfer_encoding);

    if (te) {
        auto&& new_in = tr_factory(te->value, in);
        return buffer_copy(out, new_in);
    } else if (clen) {
        return buffer_copy(out, in, std::stoi(clen->value));
    }
    return 0;
}

template <buffer_output Out, buffer_input In>
std::size_t read_response(Out&& out, In&& in) {
    return read_response(out, in, [](std::string_view te, auto&& in) {
        if (te == "chunked") {
            return chunked_buffers{in};
        }
        throw std::runtime_error(
            ufmt("Response has a Transport-Encoding, but no decoders were given to read any "
                 "encoded data. (Transport encoding is '{}')",
                 te));
    });
}

}  // namespace neo::http
