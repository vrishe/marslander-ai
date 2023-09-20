#include "global_includes.h"
#include "internal/runner_name.h"
#include "internal/runner_app.h"
#include "marslander/marslander.h"

#include "sockpp/exception.h"
#include "sockpp/inet_address.h"
#include "sockpp/socket.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

#include <getopt.h>

using namespace marslander;
using namespace std;

const string marslander::loggers::runner_logger = runner::name();

namespace {

void init_loggers() {
  auto runner_logger = spdlog::stdout_color_mt(loggers::runner_logger);
  runner_logger->set_level(spdlog::level::info);
  spdlog::set_default_logger(runner_logger);

  auto client_logger = spdlog::stdout_color_mt(loggers::client_logger);
  client_logger->set_level(spdlog::level::info);

  spdlog::set_pattern(DEFAULT_LOG_PATTERN);
}

typedef char* pstr;
typedef const char* pcstr;

void help(pcstr name) {
  // extract executable name
  for (auto i = strlen(name);i > 0;--i)
    if (name[i-1] == '/') {name = &name[i]; break;}

  cout << "Usage: " << name << " [options]\n"
"A satellite client for Genetic algorithm-based trainer for Mars Lander AI.\n"
"\n"
"Options:\n"
"  -h <IP address>,           Specify a IPv4 address of a machine on your LAN\n"
"    --host <IP address>      where Mars Lander trainer utility is runnig.\n"
"\n"
"  -p <port number>,          Specify a TCP port number at which to expect\n"
"    --port=<port number>     connections; 12345 by default.\n"
"\n"
"  --keep-replays=N           Keep N last successful landing replays.\n"
"  --replays-dir=replays/dir  Replays directory path; PWD by default.\n"
"\n"
"There is nowhere to file bugs.\n"
"You're all alone, do not expect any help.\n";

  exit(-1);
}

static constexpr char default_host[] = "localhost";
static constexpr in_port_t default_port = 12345;

void fill_defaults(runner::app_args& args) {
  args.host = ntohl(sockpp::inet_address::resolve_name(default_host));
  args.port = default_port;
  args.replays_count = 0;
  args.replays_dir = "./";
}

void parse_options(int argc, const pstr* argv, runner::app_args& args) {
  int fake_flag;
  constexpr int keep_replays_ind = 3;
  constexpr int replays_dir_ind = 4;
  struct option opts[] = {
    {"help", no_argument, nullptr, 0},
    {"host", required_argument, nullptr, 'h'},
    {"port", required_argument, nullptr, 'p'},
    {"keep-replays", optional_argument, &fake_flag, keep_replays_ind},
    {"replays-dir", required_argument, &fake_flag, replays_dir_ind},
    { NULL, 0, NULL, 0 }
  };

  int c, opt_index;

  while (1) {
    c = getopt_long(argc, argv, "h:p:", opts, &opt_index);
    if (c < 0) return;

    switch (c) {
      case 0: {
        switch (opt_index) {
          case keep_replays_ind: {
            if (optarg) {
              while (*optarg != '\0' && std::isspace(*optarg)) ++optarg;
              if (*optarg != '-')
                args.replays_count = std::strtoul(optarg, nullptr, 10);
            }
            if (args.replays_count < 1)
              args.replays_count = 1;
            break;
          }
          case replays_dir_ind: {
            if (optarg) args.replays_dir = optarg;
            break;
          }
        }
        break;
      }
      case 'h': {
        try { args.host = ntohl(sockpp::inet_address::resolve_name(optarg)); }
        catch (sockpp::getaddrinfo_error&) { goto help; }
        catch (runtime_error& err) {
          spdlog::error("Couldn't resolve address for '{}':\n{}",
            optarg, err.what());
          exit(-1);
        }
        break;
      }
      case 'p': {
        auto port = atoi(optarg);
        if (!port) goto help;
        args.port = port;
        break;
      }
      default: goto help;
    }
  }

help:
  help(*argv);
}

} // namespace

runner::app app;

int main(int argc, const pstr* argv)
{
  [[maybe_unused]]
  protobuf_scope protobuf_;
  sockpp::initialize();

  init_loggers();

  app.init_args([argv, argc](auto& args) {
    fill_defaults(args);
    parse_options(argc, argv, args);
  });

  app.run();
}
