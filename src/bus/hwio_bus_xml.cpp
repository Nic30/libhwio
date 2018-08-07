#include "hwio_bus_xml.h"

#include <stdlib.h>
#include <string.h>
#include <stdexcept>
#include <assert.h>

#include "hwio_device_mmap.h"

namespace hwio {

hwio_comp_spec parse_xml_compatible(xmlDoc * doc, xmlNode * cur) {
	xmlChar *compatible = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	hwio_comp_spec spec = { (const char *) compatible };
	xmlFree(compatible);
	return spec;
}

/**
 * Extract data from an XML component node and store them
 * in an `hwio_comp' structure.
 * @param src XML component node.
 *   XML attribute (one of `version', `name', `index', `base' or `size').
 */
hwio_device_mmap * parse_xml_device(xmlDoc * doc, xmlNode * src,
		hwio_phys_addr_t offset, const std::string & mem_file_path) {
	xmlChar *name = xmlGetProp(src, (const xmlChar *) "name");
	xmlChar *base = xmlGetProp(src, (const xmlChar *) "base");
	xmlChar *size = xmlGetProp(src, (const xmlChar *) "size");

	const char * err_msg = nullptr;
	//if (name == nullptr)
	//	err_msg = "Missing name attribute";

	if (base == nullptr)
		err_msg = "Missing base attribute";
	else if (size == nullptr)
		err_msg = "Missing size attribute";

	hwio_phys_addr_t _size;
	hwio_phys_addr_t _base;
	std::vector<hwio_comp_spec> specs;
	if (err_msg == nullptr) {
		bool compatible_found = false;
		for (xmlNode *cur_node = src->children; cur_node != nullptr; cur_node =
				cur_node->next) {
			if (cur_node->type == XML_ELEMENT_NODE
					&& xmlStrcmp(cur_node->name, (const xmlChar *) "compatible")
							== 0) {
				auto spec = parse_xml_compatible(doc, cur_node);
				spec.name_set((const char *) name);
				specs.push_back(spec);
				compatible_found = true;
			}
		}
		if (!compatible_found) {
			throw xml_wrong_format(
					"Device is missing compatible element");
		}
		_size = std::stoul((const char *) base, nullptr, 16);
		_base = std::stoul((const char *) size, nullptr, 16);
	}

	xmlFree(size);
	xmlFree(base);
	xmlFree(name);

	if (err_msg != nullptr)
		throw xml_wrong_format(err_msg);

	return new hwio_device_mmap(specs, offset + _base, _size, mem_file_path);
}

void hwio_bus_xml::load_devices(xmlDoc * doc) {
	auto root = xmlDocGetRootElement(doc);
	if (xmlStrcmp(root->name, (const xmlChar *) "devicetree") == 0) {
		xmlChar *memfile = xmlGetProp(root, (const xmlChar *) "memfile");
		std::string mem_file = hwio_device_mmap::DEFAULT_MEM_PATH;
		if (memfile != nullptr)
			mem_file = (char *) memfile;

		xmlChar *offset_str = xmlGetProp(root, (const xmlChar *) "offset");
		hwio_phys_addr_t offset = 0;
		if (offset_str != nullptr) {
			offset = std::stoul((const char *) offset_str, nullptr, 16);
		}

		for (xmlNode *n = root->xmlChildrenNode; n != nullptr; n = n->next) {
			if (n->type == XML_ELEMENT_NODE
					&& xmlStrcmp(n->name, (const xmlChar *) "device") == 0) {
				auto dev = parse_xml_device(doc, n, offset, mem_file);
				_all_devices.push_back(dev);
			}
		}
		xmlFree(memfile);
		xmlFree(offset_str);
	} else {
		throw xml_wrong_format("Root element should be named devicetree");
	}

}

hwio_bus_xml::hwio_bus_xml(xmlDoc * doc) {
	load_devices(doc);
}

hwio_bus_xml::hwio_bus_xml(const char * xml_file_name) {
	/*
	 * this initialize the library and check potential ABI mismatches
	 * between the version it was compiled for and the actual shared
	 * library used.
	 */
	LIBXML_TEST_VERSION

	xmlDoc *doc = xmlReadFile(xml_file_name, NULL, 0);
	if (doc == nullptr)
		throw xml_wrong_format(std::string("Failed to parse or open file:") + xml_file_name);

	load_devices(doc);

	xmlFreeDoc(doc);
	/*
	 * Cleanup function for the XML library.
	 */
	xmlCleanupParser();
}

std::vector<ihwio_dev *> hwio_bus_xml::find_devices(
		const std::vector<hwio_comp_spec> & spec) {
	return filter_device_by_spec(_all_devices, spec);
}

hwio_bus_xml::~hwio_bus_xml() {
	for (auto dev : _all_devices) {
		delete dev;
	}
}

}
