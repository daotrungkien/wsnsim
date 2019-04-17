#include "test1.h"


using namespace std;
using namespace wsn;





bool parse_cmd(const string& line, string& cmd, vector<string>& args)
{
	auto a = kutils::split(line);
	if (a.size() == 0) return false;

	cmd = a[0];

	args.clear();
	if (a.size() > 1) args.insert(args.end(), std::make_move_iterator(a.begin() + 1), std::make_move_iterator(a.end()));

	return true;
}





int _tmain()
{
	custom_test_case::create_test_case();

	auto& tc = test_case::instance;

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
			
			if (!tc->process_command(cmd, args))
				cout << "Command error" << endl;
		}
	}

lb_end:

	if (tc->world->is_started())
		tc->world->stop();

	return 0;
}