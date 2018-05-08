/*
 * Copyright 2018 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_UTIL_FILE_HPP
#define JLM_UTIL_FILE_HPP

#include <string>

namespace jlm {

class file final {
public:
	inline
	file(const std::string & f)
	: file_(f)
	{}

	/**
	* \brief Returns the base name of the file without the path.
	*
	* Example:
	*    jlm::file f("/tmp/archive.tar.gz");
	*    auto base = f.base(); // base = "archive"
	*/
	inline std::string
	base() const noexcept
	{
		auto fn = name();
		auto pos = fn.find_first_of(".");
		if (pos == std::string::npos)
			return fn;

		return fn.substr(0, pos);
	}

	/**
	* \brief Returns the name of the file, excluding the path.
	*
	* Example:
	*    jlm::file f("/tmp/archive.tar.gz");
	*    auto name = f.name(); // name = "archive.tar.gz"
	*
	* @return The name of the file
	*/
	inline std::string
	name() const noexcept
	{
		auto pos = file_.find_last_of("/");
		if (pos == std::string::npos)
			return file_;

		return file_.substr(pos+1, file_.size()-pos);
	}

	/**
	* \brief Returns the complete suffix (extension) of the file.
	*
	* Example:
	*    jlm::file f("/tmp/archive.tar.gz");
	*    auto ext = f.complete_suffix(); // ext = "tar.gz"
	*/
	inline std::string
	complete_suffix() const noexcept
	{
		auto fn = name();
		auto pos = fn.find_first_of(".");
		if (pos == std::string::npos)
			return fn;

		return fn.substr(pos+1, fn.size()-pos);
	}

	inline std::string
	to_str() const noexcept
	{
		return file_;
	}

private:
	std::string file_;
};

}

#endif