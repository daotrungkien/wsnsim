#include "wsnsim.h"

namespace wsn {


static uint gen_event_id() {
	static uint id = 1;
	return id++;
}

const uint basic_battery::event_charge = gen_event_id();
const uint basic_battery::event_discharge = gen_event_id();
const uint basic_battery::event_full = gen_event_id();
const uint basic_battery::event_empty = gen_event_id();
const uint basic_node::event_starting = gen_event_id();
const uint basic_node::event_started = gen_event_id();
const uint basic_node::event_stopping = gen_event_id();
const uint basic_node::event_stopped = gen_event_id();
const uint network::event_starting = gen_event_id();
const uint network::event_started = gen_event_id();
const uint network::event_stopping = gen_event_id();
const uint network::event_stopped = gen_event_id();




tstring format_time(double t, int prec)
{
	int h, m, s, f;
	h = (int)(t / 3600);
	t -= h * 3600;
	m = (int)(t / 60);
	t -= m * 60;
	s = (int)t;
	f = (int)((t - s)*std::pow(10, prec));

	tstring fmtstr = formatstr<TCHAR>(_T("%%02d:%%02d:%%02d.%%0%dd"), prec);
	return formatstr<TCHAR>(fmtstr.c_str(), h, m, s, f);
}






void min_max_battery::charge(double d)
{
	if (level >= maxl) return;

	level += d;
	if (level > maxl) level = maxl;

	node->on(event_charge);
	if (level >= maxl) node->on(event_full);
}

void min_max_battery::discharge(double d)
{
	if (level <= minl) return;

	level -= d;
	if (level < minl) level = minl;

	node->on(event_discharge);
	if (level <= minl) node->on(event_empty);
}






void basic_node::on(uint event)
{
	wsn->on(event, this);
}



}