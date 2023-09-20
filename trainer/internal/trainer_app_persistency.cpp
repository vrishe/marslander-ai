#include "trainer_app.h"

#include "crc32.h"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace marslander::trainer {

using namespace std;

template<class CharT, class Traits, typename T,
  enable_if_t<is_trivial_v<T>, bool> = true>
basic_istream<CharT, Traits>& binary_read(
    basic_istream<CharT, Traits>& is, T& value) {
  return is.read(reinterpret_cast<char*>(&value), sizeof(T));
}

template<class CharT, class Traits, typename T, class Allocator>
basic_istream<CharT, Traits>& binary_read(basic_istream<CharT, Traits>& is,
    vector<T, Allocator>& value) {
  size_t sz; binary_read(is, sz); value.resize(sz);
  if (sz <= 0) return is;
  return is.read(reinterpret_cast<char*>(value.data()),
    sizeof(value_type_of(value)) * sz);
}

template<class CharT, class Traits,
  typename StrCharT, class StrTraits, class StrAllocator>
basic_istream<CharT, Traits>& binary_read(basic_istream<CharT, Traits>& is,
    basic_string<StrCharT, StrTraits, StrAllocator>& value) {
  size_t sz; binary_read(is, sz); value.resize(sz);
  if (sz <= 0) return is;
  return is.read(reinterpret_cast<char*>(value.data()),
    sizeof(value_type_of(value)) * sz);
}

template<class CharT, class Traits>
basic_istream<CharT, Traits>& binary_read( basic_istream<CharT, Traits>& is,
    algorithm_args& value) {
  binary_read(is, value.name);
  binary_read(is, value.values);
  return is;
}

istream& app_state::read(istream& is, read_mode mode) {
  if (mode & header) {
    binary_read(is, check);
    binary_read(is, generation);
    binary_read(is, cases_count);
    binary_read(is, population_size);
    binary_read(is, elite_count);
    binary_read(is, tournament_size);
    binary_read(is, crossover);
    binary_read(is, mutation);

    uid_t uid;
    binary_read(is, uid);
    uids = uid_source(uid);
  }

  if (mode & body) {
    size_t sz;
    using namespace google::protobuf::io;
    IstreamInputStream iis(&is);
    CodedInputStream cis(&iis);
    cases.resize(cases_count);
    for (auto& item : cases) {
      cis.ReadRaw(static_cast<void*>(&sz), sizeof(sz));
      [[maybe_unused]] pb::CodedInputStreamLimitScope sentry_(&cis, sz);
      // TODO: consider making error handling.
      item.ParseFromCodedStream(&cis);
    }
    population.resize(population_size);
    for (auto& item : population) {
      cis.ReadRaw(static_cast<void*>(&sz), sizeof(sz));
      [[maybe_unused]] pb::CodedInputStreamLimitScope sentry_(&cis, sz);
      item.ParseFromCodedStream(&cis);
    }
  }

  return is;
}

template<class CharT, class Traits, typename T,
  enable_if_t<is_trivial_v<T>, bool> = true>
basic_ostream<CharT, Traits>& binary_write( basic_ostream<CharT, Traits>& os,
    const T& value) {
  return os.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template<class CharT, class Traits, typename T, class Allocator>
basic_ostream<CharT, Traits>& binary_write(basic_ostream<CharT, Traits>& os,
    const vector<T, Allocator>& value) {
  size_t sz = value.size();
  binary_write(os, sz);
  if (sz <= 0) return os;
  return os.write(reinterpret_cast<const char*>(value.data()),
      sizeof(value_type_of(value)) * sz);
}

template<class CharT, class Traits,
  typename StrCharT, class StrTraits, class StrAllocator>
basic_ostream<CharT, Traits>& binary_write(basic_ostream<CharT, Traits>& os,
    const basic_string<StrCharT, StrTraits, StrAllocator>& value) {
  size_t sz = value.size();
  binary_write(os, sz);
  if (sz <= 0) return os;
  return os.write(reinterpret_cast<const char*>(value.data()),
      sizeof(value_type_of(value)) * sz);
}

template<class CharT, class Traits>
basic_ostream<CharT, Traits>& binary_write(basic_ostream<CharT, Traits>& os,
    const algorithm_args& value) {
  binary_write(os, value.name);
  binary_write(os, value.values);
  return os;
}

ostream& app_state::write(ostream& os) const {
  binary_write(os, check);
  binary_write(os, generation);
  binary_write(os, cases_count);
  binary_write(os, population_size);
  binary_write(os, elite_count);
  binary_write(os, tournament_size);
  binary_write(os, crossover);
  binary_write(os, mutation);
  binary_write(os, uids.value());

  {
    size_t sz;
    using namespace google::protobuf::io;
    OstreamOutputStream oos(&os);
    CodedOutputStream cos(&oos);
    for (auto& item : cases) {
      sz = item.ByteSizeLong();
      cos.WriteRaw(static_cast<void*>(&sz), sizeof(sz));
      // TODO: consider making error handling.
      item.SerializeToCodedStream(&cos);
    }
    for (auto& item : population) {
      sz = item.ByteSizeLong();
      cos.WriteRaw(static_cast<void*>(&sz), sizeof(sz));
      item.SerializeToCodedStream(&cos);
    }
  }

  return os;
}

namespace {

void back_up_existing(const string_view& filename, const app_state& state) {
  if (state.generation <= 0) {
    SPDLOG_LOGGER_TRACE(_logger, "0th generation, nothing to back up.");
    return;
  }

  {
    ifstream f(string(filename), ios_base::binary);
    if (!f) {
      SPDLOG_LOGGER_TRACE(_logger, "Failed to open {}; no backup performed.", filename);
      return;
    }

    decltype(app_state::check) check;
    binary_read(f, check);
    if (check != state.check) {
      SPDLOG_LOGGER_TRACE(_logger, "Expected check is {}, "
          "but the existing file contains {}; no back up performed.",
        state.check, check);
      return;
    }
  }

  auto to = (stringstream() << filename << ".gen" << state.generation - 1);
  filesystem::copy_file(filename, to.str(),
    filesystem::copy_options::overwrite_existing);
}

using namespace marslander::checksum;

template<class CharT, class Traits>
crc32_t find_checksum(basic_istream<CharT, Traits>& is) {
  if (!is) return crc32_t{};
  constexpr size_t buf_sz = 1024 * 1024;
  auto buf_ptr = make_unique<char[]>(buf_sz);
  crc32 crc;
  while (is) {
    is.read(buf_ptr.get(), buf_sz);
    crc.update(static_cast<void*>(buf_ptr.get()), is.gcount());
  }
  return crc;
}

} // namespace

std::ifstream& app::check_integrity(std::ifstream& is) {
  if (is) {
    crc32_t cs_actual;
    binary_read(is, cs_actual);

    auto ofs_beg = is.tellg();
    auto cs_expected = find_checksum(is);

    is.clear();
    is.seekg(ofs_beg, ios_base::beg);

    if (cs_expected != cs_actual) {
      is.setstate(ios_base::badbit);
      SPDLOG_LOGGER_DEBUG(_logger, "Checksum mismatch! Found {}, "
        "but {} was expected.", cs_actual, cs_expected);
    }
    else {
      SPDLOG_LOGGER_TRACE(_logger, "Checksum is OK ({}).", cs_actual);
    }
  }

  return is;
}

void app::persist_state(const app_state& state) {
  // back_up_existing(training_filename, state);

  constexpr auto file_mode = ios_base::in | ios_base::out
    | ios_base::binary | ios_base::trunc;
  fstream f(training_filename, file_mode);

  using namespace checksum;
  crc32_t cs{};
  auto cs_ofs_beg = f.tellp();
  binary_write(f, cs);
  auto data_ofs_beg = f.tellp();
  state.write(f);
  cs = find_checksum(f.seekg(data_ofs_beg, ios_base::beg));
  binary_write((f.clear(), f.seekp(cs_ofs_beg, ios_base::beg)), cs);
}

} // namespace marslander::trainer
