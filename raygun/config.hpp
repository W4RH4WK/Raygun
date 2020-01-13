// The MIT License (MIT)
//
// Copyright (c) 2019,2020 The Raygun Authors.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#pragma once

namespace raygun {

class Config {
  public:
    /// Default config without associated file.
    Config();

    /// Associate config with specified file.
    Config(const fs::path& path, bool autoLoad = true);

    void load();
    void save() const;

    //////////////////////////////////////////////////////////////////////////

#define CONFIG_BOOL(_identifier, _default) bool _identifier = _default;

#define CONFIG_INT(_identifier, _default) int _identifier = _default;

#define CONFIG_DOUBLE(_identifier, _default) double _identifier = _default;

#define CONFIG_ENUM(_identifier, _enum) enum class _enum {
#define CONFIG_ENUM_ENTRY(_identifier, _enum, _entry) _entry,
#define CONFIG_ENUM_END(_identifier, _enum, _default) \
    } \
    _identifier = _enum::##_default;

#include "raygun/config.def"

    //////////////////////////////////////////////////////////////////////////

  private:
    fs::path configFile;

    static constexpr auto CONFIG_TYPE_MARKER = "Config";
};

using UniqueConfig = std::unique_ptr<Config>;

fs::path configDirectory();

} // namespace raygun
