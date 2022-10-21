#pragma once
#include <cstdint>
#include <atomic>
#include <functional>

class barrier
{
    const uint64_t m_num;
    std::atomic_uint64_t m_i = 0;
    std::atomic_bool m_wait = true;
    const std::function<void()> m_on_barrier;

public:
	template <class T>
	barrier(const uint64_t num, T &&on_barrier)
		: m_num(num), m_on_barrier(std::forward<T>(on_barrier)) {}

	void wait()
    {
        while (!m_wait)
        {
            // spinlock
        }
        if (++m_i < m_num)
        {
            while (m_wait)
            {
                // spinlock
            }
            --m_i;
        }
        else
        {
            m_on_barrier();
            --m_i;
            m_wait = false;
            while (m_i != 0)
            {
                // spinlock
            }
            m_wait = true;
        }
    }
};
