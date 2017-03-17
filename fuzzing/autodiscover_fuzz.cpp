
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

#include "fuzzmock.hpp"

#include <ostream>
#include <exception>
#include <cstdlib>

using namespace fuzztest;

int main(int argc, char** argv)
{
    int res = EXIT_SUCCESS;

    try
    {
        auto fuzz = Fuzztest();
        auto service = fuzz.service();
        auto url = std::string(argv[1]);

		fuzz.set_next_fake_response(
            read_file(fuzz.assets_dir() / "autodiscover_response.xml"));

		ews::autodiscover_hints hints;
        hints.autodiscover_url = url;

        auto result = ews::get_exchange_web_services_url<http_request_mock>(
            fuzz.address(), fuzz.credentials(), hints);
    }
    catch (std::exception& exc)
    {
        std::cout << exc.what() << std::endl;
        res = EXIT_FAILURE;
    }

    return res;
}

// vim:et ts=4 sw=4
