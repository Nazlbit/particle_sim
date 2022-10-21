#include <random>
#include "math.hpp"

static std::random_device r;
static std::seed_seq seed{r(), r(), r(), r(), r(), r(), r(), r()};
static std::mt19937_64 engine(seed);

double random_double(double from, double to)
{
	std::uniform_real_distribution dis(from, to);
	return dis(engine);
}
