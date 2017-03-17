
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

int main()
{
    int res = EXIT_SUCCESS;

    try
    {
        auto fuzz = Fuzztest();
        auto service = fuzz.service();

        fuzz.set_next_fake_response(
            read_file(fuzz.assets_dir() / "create_contact_response.xml"));

        auto contact = ews::contact();
        contact.set_given_name("Darkwing");
        contact.set_surname("Duck");
        contact.set_email_address(
                ews::email_address(ews::email_address::key::email_address_1,
                    "superhero@ducktales.com"));
        contact.set_job_title("Average Citizen");

        auto item_id = service.create_item(contact);

        fuzz.set_next_fake_response(
            read_file(fuzz.assets_dir() / "get_contact_response.xml"));

        auto cont = service.get_contact(item_id);
    }
    catch (std::exception& exc)
    {
        std::cout << exc.what() << std::endl;
        res = EXIT_FAILURE;
    }

    return res;
}

// vim:et ts=4 sw=4
