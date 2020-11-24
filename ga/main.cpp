#include "ga_test.h"
#include "ga_scenario1.h"


int main() {
	ga_scenario1 g;
	g.set_population_size(50);
	g.set_selection_rate(.2);

	auto sol = g.run();

	cout << "Solution: " << g.individual2string(sol.first) << " (" << sol.second << ")" << endl;

	return 0;
}
