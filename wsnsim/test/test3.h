#include "common.h"


using namespace std;
using namespace wsn;


namespace test3 {


	mutex writemx;

	shared_ptr<basic_node> master_node;
	double measure_time;


	void log_info(shared_ptr<basic_node> node, shared_ptr<basic_node> from, double t, const char* action) {
		logger.log(kutils::formatstr("%s\t%s\t%s",
			node->get_name().c_str(),
			(from ? from->get_name().c_str() : "-"),
			action));
	}


	class controller_measure_then_sleep : public basic_controller {
	protected:
		chrono::duration<double> sampling_time;

	public:
		void init() override {
			auto node = get_node();

			node->on(basic_sensor::event_measure, [node](event& ev, any value, double time) {
				double t = node->get_world_clock_time();
				measure_time = t;

				{
					lock_guard lock(writemx);
					cout << format_time(t) << ": Node " << node->get_name() << " (" << node->get_id() << ") measurement updated: " << any_cast<double>(value) << endl;
					log_info(node, nullptr, t, "measure");
				}

				if (!node->is_same(master_node)) {
					string msg = kutils::formatstr("node %d value %.3lf at %s", node->get_id(), any_cast<double>(value), format_time(node->get_reference_time()).c_str());
					node->get_comm()->route(std::vector<uchar>(msg.begin(), msg.end()), master_node);
				}
			});

			node->on(basic_comm::event_receive, [node](event& ev, std::shared_ptr<std::vector<uchar>> data, std::shared_ptr<basic_node> from) {
				double t = node->get_world_clock_time();

				lock_guard lock(writemx);
				cout << format_time(t) << ": Node " << node->get_name() << " (" << node->get_id() << ") received from "
					<< from->get_name() << " (" << from->get_id() << "): " << format_binary_string(*data) << endl;
				log_info(node, from, t, "receive");
			});

			node->on(basic_comm::event_forward, [node](event& ev, std::shared_ptr<std::vector<uchar>> data, std::shared_ptr<basic_node> from) {
				double t = node->get_world_clock_time();

				lock_guard lock(writemx);
				cout << format_time(t) << ": Node " << node->get_name() << " (" << node->get_id() << ") forwarded "
					<< data->size() << " bytes for " << from->get_name() << " (" << from->get_id() << ")" << endl;
				log_info(node, from, t, "forward");
			});

			node->on(basic_comm::event_drop, [node](event& ev, std::shared_ptr<std::vector<uchar>> data, std::shared_ptr<basic_node> from) {
				double t = node->get_world_clock_time();

				lock_guard lock(writemx);
				cout << format_time(t) << ": Node " << node->get_name() << " (" << node->get_id() << ") dropped "
					<< data->size() << " bytes from " << from->get_name() << " (" << from->get_id() << ")" << endl;
				log_info(node, from, t, "drop");
			});
		}

		template <typename _Rep, typename _Period>
		void set_sampling_time(chrono::duration<_Rep, _Period> st) {
			sampling_time = chrono::duration_cast<chrono::duration<double>>(st);
		}
	};




	class custom_comm : public with_super<
		comm::packaged<
		comm::fixed_routing<
		comm::with_loss<
		comm::with_delay_linear<basic_comm>>>>> {
	public:
		void init() override {
			super::init();

			on_self(event_first_start, [this](event& ev) {
				set_delay(10ms, 5ms, 1ms, 0ms, 0ms);
				set_loss_rate(0.1);
			});
		}
	};


	class temp_node : public generic_node<
		custom_comm,
		sensor::with_noise<sensor::with_ambient>,
		battery::none,
		power::none,
		controller_measure_then_sleep> {
	public:
		using generic_node<custom_comm, sensor::with_noise<sensor::with_ambient>, battery::none, power::none, controller_measure_then_sleep>::generic_node;
	};





	class custom_test_case : public test_case {
	public:
		string get_test_name() const override {
			return "test3";
		}

		string get_test_description() const override {
			return "Broadcast routing";
		}

		void print_node_info(shared_ptr<basic_node> node, ostream& stream) override {
			auto loc = node->get_location();
			stream << node->get_id() << "\t" << node->get_name() << "\t" << loc.x << "\t" << loc.y << "\t" << loc.z
				<< "\t" << (node->is_started() ? "started" : "stopped");

			auto& parents = dynamic_pointer_cast<temp_node>(node)->get_comm_t()->get_neighbors();
			if (parents.empty())
				stream << "\t-";
			else {
				vector<string> pnames(parents.size());
				transform(parents.begin(), parents.end(), pnames.begin(), [](auto n) {
					return n->get_name();
					});

				stream << "\t" << kutils::join(pnames, ",");
			}

			stream << endl;
		}

		void setup() override {
			world = generic_world<basic_network>::new_world();

			auto& ref_frame = world->get_reference_frame();
			ref_frame.set_frame(21.0041527314897, 105.84660046011209, 0., 21.00420833245197, 105.84666804469794, 0.);
			ref_frame.set_timezone(7);

			auto& clock = world->get_clock();
			clock.set(clock::mktime(2018, 5, 1, 12, 0, 0), 20);

			auto temp_ambient = world->new_ambient<smooth_real_value_ambient>(basic_ambient::temperature, 25, 1. / 3600);

			auto wsn = world->get_network();

			std::default_random_engine random_generator((uint)time(NULL));
			std::uniform_real_distribution<double> random30(0., 30.);

			for (uint i = 0; i < 150; i++) {
				location loc;
				if (i == 0) loc = location(0, 0, 0);
				else if (i == 1) loc = location(20, 20, 0);
				else loc = location(random30(random_generator), random30(random_generator), 0.);

				auto node = wsn->new_node<temp_node>(kutils::formatstr("%d", i + 1), loc);

				auto sensor = node->get_sensor_t();
				sensor->set_ambient(temp_ambient);
				sensor->add_noise(make_shared<noise::gaussian>(0., 3.));

				if (i == 0) master_node = node;
			}


			function<void(shared_ptr<basic_node>)> set_neighbors;
			set_neighbors = [&set_neighbors, &wsn](shared_ptr<basic_node> n) {
				auto range_list = wsn->find_nodes_in_range(n->get_location(), 5);
				int i = 0;

				decltype(range_list) nblist;
				for (auto nb : range_list) {
					if (nb->is_same(n)) continue;
					if (nb->is_same(master_node)) continue;

					auto comm = dynamic_pointer_cast<temp_node>(nb)->get_comm_t();
					if (comm->get_neighbors().size() >= 1) continue;
					if (comm->has_neighbor(n)) continue;

					comm->add_neighbor(n);
					nblist.push_back(nb);
					if (++i == 3) break;
				}

				for (auto nb : nblist) {
					set_neighbors(nb);
				}
			};

			set_neighbors(master_node);


			json jarr = json::array();
			for (auto& node : wsn->get_nodes()) {
				auto& l = node->get_location();
				jarr.push_back({
					{"id", node->get_id()},
					{"name", node->get_name()},
					{"x", l.x},
					{"y", l.y},
					{"z", l.z},
					{"master", master_node == node}
					});
			}
			logger.log(kutils::formatstr("reset\t%s", jarr.dump().c_str()));



			wsn->on(basic_node::event_start, [](event& ev) {
				if (auto n = dynamic_pointer_cast<temp_node>(ev.target); n) {
					lock_guard lock(writemx);
					auto loc = n->get_location();
					cout << format_time(n->get_reference_time()) << ": "
						<< "Node " << n->get_name() << " (" << n->get_id() << ") started: " << loc.x << ", " << loc.y << ", " << loc.z << endl;
				}
			});

			wsn->on(basic_node::event_stop, [](event& ev) {
				if (auto n = dynamic_pointer_cast<temp_node>(ev.target); n) {
					lock_guard lock(writemx);
					cout << format_time(n->get_reference_time()) << ": "
						<< "Node " << n->get_name() << " (" << n->get_id() << ") stopped" << endl;
				}
			});


			add_command("measure", [this](auto& cmd, auto& args) {
				if (args.size() != 1) {
					cout << cmd << " node-name" << endl;
					return;
				}

				auto node = world->get_network()->node_by_name(args[0]);
				if (!node) {
					cout << "Node node found: " << args[0] << endl;
					return;
				}

				node->get_sensor()->measure();
			});
		}


		static void create_test_case()
		{
			auto tc = make_shared<custom_test_case>();
			test_case::instance = tc;
			tc->init();
		}
	};


}