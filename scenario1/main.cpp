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

	double fitness = 1. / (the_test::measurements.size() + 1);

	// penalize if the node does not die before the end of simulation
	if (stop_time < the_test::runtime_limit)
		fitness += (the_test::runtime_limit - stop_time) * 1000.;

	// penalize if the final battery level is not lower than the inital one
	auto battery_level = tc->world->get_network()->get_nodes().front()->get_battery()->get_capacity();
	if (battery_level < the_test::battery_initial)
		fitness += (the_test::battery_initial - battery_level) / the_test::battery_max_level * 1000.;

	// penalize if there is no measurement within 30min
	for (unsigned int i = 0; i < the_test::measurements.size() - 1; i++) {
		auto d = (the_test::measurements[i + 1].timestamp - the_test::measurements[i].timestamp) / 60.;
		if (d >= 30.) fitness += (d - 30.) * 100.;
	}

	return fitness;
}




int main(int argc, const char* argv[]) {
	try {
		the_test::network_schedule schedule;

		if (argc < 2) throw "Incorrect parameters!";

		string argv1 = argv[1];
		if (argv1 == "-d") schedule.init_default();
		else if (argv1 == "-r") schedule.restore();
		else if (argv1 == "-s") {
			if (argc < 3) throw "Incorrect parameters!";
			schedule.set(argv[2]);
		}

		ofstream file(the_test::filename_output, ios::binary);
		double fitness = run_wsnsim(schedule);
		file.write((const char*)&fitness, sizeof(fitness));

		cout << "Fitness: " << fitness << endl;

		return 0;
	}
	catch (const char* msg) {
		cout << msg << endl;
		return 1;
	}
}