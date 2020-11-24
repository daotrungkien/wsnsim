#include "wsnsim.h"


namespace wsn {




	location operator *(double d, const location& l)
	{
		return location(d*l.x, d*l.y, d*l.z);
	}

	std::string format_time(double t, int prec)
	{
		int h, m, s, f;
		h = (int)(t / 3600);
		t -= h * 3600;
		m = (int)(t / 60);
		t -= m * 60;
		s = (int)t;
		f = (int)((t - s)*std::pow(10, prec));

		std::string fmtstr = kutils::formatstr<char>("%%02d:%%02d:%%02d.%%0%dd", prec);
		return kutils::formatstr<char>(fmtstr.c_str(), h, m, s, f);
	}

	std::string format_time(std::chrono::system_clock::time_point t, const char* format, int prec)
	{
		auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(t.time_since_epoch()).count() % (60 * 1000);
		float secs = millis / 1000.f;

		std::string fmt1 = kutils::formatstr("%%02.0%df", prec);
		std::string ssecs = kutils::formatstr(fmt1.c_str(), secs);
		std::string nf = kutils::replace_all(std::string(format), std::string("%S"), ssecs);
		return kutils::format_time(std::chrono::system_clock::to_time_t(t), nf.c_str());
	}




	/*
	global = invmat * local + org
	local = mat * (global - org)
	*/
	void reference_frame::set_frame(double org_lat, double org_lon, double org_alt,
		double ext1_lat, double ext1_lon, double ext1_alt,
		double ext2_lat, double ext2_lon, double ext2_alt)
	{
		orglla[0] = org_lat;
		orglla[1] = org_lon;
		orglla[2] = org_alt;

		vector3 ext1, ext2;
		lla2global(org_lat, org_lon, org_alt, org.x(), org.y(), org.z());
		lla2global(ext1_lat, ext1_lon, ext1_alt, ext1.x(), ext1.y(), ext1.z());
		lla2global(ext2_lat, ext2_lon, ext2_alt, ext2.x(), ext2.y(), ext2.z());

		vector3 ix(ext1 - org), iy(ext2 - org), iz;
		ix.normalize();
		iy.normalize();
		iz = ix.cross(iy);

		for (int i = 0; i < 3; i++) {
			invmat(0, i) = ix[i];
			invmat(1, i) = iy[i];
			invmat(2, i) = iz[i];
		}
		mat = invmat.inverse();
	}


	void reference_frame::set_frame(double org_lat, double org_lon, double org_alt,
		double ext_lat, double ext_lon, double ext_alt)
	{
		orglla[0] = org_lat;
		orglla[1] = org_lon;
		orglla[2] = org_alt;

		vector3 ext;
		lla2global(org_lat, org_lon, org_alt, org.x(), org.y(), org.z());
		lla2global(ext_lat, ext_lon, ext_alt, ext.x(), ext.y(), ext.z());

		vector3 ph;
		lla2global(org_lat, org_lon, ext_alt + 1., ph.x(), ph.y(), ph.z());

		vector3 ix(ext - org), iy, iz(ph - org);
		ix.normalize();
		iz.normalize();
		iy = iz.cross(ix);

		for (int i = 0; i < 3; i++) {
			invmat(0, i) = ix[i];
			invmat(1, i) = iy[i];
			invmat(2, i) = iz[i];
		}
		mat = invmat.inverse();
	}

	vector3 reference_frame::zenith_dir() const
	{
		vector3 dir;
		lla2local(orglla[0], orglla[1], orglla[2] + 1e-5, dir.x(), dir.y(), dir.z());
		dir.normalize();
		return std::move(dir);
	}

	vector3 reference_frame::north_dir() const
	{
		vector3 dir;
		lla2local(orglla[0] + 1e-5, orglla[1], orglla[2], dir.x(), dir.y(), dir.z());
		dir.normalize();
		return std::move(dir);
	}

	void reference_frame::local2global(double xl, double yl, double zl, double& xg, double& yg, double& zg) const
	{
		vector3 v = invmat * vector3(xl, yl, zl) + org;
		xg = v.x();
		yg = v.y();
		zg = v.z();
	}

	void reference_frame::local2lla(double xl, double yl, double zl, double& lat, double& lon, double& alt) const
	{
		double xg, yg, zg;
		local2global(xl, yl, zl, xg, yg, zg);
		global2lla(xg, yg, zg, lat, lon, alt);
	}

	void reference_frame::global2local(double xg, double yg, double zg, double& xl, double& yl, double& zl) const
	{
		vector3 v = mat * (vector3(xg, yg, zg) - org);
		xl = v.x();
		yl = v.y();
		zl = v.z();
	}

	void reference_frame::lla2local(double lat, double lon, double alt, double& xl, double& yl, double& zl) const
	{
		double xg, yg, zg;
		lla2global(lat, lon, alt, xg, yg, zg);
		global2local(xg, yg, zg, xl, yl, zl);
	}



	void reference_frame::lla2global(double lat, double lon, double alt, double& xg, double& yg, double& zg)
	{
#define _a_ 6378137.0
#define _e2_ 0.006694379990141

		lat *= M_PI / 180.;
		lon *= M_PI / 180.;

		double sphi = sin(lat);
		double cphi = cos(lat);
		double N = _a_ / sqrt(1 - _e2_ * sphi*sphi);

		xg = (N + alt)*cphi*cos(lon);
		yg = (N + alt)*cphi*sin(lon);
		zg = (N*(1 - _e2_) + alt)*sphi;

		/*
		#define _Re_ 6378137.0
		#define _Rp_ 6356752.31424518

		lat *= M_PI/180.;
		lon *= M_PI/180.;

		double coslat = cos(lat),
		sinlat = sin(lat),
		coslon = cos(lon),
		sinlon = sin(lon);

		double term1 = sqrt(_Re_*_Re_*coslat*coslat + _Rp_*_Rp_*sinlat*sinlat);
		double term2 = (alt + _Re_*_Re_)*coslat/term1;

		xg = coslon*term2;
		yg = sinlon*term2;
		zg = (alt + _Rp_*_Rp_)*sinlat/term1; */
	}


	void reference_frame::global2lla(double xg, double yg, double zg, double& lat, double& lon, double& alt)
	{
		// http://www.mathworks.com/matlabcentral/fileexchange/7941
		// WGS84 ellipsoid constants:
#define _a_ 6378137.0
#define _e_ 8.1819190842622e-2

		double b = sqrt(_a_*_a_*(1 - _e_ * _e_));
		double ab = _a_ / b;
		double ep = sqrt(ab*ab - 1);
		double p = hypot(xg, yg);
		double th = atan2(_a_*zg, b*p);
		double sth = sin(th);
		double cth = cos(th);
		lon = atan2(yg, xg);
		lat = atan2((zg + ep * ep*b*sth*sth*sth), (p - _e_ * _e_*_a_*cth*cth*cth));

		double slat = sin(lat);
		double N = _a_ / sqrt(1 - _e_ * _e_*slat*slat);
		alt = p / cos(lat) - N;

		// return lon in range [0,2*pi)
		lon = fmod(lon, 2.*M_PI);

		lat *= 180. / M_PI;
		lon *= 180. / M_PI;

		// correct for numerical instability in altitude near exact poles:
		// (after this correction, error is about 2 millimeters, which is about
		// the same as the numerical precision of the overall function)
		bool k = fabs(xg) < 1. && fabs(yg) < 1.;
		if (k) alt = fabs(zg) - b;
	}







	void entity::start()
	{
		if (started) return;

		if (first_start) {
			first_start = false;
			fire(event_first_start);
		}

		started = true;
		start_time = get_world_clock_time();
		fire(event_start);
	}

	double entity::get_local_clock_time() const
	{
		if (!started) throw std::logic_error("clock not started");
		return get_world()->get_clock().clock_now() - start_time;
	}

	double entity::get_world_clock_time() const
	{
		return get_world()->get_clock().clock_now();
	}

	std::chrono::system_clock::time_point entity::get_reference_time() const
	{
		return get_world()->get_clock().reference_now();
	}

	std::chrono::system_clock::time_point entity::get_system_time() const
	{
		return get_world()->get_clock().system_now();
	}







	void basic_comm::send(const std::vector<uchar>& data, std::shared_ptr<basic_node> to)
	{
		auto rdata = make_shared_data(data);

		fire(event_send, rdata, to);

		std::thread([rdata, this, to] {
			to->get_comm()->receive(rdata, get_node());
		}).detach();
	}

	void basic_comm::broadcast(const std::vector<uchar>& data, std::function<bool(std::shared_ptr<basic_node>)> condition,
		std::function<void(std::shared_ptr<basic_node>)> sender)
	{
		auto network = get_network();
		auto nodes = network->find_nodes(condition);
		std::for_each(std::execution::par, nodes.begin(), nodes.end(), [&data, this, sender](auto& node) {
			if (!is_same(node->get_comm())) {
				if (sender == nullptr)
					send(data, node);
				else sender(node);
			}
		});
	}

	void basic_comm::broadcast_by_distance(const std::vector<uchar>& data, double range,
		std::function<void(std::shared_ptr<basic_node>)> sender)
	{
		auto& loc = get_node()->get_location();

		broadcast(data, [&loc, range](auto node) {
			return node->get_location().distance_to(loc) <= range;
		}, sender);
	}







	std::shared_ptr<basic_network> node_component::get_network() const
	{
		return get_node()->get_network();
	}

	std::shared_ptr<basic_world> node_component::get_world()
	{
		return get_node()->get_world();
	}

	std::shared_ptr<const basic_world> node_component::get_world() const
	{
		return get_node()->get_world();
	}






	void basic_sensor::measure()
	{
		auto node = get_node();
		auto network = node->get_network();

		compute_value(last_value);
		last_time = get_world_clock_time();
		fire(event_measure, last_value, last_time);
	}






	std::shared_ptr<basic_world> basic_node::get_world()
	{
		return get_network()->get_world();
	}

	std::shared_ptr<const basic_world> basic_node::get_world() const
	{
		return get_network()->get_world();
	}






	void basic_network::start()
	{
		entity::start();

		std::for_each(std::execution::par, nodes.begin(), nodes.end(), [](auto& node) {
			node->start();
		});
	}

	void basic_network::stop()
	{
		std::for_each(std::execution::par, nodes.begin(), nodes.end(), [](auto& node) {
			node->stop();
		});

		entity::stop();
	}




}