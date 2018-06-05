#pragma once

#include <stdlib.h>

#include <string>
#include <list>
#include <functional>
#include <algorithm>
#include <thread>
#include <mutex>
#include <any>
#include <chrono>
#include <execution>
#include <condition_variable>
#include <random>

#include "../lib/eigen/Eigen/Dense"

#include "utilities/utilities.h"

#undef max
#undef min
#include "../lib/json.hpp"



#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif



namespace wsn {

using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using json = nlohmann::json;
using vector3 = Eigen::Vector3d;

class entity;
class basic_node;
class basic_network;
class basic_world;


kutils::tstring format_time(double t, int prec = 3);
std::string format_time(std::chrono::system_clock::time_point t = std::chrono::system_clock::now(), const char* format = "%Y-%m-%d %H:%M:%S", int prec = 3);


template <typename type = uint>
type unique_id() {
	static type id = 1;
	return id++;
}

class wsn_type : public std::enable_shared_from_this<wsn_type> {
public:
	virtual ~wsn_type() {
	}

	virtual json to_json() const {
		return json();
	}

	virtual void from_json(const json& j) {
	}
};



class location : public wsn_type {
public:
	double x, y, z;

	location(double _x = 0., double _y = 0., double _z = 0.)
		: x(_x), y(_y), z(_z)
	{}

	location operator -() const {
		return location(-x, -y, -z);
	}

	location operator +(const location& l) const {
		return location(x + l.x, y + l.y, z + l.z);
	}

	location operator -(const location& l) const {
		return location(x - l.x, y - l.y, z - l.z);
	}

	location operator *(double d) const {
		return location(x * d, y * d, z * d);
	}

	location operator /(double d) const {
		return location(x / d, y / d, z / d);
	}

	location operator *(const location& l) const {	// dot product
		return location(x - l.x, y - l.y, z - l.z);
	}

	location operator ^(const location& l) const {	// cross product
		return location(y*l.z - z * l.y, z*l.x - x * l.y, x * l.y - y * l.x);
	}

	location& operator +=(const location& l) {
		x += l.x;
		y += l.y;
		z += l.z;
		return *this;
	}

	location& operator -=(const location& l) {
		x -= l.x;
		y -= l.y;
		z -= l.z;
		return *this;
	}

	location& operator *=(double d) {
		x *= d;
		y *= d;
		z *= d;
		return *this;
	}

	location& operator /=(double d) {
		x /= d;
		y /= d;
		z /= d;
		return *this;
	}

	location& operator ^=(const location& l) {	// cross product
		double x1 = y * l.z - z * l.y,
			y1 = z * l.x - x * l.y,
			z1 = x * l.y - y * l.x;
		x = x1;
		y = y1;
		z = z1;
		return *this;
	}



	double distance_to(const location& p) const {
		double dx = x - p.x, dy = y - p.y, dz = z - p.z;
		return sqrt(dx*dx + dy * dy + dz * dz);
	}

	json to_json() const override {
		return json({ { "x", x },{ "y", y },{ "z", z } });
	}

	void from_json(const json& j) override {
		x = j["x"];
		y = j["y"];
		z = j["z"];
	}
};


location operator *(double d, const location& l);



class reference_frame : public wsn_type {
protected:
	Eigen::Matrix3d mat, invmat;
	vector3 org, orglla;
	double timezone;

public:
	reference_frame() {
		mat.setIdentity();
		invmat.setIdentity();
	}

	void set_timezone(double tz) {
		timezone = tz;
	}

	double get_timezone() const {
		return timezone;
	}

	void set_frame(double org_lat, double org_lon, double org_alt,
		double ext1_lat, double ext1_lon, double ext1_alt,
		double ext2_lat, double ext2_lon, double ext2_alt);
	void set_frame(double org_lat, double org_lon, double org_alt,
		double ext_lat, double ext_lon, double ext_alt);

	const vector3& get_org() const {
		return org;
	}

	void get_org(double& lat, double& lon, double& alt) const {
		lat = orglla[0];
		lon = orglla[1];
		alt = orglla[2];
	}

	vector3 zenith_dir() const;
	vector3 north_dir() const;

	void local2global(double xl, double yl, double zl, double& xg, double& yg, double& zg) const;
	void local2lla(double xl, double yl, double zl, double& lat, double& lon, double& alt) const;
	void global2local(double xg, double yg, double zg, double& xl, double& yl, double& zl) const;
	void lla2local(double lat, double lon, double alt, double& xl, double& yl, double& zl) const;

	static void lla2global(double lat, double lon, double alt, double& xg, double& yg, double& zg);
	static void global2lla(double xg, double yg, double zg, double& lat, double& lon, double& alt);
};


class clock : public wsn_type {
protected:
	bool ref_same_as_sys = true;
	std::chrono::system_clock::time_point sys_start_time;	// real system start time
	std::chrono::system_clock::time_point ref_start_time;	// reference start time
	double scale = 1.;
	bool started = false;

public:
	void set(double _scale = 1.) {
		assert(!started);

		ref_same_as_sys = true;
		scale = _scale;
	}

	void set(std::chrono::system_clock::time_point _ref_start_time, double _scale = 1.) {
		assert(!started);

		ref_same_as_sys = false;
		ref_start_time = _ref_start_time;
		scale = _scale;
	}

	void start() {
		assert(!started);

		sys_start_time = std::chrono::system_clock::now();
		if (ref_same_as_sys) ref_start_time = sys_start_time;
		started = true;
	}

	void stop() {
		assert(started);

		started = false;
	}

	double get_scale() const {
		return scale;
	}

	const auto& get_reference_start_time() const {
		return ref_start_time;
	}

	const auto& get_system_start_time() const {
		return sys_start_time;
	}

	std::chrono::system_clock::time_point reference2system(std::chrono::system_clock::time_point t) const {
		assert(started);

		double dt = std::chrono::duration<double>(t - ref_start_time).count();
		auto sdt = std::chrono::duration<double>(dt / scale);
		return std::chrono::duration_cast<std::chrono::system_clock::duration>(sdt) + sys_start_time;
	}

	std::chrono::system_clock::time_point system2reference(std::chrono::system_clock::time_point t) const {
		assert(started);

		double dt = std::chrono::duration<double>(t - sys_start_time).count();
		auto sdt = std::chrono::duration<double>(dt * scale);
		return std::chrono::duration_cast<std::chrono::system_clock::duration>(sdt) + ref_start_time;
	}

	std::chrono::system_clock::time_point clock2reference(double t) const {
		assert(started);

		return ref_start_time + std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::duration<double>(t));
	}

	double reference2clock(std::chrono::system_clock::time_point t) const {
		assert(started);

		return (std::chrono::duration_cast<std::chrono::duration<double>>(t - ref_start_time)).count();
	}

	std::chrono::system_clock::time_point clock2system(double t) const {
		assert(started);

		return sys_start_time + std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::duration<double>(t / scale));
	}

	double system2clock(std::chrono::system_clock::time_point t) const {
		assert(started);

		return std::chrono::duration_cast<std::chrono::duration<double>>(t - sys_start_time).count() * scale;
	}

	std::chrono::system_clock::time_point system_now() const {
		assert(started);

		return std::chrono::system_clock::now();
	}

	std::chrono::system_clock::time_point reference_now() const {
		assert(started);

		return system2reference(std::chrono::system_clock::now());
	}

	double clock_now() const {
		assert(started);

		return system2clock(std::chrono::system_clock::now());
	}


	static std::chrono::system_clock::time_point mktime(int year, int mon, int day, int hour = 0, int minute = 0, int sec = 0) {
		tm tinf;
		tinf.tm_year = year - 1900;
		tinf.tm_mon = mon - 1;
		tinf.tm_mday = day - 1;
		tinf.tm_hour = hour;
		tinf.tm_min = minute;
		tinf.tm_sec = sec;
		return std::chrono::system_clock::from_time_t(::mktime(&tinf));
	}
};





class event {
public:
	uint event_id;
	std::shared_ptr<entity> target;
};


class entity : public wsn_type {
public:
	template <typename Function>
	struct function_traits
		: public function_traits<decltype(&Function::operator())>
	{};

	template <typename ClassType, typename ReturnType, typename... Args>
	struct function_traits<ReturnType(ClassType::*)(Args...) const> {
		typedef ReturnType(*pointer)(Args...);
		typedef std::function<ReturnType(Args...)> function;
	};

	template <typename Function>
	typename function_traits<Function>::pointer to_function_pointer(Function& lambda) {
		return static_cast<typename function_traits<Function>::pointer>(lambda);
	}

	template <typename Function>
	typename function_traits<Function>::function to_function(Function& lambda) {
		return static_cast<typename function_traits<Function>::function>(lambda);
	}


protected:
	struct event_info {
		uint key;
		void* function;
		bool self_only;
		const std::type_info* signature;
	};

	struct timer_info {
		uint key;
		bool sync_start_stop;
		enum { running, paused, stopped } status;
		std::mutex mutex;
		std::condition_variable cond;
	};

	uint id = unique_id();
	std::weak_ptr<entity> parent;
	std::multimap<uint, event_info> event_map;
	std::list<std::shared_ptr<timer_info>> timer_list;
	std::mutex event_map_mutex, timer_list_mutex;
	double start_time;
	bool started = false;
	bool first_start = true;

public:
	static inline uint event_timer = unique_id();
	static inline uint event_init = unique_id();
	static inline uint event_finalize = unique_id();
	static inline uint event_first_start = unique_id();
	static inline uint event_start = unique_id();
	static inline uint event_stop = unique_id();

	~entity() override {
		if (is_started()) stop();

		for (auto e : event_map) {
			delete static_cast<std::function<void()>*>(e.second.function);
		}
	}

	uint get_id() const {
		return id;
	}

	bool is_same(entity& other) const {
		return id == other.id;
	}

	bool is_same(std::shared_ptr<entity> other) const {
		return id == other->id;
	}

	virtual std::shared_ptr<basic_world> get_world() = 0;
	virtual std::shared_ptr<const basic_world> get_world() const = 0;

	double get_local_clock_time() const;
	double get_world_clock_time() const;
	std::chrono::system_clock::time_point get_reference_time() const;
	std::chrono::system_clock::time_point get_system_time() const;

	void set_parent(std::shared_ptr<entity> _parent) {
		parent = _parent;
	}

	void set_parent_for(std::shared_ptr<entity> child) {
		child->set_parent(std::dynamic_pointer_cast<entity>(shared_from_this()));
	}

	std::shared_ptr<entity> get_parent() const {
		auto sp = parent.lock();
		assert(sp);
		return sp;
	}

	template <typename _Rep, typename _Period>
	uint timer(std::chrono::duration<_Rep, _Period> dur, bool sync_start_stop, std::function<bool(event&)> callback) {
		auto inf = std::make_shared<timer_info>();
		inf->key = unique_id();
		inf->sync_start_stop = sync_start_stop;

		{
			std::lock_guard lock(timer_list_mutex);
			timer_list.push_back(inf);
		}

		if (sync_start_stop) {
			inf->status = started ? timer_info::running : timer_info::paused;

			on_self(event_start, [inf](event&) {
				inf->status = timer_info::running;
				inf->cond.notify_one();
			});

			on_self(event_stop, [inf](event&) {
				inf->status = timer_info::paused;
				inf->cond.notify_one();
			});
		}
		else {
			inf->status = timer_info::running;
		}

		std::thread([inf, dur, sync_start_stop, callback, this]() {
			do {
				if (inf->status == timer_info::paused) {
					std::unique_lock lock(inf->mutex);
					inf->cond.wait(lock, [inf]() { return inf->status != timer_info::paused; });
				}

				while (inf->status == timer_info::running) {
					auto rdur = dur / (get_world()->get_clock().get_scale());

					std::unique_lock lock(inf->mutex);
					if (inf->cond.wait_for(lock, rdur, [inf]() { return inf->status != timer_info::running; })) break;

					event ev;
					ev.event_id = event_timer;
					ev.target = std::dynamic_pointer_cast<entity>(shared_from_this());
					if (!callback(ev)) {
						inf->status = timer_info::stopped;
						break;
					}
				}
			} while (inf->status != timer_info::stopped);

			{
				std::lock_guard lock(timer_list_mutex);

				auto itr = std::find_if(timer_list.begin(), timer_list.end(), [inf](auto inf2) {
					return inf2->key == inf->key;
				});

				if (itr != timer_list.end()) timer_list.erase(itr);
			}
		}).detach();

		return inf->key;
	}

	template <typename _Rep, typename _Period>
	uint timer(std::chrono::duration<_Rep, _Period> dur, bool sync_start_stop, std::function<void(event&)> callback) {
		return timer(dur, sync_start_stop, (std::function<bool(event&)>)[callback](event& ev) -> bool {
			callback(ev);
			return true;
		});
	}

	template <typename _Rep, typename _Period>
	uint timer_once(std::chrono::duration<_Rep, _Period> dur, bool sync_start_stop, std::function<void(event&)> callback) {
		return timer(dur, sync_start_stop, (std::function<bool(event&)>)[callback](event& ev) -> bool {
			callback(ev);
			return false;
		});
	}

	void stop_timer(uint key) {
		auto itr = std::find_if(timer_list.begin(), timer_list.end(), [key](auto inf) {
			return key == inf->key;
		});

		if (itr == timer_list.end()) return;

		auto inf = *itr;
		std::unique_lock lock(inf->mutex);
		inf->status = timer_info::stopped;
		inf->cond.notify_one();
	}

	template <typename Function>
	uint on(uint event_id, Function lambda) {
		auto function = new decltype(to_function(lambda))(to_function(lambda));
		event_info cb;
		cb.key = unique_id();
		cb.function = static_cast<void*>(function);
		cb.signature = &typeid(function);
		cb.self_only = false;

		{
			std::lock_guard lock(event_map_mutex);
			event_map.emplace(event_id, cb);
		}
		return cb.key;
	}

	template <typename Function>
	uint on_self(uint event_id, Function lambda) {
		auto function = new decltype(to_function(lambda))(to_function(lambda));
		event_info cb;
		cb.key = unique_id();
		cb.function = static_cast<void*>(function);
		cb.signature = &typeid(function);
		cb.self_only = true;

		{
			std::lock_guard lock(event_map_mutex);
			event_map.emplace(event_id, cb);
		}
		return cb.key;
	}

	void unbind(uint key) {
		std::lock_guard lock(event_map_mutex);

		auto itr = std::find_if(event_map.begin(), event_map.end(), [key](auto e) {
			return e.second.key == key;
		});
			
		if (itr != event_map.end()) event_map.erase(itr);
	}

	template <typename ...Args>
	void fire(uint event_id, Args... args) {
		event ev;
		ev.event_id = event_id;
		ev.target = std::dynamic_pointer_cast<entity>(shared_from_this());
		fire(ev, args...);
	}

protected:
	template <typename ...Args>
	void fire(event& ev, Args... args) {
		auto range = event_map.equal_range(ev.event_id);
		for (auto itr = range.first; itr != range.second; itr++) {
			auto cb = itr->second;
			if (cb.self_only && !is_same(ev.target)) continue;

			auto function = static_cast<std::function<void(event&, Args...)>*>(cb.function);

			if (typeid(function) != *(cb.signature))
				throw std::bad_typeid();

			(*function)(ev, args...);
		}

		auto parentsp = parent.lock();
		if (parentsp) parentsp->fire(ev, args...);
	}

public:
	virtual void init() {
		fire(event_init);
	}

	virtual void finalize() {
		fire(event_finalize);
	}

	virtual void start();

	virtual void stop() {
		started = false;
		fire(event_stop);
	}

	bool is_started() const {
		return started;
	}
};





// base class for comm, power, sensor, battery, controller
class node_component : public entity {
protected:
	std::weak_ptr<basic_node> node;

public:
	void set_node(std::shared_ptr<basic_node> _node) {
		node = _node;
		set_parent(std::dynamic_pointer_cast<entity>(_node));
	}

	std::shared_ptr<basic_node> get_node() const {
		auto sp = node.lock();
		assert(sp);
		return sp;
	}

	template <typename node_type>
	std::shared_ptr<node_type> get_node() const {
		auto sp = node.lock();
		assert(sp);

		auto sp2 = std::dynamic_pointer_cast<node_type>(sp);
		assert(sp2);
		return sp2;
	}

	std::shared_ptr<basic_network> get_network() const;
	std::shared_ptr<basic_world> get_world() override;
	std::shared_ptr<const basic_world> get_world() const override;

	friend class basic_node;
	friend class basic_network;
};






class basic_comm : public node_component {
protected:

	static auto make_shared_data(const std::vector<uchar>& data) {
		return std::make_shared<std::vector<uchar>>(data);
	}

	template <typename info_type>
	static void add_header(std::vector<uchar>& data, const info_type& info) {
		data.insert(data.begin(), (const uchar*)&info, (const uchar*)&info + sizeof(info));
	}

	template <typename info_type>
	static std::vector<uchar> add_header(const std::vector<uchar>& data, const info_type& info) {
		std::vector<uchar> newdata(sizeof(info));
		std::memcpy(newdata.data(), (const uchar*)&info, sizeof(info));
		newdata.insert(newdata.end(), data.begin(), data.end());
		return std::move(newdata);
	}

	static void remove_header(std::vector<uchar>& data, uint len) {
		std::vector<uchar>(data.begin() + len, data.end()).swap(data);
	}
	
	static std::vector<uchar> remove_header(const std::vector<uchar>& data, uint len) {
		return std::vector<uchar>(data.begin() + len, data.end());
	}

	template <typename info_type>
	static void extract_header(const std::vector<uchar>& data, info_type& info) {
		std::memcpy((uchar*)&info, data.data(), sizeof(info));
	}

	template <typename info_type>
	static const info_type& extract_header(const std::vector<uchar>& data) {
		return *(const info_type*)data.data();
	}

	template <typename info_type>
	static info_type& extract_header(std::vector<uchar>& data) {
		return *(info_type*)data.data();
	}

public:
	static inline const uint event_send = unique_id();
	static inline const uint event_receive = unique_id();
	static inline const uint event_drop = unique_id();
	static inline const uint event_forward = unique_id();

	virtual void receive(std::shared_ptr<std::vector<uchar>> data, std::shared_ptr<basic_node> from) {
		fire(event_receive, data, from);
	}

	virtual void send(const std::vector<uchar>& data, std::shared_ptr<basic_node> to);

	virtual void broadcast(const std::vector<uchar>& data, std::function<bool(std::shared_ptr<basic_node>)> condition);
	virtual void broadcast_by_distance(const std::vector<uchar>& data, double range);

	virtual void route(const std::vector<uchar>& data, std::shared_ptr<basic_node> to) {
		// to be optionally implemented by derived classes
		assert(false);
	}
};






class basic_sensor : public node_component {
protected:
	std::any last_value;
	double last_time;

	virtual void compute_value(std::any& value) const = 0;

public:
	static inline const uint event_measure = unique_id();


	basic_sensor() {
		last_time = -1.;
	}

	void measure();

	bool get_last_value(std::any& value, double& time) const {
		if (last_time < 0.) return false;

		value = last_value;
		time = last_time;
		return true;
	}
};






class basic_battery : public node_component {
public:
	static inline const uint event_charge = unique_id();
	static inline const uint event_consume = unique_id();
	static inline const uint event_full = unique_id();
	static inline const uint event_empty = unique_id();


	basic_battery() {
	}

	virtual double get_soc() const {
		return get_capacity() / get_max_capacity();
	}

	virtual bool charge(double T) = 0;
	virtual bool consume(double T) = 0;

	virtual double get_norminal_voltage() const = 0;	// V
	virtual double get_capacity() const = 0;			// current capacity in Ah
	virtual double get_max_capacity() const = 0;		// max capacity in Ah
};






class basic_power : public node_component {
public:
	virtual double get_max_power() const = 0;
};







class basic_controller : public node_component {
};







class basic_node : public entity {
protected:
	std::weak_ptr<basic_network> network;
	std::string name;
	location loc;

	friend class basic_network;
	friend class basic_controller;

public:
	basic_node(const std::string _name, const location& _loc)
		: name(_name), loc(_loc)
	{
	}


	virtual std::shared_ptr<basic_comm> get_comm() const = 0;
	virtual std::shared_ptr<basic_battery> get_battery() const = 0;
	virtual std::shared_ptr<basic_power> get_power() const = 0;
	virtual std::shared_ptr<basic_sensor> get_sensor() const = 0;
	virtual std::shared_ptr<basic_controller> get_controller() const = 0;

	std::shared_ptr<basic_network> get_network() const {
		auto sp = network.lock();
		assert(sp);
		return sp;
	}

	std::shared_ptr<basic_world> get_world() override;
	std::shared_ptr<const basic_world> get_world() const override;

	const std::string& get_name() const { return name; }
	void set_name(const std::string& _name) { name = _name; }
	const location& get_location() const { return loc; }
	void set_location(const location& _loc) { loc = _loc; }

	void each_component(std::function<void(std::shared_ptr<node_component>)> callback) {
		callback(std::dynamic_pointer_cast<node_component>(get_battery()));
		callback(std::dynamic_pointer_cast<node_component>(get_power()));
		callback(std::dynamic_pointer_cast<node_component>(get_sensor()));
		callback(std::dynamic_pointer_cast<node_component>(get_comm()));
		callback(std::dynamic_pointer_cast<node_component>(get_controller()));
	}
};


template <
	typename comm_type,
	typename sensor_type,
	typename battery_type,
	typename power_type,
	typename controller_type
>
class generic_node : public basic_node {
protected:
	std::shared_ptr<comm_type> comm;
	std::shared_ptr<sensor_type> sensor;
	std::shared_ptr<battery_type> battery;
	std::shared_ptr<power_type> power;
	std::shared_ptr<controller_type> controller;

	friend class basic_network;

public:
	generic_node(const std::string _name, const location& _loc)
		: basic_node(_name, _loc),
		comm(std::make_shared<comm_type>()),
		sensor(std::make_shared<sensor_type>()),
		battery(std::make_shared<battery_type>()),
		power(std::make_shared<power_type>()),
		controller(std::make_shared<controller_type>())
	{
	}

	std::shared_ptr<comm_type> get_comm_t() const { return comm; }
	std::shared_ptr<battery_type> get_battery_t() const { return battery; }
	std::shared_ptr<power_type> get_power_t() const { return power; }
	std::shared_ptr<sensor_type> get_sensor_t() const { return sensor; }
	std::shared_ptr<controller_type> get_controller_t() const { return controller; }

	std::shared_ptr<basic_comm> get_comm() const override {
		return std::dynamic_pointer_cast<basic_comm>(comm);
	}

	std::shared_ptr<basic_battery> get_battery() const override {
		return std::dynamic_pointer_cast<basic_battery>(battery);
	}

	std::shared_ptr<basic_power> get_power() const override {
		return std::dynamic_pointer_cast<basic_power>(power);
	}

	std::shared_ptr<basic_sensor> get_sensor() const override {
		return std::dynamic_pointer_cast<basic_sensor>(sensor);
	}

	std::shared_ptr<basic_controller> get_controller() const override {
		return std::dynamic_pointer_cast<basic_controller>(controller);
	}

	void init() override {
		basic_node::init();
		each_component([](auto c) {
			c->init();
		});
	}

	void finalize() override {
		each_component([](auto c) {
			c->finalize();
		});
		basic_node::finalize();
	}

	void start() override {
		basic_node::start();
		each_component([](auto c) {
			c->start();
		});
	}

	void stop() override {
		each_component([](auto c) {
			c->stop();
		});
		basic_node::stop();
	}
};



class basic_network : public entity {
protected:
	std::weak_ptr<basic_world> world;
	std::list<std::shared_ptr<basic_node>> nodes;

	bool started = false;

	template <typename network_type> friend class generic_world;

public:
	static inline const uint event_starting = unique_id();
	static inline const uint event_started = unique_id();
	static inline const uint event_stopping = unique_id();
	static inline const uint event_stopped = unique_id();

	template <class node_type, class... Targs>
	std::shared_ptr<node_type> new_node(Targs... args) {
		std::shared_ptr<node_type> node(make_shared<node_type>(std::forward<Targs>(args)...));
		node->network = std::dynamic_pointer_cast<basic_network>(shared_from_this());
		node->id = unique_id();

		node->each_component([node](auto c) {
			c->set_node(node);
		});
		set_parent_for(node);
		nodes.push_back(node);

		node->init();
		if (started) node->start();

		return node;
	}

	std::shared_ptr<basic_node> node_by_id(uint id) const {
		auto itr = std::find_if(nodes.begin(), nodes.end(), [id](auto e) {
			return e->get_id() == id;
		});

		return (itr == nodes.end()) ? nullptr : *itr;
	}

	std::shared_ptr<basic_node> node_by_name(const std::string& name) const {
		auto itr = std::find_if(nodes.begin(), nodes.end(), [&name](auto e) {
			return e->get_name() == name;
		});

		return (itr == nodes.end()) ? nullptr : *itr;
	}

	const std::list<std::shared_ptr<basic_node>> get_nodes() const {
		return nodes;
	}

	std::list<std::shared_ptr<basic_node>> find_nodes(std::function<bool(std::shared_ptr<basic_node>)> condition) const {
		std::list<std::shared_ptr<basic_node>> list;
		for (auto& n : nodes) {
			if (condition(n)) list.push_back(n);
		}

		return std::move(list);
	}

	std::list<std::shared_ptr<basic_node>> find_nodes_in_range(const location& center, double range) const {
		return find_nodes([&center, range](auto node) {
			return node->get_location().distance_to(center) <= range;
		});
	}

	std::shared_ptr<basic_node> find_best_node(std::function<double(std::shared_ptr<basic_node>)> evaluator) const {
		std::shared_ptr<basic_node> node;
		double v, best = std::numeric_limits<double>::min();

		for (auto& n : nodes) {
			v = evaluator(n);
			if (v > best) {
				best = v;
				node = n;
			}
		}

		return node;
	}

	std::shared_ptr<basic_node> find_worst_node(std::function<double(std::shared_ptr<basic_node>)> evaluator) const {
		std::shared_ptr<basic_node> node;
		double v, worst = std::numeric_limits<double>::max();

		for (auto& n : nodes) {
			v = evaluator(n);
			if (v < worst) {
				worst = v;
				node = n;
			}
		}

		return node;
	}

	std::shared_ptr<basic_node> find_closest_node(const location& loc) const {
		return find_worst_node([&loc](auto node) {
			return node->get_location().distance_to(loc);
		});
	}

	void remove_node(uint id) {
		auto itr = std::find_if(nodes.begin(), nodes.end(), [id](auto e) {
			return e->get_id() == id;
		});

		if (itr != nodes.end()) {
			auto n = *itr;
			if (n->is_started()) n->stop();
			n->finalize();
			nodes.erase(itr);
		}
	}
	
	void each_node(std::function<void(std::shared_ptr<basic_node>)> callback) {
		for (auto node : nodes) {
			callback(node);
		}
	}

	template <typename ExecutionPolicy>
	void each_node(ExecutionPolicy policy, std::function<void(std::shared_ptr<basic_node>)> callback) {
		std::for_each(policy, nodes.begin(), nodes.end(), callback);
	}

	std::shared_ptr<basic_world> get_world() override {
		auto sp = world.lock();
		assert(sp);
		return sp;
	}

	std::shared_ptr<const basic_world> get_world() const override {
		auto sp = world.lock();
		assert(sp);
		return sp;
	}

	void start() override;
	void stop() override;
};







class basic_ambient : public entity {
protected:
	std::weak_ptr<basic_world> world;

	template <typename network_type> friend class generic_world;

public:
	static inline const uint temperature = unique_id();
	static inline const uint humidity = unique_id();
	static inline const uint light = unique_id();

	std::shared_ptr<basic_world> get_world() override {
		auto sp = world.lock();
		assert(sp);
		return sp;
	}

	std::shared_ptr<const basic_world> get_world() const override {
		auto sp = world.lock();
		assert(sp);
		return sp;
	}

	virtual void get_value(const location& loc, std::any& value) = 0;
};









class basic_world : public entity {
protected:
	reference_frame ref_frame;
	clock ref_clock;

public:
	reference_frame& get_reference_frame() {
		return ref_frame;
	}

	const reference_frame& get_reference_frame() const {
		return ref_frame;
	}

	clock& get_clock() {
		return ref_clock;
	}

	const clock& get_clock() const {
		return ref_clock;
	}

	std::shared_ptr<basic_world> get_world() override {
		return std::dynamic_pointer_cast<basic_world>(shared_from_this());
	}

	std::shared_ptr<const basic_world> get_world() const override {
		return std::dynamic_pointer_cast<const basic_world>(shared_from_this());
	}

	void start() override {
		ref_clock.start();
		entity::start();
	}

	void stop() override {
		entity::stop();
		ref_clock.stop();
	}

	virtual std::shared_ptr<basic_network> get_network() const = 0;
};



template <
	typename network_type
>
class generic_world : public basic_world {
protected:
	std::shared_ptr<network_type> network;
	std::map<uint, std::shared_ptr<basic_ambient>> ambients;

	generic_world()
		: network(make_shared<network_type>())
	{}

public:
	static std::shared_ptr<generic_world<network_type>> new_world() {
		std::shared_ptr<generic_world<network_type>> w(new generic_world<network_type>());
		w->network->world = w;
		w->network->set_parent(w);
		w->init();
		w->network->init();
		return w;
	}


	template <class ambient_type, class... Targs>
	std::shared_ptr<ambient_type> new_ambient(uint type, Targs... args) {
		// make sure only 1 ambient is created for a type
		if (ambients.find(type) != ambients.end()) return nullptr;

		std::shared_ptr<ambient_type> ambient(std::make_shared<ambient_type>(std::forward<Targs>(args)...));
		ambient->world = std::dynamic_pointer_cast<generic_world<network_type>>(shared_from_this());
		set_parent_for(ambient);

		ambients[type] = ambient;

		ambient->init();
		if (is_started()) ambient->start();

		return ambient;
	}

	std::shared_ptr<basic_ambient> ambient_by_type(uint type) {
		if (ambients.find(type) == ambients.end()) return nullptr;
		return ambients.at(type);
	}

	bool remove_ambient(uint type) {
		auto itr = ambients.find(type);
		if (itr == ambients.end()) return false;

		auto a = itr->second;
		if (a->is_started()) a->stop();
		a->finalize();

		ambients.erase(type);
		return true;
	}


	std::shared_ptr<network_type> get_network() const override {
		return network;
	}

	void start() override {
		basic_world::start();
		network->start();
		for (auto& a : ambients) {
			a.second->start();
		}
	}

	void stop() override {
		network->stop();
		for (auto& a : ambients) {
			a.second->stop();
		}
		basic_world::stop();
	}
};



}



#include "meteor.h"
#include "noise.h"

#include "battery.h"
#include "sensor.h"
#include "ambient.h"
#include "power.h"
#include "comm.h"
