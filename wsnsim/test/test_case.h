#pragma once


#include <iostream>
#include "../core/wsnsim.h"


class test_case {
public:
	std::shared_ptr<wsn::generic_world<wsn::basic_network>> world;

	int trial = 1;
	bool logging = true;
	std::ofstream logger;

	inline static std::shared_ptr<test_case> instant;


	void init() {
		if (logging) init_log_file();
	}

	void init_log_file() {
		if (!logging) return;

		if (logger.is_open()) logger.close();

		logger.open(kutils::formatstr("wsnsim-log-%s-trial-%d.txt", get_test_name().c_str(), trial), std::ios::out);
		if (!logger) std::cout << "Log file creation failed" << std::endl;
		else logging = true;
	}

	virtual std::string get_test_name() const {
		return "test";
	}

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

