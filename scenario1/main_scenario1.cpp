#pragma once

#include <iostream>
#include <array>
#include <condition_variable>

#include "../wsnsim/ga/ga.h"
#include "../wsnsim/lib/utilities.h"


#include "../wsnsim/test/test_phuong_phd_scenario1.h"


namespace the_test = test_phuong_phd_scenario1;


using namespace std;
using namespace wsn;


double run_wsnsim(const the_test::network_schedule& schedule) {
	the_test::global_schedule = schedule;

	the_test::custom_test_case::create_test_case();
	auto& tc = test_case::instance;

	mutex mtx;
	condition_variable condvar;
	bool stopped = false;
	double stop_time = 0.;

	tc->setup();

	tc->world->on(entity::event_stop, [&mtx, &stopped, &condvar, &tc, &stop_time](event& ev) {
		if (auto n = dynamic_pointer_cast<basic_node>(ev.target); n) {
			auto list = dynamic_pointer_cast<basic_network>(n->get_network())->find_nodes([](auto pn) -> bool {
				return pn->is_started();
			});

			if (list.size() == 0) {
				stop_time = n->get_world_clock_time();

				lock_guard<mutex> guard(mtx);
				stopped = true;
				condvar.notify_one();
			}
		}
	});

	tc->world->start();

	unique_lock<mutex> lock(mtx);
	condvar.wait(lock, [&stopped]() { return stopped; });

	tc->world->stop();


	vector<vector<the_test::measurement_info*>> measurements_per_node(the_test::sensor_nodes);
	for (auto& m : the_test::measurements) {
		measurements_per_node[m.node_idx].push_back(&m);
	}

	cout << "Measurements: " << the_test::measurements.size() << " (";
	for (uint i = 0; i < measurements_per_node.size(); i++) {
		if (i > 0) cout << "+";
		cout << measurements_per_node[i].size();
	}
	cout << ")" << endl;


	double fitness = 0;

	//fitness += 1. / (measurements.size() + 1);
	fitness -= the_test::measurements.size() / (24. * 60.) * 100.;
	cout << "New fitness: " << fitness << endl;

/*	for (uint node_idx = 0; node_idx < the_test::sensor_nodes; node_idx++) {
		auto& mpn = measurements_per_node[node_idx];

		double t0 = 0., dt;

		// penalize if there is no measurement within 30min
		if (mpn.size() > 0) {
			for (uint i = 0; i < mpn.size(); i++) {
				dt = (mpn[i]->timestamp - t0) / 60.;
				t0 = mpn[i]->timestamp;
				if (dt >= 0.) fitness += dt * 1. + dt * dt * 10.;
			}
		}

		dt = (the_test::runtime_limit - t0) / 60.;
		if (dt >= 0.) fitness += dt * 1. + dt * dt * 10.;
	}

	cout << "New fitness: " << fitness << endl;
	*/

	// minimize intervals between measurements
	double t0 = 0., dt;
	if (the_test::measurements.size() > 0) {
		for (uint i = 0; i < the_test::measurements.size(); i++) {
			dt = (the_test::measurements[i].timestamp - t0) / 60.;
			t0 = the_test::measurements[i].timestamp;
			if (dt >= 0.) fitness += dt * 1. + dt * dt * 1.;
		}
	}

	dt = (the_test::runtime_limit - t0) / 60.;
	if (dt >= 0.) fitness += dt * 1. + dt * dt * 10.;

	cout << "New fitness: " << fitness << endl;

	
	auto& nodes = tc->world->get_network()->get_nodes();
	uint i = 0;
	auto itr = nodes.begin();
	for (; itr != nodes.end(); itr++, i++) {
		auto& node = *itr;

		// penalize if the final battery level is not lower than the inital one
		auto battery_level = node->get_battery()->get_capacity();
		if (battery_level < the_test::battery_initial) {
			auto t = (the_test::battery_initial - battery_level) / the_test::battery_max_level[i];
			fitness += t * 1e6 + t * t * 1e6;
		}
	}
	cout << "New fitness: " << fitness << endl;

	// penalize if the node does not die before the end of simulation
	if (stop_time < the_test::runtime_limit) {
		auto t = the_test::runtime_limit - stop_time;
		fitness += t * 1e8 + t * t * 1e8;
		cout << "New fitness: " << fitness << endl;
	}

	return fitness;
}




int main(int argc, const char* argv[]) {
	try {
		if (argc < 2) throw runtime_error("Parameters not specified!");

		for (int i = 1; i < argc; i++) {
			string argi = argv[i];

			if (argi == "-n") {
				if (i >= argc - 1) throw runtime_error("Incorrect parameters!");
				int n = std::atoi(argv[i + 1]);
				if (n < 1 || n > 3) throw runtime_error("Incorrect number of nodes!");
				the_test::sensor_nodes = n;
			}
		}

		if (the_test::sensor_nodes == 0) throw runtime_error("Number of nodes not specified!");

		the_test::network_schedule schedule;

		for (int i = 1; i < argc; i++) {
			string argi = argv[i];

			if (argi == "-d") schedule.init_default();
			else if (argi == "-r") schedule.restore();
			else if (argi == "-s") {
				if (i >= argc - 1) runtime_error("Incorrect parameters!");
				schedule.set(argv[i + 1]);
			}
		}

		ofstream file(the_test::filename_output, ios::binary);
		double fitness = run_wsnsim(schedule);
		file.write((const char*)&fitness, sizeof(fitness));

		cout << "Fitness: " << fitness << endl;

		return 0;
	}
	catch (const exception& exp) {
		cout << exp.what() << endl;
		return 1;
	}
}
