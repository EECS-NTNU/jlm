/*
 * Copyright 2017 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_UTIL_STRFMT_HPP
#define JLM_UTIL_STRFMT_HPP

#include <sstream>

namespace jlm::util
{

template<typename... Args>
static inline void
format_to_stream(std::ostream & os, Args... args);

template<typename Arg>
static inline void
format_to_stream(std::ostream & os, const Arg & arg)
{
  os << arg;
}

template<typename Arg, typename... Args>
static inline void
format_to_stream(std::ostream & os, const Arg & arg, Args... args)
{
  os << arg;
  format_to_stream(os, args...);
}

template<typename... Args>
static inline std::string
strfmt(Args... args)
{
  std::ostringstream os;
  format_to_stream(os, args...);
  return os.str();
}

/**
 * Creates a string of the given length using random letters and digits.
 * Uses letters a-z, A-Z and digits 0-9
 * @param length the length of the string
 * @return the created string
 */
std::string
CreateRandomAlphanumericString(std::size_t length);

}

#endif
