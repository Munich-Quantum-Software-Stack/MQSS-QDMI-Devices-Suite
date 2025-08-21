
/*------------------------------------------------------------------------------
Copyright 2024 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://github.com/Munich-Quantum-Software-Stack/QDMI-Devices/blob/develop/LICENSE

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
------------------------------------------------------------------------------*/

#include "sensors.h"
#include <dcdb/sensor.h>
#include <ostream>
#include <unistd.h>

std::list<DCDB::SensorDataStoreReading>
DCDB_QDMI_TelemetrySensor_impl_t::query(DCDB::Connection *connection,
                                        uint64_t start, uint64_t end) {

  std::list<DCDB::SensorDataStoreReading> readings;
  DCDB::TimeStamp start_ts, end_ts;
  start_ts = DCDB::TimeStamp(start);
  end_ts = DCDB::TimeStamp(end);
  DCDB::Sensor s1(connection, id);

  s1.query(readings, start_ts, end_ts);
  return readings;
}
