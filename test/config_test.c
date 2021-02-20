/*
 *  Copyright (C) 2020 Resql Authors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "settings.h"
#include "test_util.h"

static void config_tests(void)
{
    struct settings settings;

    settings_init(&settings);
    settings_term(&settings);
}

int main(void)
{
    test_execute(config_tests);

    return 0;
}
