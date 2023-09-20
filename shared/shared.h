#pragma once

#ifndef SHARED_SHARED_H_
#define SHARED_SHARED_H_

#define   likely_(cond) __builtin_expect(cond, 1)
#define unlikely_(cond) __builtin_expect(cond, 0)

#include "proto/genome.pb.h"
#include "proto/landing_case.pb.h"
#include "proto/transmitted.pb.h"
#include "proto/message_info.h"

// Non-dependent headers
#include "internal/chrono_insert_compat.h"
#include "internal/floating_point.h"
#include "internal/landing_case_randomize.h"
#include "internal/looper.h"
#include "internal/nn_randomize.h"
#include "internal/parcel.h"
#include "internal/protobuf_iterators.h"
#include "internal/protobuf_scope.h"
#include "internal/protobuf_streams.h"
#include "internal/scope_on_exit.h"
#include "internal/signum.h"
#include "internal/string_split.h"
#include "internal/string_tolower.h"
#include "internal/string_trim.h"
#include "internal/this_binding.h"
#include "internal/traits_is_pointer.h"
#include "internal/uid_source.h"
#include "internal/value_type_of.h"

// L1-dependent headers
#include "internal/data_convert.h"
#include "internal/data_transfer.h"
#include "internal/user_input.h"

#define ITERZ_(c, name)\
  auto name##_it = c.begin(), name##_end = c.end()
#define ITERZ_END(name) (name##_it == name##_end)

#define DEFAULT_LOG_PATTERN "%Y-%m-%d %H:%M:%S.%e [%^%l%$] <%n> %v"

#endif // SHARED_SHARED_H_
