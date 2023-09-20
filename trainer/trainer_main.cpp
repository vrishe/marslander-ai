#include "global_includes.h"
#include "internal/trainer_app.h"

#include "sockpp/socket.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <iostream>

#include <getopt.h>
#include <string.h>
#include <stdlib.h>

using namespace marslander;
using namespace std;

namespace {

void init_loggers() {
  auto trainer_logger = spdlog::stdout_color_mt(loggers::trainer_logger);
  trainer_logger->set_level(
    static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL));
  spdlog::set_default_logger(trainer_logger);

  auto server_logger = spdlog::stdout_color_mt(loggers::server_logger);
  server_logger->set_level(spdlog::level::off);

  spdlog::set_pattern(DEFAULT_LOG_PATTERN);
}

typedef char* pstr;
typedef const char* pcstr;

void help(pcstr name) {
  // extract executable name
  for (auto i = strlen(name);i > 0;--i)
    if (name[i-1] == '/') {name = &name[i]; break;}

  cout << "Usage: " << name << " [options]\n"
"Genetic algorithm-based trainer for Mars Lander AI.\n"
"\n"
"Options:\n"
"  --init[=cases/file/path]   Requires to begin training process from the very\n"
"                             beginning; A population is to be created from\n"
"                             scratch. User may specify a path leading to a file\n"
"                             with predefined training cases.\n"
"\n"
"  -p <port number>,          Specify a TCP port number at which to expect\n"
"    --port=<port number>     connections; 12345 by default.\n"
"\n"
"  --replay[=cid<delim>gid]   An export routine that builds up a replay from \n"
"                             current training state and saves step-by-step\n"
"                             simulation results into a file;\n"
"                             User may specify CID (Case ID) and GID (Gene ID):\n"
"                             no additional interations will be required\n"
"                             in this case (delim: ` ,;@`).\n"
"\n"
"  --dump-session[            An export routine that dumps current training\n"
"      =dump/file/path]       session data intoJSON file of stdout.\n"
"\n"
"  --no-exit                  Execution control flag that requires trainer\n"
"                             server to keep running after the export routines\n"
"                             have completed their job.\n"
"\n"
"  -d <path/to/dir>           This is the directory where session data is to be\n"
"    --directory=<...>        located; also this is a destination for replays\n"
"                             exporter."
"\n"
"There is nowhere to file bugs.\n"
"You're all alone, do not expect any help.\n";

  exit(-1);
}

static constexpr in_port_t default_port = 12345;

void fill_defaults(trainer::app_args& args) {
  args.port = default_port;
  args.replay_case_id = 0;
  args.replay_gene_id = 0;
}

void parse_options(int argc, const pstr* argv, trainer::app_args& args) {
  constexpr int init_ind = 1;
  constexpr int export_replay_ind = 3;
  constexpr int export_dump_session_ind = 4;
  constexpr int no_exit_ind = 5;
  constexpr int directory_ind = 6;
  struct option opts[] = {
    {"help", no_argument, nullptr, 0},
    {"init", optional_argument, &args.init_flag, init_ind},
    {"port", required_argument, nullptr, 'p'},
    {"replay", optional_argument, &args.export_replay_flag, export_replay_ind},
    {"dump-session", optional_argument, &args.export_dump_session_flag, export_dump_session_ind},
    {"no-exit", no_argument, &args.no_exit_flag, no_exit_ind},
    {"directory", required_argument, nullptr, 'd'},
    { NULL, 0, NULL, 0 }
  };

  int c, opt_index;

  while (1) {
    c = getopt_long(argc, argv, "d:p:", opts, &opt_index);
    if (c < 0) return;

    switch (c) {
      case 0: {
        switch (opt_index) {
          case init_ind: {
            if (optarg) args.cases_path = optarg;
            break;
          }
          case export_replay_ind: {
            if (optarg) args.parse_replay_optarg(optarg, " ,;@");
            break;
          }
          case export_dump_session_ind: {
            if (optarg) args.dump_session_path = optarg;
            break;
          }
          case no_exit_ind: break;
          default: goto help;
        }
        break;
      }
      case 'd': {
        if (!optarg) goto help;
        args.directory = optarg;
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
  help(argv[0]);
}

} // namespace

trainer::app app;

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
