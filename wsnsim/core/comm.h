#pragma once

#include "wsnsim.h"
#include <limits>
#include <variant>


namespace wsn::comm {



	class none : public basic_comm {
	public:
		void send(const std::vector<uchar>& data, std::shared_ptr<basic_node> to) override {
			fire(event_drop, make_shared_data(data), to);
			// do nothing
		}
	};


	template <typename wrapped_comm_type>
	class with_delay_generic : public wrapped_comm_type {
	public:
		virtual std::chrono::duration<double> calc_delay(std::shared_ptr<basic_node> to) = 0;

		void send(const std::vector<uchar>& data, std::shared_ptr<basic_node> to) override {
			this->fire(basic_comm::event_send, this->make_shared_data(data), to);

			auto delay = calc_delay(to);
			this->timer_once(delay, true, [newdata = this->make_shared_data(data), to, this](event&) {
				to->get_comm()->receive(newdata, this->get_node());
			});
		}
	};


	template <typename wrapped_comm_type>
	class with_delay_linear : public with_delay_generic<wrapped_comm_type> {
	protected:
		std::default_random_engine random_generator{ (uint)time(nullptr) };
		std::uniform_real_distribution<double> random{ 0., 0. };

		std::chrono::duration<double> sender_base{ 0. }, receiver_base{ 0. }, multiplier{ 0. };

	public:
		// delay time = sender_base + receiver_base + distance * multiplier + random[min, max]
		template <typename _Rep, typename _Period>
		void set_delay(
			std::chrono::duration<_Rep, _Period> _sender_base,
			std::chrono::duration<_Rep, _Period> _receiver_base,
			std::chrono::duration<_Rep, _Period> _multiplier,
			std::chrono::duration<_Rep, _Period> _random_min,
			std::chrono::duration<_Rep, _Period> _random_max)
		{
			sender_base = _sender_base;
			receiver_base = _receiver_base;
			multiplier = _multiplier;
			random = std::uniform_real_distribution<double>(
				std::chrono::duration_cast<std::chrono::duration<double>>(_random_min).count(),
				std::chrono::duration_cast<std::chrono::duration<double>>(_random_max).count());
		}

		std::chrono::duration<double> calc_delay(std::shared_ptr<basic_node> to) override {
			auto tto = std::dynamic_pointer_cast<with_delay_linear<wrapped_comm_type>>(to->get_comm());
			assert(tto);

			double distance = this->get_node()->get_location().distance_to(to->get_location());

			return sender_base + tto->receiver_base + (multiplier * distance) +
				std::chrono::duration<double>(random(random_generator));
		}
	};



	template <typename wrapped_comm_type>
	class packaged : public wrapped_comm_type {
	protected:
		struct header {
			uint msg_id;
			ushort pkg_count;
			ushort pkg_id;
		};

		struct package_info {
			double arrival_time;
			ushort pkg_count;
			ushort pkg_id;
			std::vector<uchar> data;
		};

		uint msg_id = 1;
		uint package_size = 64;
		double time_to_keep = 60;	// seconds
		std::multimap<uint, std::shared_ptr<package_info>> buffer;
		std::mutex buffer_mutex;

		void split_data(const std::vector<uchar>& data, std::list<std::vector<uchar>>& packages) {
			uint max_pkg_data_sz = package_size - sizeof(header);

			header hdr;
			hdr.msg_id = msg_id++;
			hdr.pkg_count = ushort((data.size() + package_size - 1) / package_size);

			for (uint i = 1; i <= hdr.pkg_count; i++) {
				uint pkg_data_sz = (i < hdr.pkg_count) ? max_pkg_data_sz : (data.size() - (i - 1) * max_pkg_data_sz);
				std::vector<uchar> pkg(pkg_data_sz + sizeof(header));

				hdr.pkg_id = i;
				std::memcpy(pkg.data(), &hdr, sizeof(hdr));
				std::memcpy(pkg.data() + sizeof(hdr), data.data() + (i - 1) * max_pkg_data_sz, pkg_data_sz);
				packages.emplace_back(pkg);
			}
		}

		bool reconstruct_data(uint msg_id, std::vector<uchar>& data) {
			std::lock_guard lock(buffer_mutex);

			auto range = buffer.equal_range(msg_id);
			std::list<std::shared_ptr<package_info>> packages;
			for (auto itr = range.first; itr != range.second; itr++)
				packages.push_back(itr->second);

			if (packages.size() == 0) return false;

			auto& first = *packages.begin();
			if (packages.size() < first->pkg_count) return false;

			packages.sort([](auto& a, auto& b) {
				return a->pkg_id < b->pkg_id;
			});

			for (auto& pkg : packages) {
				data.insert(data.end(), pkg->data.begin(), pkg->data.end());
			}

			buffer.erase(msg_id);

			return true;
		}


	public:
		void set_package_size(uint ps) {
			package_size = ps;
		}

		uint get_package_size() const {
			return package_size;
		}

		void set_time_to_keep(double ttk) {
			time_to_keep = ttk;
		}

		int get_time_to_keep() const {
			return time_to_keep;
		}

		void send(const std::vector<uchar>& data, std::shared_ptr<basic_node> to) override {
			std::list<std::vector<uchar>> packages;
			split_data(data, packages);
			std::for_each(std::execution::par, packages.begin(), packages.end(), [this, to](auto& pkg) {
				wrapped_comm_type::send(pkg, to);
			});
		}

		void route(const std::vector<uchar>& data, std::shared_ptr<basic_node> to) override {
			std::list<std::vector<uchar>> packages;
			split_data(data, packages);
			std::for_each(std::execution::par, packages.begin(), packages.end(), [this, to](auto& pkg) {
				wrapped_comm_type::route(pkg, to);
			});
		}

		bool receive(std::shared_ptr<std::vector<uchar>> data, std::shared_ptr<basic_node> from) override {
			if (!wrapped_comm_type::receive(data, from)) return false;

			header hdr;
			this->extract_header(*data, hdr);
			this->remove_header(*data, sizeof(hdr));

			{
				std::lock_guard lock(buffer_mutex);
				// make sure no duplicate when something goes wrong!
				auto range = buffer.equal_range(hdr.msg_id);
				for (auto itr = range.first; itr != range.second; itr++) {
					if (itr->second->pkg_id == hdr.pkg_id) return false;
				}

				// push package to buffer
				auto inf = std::make_shared<package_info>();
				inf->arrival_time = this->get_local_clock_time();
				inf->pkg_count = hdr.pkg_count;
				inf->pkg_id = hdr.pkg_id;
				inf->data = *data;

				buffer.emplace(hdr.msg_id, inf);
			}

			auto orgdata = std::make_shared<std::vector<uchar>>();
			bool ok = reconstruct_data(hdr.msg_id, *orgdata);
			if (ok) data = orgdata;

			// remove expired packages!
			double good_arrival_time = this->get_local_clock_time() - time_to_keep;
			{
				std::lock_guard lock(buffer_mutex);
				for (auto itr = buffer.begin(); itr != buffer.end(); ) {
					if (itr->second->arrival_time < good_arrival_time)
						itr = buffer.erase(itr);
					else itr++;
				}
			}

			return ok;
		}
	};




	template <typename wrapped_comm_type>
	class with_loss : public wrapped_comm_type {
	protected:
		double rate = 0.;	// 0..1

		std::default_random_engine random_generator{ (uint)time(nullptr) };
		std::uniform_real_distribution<double> random{ 0., 1. };

	public:
		void set_loss_rate(double _rate) {
			rate = _rate;
		}

		double get_loss_rate() const {
			return rate;
		}

		bool receive(std::shared_ptr<std::vector<uchar>> data, std::shared_ptr<basic_node> from) override {
			if (random(random_generator) <= rate) return false;
			return wrapped_comm_type::receive(data, from);
		}
	};




	template <typename msg_id_type, typename wrapped_comm_type>
	class loop_avoidance : public wrapped_comm_type {
	protected:
		struct msg_info {
			double arrival_time;
			msg_id_type msg_id;
		};

		std::mutex last_messages_mutex;
		std::vector<msg_info> last_messages;
		uint last_messages_max = 100;	// max length, disabled by default
		double last_messages_time = 30;	// seconds, disabled by default

		void add_last_message_info(const msg_id_type& msg_id) {
			msg_info inf;
			inf.arrival_time = this->get_local_clock_time();
			inf.msg_id = msg_id;
			last_messages.push_back(inf);

			if (last_messages_max >= 0) {
				while (last_messages.size() > last_messages_max)
					last_messages.erase(last_messages.end() - 1);
			}

			if (last_messages_time >= 0) {
				double last_good_time = this->get_local_clock_time() - last_messages_time;
				auto itr = std::remove_if(last_messages.begin(), last_messages.end(), [last_good_time](auto& e) {
					return e.arrival_time < last_good_time;
				});
				last_messages.erase(itr, last_messages.end());
			}
		}

	public:
		uint get_max_last_messages() const {
			return last_messages_max;
		}

		void set_max_last_messages(uint v) {
			last_messages_max = v;
		}

		uint get_last_messages_time() const {
			return last_messages_time;
		}

		void set_last_messages_time(double v) {
			last_messages_time = v;
		}

		virtual msg_id_type get_message_id(const std::vector<uchar>& data) const = 0;

		bool receive(std::shared_ptr<std::vector<uchar>> data, std::shared_ptr<basic_node> from) override {
			if (!wrapped_comm_type::receive(data, from)) return false;

			auto msg_id = get_message_id(*data);

			std::lock_guard lock(last_messages_mutex);
			if (std::find_if(last_messages.begin(), last_messages.end(), [msg_id](auto& e) {
				return msg_id == e.msg_id;
			}) != last_messages.end()) return false;

			add_last_message_info(msg_id);
			return true;
		}
	};



	template <typename wrapped_comm_type>
	class hierarchical_routing : public wrapped_comm_type {
	protected:
		struct header {
			uint dest_id;
		};

		std::list<std::shared_ptr<basic_node>> neighbors;

	public:
		const std::list<std::shared_ptr<basic_node>>& get_neighbors() const {
			return neighbors;
		}

		bool has_neighbor(std::shared_ptr<basic_node> p) const {
			return std::find(neighbors.begin(), neighbors.end(), p) != neighbors.end();
		}

		void add_neighbor(std::shared_ptr<basic_node> p) {
			neighbors.push_back(p);
		}

		void add_neighbors(std::initializer_list<std::shared_ptr<basic_node>> p) {
			neighbors.insert(neighbors.end(), p);
		}

		void remove_neighbor(std::shared_ptr<basic_node> p) {
			neighbors.remove(p);
		}

		void remove_neighbors(std::initializer_list<std::shared_ptr<basic_node>> p) {
			for (auto& pp : p)
				neighbors.remove(p);
		}

		void remove_all_neighbors() {
			neighbors.clear();
		}

		bool receive(std::shared_ptr<std::vector<uchar>> data, std::shared_ptr<basic_node> from) override {
			if (!wrapped_comm_type::receive(data, from)) return false;

			header hdr;
			this->extract_header(*data, hdr);

			if (hdr.dest_id == this->get_node()->get_id()) return true;

			if (neighbors.size() > 0) {
				std::for_each(std::execution::par, neighbors.begin(), neighbors.end(), [this, data](auto p) {
					wrapped_comm_type::send(*data, p);
				});

				this->fire(basic_comm::event_forward, data, from);
			}
			return true;
		}

		void send(const std::vector<uchar>& data, std::shared_ptr<basic_node> to) override {
			assert(false);
		}

		void route(const std::vector<uchar>& data, std::shared_ptr<basic_node> to) override {
			if (to->is_same(this->get_node())) return;

			header hdr;
			hdr.dest_id = to->get_id();

			auto newdata = data;
			this->add_header(newdata, hdr);
			std::for_each(std::execution::par, neighbors.begin(), neighbors.end(), [this, &newdata](auto p) {
				wrapped_comm_type::send(newdata, p);
			});
		}
	};


	
	// similar to hierarchical_routing but with loop_avoidance
	template <typename wrapped_comm_type>
	class fixed_routing : public hierarchical_routing<loop_avoidance<std::pair<uint, uint>, wrapped_comm_type>> {
	protected:
		using base_class = loop_avoidance<std::pair<uint, uint>, wrapped_comm_type>;

		struct header {
			uint dest_id;
			uint msg_id;
			uint source_id;
		};

		uint message_id = 1;

	public:
		std::pair<uint, uint> get_message_id(const std::vector<uchar>& data) const override {
			header hdr;
			this->extract_header(data, hdr);
			return std::make_pair(hdr.source_id, hdr.msg_id);
		}

		bool receive(std::shared_ptr<std::vector<uchar>> data, std::shared_ptr<basic_node> from) override {
			if (!base_class::receive(data, from)) return false;

			header hdr;
			this->extract_header(*data, hdr);

			if (hdr.dest_id == this->get_node()->get_id()) return true;

			if (this->neighbors.size() > 0) {
				std::for_each(std::execution::par, this->neighbors.begin(), this->neighbors.end(), [this, data](auto p) {
					base_class::send(*data, p);
				});

				this->fire(basic_comm::event_forward, data, from);
			}
			return true;
		}

		void route(const std::vector<uchar>& data, std::shared_ptr<basic_node> to) override {
			if (to->is_same(this->get_node())) return;

			header hdr;
			hdr.dest_id = to->get_id();
			hdr.source_id = this->get_id();
			hdr.msg_id = message_id++;

			auto newdata = data;
			this->add_header(newdata, hdr);
			std::for_each(std::execution::par, this->neighbors.begin(), this->neighbors.end(), [this, &newdata](auto p) {
				base_class::send(newdata, p);
			});
		}
	};




	template <typename wrapped_comm_type>
	class broadcast_routing : public loop_avoidance<std::pair<uint, uint>, wrapped_comm_type> {
	protected:
		using base_class = loop_avoidance<std::pair<uint, uint>, wrapped_comm_type>;

		struct header {
			uint dest_id;
			uint source_id;
			uint msg_id;
			double transmission_time;
			int hops_to_live;	// hops to live, -1 means unused
			int time_to_live;	// time to live in seconds, -1 means unused
		};

		uint message_id = 1;
		double broadcast_range = 1.;	// in meters
		int hops_to_live = -1;
		int time_to_live = -1;

	public:
		double get_broadcast_range() const {
			return broadcast_range;
		}

		void set_broadcast_range(double _range) {
			broadcast_range = _range;
		}

		int get_time_to_live() const {
			return time_to_live;
		}

		void set_time_to_live(int ttl) {
			time_to_live = ttl;
		}

		int get_hops_to_live() const {
			return hops_to_live;
		}

		void set_hops_to_live(int htl) {
			hops_to_live = htl;
		}

		std::pair<uint, uint> get_message_id(const std::vector<uchar>& data) const override {
			header hdr;
			this->extract_header(data, hdr);
			return std::make_pair(hdr.source_id, hdr.msg_id);
		}

		bool receive(std::shared_ptr<std::vector<uchar>> data, std::shared_ptr<basic_node> from) override {
			if (!base_class::receive(data, from)) return false;

			auto& hdr = this->extract_header<header>(*data);

			// the message is to me!
			if (hdr.dest_id == this->get_node()->get_id()) {
				this->remove_header(*data, sizeof(hdr));
				return true;
			}

			// forward if not expired
			auto newdata = *data;
			auto& newhdr = this->extract_header<header>(newdata);

			if (newhdr.time_to_live > 0 && this->get_world_clock_time() - newhdr.time_to_live >= newhdr.transmission_time) {
				this->fire(basic_comm::event_drop, data, from);
				return false;
			}

			if (newhdr.hops_to_live == 0 || newhdr.hops_to_live == 1) {
				this->fire(basic_comm::event_drop, data, from);
				return false;
			}
			else if (newhdr.hops_to_live > 1) {
				newhdr.hops_to_live--;
			}

			this->fire(basic_comm::event_forward, this->make_shared_data(newdata), from);
			this->broadcast_by_distance(newdata, broadcast_range, [this, &newdata](auto p) {
				base_class::send(newdata, p);
			});
			return false;
		}

		void send(const std::vector<uchar>& data, std::shared_ptr<basic_node> to) override {
			assert(false);
		}

		void route(const std::vector<uchar>& data, std::shared_ptr<basic_node> to) override {
			if (to->is_same(this->get_node())) return;

			header hdr;
			hdr.dest_id = to->get_id();
			hdr.source_id = this->get_id();
			hdr.msg_id = message_id++;
			hdr.transmission_time = this->get_world_clock_time();
			hdr.time_to_live = time_to_live;
			hdr.hops_to_live = hops_to_live;

			auto newdata = data;
			this->add_header(newdata, hdr);
			this->broadcast_by_distance(newdata, broadcast_range, [this, &newdata](auto p) {
				base_class::send(newdata, p);
			});
		}
	};




	template <typename wrapped_comm_type>
	class spanning_tree_routing : public wrapped_comm_type {
	protected:
		enum class message_type : uchar {
			notify_root,
			update_child,
			send_data
		};

		struct notify_root_data_type {
			double cost;
			uint root_id;
		};

		struct update_child_data_type {
			uint child_id;
		};

		struct header {
			message_type msg_type;
			double send_time;
			std::variant<notify_root_data_type, update_child_data_type> data;
		};

		bool i_am_root = true;
		uint root_id = -1;
		double cost_to_root = 0.;
		std::shared_ptr<basic_node> parent;
		std::list<uint> children;

		header& prepare_message(message_type type, std::vector<uchar>& data) {
			header hdr;
			hdr.msg_type = type;
			hdr.send_time = this->get_world_clock_time();
			this->add_header(data, hdr);

			return this->extract_header<header>(data);
		}

	public:
		virtual void start() override {
			wrapped_comm_type::start();

			i_am_root = true;
			root_id = this->get_node()->get_id();
			cost_to_root = 0.;
			parent = nullptr;
			children.clear();

			std::vector<uchar> data;
			auto& hdr = prepare_message(message_type::notify_root, data);
			auto& hdr_data = std::get<notify_root_data_type>(hdr.data);
			hdr_data.root_id = root_id;
			hdr_data.cost = 0.;
			this->broadcast_by_distance(data, 5);
		}

		virtual bool receive(std::shared_ptr<std::vector<uchar>> data, std::shared_ptr<basic_node> from) override {
			if (!wrapped_comm_type::receive(data, from)) return false;

			auto& hdr = this->extract_header<header>(*data);
			auto& hdr_data = std::get<notify_root_data_type>(hdr.data);
			if (hdr.msg_type == message_type::notify_root) {
				if (hdr_data.root_id < root_id) {
					i_am_root = false;
					root_id = hdr_data.root_id;
					cost_to_root = hdr_data.cost + this->get_world_clock_time() - hdr.send_time;
					parent = from;

					std::vector<uchar> data2;
					auto& hdr2 = prepare_message(message_type::update_child, data2);
					auto& hdr2_data = std::get<update_child_data_type>(hdr2.data);
					hdr2_data.child_id = this->get_node()->get_id();
					wrapped_comm_type::send(data2, parent);

					std::vector<uchar> data3;
					auto& hdr3 = prepare_message(message_type::notify_root, data3);
					auto& hdr3_data = std::get<notify_root_data_type>(hdr3.data);
					hdr3_data.root_id = root_id;
					hdr3_data.cost = 0.;
					this->broadcast_by_distance(data3, 5);
				}
				else {
					std::vector<uchar> data2;
					auto& hdr2 = prepare_message(message_type::notify_root, data2);
					auto& hdr2_data = std::get<notify_root_data_type>(hdr2.data);
					hdr2_data.root_id = root_id;
					hdr2_data.cost = cost_to_root;
					wrapped_comm_type::send(data2, parent);
				}

			}
			else if (hdr.msg_type == message_type::update_child) {

			}

		}
	};
}