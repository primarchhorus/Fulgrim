#pragma once

#include <atomic>
#include <array>
#include <cstdint>
#include <type_traits>
#include <new>
#include <iostream>  // Add this line for std::cerr

// General purpose lock free buffer, single producer single consumer
template <typename T, size_t buffer_size>
class CircularBuffer
{
public:
    CircularBuffer() : _head_index(0), _tail_index(0)
    {
        static_assert(buffer_size > 0, "_buffer size must be greater than 0");
    }

    ~CircularBuffer()
    {
        try
        {
            destroyAllInBuffer();
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }
    }

    CircularBuffer(const CircularBuffer &) = delete;
    CircularBuffer &operator=(const CircularBuffer &) = delete;

    CircularBuffer(CircularBuffer &&other) noexcept
        : _head_index(other._head_index.load(std::memory_order_relaxed)),
          _tail_index(other._tail_index.load(std::memory_order_relaxed))
    {
        for (size_t i = 0; i < buffer_size; ++i)
        {
            if (other.checkIndexHasConstructed(i))
            {
                new (&_buffer[i]) T(std::move(other._buffer[i]));
                other.destroyConstructedAtIndex(i);
            }
        }
        other._head_index.store(0, std::memory_order_relaxed);
        other._tail_index.store(0, std::memory_order_relaxed);
    }

    CircularBuffer &operator=(CircularBuffer &&other) noexcept
    {
        if (this != &other)
        {
            destroyAllInBuffer();
            _head_index.store(other._head_index.load(std::memory_order_relaxed), std::memory_order_relaxed);
            _tail_index.store(other._tail_index.load(std::memory_order_relaxed), std::memory_order_relaxed);
            for (size_t i = 0; i < buffer_size; ++i)
            {
                if (other.checkIndexHasConstructed(i))
                {
                    new (&_buffer[i]) T(std::move(other._buffer[i]));
                    other.destroyConstructedAtIndex(i);
                }
            }
            other._head_index.store(0, std::memory_order_relaxed);
            other._tail_index.store(0, std::memory_order_relaxed);
        }
        return *this;
    }

    bool push(const T &item)
    {

        if (full())
        {
            return false;
        }

        size_t current_tail_index = _tail_index.load(std::memory_order_relaxed);
        // This nonsense is support for non trivially destructable types of T
        // Check if a current tail has a constructed T in it
        if (checkIndexHasConstructed(current_tail_index))
        {
            // delete the constructed T
            destroyConstructedAtIndex(current_tail_index);
        }
        // placement new a new T at the current tail
        new (&_buffer[current_tail_index]) T(item);
          /*
        memory order and the fence are an attempt to make sure all the previous updates 
        are made before the update to the tail index so that any thread after the new items 
        gets them onky after the tail is updated
        */ 
        _tail_index.store(incrementIndex(current_tail_index), std::memory_order_release);
        std::atomic_thread_fence(std::memory_order_seq_cst);

        return true;
    }

    bool pop(T &item)
    {
        if (empty())
        {
            return false;
        }

        size_t current_head_index = _head_index.load(std::memory_order_relaxed);
        // This nonsense is support for non trivially destructable types of T
        item = std::move(_buffer[current_head_index]); // Move a T out of the buffer into item
        destroyConstructedAtIndex(current_head_index); // Destroy the original and free up an index
        // Update the head index with a incremented value
        /*
        memory order and the fence are an attempt to make sure all the previous updates 
        are made before the update to the head index 
        */ 
         
        _head_index.store(incrementIndex(current_head_index), std::memory_order_release);
        std::atomic_thread_fence(std::memory_order_seq_cst);

        return true;
    }

    bool empty() const
    {
        return _head_index.load(std::memory_order_acquire) ==
               _tail_index.load(std::memory_order_acquire);
    }

    bool full() const
    {
        size_t next_tail_index = incrementIndex(_tail_index.load(std::memory_order_acquire));
        return next_tail_index == _head_index.load(std::memory_order_acquire);
    }

    size_t size() const
    {
        size_t _head_index_val = _head_index.load(std::memory_order_acquire);
        size_t _tail_index_val = _tail_index.load(std::memory_order_acquire);
        return (_tail_index_val - _head_index_val + buffer_size) % buffer_size;
    }

    void destroyAllInBuffer()
    {
        size_t current_head_index = _head_index.load(std::memory_order_relaxed);
        size_t current_tail_index = _tail_index.load(std::memory_order_relaxed);

        while (current_head_index != current_tail_index)
        {
            if (checkIndexHasConstructed(current_head_index))
            {
                destroyConstructedAtIndex(current_head_index);
            }
            current_head_index = incrementIndex(current_head_index);
        }
        _head_index.store(0, std::memory_order_relaxed);
        _tail_index.store(0, std::memory_order_relaxed);
    }

private:
    size_t incrementIndex(size_t n) const
    {
        return (n + 1) % buffer_size;
    }

    bool checkIndexHasConstructed(size_t index) const
    {
        size_t _head_index_val = _head_index.load(std::memory_order_relaxed);
        size_t _tail_index_val = _tail_index.load(std::memory_order_relaxed);

        bool non_wrapped = (_head_index_val <= _tail_index_val);
        bool has_constructed_non_wrapped = (index >= _head_index_val) && (index < _tail_index_val);
        bool has_constructed_wrapped = (index >= _head_index_val) || (index < _tail_index_val);
        return non_wrapped ? has_constructed_non_wrapped : has_constructed_wrapped;
        ;
    }

    void destroyConstructedAtIndex(size_t index)
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            _buffer[index].~T();
        }
    }

    alignas(T) std::array<std::byte, sizeof(T) * buffer_size> _storage;
    T *_buffer = reinterpret_cast<T *>(_storage.data());
    std::atomic<size_t> _head_index;
    std::atomic<size_t> _tail_index;
};