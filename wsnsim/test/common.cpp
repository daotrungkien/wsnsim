#include "common.h"


logging logger;



void logging::init_log_file()
{
	if (!to_file) return;

	if (file.is_open()) file.close();

	auto tc = test_case::instance;
	auto filename = kutils::formatstr("wsnsim-log-%s-trial-%d.txt", tc->get_test_name().c_str(), tc->trial); // kutils::format_time(time(nullptr), "%Y%m%d%H%M").c_str());
	file.open(filename, std::ios::out);

	if (!file) std::cout << "Log file creation failed: " << filename << std::endl;
}


void logging::init_zmq()
{
	if (!to_zmq) return;

	try {
		sock.connect(config::remote_logger_connection);
	}
	catch (zmq::error_t err) {
		std::cout << "Remote logger connection to " << config::remote_logger_connection << " failed: " << err.what() << std::endl;
	}
}


void logging::log(std::string msg, bool check_clock)
{
	auto tc = test_case::instance;
	auto& clock = tc->world->get_clock(); 
	if (check_clock && !clock.is_started()) return;

	auto s = kutils::formatstr("%.3f\t%s", clock.is_started() ? clock.clock_now() : -1., msg.c_str());

	if (to_console) std::cout << s << std::endl;

	if (to_file && file) file << s << std::endl;

	if (to_zmq && sock.connected()) {
		sock.send(zmq::message_t("log", 3), zmq::send_flags::sndmore);
		sock.send(zmq::message_t(s.c_str(), s.length()), zmq::send_flags::none);
	}
}
