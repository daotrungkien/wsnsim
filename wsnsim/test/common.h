#pragma once


#include <iostream>
#include "../lib/zmq.hpp"
#include "../core/wsnsim.h"

struct config {
	inline static const char* remote_logger_connection = "tcp://localhost:2000";
};


class logging {
protected:
	const bool to_console = true;
	const bool to_file = true;
	const bool to_zmq = true;

	std::ofstream file;

	zmq::context_t ctx;
	zmq::socket_t sock;

	void init_log_file();
	void init_zmq();

public:
	logging()
		: sock(ctx, zmq::socket_type::pub)
	{
	}

	void init() {
		init_log_file();
		init_zmq();
	}

	void reset() {
		init_log_file();
	}

	void log(std::string msg, bool check_clock = true);
};

extern logging logger;




class test_case {
public:
	struct command_item {
		std::string cmd;
		std::function<void(const std::string&, const std::vector<std::string>&)> action;
	};

	std::shared_ptr<wsn::generic_world<wsn::basic_network>> world;

	int trial = 1;

	std::list<command_item> commands;

	inline static std::shared_ptr<test_case> instance;


	void init() {
		logger.init();

		add_command("info", [this]() {
			auto& clock = world->get_clock();
			double time = clock.clock_now();

			std::cout << "Test case: " << get_test_name() << std::endl
				<< "Simulation global clock time: " << wsn::format_time(time) << std::endl
				<< "Simulation reference time: " << wsn::format_time(clock.clock2reference(time)) << std::endl
				<< "System time: " << wsn::format_time(clock.clock2system(time)) << std::endl
				<< "Commands:" << std::endl;

			for (auto& cmd : commands) {
				std::cout << "- " << cmd.cmd << std::endl;
			}
		});

		add_command("start", [this]() {
			world->start();
		});

		add_command("stop", [this] {
			world->stop();
		});

		add_command("clear-log", [this] {
			logger.reset();
		});

		add_command("new-trial", [this] {
			trial++;
			logger.reset();
		});

		add_command("node-info", [this] (auto& cmd, auto& args) {
			node_info(world, args);
		});
	}

	void add_command(const std::string& cmd, std::function<void(const std::string&, const std::vector<std::string>&)> action) {
		commands.push_back({ cmd, action });
	}

	void add_command(const std::string& cmd, std::function<void()> action) {
		add_command(cmd, [action](const std::string&, const std::vector<std::string>&) {
			action();
		});
	}

	bool process_command(const std::string& cmd, const std::vector<std::string>& args) {
		auto itr = std::find_if(commands.begin(), commands.end(), [&cmd](auto& e) {
			return e.cmd == cmd;
		});
		if (itr == commands.end()) return false;

		itr->action(cmd, args);
		return true;
	}


	void node_info(std::shared_ptr<wsn::basic_world> w, const std::vector<std::string>& args) {
		if (args.size() < 1 || args.size() > 2) {
			std::cout << "node-info all|node-name [>file]" << std::endl;
			return;
		}

		bool to_file = false;
		std::ofstream file;
		if (args.size() == 2) {
			if (args[1] != ">file") {
				std::cout << "Last argument must be '>file'" << std::endl;
				return;
			}

			file.open(kutils::formatstr("wsnsim-log-%s-nodes.txt", get_test_name().c_str()), std::ios::out);
			if (!file) {
				std::cout << "Output file creation failed" << std::endl;
				return;
			}

			to_file = true;
		}

		if (args[0] == "all") {
			w->get_network()->each_node([this, to_file, &file](auto node) {
				print_node_info(node, to_file ? file : std::cout);
			});
		}
		else {
			auto node = w->get_network()->node_by_name(args[0]);
			if (!node) {
				std::cout << "Node not found: " << args[0] << std::endl;
				return;
			}

			print_node_info(node, to_file ? file : std::cout);
		}
	}


	virtual std::string get_test_name() const = 0;
	virtual std::string get_test_description() const = 0;

	virtual void print_node_info(std::shared_ptr<wsn::basic_node> node, std::ostream& stream) {
		auto loc = node->get_location();
		stream << node->get_id() << "\t" << node->get_name() << "\t" << loc.x << "\t" << loc.y << "\t" << loc.z
			<< "\t" << (node->is_started() ? "started" : "stopped") << std::endl;
	}

	virtual bool custom_command(const std::string& cmd, const std::vector<std::string>& args) {
		return false;
	}

	virtual void setup() = 0;
};



template <typename T>
std::basic_string<T> format_binary_string(const T* buffer, int length)
{
	std::basic_ostringstream<T> ss;
	ss << std::hex << std::setfill(T('0'));

	T c;
	for (int i = 0; i < length; i++) {
		c = buffer[i];
		if ((c >= 32 && c <= 126) || (c >= 128 && c <= 254)) ss << c;
		else ss << '[' << std::setw(2) << ((unsigned int)(unsigned char)c) << ']';
	}

	return ss.str();
}

template <typename T>
std::basic_string<T> format_binary_string(const std::basic_string<T>& buffer)
{
	return format_binary_string(buffer.data(), buffer.length());
}

template <typename T>
std::basic_string<T> format_binary_string(const std::vector<T>& buffer)
{
	std::basic_string<T> s(buffer.begin(), buffer.end());
	return format_binary_string(s);
}

inline std::string format_binary_string(const std::vector<unsigned char>& buffer)
{
	std::string s(buffer.begin(), buffer.end());
	return format_binary_string(s);
}
