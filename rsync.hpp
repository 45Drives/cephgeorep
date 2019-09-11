#pragma once

#include <vector>
#include <boost/filesystem.hpp>

#define ARG_SZ 65536

namespace fs = boost::filesystem;

void launch_rsync(std::vector<fs::path> queue);
// fork and exec rsync with flags and pass queue
