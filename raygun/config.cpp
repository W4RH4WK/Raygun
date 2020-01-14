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

#include "raygun/config.hpp"

#include "raygun/assert.hpp"
#include "raygun/info.hpp"
#include "raygun/logging.hpp"

namespace raygun {

Config::Config() : Config({}, false) {}

Config::Config(const fs::path& path, bool autoLoad) : configFile(path)
{
    if(autoLoad) {
        if(fs::exists(configFile)) {
            load();
        }
        else {
            save();
        }
    }
}

void Config::load()
{
    if(configFile.empty()) {
        RAYGUN_ERROR("No config file associated.");
        return;
    }

    RAYGUN_INFO("Loading config: {}", configFile);

    std::ifstream in(configFile);
    RAYGUN_ASSERT(in);

    json data;
    in >> data;

    if(data.at("type") != CONFIG_TYPE_MARKER) {
        RAYGUN_ERROR("Not a config file");
        return;
    }

#define CONFIG_BOOL(_identifier, _default) \
    if(data.contains(#_identifier) && data[#_identifier].is_boolean()) { \
        _identifier = data[#_identifier]; \
    }

#define CONFIG_INT(_identifier, _default) \
    if(data.contains(#_identifier) && data[#_identifier].is_number_integer()) { \
        _identifier = data[#_identifier]; \
    }

#define CONFIG_DOUBLE(_identifier, _default) \
    if(data.contains(#_identifier) && data[#_identifier].is_number_float()) { \
        _identifier = data[#_identifier]; \
    }

#define CONFIG_ENUM(_identifier, _enum) if(data.contains(#_identifier) && data[#_identifier].is_string()) {

#define CONFIG_ENUM_ENTRY(_identifier, _enum, _entry) \
    if(data[#_identifier] == #_entry) { \
        _identifier = _enum::_entry; \
    }

#define CONFIG_ENUM_END(_identifier, _enum, _default) }

#include "raygun/config.def"
}

void Config::save() const
{
    if(configFile.empty()) {
        RAYGUN_ERROR("No config file associated.");
        return;
    }

    RAYGUN_INFO("Saving config: {}", configFile);

    json data;
    data["type"] = CONFIG_TYPE_MARKER;

#define CONFIG_BOOL(_identifier, _default) data[#_identifier] = _identifier;

#define CONFIG_INT(_identifier, _default) data[#_identifier] = _identifier;

#define CONFIG_DOUBLE(_identifier, _default) data[#_identifier] = _identifier;

#define CONFIG_ENUM_ENTRY(_identifier, _enum, _entry) \
    if(_identifier == _enum::_entry) { \
        data[#_identifier] = #_entry; \
    }

#include "raygun/config.def"

    std::ofstream out(configFile);
    RAYGUN_ASSERT(out);

    out << data.dump(2);
}

fs::path configDirectory()
{
    const fs::path path{"config"};

    std::error_code err;
    fs::create_directories(path, err);
    if(err) {
        RAYGUN_WARN("Unable to create config directory, using working directory");
        return fs::current_path();
    }

    return path;
}

} // namespace raygun
