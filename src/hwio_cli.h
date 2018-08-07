#pragma once

#include <vector>
#include <getopt.h>

#include "ihwio_bus.h"
#include "hwio.h"

namespace hwio {

const char * hwio_help_str();

ihwio_bus * hwio_init(int & argc, char * argv[]);

char ** copy_argv(int argc, char * argv[], std::vector<void *> & to_free);

}
