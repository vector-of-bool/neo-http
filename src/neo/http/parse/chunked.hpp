#pragma once

#include "./common.hpp"

#include <neo/buffer_algorithm.hpp>
#include <neo/buffer_source.hpp>
#include <neo/bytewise_iterator.hpp>
#include <neo/const_buffer.hpp>
#include <neo/switch_coro.hpp>

#include <algorithm>

namespace neo::http {

struct chunk_head {
    std::size_t  chunk_size = size_t(-1);
    const_buffer ext_buf;

    const_buffer parse_tail;

    static chunk_head parse(const_buffer buf) noexcept;

    constexpr std::size_t byte_size() const noexcept {
        std::size_t size_len = 0;
        auto        csize    = chunk_size;
        do {
            ++size_len;
        } while (csize >>= 4);
        return size_len + ext_buf.size() + 2;  // +2 for CRLF
    }

    constexpr bool valid() const noexcept { return chunk_size != size_t(-1); }
};

template <buffer_source InnerSource, std::size_t HeadMaxSize = 256>
class chunked_buffers {
public:
    enum class state_t {
        waiting_header,
        chunk_data,
        done,
    };

private:
    wrap_refs_t<InnerSource> _inner;

    state_t _state = state_t::waiting_header;

    // Copy bytes out from the inner buffer until we find a chunk-head line
    std::array<std::byte, HeadMaxSize> _pending;
    // Number of bytes we've copied into _pending
    std::size_t _n_pending = 0;
    // Keep track of how many bytes remaining in the next chunk
    std::size_t _n_chunk_pending = 0;
    // Coroutine state
    int _coro_state = 0;

    auto _pending_buf() const noexcept { return as_buffer(_pending, _n_pending); }

    bool _try_read_chunk_trailer() {
        /// XXX: Read more than just the CRLF at the end?
        auto& inner        = next_layer();
        auto  prev_pending = _n_pending;
        auto  next_in      = inner.next(2);
        auto  n_copied     = buffer_copy(as_buffer(_pending) + _n_pending, next_in);
        _n_pending += n_copied;
        if (n_copied == 0) {
            // We didn't read anything from the stream
            return false;
        }
        auto pbuf = _pending_buf();
        if (begins_with_crlf(pbuf)) {
            // We've found the CRLF at the end of the buffer
            inner.consume(2 - prev_pending);
            _n_pending = 0;
            return true;
        }
        if (_n_pending >= 2) {
            // We're never going to be able to parse enough to actually see it.
            throw std::runtime_error(
                "neo::http chunk decoder can't find an expected chunk trailer in the stream");
        }
        inner.consume(n_copied);
        return _try_read_chunk_trailer();
    }

    bool _try_read_chunk_header() {
        auto& inner        = next_layer();
        auto  next_in      = inner.next(HeadMaxSize);
        auto  prev_pending = _n_pending;
        auto  n_copied     = buffer_copy(as_buffer(_pending) + _n_pending, next_in);
        _n_pending += n_copied;
        if (n_copied == 0) {
            // Didn't read any more from the stream.
            return false;
        }

        auto pending_buffer = _pending_buf();
        auto head           = chunk_head::parse(pending_buffer);
        if (head.valid()) {
            _n_pending       = 0;
            _n_chunk_pending = head.chunk_size;
            auto head_size   = head.parse_tail.data() - pending_buffer.data();
            inner.consume(head_size - prev_pending);
            return true;
        }
        if (_n_pending == HeadMaxSize) {
            // We're never going to be able to parse enough to actually see it.
            throw std::runtime_error(
                "neo::http chunk decoder can't find the next chunk-head in the stream");
        }
        inner.consume(n_copied);
        return _try_read_chunk_header();
    }

public:
    chunked_buffers() = default;
    explicit chunked_buffers(InnerSource&& src)
        : _inner(NEO_FWD(src)) {}

    constexpr state_t state() const noexcept { return _state; }
    constexpr bool    done() const noexcept { return state() == state_t::done; }

    NEO_DECL_UNREF_GETTER(next_layer, _inner);

    decltype(auto) next(std::size_t n) {
        auto& inner = next_layer();

        NEO_CORO_BEGIN(_coro_state);

        while (!done()) {
            while (!_try_read_chunk_header()) {
                NEO_CORO_YIELD(inner.next(0));
            }

            if (_n_chunk_pending == 0) {
                while (!_try_read_chunk_trailer()) {
                    NEO_CORO_YIELD(inner.next(0));
                }
                _state = state_t::done;
                break;
            }

            while (_n_chunk_pending != 0) {
                NEO_CORO_YIELD(inner.next((std::min)(n, _n_chunk_pending)));
            }

            while (!_try_read_chunk_trailer()) {
                NEO_CORO_YIELD(inner.next(0));
            }
        }

        NEO_CORO_END;

        return inner.next(0);
    }

    void consume(std::size_t n) noexcept {
        neo_assert(expects,
                   n <= _n_chunk_pending,
                   "Cannot consume more than a full chunk of data from a chunk stream source",
                   n,
                   _n_chunk_pending);
        _n_chunk_pending -= n;
        next_layer().consume(n);
    }
};

template <buffer_source S>
explicit chunked_buffers(S &&) -> chunked_buffers<S>;

}  // namespace neo::http
