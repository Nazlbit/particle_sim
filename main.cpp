#include "helper.hpp"
#include "application.hpp"

int main()
{
	try
	{
		application application;
		application.run();
	}
	catch(const std::exception &ex)
	{
		ERROR("%s", ex.what());
		return -1;
	}

	return 0;
}
