#include "hwio_cli.h"

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <assert.h>
#include <algorithm>

#include "bus/hwio_bus_json.h"
#include "hwio_bus_remote.h"
#include "hwio_bus_devicetree.h"

namespace hwio {

const char * hwio_help_str() {
	return "HWIO " HWIO_VERSION "(build " __TIMESTAMP__ ") params:\n" //
	"   if no configuration file is specified ~/.hwio/config.json is used, if it does not exists /etc/hwio/config.json is used\n"//
	"   --hwio_config <path.json>   load hwio configuration from json file\n"//
	"   --hwio_devicetree <path>   root of devicetree to load device info from (def. \"/proc/device-tree\")\n"//
	"   --hwio_device_mem <path>   file with memory space of devices, use with -hwio_devicetree (def. \"/dev/mem\")\n"//
	"   --hwio_remote <ip:port>    connect to remote hwio server\n"//
	"   --hwio_json <path.json>      load devices from json file\n";
}

/**
 * load hwio_bus from json node
 **/
ihwio_bus * hwio_bus_from_ptree(hwio_bus_json::ptree& n) {
	auto type = n.get<std::string>("type");
	if (type == "remote") {
		auto host = n.get<std::string>("host", "");
		if (host == ""){
			throw wrong_format(
					"definition of remote bus in json missing \"host\" attribute");
		}
		return new hwio_bus_remote(host);
	} else if (type == "json") {
		auto file = n.get<std::string>("file", "");
		if (file == ""){
			throw wrong_format(
					"definition of json bus in json missing \"file\" attribute");
		}
		return new hwio_bus_json(file);
	} else if (type == "devicetree") {
		auto devicetree = n.get<std::string>("devicetree", "");
		if (devicetree == ""){
			throw wrong_format(
					"definition of devicetree bus in json missing \"devicetree\" attribute");
		}
		auto mem = n.get<std::string>("mem", "");
		if (mem == "") {
			throw wrong_format(
					"definition of devicetree bus in json missing \"mem\" attribute");
		}
		return new hwio_bus_devicetree(devicetree, mem);
	} else {
		throw wrong_format(
				std::string("unknown definition of bus (") + type + ")");
	}
}

std::vector<ihwio_bus *> hwio_config_load(hwio_bus_json::ptree& root) {
	std::vector<ihwio_bus *> res;
	for (hwio_bus_json::value_type& busNode : root.get_child("buses")) {
		assert(busNode.first.empty());
		auto b = hwio_bus_from_ptree(busNode.second);
		res.push_back(b);
	}
	return res;
}

std::vector<ihwio_bus *> hwio_config_load(const std::string& file_name) {
	if (access(file_name.c_str(), R_OK) != 0) {
		throw std::runtime_error(
				std::string("Specified config file for hwio_config_load does not exists ")
						+ file_name);
	}
	hwio_bus_json::ptree root;
	boost::property_tree::read_json(file_name, root);
	return hwio_config_load(root);
}

std::vector<ihwio_bus *> hwio_load_default_config() {
	struct passwd *pw = getpwuid(getuid());
	const char *homedir = pw->pw_dir;
	std::string config_in_home(homedir);
	config_in_home += "/.hwio/config.json";

	std::vector<std::string> config_names = { config_in_home,
			"/etc/hwio/config.json" };
	for (auto & name : config_names) {
		struct stat buffer;
		bool file_exists = stat(name.c_str(), &buffer) == 0;
		if (file_exists)
			return hwio_config_load(name);
	}
	throw std::runtime_error(
			"[HWIO, cli] can not find any config file (~/.hwio/config.json or /etc/hwio/config.json)");
}

void rm_comsumed_args(const option long_opts[], int & argc, char * argv[]) {
	// build names of options
	std::vector<int> consumed_args;
	std::vector<std::string> reducible_args;
	for (const option * o = long_opts; o->name != nullptr; o++) {
		std::string opt = std::string("--") + o->name;
		assert(o->has_arg == required_argument);
	}

	// search for options in argv
	for (int i = 1; i < argc; i++) {
		for (auto opt : reducible_args) {
			if (opt == argv[i]) {
				consumed_args.push_back(i);
				consumed_args.push_back(i + 1);
				i++;
				break;
			}
		}
	}

	// remove options and it's arguments
	std::remove_if(argv, argv + argc, [&argv, &consumed_args](char * & d) {
		int i = (&d - &*argv);
		bool do_remove = std::find(consumed_args.begin(),
				consumed_args.end(),
				i) != consumed_args.end();
		return !do_remove;
	});
	argc -= consumed_args.size();
}

char ** copy_argv(int argc, char * argv[], std::vector<void *> & to_free) {
	char ** _argv = (char **) malloc(argc* sizeof(char *));
	for (int i = 0; i < argc; i++) {
		auto tmp =  strdup(argv[i]);
		_argv[i] = tmp;
		to_free.push_back((void*)tmp);
	}
	to_free.push_back((void*) _argv);
	return _argv;
}


ihwio_bus * hwio_init(int & argc, char * argv[]) {
	const option long_opts[] = { //
			{ "hwio_config", required_argument, nullptr, 'c' },     //
		    { "hwio_devicetree", required_argument, nullptr, 'd' }, //
		    { "hwio_device_mem", required_argument, nullptr, 'm' }, //
		    { "hwio_remote", required_argument, nullptr, 'r' },     //
		    { "hwio_json", required_argument, nullptr, 'j' },       //
		    { nullptr, no_argument, nullptr, 0 }                    //
	};

	// copy argv because options will be removed
	std::vector<void *> to_free;
	char ** _argv = copy_argv(argc, argv, to_free);
	std::vector<ihwio_bus *> buses;
	const char * hwio_devicetree = nullptr;
	const char * hwio_device_mem = nullptr;
	bool config_specified = false;

	int original_opterr = opterr;
	opterr = 0;
	optind = 0;
	try {
		while (true) {
			const auto opt = getopt_long(argc, _argv, "", long_opts, nullptr);
			ihwio_bus * bus = nullptr;
			if (-1 == opt)
				break;

			switch (opt) {
			case 'c':
				for (auto b : hwio_config_load(optarg))
					buses.push_back(b);
				config_specified = true;

				break;

			case 'r':
				buses.push_back(new hwio_bus_remote(optarg));
				break;

			case 'j':
				buses.push_back(new hwio_bus_json(optarg));
				break;

			case 'd':
				if (hwio_devicetree != nullptr) {
					if (hwio_device_mem == nullptr)
						bus = new hwio_bus_devicetree(hwio_devicetree);
					else
						bus = new hwio_bus_devicetree(hwio_devicetree,
								hwio_device_mem);
					buses.push_back(bus);
					hwio_device_mem = nullptr;
				}
				hwio_devicetree = optarg;
				break;

			case 'm':
				if (hwio_device_mem != nullptr)
					throw std::runtime_error(
							"hwio_device_mem cli arg redefinition without hwio_devicetree specified");
				else
					hwio_device_mem = optarg;
				break;

			default:
				// skip unknown args and let them for main application
				break;
			}
		}
	} catch (const std::exception & e) {
		opterr = original_opterr;
		optind = 0;
		for (auto o: to_free) {
			free(o);
		}
		throw e;
	}
	// put back original state of getopt
	opterr = original_opterr;
	optind = 0;
	for (auto o: to_free) {
		free(o);
	}

	// consume potential leftover
	if (hwio_devicetree != nullptr) {
		ihwio_bus * bus = nullptr;
		if (hwio_device_mem == nullptr)
			bus = new hwio_bus_devicetree(hwio_devicetree);
		else
			bus = new hwio_bus_devicetree(hwio_devicetree, hwio_device_mem);
		buses.push_back(bus);
	}
	rm_comsumed_args(long_opts, argc, argv);

	if (!config_specified)
		for (auto b : hwio_load_default_config())
			buses.push_back(b);

	if (buses.size() == 0)
		throw std::runtime_error("[HWIO, cli] does not have any bus specified");
	else if (buses.size() > 1)
		assert(false && "[TODO] merge buses into composite bus");

	return buses.at(0);
}


std::vector<ihwio_dev*> hwio_select_devs_from_vector(const std::vector<ihwio_dev*>& devices, int index) {
	std::vector<ihwio_dev*> dev_to_use;
	if (index < 0) {
		if (devices.size() <= 1) {
			index = 0;
		} else {
			throw std::out_of_range(std::string("Multiple devices present, device index specification is required (devices_cnt=")
					+ std::to_string(devices.size()) + ")");
		}

	}
    if ((unsigned)index >= devices.size()) {
        throw std::out_of_range(std::string("Can not use device ") + std::to_string(index)
                + " because platform has only " + std::to_string(devices.size())
                + " devices");
    } else {
        dev_to_use.push_back(devices.at(index));
    }

    return dev_to_use;
}

}
