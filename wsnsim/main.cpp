#include "wsnsim.h"
#include <mutex>

using namespace wsn;
using namespace std;


class simple_node : public generic_node<comm_none, sensor_none, min_max_battery, power_none> {
public:
	using generic_node<comm_none, sensor_none, min_max_battery, power_none>::generic_node;

	virtual void cycle() {
		battery.charge(5);
		this_thread::sleep_for(chrono::milliseconds(300));
		battery.discharge(10);
	}
};

mutex writemx;



void dump_battery(uint event, basic_node* node) {
	lock_guard<mutex> lock(writemx);
	auto n = static_cast<simple_node*>(node);
	cout << format_time(n->get_network()->get_time()) << ": ";
	cout << "Node " << n->get_name() << " (" << n->get_id() << ") started" << endl;
}

void _tmain() {
	network wsn;

	wsn.add_node<simple_node>(_T("temperature"), location3d<>(0, 0, 0), 0.7);
	wsn.add_node<simple_node>(_T("humidity"), location3d<>(10, 0, 0), 0.5);
	wsn.add_node<simple_node>(_T("light"), location3d<>(0, 10, 0), 0.2);

	wsn.bind(basic_node::event_started, [](uint event, basic_node* node) {
		lock_guard<mutex> lock(writemx);
		auto n = static_cast<simple_node*>(node);
		cout << format_time(n->get_network()->get_time()) << ": ";
		cout << "Node " << n->get_name() << " (" << n->get_id() << ") started" << endl;
	});

	wsn.bind(basic_node::event_stopped, [](uint event, basic_node* node) {
		lock_guard<mutex> lock(writemx);
		auto n = static_cast<simple_node*>(node);
		cout << format_time(n->get_network()->get_time()) << ": ";
		cout << "Node " << n->get_name() << " (" << n->get_id() << ") stopped" << endl;
	});

	wsn.bind(basic_battery::event_charge, [](uint event, basic_node* node) {
		lock_guard<mutex> lock(writemx);
		auto n = static_cast<simple_node*>(node);
		cout << format_time(n->get_network()->get_time()) << ": ";
		cout << "Node " << n->get_name() << " (" << n->get_id() << ") charges, level = " << n->get_battery().get_level() << endl;
	});

	wsn.bind(basic_battery::event_discharge, [](uint event, basic_node* node) {
		lock_guard<mutex> lock(writemx);
		auto n = static_cast<simple_node*>(node);
		cout << format_time(n->get_network()->get_time()) << ": ";
		cout << "Node " << n->get_name() << " (" << n->get_id() << ") discharges, level = " << n->get_battery().get_level() << endl;
	});

	wsn.start();

	string cmd;
	while (true) {
		cout << ">> ";
		cin >> cmd;

		if (cmd == "exit") break;
	}

	wsn.stop();
}