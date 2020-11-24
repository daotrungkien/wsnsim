#include "common.h"


using namespace std;
using namespace wsn;


namespace test1 {



	mutex writemx;

	void log_info(shared_ptr<basic_node> node)
	{
		logger.log(kutils::formatstr("%s\t%.3f", node->get_name().c_str(), node->get_battery()->get_capacity()));
	}



	class controller_measure_then_sleep : public basic_controller {
	protected:
		bool is_charging = 0;
		chrono::duration<double> sampling_time;

	public:
		void init() override;

		template <typename _Rep, typename _Period>
		void set_sampling_time(chrono::duration<_Rep, _Period> st) {
			sampling_time = chrono::duration_cast<chrono::duration<double>>(st);
		}
	};


	class simple_node : public generic_node<
		comm::none,
		sensor::with_noise<sensor::with_ambient<basic_sensor>>,
		battery::linear,
		power::solar,
		controller_measure_then_sleep> {
	public:
		using generic_node<comm::none, sensor::with_noise<sensor::with_ambient<basic_sensor>>, battery::linear, power::solar, controller_measure_then_sleep>::generic_node;
	};



	inline void controller_measure_then_sleep::init()
	{
		basic_controller::init();

		auto node = get_node();
		node->on(basic_battery::event_full, [this](event&) {
			is_charging = false;
		});

		node->on(basic_battery::event_consume, [this](event& ev, double d) {
			if (dynamic_pointer_cast<basic_battery>(ev.target)->get_soc() <= 0.2)
				is_charging = true;
		});

		node->on_self(entity::event_first_start, [this, node](event& ev) {
			timer(sampling_time, true, [this, node](event& ev) {
				auto battery = node->get_battery();

				if (is_charging) {
					battery->charge(sampling_time.count());
				}

				if (battery->consume(sampling_time.count()))
					node->get_sensor()->measure();
				});
		});
	}





	class custom_test_case : public test_case {
	public:
		string get_test_name() const override {
			return "test1";
		}

		string get_test_description() const override {
			return "Battery & solar panels";
		}

		void setup() override {
			world = generic_world<basic_network>::new_world();

			auto& ref_frame = world->get_reference_frame();
			ref_frame.set_frame(21.0041527314897, 105.84660046011209, 0., 21.00420833245197, 105.84666804469794, 0.);
			ref_frame.set_timezone(7);

			auto& clock = world->get_clock();
			clock.set(clock::mktime(2018, 5, 23, 5, 0, 0), 3600);

			auto temp_ambient = world->new_ambient<smooth_real_value_ambient>(basic_ambient::temperature, 25, 1. / 3600);
			auto humidity_ambient = world->new_ambient<smooth_real_value_ambient>(basic_ambient::humidity, 80, 1. / 2000);
			auto light_ambient = world->new_ambient<smooth_real_value_ambient>(basic_ambient::light, 100000, 1. / 2345);

			auto wsn = world->get_network();

			auto node1 = wsn->new_node<simple_node>("temperature", location(0, 0, 0));
			node1->get_controller_t()->set_sampling_time(30min);

			auto batt = node1->get_battery_t();
			batt->set_max_level(3500);
			batt->set_level(3500);
			batt->set_rates(.35, .051);

			auto sensor1 = node1->get_sensor_t();
			sensor1->set_ambient(temp_ambient);
			sensor1->add_noise(make_shared<noise::gaussian>(0., 3.));

			auto node2 = wsn->new_node<simple_node>("humidity", location(10, 0, 0));
			node2->get_controller_t()->set_sampling_time(10min);

			batt = node2->get_battery_t();
			batt->set_max_level(4000);
			batt->set_level(4000);
			batt->set_rates(.5, .042);

			auto sensor2 = node2->get_sensor_t();
			sensor2->set_ambient(humidity_ambient);
			sensor2->add_noise(make_shared<noise::gaussian>(0., 5.));

			auto node3 = wsn->new_node<simple_node>("light", location(0, 10, 0));
			node3->get_controller_t()->set_sampling_time(5min);

			batt = node3->get_battery_t();
			batt->set_max_level(2500);
			batt->set_level(2500);
			batt->set_rates(.29, .0198);

			auto sensor3 = node3->get_sensor_t();
			sensor3->set_ambient(light_ambient);
			sensor3->add_noise(make_shared<noise::gaussian>(100., 500.));


			wsn->on(basic_sensor::event_measure, [](event& ev, std::any value, double time) {
				auto n = dynamic_pointer_cast<basic_sensor>(ev.target)->get_node();

				lock_guard lock(writemx);
				cout << format_time(n->get_reference_time()) << ": "
					<< "Node " << n->get_name() << " (" << n->get_id() << ") measurement updated: " << any_cast<double>(value) << endl;
			});

			wsn->on(basic_node::event_start, [](event& ev) {
				if (auto n = dynamic_pointer_cast<simple_node>(ev.target); n) {
					lock_guard lock(writemx);
					cout << format_time(n->get_reference_time()) << ": "
						<< "Node " << n->get_name() << " (" << n->get_id() << ") started" << endl;
					log_info(n);
				}
			});

			wsn->on(basic_node::event_stop, [](event& ev) {
				if (auto n = dynamic_pointer_cast<simple_node>(ev.target); n) {
					lock_guard lock(writemx);
					cout << format_time(n->get_reference_time()) << ": "
						<< "Node " << n->get_name() << " (" << n->get_id() << ") stopped" << endl;
				}
			});

			wsn->on(basic_battery::event_charge, [](event& ev, double d) {
				lock_guard lock(writemx);
				auto n = dynamic_pointer_cast<basic_battery>(ev.target)->get_node();
				cout << format_time(n->get_reference_time()) << ": "
					<< "Node " << n->get_name() << " (" << n->get_id() << ") charged, level = " << n->get_battery()->get_soc() << endl;
				log_info(n);
			});

			wsn->on(basic_battery::event_consume, [](event& ev, double d) {
				lock_guard lock(writemx);
				auto n = dynamic_pointer_cast<basic_battery>(ev.target)->get_node();
				cout << format_time(n->get_reference_time()) << ": "
					<< "Node " << n->get_name() << " (" << n->get_id() << ") consumed, level = " << n->get_battery()->get_soc() << endl;
				log_info(n);
			});

			wsn->on(basic_battery::event_empty, [](event& ev) {
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