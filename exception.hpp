#pragma once
#include <exception>
#include <cstdint>
#include <cstring>

class exception : public std::exception
{
public:
	exception(const char *const msg) noexcept
	{
		std::strncpy(m_msg, msg, msg_size_limit);
		m_msg[msg_size_limit - 1] = '\0';
	}

	virtual const char *what() const noexcept override
	{
		return m_msg;
	}

private:
	static constexpr uint16_t msg_size_limit = 128;
	char m_msg[msg_size_limit];
};
