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
 * QDMI_TelemetrySensor on the device-side.
 * @details Implemented to hold all the required input to query a
 * `QDMI_TelemetrySensor` using DCDB and its output. In this device, the
 * `QDMI_TelemetrySensor`s corresponses to the sensors at the HPC Telemetry
 * located at the LRZ.
 * @note QDMI_Device_Telemetry_Query
 * is encapsulated in the QDMI Device Telemetry Query interface to allow the
 * device implement the type as needed.
 */
typedef struct DCDB_QDMI_TelemetrySensor_impl_d {
  /// The unique ID to identify the telemetry.
  const std::string id;
  /// The unit of an telemetry variable, e.g., Kelvin for temperature.
  const std::string unit;
  /// `float` The samples per second of an telemetry.
  const std::chrono::duration<float> sampling_rate{};
  /**
   @brief This function is used to query the telemetry data from the DCDB.
   @details This function queries telemetryal data in a given time interval
   using DCDB's sensor interface.
   @param[in] connection The connection object that used to connect DCDB host.
   @param[in] start The start time as an UNIX timestamp of the time interval
   @param[in] end The end time as an UNIX timestamp of the time interval

  */
  std::list<DCDB::SensorDataStoreReading> query(DCDB::Connection *connection,
                                                uint64_t start, uint64_t end);
} DCDB_QDMI_TelemetrySensor_impl_t;

const DCDB_QDMI_TelemetrySensor_impl_t TEL0{"/qct/q-exa/bluefors/tmixing", "uK",
                                        std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL1{"/qct/daqc/bluefors/tmixing", "uK",
                                        std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL2{"/qct/q-exa/bluefors/t4k", "K",
                                        std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL3{"/qct/daqc/bluefors/t4k", "K",
                                        std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL4{"/qct/daqc/bluefors/p1", "Pa",
                                        std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL5{"/qct/q-exa/bluefors/p1", "Pa",
                                        std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL6{"/qic/warmlab/liquidN2", "L",
                                        std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL7{"/qic/NSR1/liquidN2", "L",
                                        std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL8{"/qic/warmlab/light", "lux",
                                        std::chrono::duration<int>{0}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL9{"/qic/warmlab/loudness", "dB",
                                        std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL10{"/qic/warmlab/movement", "movement",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL11{"/qic/warmlab/door/east/lock", "lock",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL12{"/qic/warmlab/door/west/lock", "lock",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL13{"/qic/warmlab/door/west/ajar", "lock",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL14{"/qic/warmlab/door/east/ajar", "lock",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL15{
    "/qct/daqc/daqc1pdu1/energy1", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL16{
    "/qct/daqc/daqc1pdu1/energy2", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL17{
    "/qct/daqc/daqc1pdu1/energy3", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL18{
    "/qct/daqc/daqc1pdu1/energy4", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL19{
    "/qct/daqc/daqc1pdu1/energy5", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL20{
    "/qct/daqc/daqc1pdu1/energy6", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL21{
    "/qct/daqc/daqc1pdu1/energy7", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL22{
    "/qct/daqc/daqc1pdu1/energy8", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL23{
    "/qct/daqc/daqc1pdu1/energy9", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL24{
    "/qct/daqc/daqc1pdu1/energy10", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL25{
    "/qct/daqc/daqc1pdu1/energy11", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL26{
    "/qct/daqc/daqc1pdu1/energy12", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL27{
    "/qct/daqc/daqc1pdu1/energy13", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL28{
    "/qct/daqc/daqc1pdu1/energy14", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL29{
    "/qct/daqc/daqc1pdu1/energy15", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL30{
    "/qct/daqc/daqc1pdu1/energy16", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL31{
    "/qct/daqc/daqc1pdu1/energy17", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL32{
    "/qct/daqc/daqc1pdu1/energy18", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL33{
    "/qct/daqc/daqc1pdu1/energy19", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL34{
    "/qct/daqc/daqc1pdu1/energy20", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL35{
    "/qct/daqc/daqc1pdu1/energy21", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL36{
    "/qct/daqc/daqc1pdu1/energy22", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL37{
    "/qct/daqc/daqc1pdu1/energy23", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL38{
    "/qct/daqc/daqc1pdu1/energy24", "energy", std::chrono::duration<int>{60}};

const DCDB_QDMI_TelemetrySensor_impl_t TEL39{
    "/qic/warmlab/temperature", "temperature", std::chrono::duration<int>{60}};

const DCDB_QDMI_TelemetrySensor_impl_t TEL40{"/qic/warmlab/pressure", "pressure",
                                         std::chrono::duration<int>{60}};

const DCDB_QDMI_TelemetrySensor_impl_t TEL41{"/qic/warmlab/DAQC/heat/temp-fwd",
                                         "temperature",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL42{"/qic/warmlab/DAQC/heat/temp-ret",
                                         "temperature",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL43{"/qic/warmlab/UKG1/heat/temp-fwd",
                                         "temperature",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL44{"/qic/warmlab/UKG2/heat/temp-ret",
                                         "temperature",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL45{"/qic/warmlab/UKG1/heat/temp-fwd",
                                         "temperature",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL46{"/qic/warmlab/UKG2/heat/temp-ret",
                                         "temperature",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL47{
    "/qic/warmlab/DAQC/heat/energy", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL48{
    "/qic/warmlab/UKG1/heat/energy", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL49{
    "/qic/warmlab/UKG2/heat/energy", "energy", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL50{"/qic/warmlab/humidity", "humidity",
                                         std::chrono::duration<int>{60}};

const DCDB_QDMI_TelemetrySensor_impl_t TEL51{"/qct/daqc/daqc1pdu1/power1", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL52{"/qct/daqc/daqc1pdu1/power2", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL53{"/qct/daqc/daqc1pdu1/power3", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL54{"/qct/daqc/daqc1pdu1/power4", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL55{"/qct/daqc/daqc1pdu1/power5", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL56{"/qct/daqc/daqc1pdu1/power6", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL57{"/qct/daqc/daqc1pdu1/power7", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL58{"/qct/daqc/daqc1pdu1/power8", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL59{"/qct/daqc/daqc1pdu1/power9", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL60{"/qct/daqc/daqc1pdu1/power10", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL61{"/qct/daqc/daqc1pdu1/power11", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL62{"/qct/daqc/daqc1pdu1/power12", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL63{"/qct/daqc/daqc1pdu1/power13", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL64{"/qct/daqc/daqc1pdu1/power14", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL65{"/qct/daqc/daqc1pdu1/power15", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL66{"/qct/daqc/daqc1pdu1/power16", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL67{"/qct/daqc/daqc1pdu1/power17", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL68{"/qct/daqc/daqc1pdu1/power18", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL69{"/qct/daqc/daqc1pdu1/power19", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL70{"/qct/daqc/daqc1pdu1/power20", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL71{"/qct/daqc/daqc1pdu1/power21", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL72{"/qct/daqc/daqc1pdu1/power22", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL73{"/qct/daqc/daqc1pdu1/power23", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL74{"/qct/daqc/daqc1pdu1/power24", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL75{
    "/qct/daqc/daqc1pdu1/power_total", "power", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL76{"/qct/daqc/daqc1pdu1/energy_total",
                                         "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL77{"/qct/daqc/daqc1ups1/BatTimeRemaining",
                                         "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL78{"/qct/daqc/daqc1ups1/BatteryAbmStatus",
                                         "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL79{
    "/qct/daqc/daqc1ups1/BatteryAged", "power", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL80{"/qct/daqc/daqc1ups1/BatteryFailure",
                                         "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL81{
    "/qct/daqc/daqc1ups1/BatteryLowCapacity", "power",
    std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL82{
    "/qct/daqc/daqc1ups1/BatteryNotPresent", "power",
    std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL83{"/qct/daqc/daqc1ups1/OutputStatus",
                                         "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL84{"/qct/daqc/daqc1ups1/apparent_power",
                                         "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL85{
    "/qct/daqc/daqc1ups1/capacity", "power", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL86{"/qct/daqc/daqc1ups1/load", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL87{"/qct/daqc/daqc1ups1/power", "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL88{
    "/qct/daqc/qan01/stats/msgsSent", "power", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL89{
    "/qct/daqc/qan01/stats/readings", "power", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL90{"/qct/daqc/qan01/stats/readingsSent",
                                         "power",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL91{
    "/qic/warmlab/DAQC/heat/volume", "volume", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL92{
    "/qic/warmlab/UKG1/heat/volume", "volume", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL93{
    "/qic/warmlab/UKG2/heat/volume", "volume", std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL94{"/qic/warmlab/magnetometer/X",
                                         "magnetometer",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL95{"/qic/warmlab/magnetometer/Y",
                                         "magnetometer",
                                         std::chrono::duration<int>{60}};
const DCDB_QDMI_TelemetrySensor_impl_t TEL96{"/qic/warmlab/magnetometer/Z",
                                         "magnetometer",
                                         std::chrono::duration<int>{60}};
constexpr std::array<const DCDB_QDMI_TelemetrySensor_impl_t *, 97>
    DCDB_DEVICE_TELEMETRYSENSORS = {
        &TEL0,  &TEL1,  &TEL2,  &TEL3,  &TEL4,  &TEL5,  &TEL6,  &TEL7,  &TEL8,
        &TEL9,  &TEL10, &TEL11, &TEL12, &TEL13, &TEL14, &TEL15, &TEL16, &TEL17,
        &TEL18, &TEL19, &TEL20, &TEL21, &TEL22, &TEL23, &TEL24, &TEL25, &TEL26,
        &TEL27, &TEL28, &TEL29, &TEL30, &TEL31, &TEL32, &TEL33, &TEL34, &TEL35,
        &TEL36, &TEL37, &TEL38, &TEL39, &TEL40, &TEL41, &TEL42, &TEL43, &TEL44,
        &TEL45, &TEL46, &TEL47, &TEL48, &TEL49, &TEL50, &TEL51, &TEL52, &TEL53,
        &TEL54, &TEL55, &TEL56, &TEL57, &TEL58, &TEL59, &TEL60, &TEL61, &TEL62,
        &TEL63, &TEL64, &TEL65, &TEL66, &TEL67, &TEL68, &TEL69, &TEL70, &TEL71,
        &TEL72, &TEL73, &TEL74, &TEL75, &TEL76, &TEL77, &TEL78, &TEL79, &TEL80,
        &TEL81, &TEL82, &TEL83, &TEL84, &TEL85, &TEL86, &TEL87, &TEL88, &TEL89,
        &TEL90, &TEL91, &TEL92, &TEL93, &TEL94, &TEL95, &TEL96};
