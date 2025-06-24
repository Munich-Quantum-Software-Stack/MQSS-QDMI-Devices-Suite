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

#include "dcdb_qdmi/device.h"
#include "qdmi/constants.h"
#include <array>
#include <chrono>

#include <dcdb/sensor.h>
#include <iostream>
#include <string>

/** @file
 * @brief The header file for sensor implementation
 */

/**
 * @brief The implementation of the encapsulated type
 * QDMI_EnvironmentSensor on the device-side.
 * @details Implemented to hold all the required input to query a
 * `QDMI_EnvironmentSensor` using DCDB and its output. In this device, the
 * `QDMI_EnvironmentSensor`s corresponses to the sensors at the HPC Environment
 * located at the LRZ.
 * @note QDMI_Device_Environment_Query
 * is encapsulated in the QDMI Device Environment Query interface to allow the
 * device implement the type as needed.
 */
typedef struct DCDB_QDMI_EnvironmentSensor_impl_d {
  /// The unique ID to identify the environment.
  const std::string id;
  /// The unit of an environment variable, e.g., Kelvin for temperature.
  const std::string unit;
  /// `float` The samples per second of an environment.
  const std::chrono::duration<float> sampling_rate{};
  /**
   @brief This function is used to query the environment data from the DCDB.
   @details This function queries environmental data in a given time interval
   using DCDB's sensor interface.
   @param[in] connection The connection object that used to connect DCDB host.
   @param[in] start The start time as an UNIX timestamp of the time interval
   @param[in] end The end time as an UNIX timestamp of the time interval

  */
  std::list<DCDB::SensorDataStoreReading> query(DCDB::Connection *connection,
                                                uint64_t start, uint64_t end);
} DCDB_QDMI_EnvironmentSensor_impl_t;

const DCDB_QDMI_EnvironmentSensor_impl_t ENV0{"/qct/q-exa/bluefors/tmixing", "uK",
                                        std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV1{"/qct/daqc/bluefors/tmixing", "uK",
                                        std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV2{"/qct/q-exa/bluefors/t4k", "K",
                                        std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV3{"/qct/daqc/bluefors/t4k", "K",
                                        std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV4{"/qct/daqc/bluefors/p1", "Pa",
                                        std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV5{"/qct/q-exa/bluefors/p1", "Pa",
                                        std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV6{"/qic/warmlab/liquidN2", "L",
                                        std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV7{"/qic/NSR1/liquidN2", "L",
                                        std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV8{"/qic/warmlab/light", "lux",
                                        std::chrono::duration<int>{0}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV9{"/qic/warmlab/loudness", "dB",
                                        std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV10{"/qic/warmlab/movement", "movement",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV11{"/qic/warmlab/door/east/lock", "lock",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV12{"/qic/warmlab/door/west/lock", "lock",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV13{"/qic/warmlab/door/west/ajar", "lock",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV14{"/qic/warmlab/door/east/ajar", "lock",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV15{
    "/qct/daqc/daqc1pdu1/energy1", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV16{
    "/qct/daqc/daqc1pdu1/energy2", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV17{
    "/qct/daqc/daqc1pdu1/energy3", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV18{
    "/qct/daqc/daqc1pdu1/energy4", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV19{
    "/qct/daqc/daqc1pdu1/energy5", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV20{
    "/qct/daqc/daqc1pdu1/energy6", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV21{
    "/qct/daqc/daqc1pdu1/energy7", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV22{
    "/qct/daqc/daqc1pdu1/energy8", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV23{
    "/qct/daqc/daqc1pdu1/energy9", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV24{
    "/qct/daqc/daqc1pdu1/energy10", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV25{
    "/qct/daqc/daqc1pdu1/energy11", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV26{
    "/qct/daqc/daqc1pdu1/energy12", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV27{
    "/qct/daqc/daqc1pdu1/energy13", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV28{
    "/qct/daqc/daqc1pdu1/energy14", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV29{
    "/qct/daqc/daqc1pdu1/energy15", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV30{
    "/qct/daqc/daqc1pdu1/energy16", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV31{
    "/qct/daqc/daqc1pdu1/energy17", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV32{
    "/qct/daqc/daqc1pdu1/energy18", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV33{
    "/qct/daqc/daqc1pdu1/energy19", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV34{
    "/qct/daqc/daqc1pdu1/energy20", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV35{
    "/qct/daqc/daqc1pdu1/energy21", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV36{
    "/qct/daqc/daqc1pdu1/energy22", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV37{
    "/qct/daqc/daqc1pdu1/energy23", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV38{
    "/qct/daqc/daqc1pdu1/energy24", "energy", std::chrono::duration<int>{60}};

const DCDB_QDMI_EnvironmentSensor_impl_t ENV39{
    "/qic/warmlab/temperature", "temperature", std::chrono::duration<int>{60}};

const DCDB_QDMI_EnvironmentSensor_impl_t ENV40{"/qic/warmlab/pressure", "pressure",
                                         std::chrono::duration<int>{60}};

const DCDB_QDMI_EnvironmentSensor_impl_t ENV41{"/qic/warmlab/DAQC/heat/temp-fwd",
                                         "temperature",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV42{"/qic/warmlab/DAQC/heat/temp-ret",
                                         "temperature",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV43{"/qic/warmlab/UKG1/heat/temp-fwd",
                                         "temperature",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV44{"/qic/warmlab/UKG2/heat/temp-ret",
                                         "temperature",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV45{"/qic/warmlab/UKG1/heat/temp-fwd",
                                         "temperature",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV46{"/qic/warmlab/UKG2/heat/temp-ret",
                                         "temperature",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV47{
    "/qic/warmlab/DAQC/heat/energy", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV48{
    "/qic/warmlab/UKG1/heat/energy", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV49{
    "/qic/warmlab/UKG2/heat/energy", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV50{"/qic/warmlab/humidity", "humidity",
                                         std::chrono::duration<int>{60}};

const DCDB_QDMI_EnvironmentSensor_impl_t ENV51{"/qct/daqc/daqc1pdu1/power1", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV52{"/qct/daqc/daqc1pdu1/power2", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV53{"/qct/daqc/daqc1pdu1/power3", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV54{"/qct/daqc/daqc1pdu1/power4", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV55{"/qct/daqc/daqc1pdu1/power5", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV56{"/qct/daqc/daqc1pdu1/power6", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV57{"/qct/daqc/daqc1pdu1/power7", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV58{"/qct/daqc/daqc1pdu1/power8", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV59{"/qct/daqc/daqc1pdu1/power9", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV60{"/qct/daqc/daqc1pdu1/power10", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV61{"/qct/daqc/daqc1pdu1/power11", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV62{"/qct/daqc/daqc1pdu1/power12", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV63{"/qct/daqc/daqc1pdu1/power13", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV64{"/qct/daqc/daqc1pdu1/power14", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV65{"/qct/daqc/daqc1pdu1/power15", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV66{"/qct/daqc/daqc1pdu1/power16", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV67{"/qct/daqc/daqc1pdu1/power17", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV68{"/qct/daqc/daqc1pdu1/power18", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV69{"/qct/daqc/daqc1pdu1/power19", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV70{"/qct/daqc/daqc1pdu1/power20", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV71{"/qct/daqc/daqc1pdu1/power21", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV72{"/qct/daqc/daqc1pdu1/power22", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV73{"/qct/daqc/daqc1pdu1/power23", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV74{"/qct/daqc/daqc1pdu1/power24", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV75{
    "/qct/daqc/daqc1pdu1/power_total", "power", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV76{"/qct/daqc/daqc1pdu1/energy_total",
                                         "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV77{"/qct/daqc/daqc1ups1/BatTimeRemaining",
                                         "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV78{"/qct/daqc/daqc1ups1/BatteryAbmStatus",
                                         "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV79{
    "/qct/daqc/daqc1ups1/BatteryAged", "power", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV80{"/qct/daqc/daqc1ups1/BatteryFailure",
                                         "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV81{
    "/qct/daqc/daqc1ups1/BatteryLowCapacity", "power",
    std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV82{
    "/qct/daqc/daqc1ups1/BatteryNotPresent", "power",
    std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV83{"/qct/daqc/daqc1ups1/OutputStatus",
                                         "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV84{"/qct/daqc/daqc1ups1/apparent_power",
                                         "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV85{
    "/qct/daqc/daqc1ups1/capacity", "power", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV86{"/qct/daqc/daqc1ups1/load", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV87{"/qct/daqc/daqc1ups1/power", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV88{
    "/qct/daqc/qan01/stats/msgsSent", "power", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV89{
    "/qct/daqc/qan01/stats/readings", "power", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV90{"/qct/daqc/qan01/stats/readingsSent",
                                         "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV91{
    "/qic/warmlab/DAQC/heat/volume", "volume", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV92{
    "/qic/warmlab/UKG1/heat/volume", "volume", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV93{
    "/qic/warmlab/UKG2/heat/volume", "volume", std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV94{"/qic/warmlab/magnetometer/X",
                                         "magnetometer",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV95{"/qic/warmlab/magnetometer/Y",
                                         "magnetometer",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_EnvironmentSensor_impl_t ENV96{"/qic/warmlab/magnetometer/Z",
                                         "magnetometer",
                                         std::chrono::duration<int>{60}};
constexpr std::array<const DCDB_QDMI_EnvironmentSensor_impl_t *, 97>
    DCDB_DEVICE_ENVIRONMENTSENSORS = {
        &ENV0,  &ENV1,  &ENV2,  &ENV3,  &ENV4,  &ENV5,  &ENV6,  &ENV7,  &ENV8,
        &ENV9,  &ENV10, &ENV11, &ENV12, &ENV13, &ENV14, &ENV15, &ENV16, &ENV17,
        &ENV18, &ENV19, &ENV20, &ENV21, &ENV22, &ENV23, &ENV24, &ENV25, &ENV26,
        &ENV27, &ENV28, &ENV29, &ENV30, &ENV31, &ENV32, &ENV33, &ENV34, &ENV35,
        &ENV36, &ENV37, &ENV38, &ENV39, &ENV40, &ENV41, &ENV42, &ENV43, &ENV44,
        &ENV45, &ENV46, &ENV47, &ENV48, &ENV49, &ENV50, &ENV51, &ENV52, &ENV53,
        &ENV54, &ENV55, &ENV56, &ENV57, &ENV58, &ENV59, &ENV60, &ENV61, &ENV62,
        &ENV63, &ENV64, &ENV65, &ENV66, &ENV67, &ENV68, &ENV69, &ENV70, &ENV71,
        &ENV72, &ENV73, &ENV74, &ENV75, &ENV76, &ENV77, &ENV78, &ENV79, &ENV80,
        &ENV81, &ENV82, &ENV83, &ENV84, &ENV85, &ENV86, &ENV87, &ENV88, &ENV89,
        &ENV90, &ENV91, &ENV92, &ENV93, &ENV94, &ENV95, &ENV96};
        
