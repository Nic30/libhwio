#include "hwio_cli.h"

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <assert.h>
#include <algorithm>

#include "hwio_bus_remote.h"
#include "hwio_bus_devicetree.h"
#include "hwio_bus_xml.h"

namespace hwio {

const char * hwio_help_str() {
	return "HWIO " HWIO_VERSION " params:\n" //
	"   if no config is specified ~/.hwio_config.xml is used, if it does not exists /etc/hwio/default.xml is used\n"//
	"   --hwio_config <path.xml>   load hwio configuration from xml file\n"//
	"   --hwio_devicetree <path>   root of devicetree to load device info from (def. \"/proc/device-tree\")\n"//
	"   --hwio_device_mem <path>   file with memory space of devices, use with -hwio_devicetree (def. \"/dev/mem\")\n"//
	"   --hwio_remote <ip:port>    connect to remote hwio server\n"//
	"   --hwio_xml <path.xml>      load devices from xml file\n";
}

/**
 * load hwio_bus from xml node
 **/
ihwio_bus * hwio_bus_from_xml_node(xmlNode * n) {
	if (xmlStrcmp(n->name, (const xmlChar *) "remote") == 0) {
		xmlChar *host = xmlGetProp(n, (const xmlChar *) "host");
		if (host == nullptr)
			throw xml_wrong_format(
					"definition of remote bus in xml missing \"host\" attribute");
		return new hwio_bus_remote((char *) host);
	} else if (xmlStrcmp(n->name, (const xmlChar *) "xml") == 0) {
		xmlChar *file = xmlGetProp(n, (const xmlChar *) "file");
		if (file == nullptr)
			throw xml_wrong_format(
					"definition of xml bus in xml missing \"file\" attribute");
		return new hwio_bus_xml((char *) file);
	} else if (xmlStrcmp(n->name, (const xmlChar *) "devicetree") == 0) {
		xmlChar *devicetree = xmlGetProp(n, (const xmlChar *) "devicetree");
		if (devicetree == nullptr)
			throw xml_wrong_format(
					"definition of devicetree bus in xml missing \"devicetree\" attribute");
		xmlChar *mem = xmlGetProp(n, (const xmlChar *) "mem");
		if (mem == nullptr) {
			xmlFree(devicetree);
			throw xml_wrong_format(
					"definition of devicetree bus in xml missing \"mem\" attribute");
		}
		return new hwio_bus_devicetree((char *) devicetree, (char *) mem);
	} else {
		throw xml_wrong_format(
				std::string("unknown definition of bus (")
						+ (const char *) n->name + ")");
	}
}

std::vector<ihwio_bus *> hwio_config_load(xmlDoc * doc) {
	std::vector<ihwio_bus *> res;
	auto root = xmlDocGetRootElement(doc);
	if (xmlStrcmp(root->name, (const xmlChar *) "hwio_config") == 0) {
		for (xmlNode *n0 = root->xmlChildrenNode; n0 != nullptr; n0 =
				n0->next) {
			if (n0->type == XML_ELEMENT_NODE
					&& xmlStrcmp(n0->name, (const xmlChar *) "bus") == 0) {
				for (xmlNode *n1 = n0->xmlChildrenNode; n1 != nullptr;
						n1 = n1->next) {
					if (n1->type == XML_ELEMENT_NODE) {
						auto b = hwio_bus_from_xml_node(n1);
						res.push_back(b);
					}
				}
			}
		}
	} else {
		throw xml_wrong_format("Root element should be named hwio_config");
	}
	return res;
}

std::vector<ihwio_bus *> hwio_config_load(const char * xml_file_name) {
	/*
	 * this initialize the library and check potential ABI mismatches
	 * between the version it was compiled for and the actual shared
	 * library used.
	 */
	LIBXML_TEST_VERSION

	xmlDoc *doc = xmlReadFile(xml_file_name, NULL, 0);
	if (doc == nullptr)
		throw xml_wrong_format("Failed to parse");

	auto res = hwio_config_load(doc);

	xmlFreeDoc(doc);
	/*
	 * Cleanup function for the XML library.
	 */
	xmlCleanupParser();

	return res;
}

std::vector<ihwio_bus *> hwio_load_default_config() {
	struct passwd *pw = getpwuid(getuid());
	const char *homedir = pw->pw_dir;
	std::string config_in_home(homedir);
	config_in_home += "/.hwio_config.xml";

	std::vector<std::string> config_names = { config_in_home,
			"/etc/hwio/default.xml" };
	for (auto & name : config_names) {
		struct stat buffer;
		bool file_exists = stat(name.c_str(), &buffer) == 0;
		if (file_exists)
			return hwio_config_load(name.c_str());
	}
	throw std::runtime_error(
			"Hwio can not find any config file (~/.hwio_config.xml or /etc/hwio/default.xml)");
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

ihwio_bus * hwio_init(int & argc, char * argv[]) {
	const option long_opts[] = { //
			{ "hwio_config", required_argument, nullptr, 'c' },     //
					{ "hwio_devicetree", required_argument, nullptr, 'd' }, //
					{ "hwio_device_mem", required_argument, nullptr, 'm' }, //
					{ "hwio_remote", required_argument, nullptr, 'r' },     //
					{ "hwio_xml", required_argument, nullptr, 'x' },        //
					{ nullptr, no_argument, nullptr, 0 }                    //
			};

	std::vector<ihwio_bus *> buses;
	const char * hwio_devicetree = nullptr;
	const char * hwio_device_mem = nullptr;
	bool config_specified = false;

	int original_opterr = opterr;
	opterr = 0;
	try {
		while (true) {
			const auto opt = getopt_long(argc, argv, "", long_opts, nullptr);
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

			case 'x':
				buses.push_back(new hwio_bus_xml(optarg));
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
		optind = 1;
		throw e;
	}
	// put back original state of getopt
	opterr = original_opterr;
	optind = 1;

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
		throw std::runtime_error("hwio does not have any bus specified");
	else if (buses.size() > 1)
		assert(false && "[TODO] merge buses into composite bus");

	return buses.at(0);
}

}
