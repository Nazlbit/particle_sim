#pragma once
#include <exception>
#include <cstdint>
#include <cstring>

class exception : public std::exception
{
public:
	exception(const char *const msg) noexcept
	{
		std::snprintf(m_msg, msg_size_limit, "%s", msg);
	}

	virtual const char *what() const noexcept override
	{
		return m_msg;
	}

	static constexpr uint16_t msg_size_limit = 128;

private:
	char m_msg[msg_size_limit];
};
