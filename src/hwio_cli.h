#pragma once

#include <vector>
#include <getopt.h>

#include "ihwio_bus.h"
#include "hwio.h"

namespace hwio {

const char * hwio_help_str();

ihwio_bus * hwio_init(int & argc, char * argv[]);

std::vector<ihwio_dev*> hwio_select_devs_from_vector(const std::vector<ihwio_dev*>& devices, int index);

char ** copy_argv(int argc, char * argv[], std::vector<char *> & to_free);

}
