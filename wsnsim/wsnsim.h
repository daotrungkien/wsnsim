#pragma once

#include <stdlib.h>

#include <string>
#include <list>
#include <functional>
#include <algorithm>
#include <thread>
#include <mutex>

#include "Utilities.h"

#undef max
#undef min
#include "json.hpp"



using json = nlohmann::json;


namespace wsn {

using uint = unsigned int;


class wsn_type : public std::enable_shared_from_this<wsn_type> {
public:
	virtual json to_json() const {
		return json();
	}
	virtual void from_json(const json& j) {
	}
};



template <class key_type = uint, class... Targs>
class event_manager : public wsn_type {
protected:
	typedef std::function<void(Targs... args)> consumer_type;

	std::multimap<key_type, consumer_type > subscribers;

public:
	void bind(key_type key, consumer_type consumer) {
		subscribers.insert(std::make_pair<>(key, consumer));
	}

	void on(key_type key) const {
		auto range = subscribers.equal_range(key);
		for (auto itr = range.first; itr != itr.second; itr++)
			itr->second();
	}

	template <class... Targs>
	void on(key_type key, Targs... args) const {
		auto range = subscribers.equal_range(key);
		for (auto itr = range.first; itr != range.second; itr++)
			itr->second(std::forward<Targs>(args)...);
	}
};



class basic_name : public wsn_type {
public:
};

class string_name : public tstring, public basic_name {
public:
	using tstring::tstring;

	virtual json to_json() const {
		return as_string(*this).c_str();
	}
	virtual void from_json(const json& j) {
	}
};





class basic_location : public wsn_type {
public:
};

template <class coord_type = double>
class location2d : public basic_location {
public:
	coord_type x, y;

	location2d(coord_type _x, coord_type _y)
		: x(_x), y(_y)
	{}

	double distance_to(const location2d& p) const {
		double dx = x - p.x, dy = y - p.y;
		return sqrt(dx*dx + dy*dy)
	}

	virtual json to_json() const {
		return json({ { "x", x }, { "y", y } });
	}
	virtual void from_json(const json& j) {
		x = j["x"];
		y = j["y"];
	}
};

template <class coord_type = double>
class location3d : public basic_location {
public:
	coord_type x, y, z;

	location3d(coord_type _x, coord_type _y, coord_type _z)
		: x(_x), y(_y), z(_z)
	{}

	double distance_to(const location3d& p) const {
		double dx = x - p.x, dy = y - p.y, dz = z - p.z;
		return sqrt(dx*dx + dy*dy + dz*dz)
	}

	virtual json to_json() const {
		return json({ { "x", x }, { "y", y }, { "z", z } });
	}
	virtual void from_json(const json& j) {
		x = j["x"];
		y = j["y"];
		z = j["z"];
	}
};


tstring format_time(double t, int prec = 3);










class basic_node;
class network;

class basic_comm : public wsn_type {
protected:
	basic_node* node;
public:
	basic_comm(basic_node* _node)
		: node(_node)
	{}

	friend basic_node;
};

class comm_none : public basic_comm {
public:
	comm_none(basic_node* _node)
		: basic_comm(_node)
	{}
};



class basic_sensor : public wsn_type {
protected:
	basic_node* node;
public:
	basic_sensor(basic_node* _node)
		: node(_node)
	{}
};

class sensor_none : public basic_sensor {
public:
	sensor_none(basic_node* _node)
		: basic_sensor(_node)
	{}
};



class basic_battery : public wsn_type {
protected:
	basic_node* node;
public:
	static const uint event_charge;
	static const uint event_discharge;
	static const uint event_full;
	static const uint event_empty;

	basic_battery(basic_node* _node)
		: node(_node)
	{}

	bool consume(double d) {
		if (get_level() - get_min_level() < d) return false;
		discharge(d);
		return true;
	}

	virtual void charge(double d) = 0;
	virtual void discharge(double d) = 0;
	virtual double get_level() const = 0;
	virtual double get_min_level() const = 0;
	virtual double get_max_level() const = 0;
};


class min_max_battery : public basic_battery {
protected:
	double minl, maxl, level;
public:
	min_max_battery(basic_node* _node)
		: basic_battery(_node)
	{
		minl = 0;
		maxl = 100;
		level = maxl;
	}

	virtual void charge(double d);
	virtual void discharge(double d);

	void set_min_level(double _minl) {
		minl = _minl;
		if (level < minl) level = minl;
	}
	void set_max_level(double _maxl) {
		maxl = _maxl;
		if (level > maxl) level = maxl;
	}

	virtual double get_level() const {
		return level;
	}

	virtual double get_min_level() const {
		return minl;
	}

	virtual double get_max_level() const {
		return maxl;
	}
};




class basic_power : public wsn_type {
protected:
	basic_node* node;
public:
	basic_power(basic_node* _node)
		: node(_node)
	{}
};

class power_none : public basic_power {
public:
	power_none(basic_node* _node)
		: basic_power(_node)
	{}
};





class basic_node : public wsn_type {
protected:
	int id;
	network* wsn;

	bool retired = true;
	std::mutex thread_lock;
	std::thread thread;

public:
	static const uint event_starting;
	static const uint event_started;
	static const uint event_stopping;
	static const uint event_stopped;

	basic_node() {
	}

	virtual ~basic_node() {
		stop();
	}

	int get_id() const {
		return id;
	}

	network* get_network() const {
		return wsn;
	}

	void on(uint event);

	virtual void run() = 0;

	void start() {
		std::lock_guard<std::mutex> lock(thread_lock);
		if (thread.joinable()) return;

		retired = false;
		on(event_starting);
		thread = std::thread([this]() {
			run();
		});
	}

	bool is_started() const {
		return thread.joinable();
	}

	void stop() {
		std::lock_guard<std::mutex> lock(thread_lock);
		if (!thread.joinable()) return;

		retired = true;
		on(event_stopping);

		thread.join();
	}

	friend network;
};


template <
		class comm_type,
		class sensor_type,
		class battery_type,
		class power_type,
		class location_type = location3d<>,
		class name_type = string_name
	>
class generic_node : public basic_node {
protected:
	name_type name;
	location_type location;
	comm_type comm{ this };
	sensor_type sensor{ this };
	battery_type battery{ this };
	power_type power{ this };
	double cycle_time = 1.;

public:
	generic_node(const name_type& _name, const location_type& _loc, double _cycle_time = 1.)
		: name(_name), location(_loc), cycle_time(_cycle_time) {
	}

	const location_type& get_location() const		{ return location; }
	void set_location(const location_type& _loc)	{ location = _loc; }
	const comm_type& get_comm() const				{ return comm; }
	const name_type& get_name() const				{ return name; }
	void set_name(const name_type& _name)			{ name = _name; }
	const battery_type& get_battery() const			{ return battery; }
	const sensor_type& get_sensor() const			{ return sensor; }
	const power_type& get_power() const				{ return power; }
	double get_cycle_time() const					{ return cycle_time; }
	void set_cycle_time(double ct) const			{ cycle_time = ct; }

	virtual ~generic_node() {
	}

	virtual void cycle() {
	}

	virtual void run() {
		on(event_started);

		while (!retired) {
			cycle();

			if (cycle_time > 0) std::this_thread::sleep_for(chrono::duration<double>(cycle_time));
		}

		on(event_stopped);
	}
};


class network : public wsn_type {
protected:
	int next_auto_id = 1;

	int generate_new_id() {
		return next_auto_id++;
	}

protected:
	std::list< std::shared_ptr<basic_node> > nodes;
	event_manager<uint, uint, basic_node*> events;

	bool started = false;
	std::chrono::steady_clock::time_point start_time;

public:
	static const uint event_starting;
	static const uint event_started;
	static const uint event_stopping;
	static const uint event_stopped;

	network() {
	}

	virtual ~network() {
	}

	template <class node_type, class... Targs>
	std::shared_ptr<node_type> add_node(Targs... args) {
		std::shared_ptr<node_type> node(new node_type(std::forward<Targs>(args)...));
		node->wsn = this;
		node->id = generate_new_id();
		nodes.push_back(node);

		return node;
	}

	std::shared_ptr<basic_node> find_node(int id) {
		for (auto i = nodes.begin(); i != nodes.end(); i++) {
			if ((*i)->get_id() == id)
				return *i;
		}

		return nullptr;
	}

	void remove_node(int id) {
		for (auto i = nodes.begin(); i != nodes.end(); i++) {
			if ((*i)->get_id() == id) {
				nodes.erase(i);
				break;
			}
		}
	}

	void bind(uint event, std::function<void(uint, basic_node*)> consumer) {
		events.bind(event, consumer);
	}

	void on(uint event, basic_node* node = nullptr) {
		events.on(event, event, node);
	}

	virtual void start() {
		started = true;
		start_time = std::chrono::steady_clock::now();
		on(event_starting);

		for (auto& node : nodes)
			node->start();

		on(event_started);
	}

	virtual void stop() {
		started = false;
		on(event_stopping);

		for (auto& node : nodes)
			node->stop();

		on(event_stopped);
	}

	bool is_started() const {
		return started;
	}

	double get_time() const {
		auto t = std::chrono::steady_clock::now() - start_time;
		return std::chrono::duration_cast<std::chrono::duration<double>>(t).count();
	}

};



}