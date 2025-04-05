// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mpconfigboard.h"
#include "stdio.h"
#include <stdlib.h>
#include <nofrendo.h>
#include <nesstate.h>

#include "app_nes_start.h"
void app_nes_start(const char *filename)
{
	
    //Create game route on the SD card
    char *argv[1];
    argv[0] = malloc(256);
    sprintf(argv[0],"%s",(char *)filename);
    //Execute nofrendo emulator
    nofrendo_main(1, argv);
    free(argv[0]);
}


