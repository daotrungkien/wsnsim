#include "test_case.h"
#include "test1.h"
#include "test2.h"
#include "test3.h"


using namespace std;
using namespace wsn;


namespace test = test3;




bool parse_cmd(const string& line, string& cmd, vector<string>& args)
{
	auto a = kutils::split(line);
	if (a.size() == 0) return false;

	cmd = a[0];

	args.clear();
	if (a.size() > 1) args.insert(args.end(), std::make_move_iterator(a.begin() + 1), std::make_move_iterator(a.end()));

	return true;
}


void node_info(shared_ptr<basic_world> w, const vector<string>& args)
{
	if (args.size() < 1 || args.size() > 2) {
		cout << "node-info all|node-name [>file]" << endl;
		return;
	}

	bool to_file = false;
	ofstream file;
	if (args.size() == 2) {
		if (args[1] != ">file") {
			cout << "Last argument must be '>file'" << endl;
			return;
		}

		file.open(kutils::formatstr("wsnsim-log-%s-nodes.txt", test_case::instant->get_test_name().c_str()), ios::out);
		if (!file) {
			cout << "Output file creation failed" << endl;
			return;
		}
		
		to_file = true;
	}

	if (args[0] == "all") {
		w->get_network()->each_node([to_file, &file](auto node) {
			test_case::instant->print_node_info(node, to_file ? file : cout);
		});
	}
	else {
		auto node = w->get_network()->node_by_name(args[0]);
		if (!node) {
			cout << "Node not found: " << args[0] << endl;
			return;
		}

		test_case::instant->print_node_info(node, to_file ? file : cout);
	}
}


int _tmain()
{
	test::custom_test_case::create_test_case();

	auto& tc = test_case::instant;
	tc->setup();

	tc->world->start();

	string input;
	while (true) {
		cout << ">> ";
		getline(cin, input);

		for (auto& line : kutils::split(input, ";")) {
			std::string cmd;
			std::vector<std::string> args;
			if (!parse_cmd(line, cmd, args)) continue;

			if (cmd == "exit") goto lb_end;
			else if (cmd == "stop") tc->world->stop();
			else if (cmd == "start") tc->world->start();
			else if (cmd == "clear-log") tc->init_log_file();
			else if (cmd == "new-trial") {
				tc->trial++;
				tc->init_log_file();
			}
			else if (cmd == "node-info") node_info(tc->world, args);
			else if (!tc->custom_command(cmd, args))
				cout << "Command error" << endl;
		}
	}

lb_end:

	if (tc->world->is_started())
		tc->world->stop();

	return 0;
}