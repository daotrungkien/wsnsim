#include "ga_test.h"
#include "ga_scenario1.h"

#define FIXED_LENGTH



ga_logging ga_logger;
vector<ga_scenario1::individual> ga_scenario1::init_population;


int main(uint argc, const char* argv[]) {
	try {
		shared_ptr<ga_scenario1> g;

		for (uint i = 1; i < argc; i++) {
			string argi = argv[i];

			if (argi == "--nodes") {
				if (i >= argc - 1) throw runtime_error("Incorrect parameters!");
				int n = std::atoi(argv[i + 1]);
				if (n < 1 || n > 3) throw runtime_error("Incorrect number of nodes!");
				the_test::sensor_nodes = n;
			}
		}

		if (the_test::sensor_nodes == 0) throw runtime_error("Number of nodes not specified!");

		for (uint i = 1; i < argc; i++) {
			string argi = argv[i];

			if (argi == "--fixed") {
				g = make_shared<ga_scenario1_fixed>(24 * 4);
			}
			else if (argi == "--variable") {
				g = make_shared<ga_scenario1_variable>();
			}
		}

		if (g == nullptr) throw runtime_error("Chromosome type not specified!");
		g->set_population_size(100);
		g->set_selection_rate(.2);

		for (uint i = 1; i < argc; i++) {
			string argi = argv[i];
			
			if (argi == "--population-size") {
				if (i >= argc - 1) throw runtime_error("Incorrect parameters!");
				g->set_population_size(stoi(argv[i + 1]));
				i++;
			}
			else if (argi == "--selection-rate") {
				if (i >= argc - 1) throw runtime_error("Incorrect parameters!");
				g->set_selection_rate(stod(argv[i + 1]));
				i++;
			}
			else if (argi == "--init-file") {
				if (i >= argc - 1) throw runtime_error("Incorrect parameters!");

				ifstream file(argv[i + 1]);
				if (!file) throw runtime_error("Cannot read initialization file");

				std::string line;
				ga_scenario1::init_population.clear();
				while (std::getline(file, line)) {
					ga_scenario1::init_population.push_back(kutils::trim(line).c_str());
				}
			}
			else if (argi == "--copy-rate") {
				if (i >= argc - 1) throw runtime_error("Incorrect parameters!");
				g->set_copy_rate(stod(argv[i + 1]));
				i++;
			}
		}

		auto sol = g->run();

		cout << "Solution: " << g->individual2string(sol.first) << " (" << sol.second << ")" << endl;

		return 0;
	}
	catch (const exception& exp) {
		cout << exp.what() << endl;
		return 1;
	}
}
