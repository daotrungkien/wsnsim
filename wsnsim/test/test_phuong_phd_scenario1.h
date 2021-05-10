#include "common.h"
#include <array>


using namespace std;
using namespace wsn;



namespace test_phuong_phd_scenario1 {
	const std::chrono::system_clock::time_point schedule_start = clock::mktime(2020, 5, 23, 0, 0, 0);
	const double runtime_limit = 3600. * 24 * 3;	// s

	const double battery_max_level[] = { 3500 * 3.6, 3500 * 3.6 * 1.5, 3500 * 3.6 * 2 };
	const double battery_initial = .7 * battery_max_level[0];

	const double battery_charge_rate = .8;				// W
	const double battery_discharge_rate_active = .26;	// W
	const double battery_discharge_rate_inactive = .05;	// W
	const double battery_discharge_activate = 1.28;		// Ws
	const double battery_discharge_sleep = 0.97;		// Ws

	uint sensor_nodes = 0;

	const string filename_input = "scenario1-schedule.dat";
	const string filename_output = "scenario1-result.dat";


	mutex writemx;

	void log_info(shared_ptr<basic_node> node) {
		logger.log(kutils::formatstr("%s\t%.3f", node->get_name().c_str(), node->get_battery()->get_capacity()));
	}





	class network_schedule {
	public:
		class interval {
		public:
			double start;
			bool active;

			interval() {
			}

			interval(double _start, bool _active)
				: start(_start), active(_active)
			{}
		};

		vector<vector<interval>> node_schedules;


		network_schedule() {
			init_default();
		}

		network_schedule(const char* chromosome) {
			set(chromosome);
		}

		void set(uint node_idx, const vector<interval>& s) {
			node_schedules[node_idx] = s;
		}

		void set(const string& chromosome) {
			auto node_chromosomes = kutils::split(chromosome, ";");
			if (node_chromosomes.size() != sensor_nodes) throw runtime_error(kutils::formatstr("Chromosome not correct: %s", chromosome.c_str()));

			node_schedules.resize(sensor_nodes);
			for (uint node_idx = 0; node_idx < sensor_nodes; node_idx++) {
				node_schedules[node_idx].clear();

				auto a = kutils::split(node_chromosomes[node_idx]);
				for (uint i = 0; i < a.size(); i++) {
					string s = a[i];

					interval t;
					t.active = s.back() == '+';
					if (t.active) s.erase(s.end() - 1);

					t.start = stod(s);

					node_schedules[node_idx].push_back(t);
				}
			}
		}

		void init_default() {
			node_schedules.resize(sensor_nodes);

			vector<interval> x;

			bool active = false;
			for (int i = 0; i <= 24; i++, active = !active) {
				x.push_back(interval(i * 60., active));
			}

			for (uint i = 0; i < node_schedules.size(); i++) {
				set(i, x);
			}
		}

		void save() const {
			ofstream file(filename_input, ios::binary);

			for (auto& ns : node_schedules) {
				uint n = ns.size();
				file.write((const char*)&n, sizeof(n));

				file.write((const char*)ns.data(), sizeof(interval) * n);
			}
		}

		void restore() {
			ifstream file(filename_input, ios::binary);

			for (auto& ns : node_schedules) {
				uint n;
				file.read((char*)&n, sizeof(n));

				ns.resize(n);
				file.read((char*)ns.data(), sizeof(interval) * n);
			}
		}
	};

	class measurement_info {
	public:
		double timestamp;
		double value;
		uint node_idx;
	};

	static network_schedule global_schedule;
	static vector<measurement_info> measurements;


	

	class phuong_controller : public basic_controller {
	public:
		static inline uint event_inactive = unique_id();
		static inline uint event_active = unique_id();

	protected:
		chrono::duration<double> sampling_time;
		chrono::system_clock::time_point last_battery_update;

		bool active = false;

	public:
		void init() override;

		template <typename _Rep, typename _Period>
		void set_sampling_time(chrono::duration<_Rep, _Period> st) {
			sampling_time = chrono::duration_cast<chrono::duration<double>>(st);
		}

		bool is_active() const {
			return active;
		}

		void set_active(bool a) {
			if (active == a) return;
			active = a;
			
			auto battery = std::dynamic_pointer_cast<battery::chemical>(get_node()->get_battery());

			battery->set_load(active ? battery_discharge_activate : battery_discharge_sleep);

			if (is_started()) {
				auto now = get_reference_time();
				battery->consume(chrono::duration_cast<chrono::duration<double>>(now - last_battery_update).count());
				last_battery_update = now;
			}

			battery->set_load(active ? battery_discharge_rate_active : battery_discharge_rate_inactive);
			fire(active ? event_active : event_inactive);
		}
	};


	class phuong_node : public generic_node<
		comm::none,
		sensor::with_ambient<basic_sensor>,
		battery::chemical,
		power::solar,
		phuong_controller> {
	protected:
		uint node_idx = -1;

	public:
		using generic_node<comm::none, sensor::with_ambient<basic_sensor>, battery::chemical, power::solar, phuong_controller>::generic_node;

		void set_node_idx(uint i) {
			node_idx = i;
		}

		uint get_node_idx() const {
			return node_idx;
		}
	};



	inline void phuong_controller::init()
	{
		basic_controller::init();

		set_active(false);

		auto node = std::dynamic_pointer_cast<phuong_node>(get_node());

		node->on_self(entity::event_first_start, [this, node](event& ev) {
			last_battery_update = get_reference_time();

			timer(sampling_time, true, [this, node](event& ev) {
				auto battery = node->get_battery();

				auto now = get_reference_time();

				battery->charge(sampling_time.count());

				if (battery->consume(chrono::duration_cast<chrono::duration<double>>(now - last_battery_update).count())) {
					if (active) node->get_sensor()->measure();
				}

				last_battery_update = now;
			});
		});

		timer(1min, true, (std::function<bool(event&)>)[this, node](event& ev) -> bool {
			try {
				auto now = get_reference_time();
				auto now_t = chrono::system_clock::to_time_t(now);

				tm ltime;
				localtime_s(&ltime, &now_t);
				double minutes = ltime.tm_hour * 60. + ltime.tm_min + ltime.tm_sec / 60.;	// number of minutes since 00:00 of the day

				auto& schedule = global_schedule.node_schedules[node->get_node_idx()];
				uint current_step = 0;
				for (; current_step < schedule.size() - 1 && minutes > schedule[current_step + 1].start; current_step++);

				set_active(schedule[current_step].active);

				double now_c = get_world()->get_clock().reference2clock(now);
				if (now_c > runtime_limit)
					node->stop();
			}
			catch (...) {
				cout << "----------------" << endl;
				return false;
			}

			return true;
		});
	}





	class custom_test_case : public test_case {
	public:
		string get_test_name() const override {
			return "phuong1";
		}

		string get_test_description() const override {
			return "Phuong's scenario #1";
		}

		void setup() override {
			measurements.clear();


			world = generic_world<basic_network>::new_world();

			auto& ref_frame = world->get_reference_frame();
			ref_frame.set_frame(21.0041527314897, 105.84660046011209, 0., 21.00420833245197, 105.84666804469794, 0.);
			ref_frame.set_timezone(7);

			auto& clock = world->get_clock();
			clock.set(schedule_start, 3600.);

			auto ambient = world->new_ambient<smooth_real_value_ambient>(basic_ambient::temperature, 25, 1. / 3600);

			auto sn = world->get_network();

			vector<shared_ptr<phuong_node>> nodes;
			for (uint i = 0; i < sensor_nodes; i++) {
				auto n = sn->new_node<phuong_node>(kutils::formatstr("S%d", i + 1).c_str(), location(0, 0, 0));
				n->set_node_idx(i);
				nodes.push_back(n);
			}

			for (uint i = 0; i < sensor_nodes; i++) {
				auto& node = nodes[i];

				node->get_controller_t()->set_sampling_time(5min);

				auto batt = node->get_battery_t();
				batt->set_rated_capacity(battery_max_level[i]);
//				batt->set_level(battery_initial);
				batt->set_load(battery_charge_rate);

				auto sensor = node->get_sensor_t();
				sensor->set_ambient(ambient);
			}

			sn->on(basic_sensor::event_measure, [](event& ev, std::any value, double time) {
				auto n = dynamic_pointer_cast<phuong_node>( dynamic_pointer_cast<basic_sensor>(ev.target)->get_node() );

				lock_guard lock(writemx);
				cout << format_time(n->get_reference_time()) << ": "
					<< "Node " << n->get_name() << " (" << n->get_id() << ") measurement updated: " << any_cast<double>(value) << endl;

				measurement_info inf;
				inf.timestamp = time;
				inf.node_idx = n->get_node_idx();
				inf.value = any_cast<double>(value);
				measurements.push_back(inf);
			});

			sn->on(basic_node::event_start, [](event& ev) {
				if (auto n = dynamic_pointer_cast<phuong_node>(ev.target); n) {
					lock_guard lock(writemx);
					cout << format_time(n->get_reference_time()) << ": "
						<< "Node " << n->get_name() << " (" << n->get_id() << ") started" << endl;
					log_info(n);
				}
			});

			sn->on(basic_node::event_stop, [](event& ev) {
				if (auto n = dynamic_pointer_cast<phuong_node>(ev.target); n) {
					lock_guard lock(writemx);
					cout << format_time(n->get_reference_time()) << ": "
						<< "Node " << n->get_name() << " (" << n->get_id() << ") stopped" << endl;
				}
			});

			sn->on(phuong_controller::event_active, [](event& ev) {
				auto n = dynamic_pointer_cast<phuong_controller>(ev.target)->get_node();
				lock_guard lock(writemx);
				cout << format_time(n->get_reference_time()) << ": "
					<< "Node " << n->get_name() << " (" << n->get_id() << ") is active" << endl;
			});

			sn->on(phuong_controller::event_inactive, [](event& ev) {
				auto n = dynamic_pointer_cast<phuong_controller>(ev.target)->get_node();
				lock_guard lock(writemx);
				cout << format_time(n->get_reference_time()) << ": "
					<< "Node " << n->get_name() << " (" << n->get_id() << ") is inactive" << endl;
			});

			sn->on(basic_battery::event_charge, [](event& ev, double d) {
				lock_guard lock(writemx);
				auto n = dynamic_pointer_cast<basic_battery>(ev.target)->get_node();
				cout << format_time(n->get_reference_time()) << ": "
					<< "Node " << n->get_name() << " (" << n->get_id() << ") charged, level = " << n->get_battery()->get_soc() << endl;
				log_info(n);
			});

			sn->on(basic_battery::event_consume, [](event& ev, double d) {
				lock_guard lock(writemx);
				auto n = dynamic_pointer_cast<basic_battery>(ev.target)->get_node();
				cout << format_time(n->get_reference_time()) << ": "
					<< "Node " << n->get_name() << " (" << n->get_id() << ") consumed, level = " << n->get_battery()->get_soc() << endl;
				log_info(n);
			});

			sn->on(basic_battery::event_empty, [](event& ev) {
				auto n = dynamic_pointer_cast<basic_battery>(ev.target)->get_node();
				{
					lock_guard lock(writemx);
					cout << format_time(n->get_reference_time()) << ": "
						<< "Node " << n->get_name() << " (" << n->get_id() << ") battery empty" << endl;
				}
				n->stop();
			});
		}


		static void create_test_case()
		{
			auto tc = make_shared<custom_test_case>();
			test_case::instance = tc;
			tc->init();
		}
	};



};