
//   Copyright 2016 otris software AG
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//   This project is hosted at https://github.com/otris

#pragma once

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <ews/ews.hpp>
#include <ews/ews_test_support.hpp>
#include <ews/rapidxml/rapidxml.hpp>

#ifdef EWS_USE_BOOST_LIBRARY
#include "boost-filesystem-wrapper.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#endif

namespace fuzztest
{
// Check if cont contains the element val
template <typename ContainerType, typename ValueType>
inline bool contains(const ContainerType& cont, const ValueType& val)
{
    return std::find(begin(cont), end(cont), val) != end(cont);
}

// Check if cont contains an element for which pred evaluates to true
template <typename ContainerType, typename Predicate>
inline bool contains_if(const ContainerType& cont, Predicate pred)
{
    return std::find_if(begin(cont), end(cont), pred) != end(cont);
}

#ifdef EWS_USE_BOOST_LIBRARY
// Read file contents into a buffer
inline std::vector<char> read_file(const boost::filesystem::path& path)
{
    std::ifstream ifstr(path.string(), std::ifstream::in | std::ios::binary);
    if (!ifstr.is_open())
    {
        throw std::runtime_error("Could not open file for reading: " +
                                 path.string());
    }

    ifstr.unsetf(std::ios::skipws);

    ifstr.seekg(0, std::ios::end);
    const auto file_size = ifstr.tellg();
    ifstr.seekg(0, std::ios::beg);

    auto contents = std::vector<char>();
    contents.reserve(file_size);

    contents.insert(begin(contents),
                    std::istream_iterator<unsigned char>(ifstr),
                    std::istream_iterator<unsigned char>());
    ifstr.close();
    return contents;
}
#endif // EWS_USE_BOOST_LIBRARY

struct http_request_mock final
{
    struct storage
    {
        static storage& instance()
        {
#ifdef EWS_HAS_THREAD_LOCAL_STORAGE
            thread_local storage inst;
#else
            static storage inst;
#endif
            return inst;
        }

        std::string request_string;
        std::vector<char> fake_response;
        std::string url;
    };

#ifdef EWS_HAS_DEFAULT_AND_DELETE
    http_request_mock() = default;
#else
    http_request_mock() {}
#endif

    bool header_contains(const std::string& search_str) const
    {
        const auto& request_str = storage::instance().request_string;
        const auto header_end_idx = request_str.find("</soap:Header>");
        const auto search_str_idx = request_str.find(search_str);
        return search_str_idx != std::string::npos &&
               search_str_idx < header_end_idx;
    }

    // Below same public interface as ews::internal::http_request class
    enum class method
    {
        POST
    };

    explicit http_request_mock(const std::string& url)
    {
        auto& s = storage::instance();
        s.url = url;
    }

    void set_method(method) {}

    void set_content_type(const std::string&) {}

    void set_content_length(std::size_t) {}

    void set_credentials(const ews::internal::credentials&) {}

#ifdef EWS_HAS_VARIADIC_TEMPLATES
    template <typename... Args> void set_option(CURLoption, Args...) {}
#else
    template <typename T1> void set_option(CURLoption option, T1) {}

    template <typename T1, typename T2>
    void set_option(CURLoption option, T1, T2)
    {
    }
#endif

    ews::internal::http_response send(const std::string& request)
    {
        auto& s = storage::instance();
        s.request_string = request;
        auto response_bytes = s.fake_response;
        return ews::internal::http_response(200, std::move(response_bytes));
    }
};

#ifdef EWS_USE_BOOST_LIBRARY
class TemporaryDirectory
{
public:
    TemporaryDirectory()
    {
        assets_ = (boost::filesystem::current_path().string() + "/../fuzzing/assets/");
        olddir_ = boost::filesystem::current_path();
        workingdir_ = boost::filesystem::unique_path(
            boost::filesystem::temp_directory_path() / "%%%%-%%%%-%%%%-%%%%");
        boost::filesystem::create_directory(workingdir_);
        boost::filesystem::current_path(workingdir_);
    }

    ~TemporaryDirectory()
    {
        boost::filesystem::is_empty(workingdir_);
        boost::filesystem::current_path(olddir_);
        boost::filesystem::remove_all(workingdir_);
    }

    const boost::filesystem::path& cwd() const { return workingdir_; }

    const boost::filesystem::path assets_dir() const
    {
        return boost::filesystem::path(assets_);
    }

private:
    boost::filesystem::path olddir_;
    boost::filesystem::path workingdir_;
    std::string assets_;
};
#endif // EWS_USE_BOOST_LIBRARY

class Fuzztest : public TemporaryDirectory
{
public:
    Fuzztest() : creds_("", ""), smtp_address_() {
    #ifdef EWS_HAS_MAKE_UNIQUE
        service_ptr_ = std::make_unique<ews::basic_service<http_request_mock>>(
            "https://example.com/ews/Exchange.asmx", "FAKEDOMAIN", "fakeuser",
            "fakepassword");
    #else
        service_ptr_ = std::unique_ptr<ews::basic_service<http_request_mock>>(
            new ews::basic_service<http_request_mock>(
                "https://example.com/ews/Exchange.asmx", "FAKEDOMAIN",
                "fakeuser", "fakepassword"));
    #endif
        creds_ =
            ews::basic_credentials("dduck@duckburg.onmicrosoft.com", "quack");
        smtp_address_ = "dduck@duckburg.onmicrosoft.com";
    }

    ews::basic_service<http_request_mock>& service() { return *service_ptr_; }

    ews::basic_credentials& credentials() { return creds_; }

    std::string& address() { return smtp_address_; }

    void set_next_fake_response(const std::vector<char>& bytes)
    {
        auto& storage = http_request_mock::storage::instance();
        storage.fake_response = bytes;
        storage.fake_response.push_back('\0');
    }

    http_request_mock get_last_request() { return http_request_mock(); }

private:
    ews::basic_credentials creds_;
    std::string smtp_address_;
    std::unique_ptr<ews::basic_service<http_request_mock>> service_ptr_;
};
}

// vim:et ts=4 sw=4
