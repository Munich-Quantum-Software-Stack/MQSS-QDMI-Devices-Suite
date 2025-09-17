/*------------------------------------------------------------------------------
Copyright 2024 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"}; you may not use this file except in compliance with the License.
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
   * @brief This function is used to query the telemetry data from the DCDB.
   * @details This function queries telemetryal data in a given time interval
   * using DCDB's sensor interface.
   * @param[in] connection The connection object that used to connect DCDB host.
   * @param[in] start The start time as an UNIX timestamp of the time interval
   * @param[in] end The end time as an UNIX timestamp of the time interval
   * @return The reading from the given sensor within the siven time intervals
   */
  std::list<DCDB::SensorDataStoreReading> query(DCDB::Connection *connection,
                                                uint64_t start, uint64_t end);
} DCDB_QDMI_TelemetrySensor_impl_t;
/// The temperature of the mixing chamber in the QExa cryostat
const DCDB_QDMI_TelemetrySensor_impl_t TEL0{"/qct/q-exa/bluefors/tmixing", "uK",
                                            std::chrono::duration<float>(60)};
/// The temperature of the mixing chamber in the Daqc cryostat
const DCDB_QDMI_TelemetrySensor_impl_t TEL1{"/qct/daqc/bluefors/tmixing", "uK",
                                            std::chrono::duration<float>(60)};

/// The temperature of the 4 Kelvin chamber in the QExa cryostat
const DCDB_QDMI_TelemetrySensor_impl_t TEL2{"/qct/q-exa/bluefors/t4k", "K",
                                            std::chrono::duration<float>(60)};
/// The temperature of the 4 Kelvin chamber in the Daqc cryostat
const DCDB_QDMI_TelemetrySensor_impl_t TEL3{"/qct/daqc/bluefors/t4k", "K",
                                            std::chrono::duration<float>(60)};
/// The temperature of the 50 Kelvin chamber in the QExa cryostat
const DCDB_QDMI_TelemetrySensor_impl_t TEL4{"/qct/q-exa/bluefors/t50k", "K",
                                            std::chrono::duration<float>(60)};
/// The temperature of the 50 Kelvin chamber in the Daqc cryostat
const DCDB_QDMI_TelemetrySensor_impl_t TEL5{"/qct/daqc/bluefors/t50k", "K",
                                            std::chrono::duration<float>(60)};
/// TODO
const DCDB_QDMI_TelemetrySensor_impl_t TEL6{"/qct/q-exa/bluefors/tstill", "K",
                                            std::chrono::duration<float>(60)};
/// TODO
const DCDB_QDMI_TelemetrySensor_impl_t TEL7{"/qct/daqc/bluefors/tstill", "K",
                                            std::chrono::duration<float>(60)};

/// The vacuum can pressure in the Daqc cryostat - 1
const DCDB_QDMI_TelemetrySensor_impl_t TEL8{"/qct/daqc/bluefors/p1", "Pa",
                                            std::chrono::duration<float>(60)};
/// The vacuum can pressure in the Qexa cryostat - 1
const DCDB_QDMI_TelemetrySensor_impl_t TEL9{"/qct/q-exa/bluefors/p1", "Pa",
                                            std::chrono::duration<float>(60)};
/// The vacuum can pressure in the Daqc cryostat - 2
const DCDB_QDMI_TelemetrySensor_impl_t TEL10{"/qct/daqc/bluefors/p2", "Pa",
                                             std::chrono::duration<float>(60)};
/// The vacuum can pressure in the Qexa cryostat - 2
const DCDB_QDMI_TelemetrySensor_impl_t TEL11{"/qct/q-exa/bluefors/p2", "Pa",
                                             std::chrono::duration<float>(60)};
/// The vacuum can pressure in the Daqc cryostat - 3
const DCDB_QDMI_TelemetrySensor_impl_t TEL12{"/qct/daqc/bluefors/p3", "Pa",
                                             std::chrono::duration<float>(60)};
/// The vacuum can pressure in the Qexa cryostat - 3
const DCDB_QDMI_TelemetrySensor_impl_t TEL13{"/qct/q-exa/bluefors/p3", "Pa",
                                             std::chrono::duration<float>(60)};
/// The vacuum can pressure in the Daqc cryostat - 4
const DCDB_QDMI_TelemetrySensor_impl_t TEL14{"/qct/daqc/bluefors/p4", "Pa",
                                             std::chrono::duration<float>(60)};
/// The vacuum can pressure in the Qexa cryostat - 4
const DCDB_QDMI_TelemetrySensor_impl_t TEL15{"/qct/q-exa/bluefors/p4", "Pa",
                                             std::chrono::duration<float>(60)};
/// The vacuum can pressure in the Daqc cryostat - 5
const DCDB_QDMI_TelemetrySensor_impl_t TEL16{"/qct/daqc/bluefors/p5", "Pa",
                                             std::chrono::duration<float>(60)};
/// The vacuum can pressure in the Qexa cryostat - 5
const DCDB_QDMI_TelemetrySensor_impl_t TEL17{"/qct/q-exa/bluefors/p5", "Pa",
                                             std::chrono::duration<float>(60)};
/// The vacuum can pressure in the Daqc cryostat - 6
const DCDB_QDMI_TelemetrySensor_impl_t TEL18{"/qct/daqc/bluefors/p6", "Pa",
                                             std::chrono::duration<float>(60)};
/// The vacuum can pressure in the Qexa cryostat - 6
const DCDB_QDMI_TelemetrySensor_impl_t TEL19{"/qct/q-exa/bluefors/p6", "Pa",
                                             std::chrono::duration<float>(60)};

/// The sunlight intensity in the Warmlab
const DCDB_QDMI_TelemetrySensor_impl_t TEL20{"/qic/warmlab/light", "lux",
                                             std::chrono::duration<float>(60)};
/// The sunlight intensity in the Coldlab
const DCDB_QDMI_TelemetrySensor_impl_t TEL21{"/qic/coldlab/light", "lux",
                                             std::chrono::duration<float>(60)};

/// The loudness in the Warmlab
const DCDB_QDMI_TelemetrySensor_impl_t TEL22{"/qic/warmlab/loudness", "dB",
                                             std::chrono::duration<float>(60)};

/// The high frequency microphone in the Coldlab
const DCDB_QDMI_TelemetrySensor_impl_t TEL23{
    "/qic/coldlab/MR1/microphone", "dB", std::chrono::duration<float>(2e-05)};

/// The high frequency seismometer in the Coldlab - x
const DCDB_QDMI_TelemetrySensor_impl_t TEL24{
    "/qic/coldlab/seismometer/X", "ug", std::chrono::duration<float>(0.00002)};
/// The high frequency seismometer in the Coldlab - Y
const DCDB_QDMI_TelemetrySensor_impl_t TEL25{
    "/qic/coldlab/seismometer/Y", "ug", std::chrono::duration<float>(0.00002)};
/// The high frequency seismometer in the Coldlab - Z
const DCDB_QDMI_TelemetrySensor_impl_t TEL26{
    "/qic/coldlab/seismometer/Z", "ug", std::chrono::duration<float>(0.00002)};

/// The high frequency magnetometer in the Coldlab - x
const DCDB_QDMI_TelemetrySensor_impl_t TEL27{
    "/qic/coldlab/magnetometer/X", "nT", std::chrono::duration<float>(0.00002)};
/// The high frequency magnetometer in the Coldlab - Y
const DCDB_QDMI_TelemetrySensor_impl_t TEL28{
    "/qic/coldlab/magnetometer/Y", "nT", std::chrono::duration<float>(0.00002)};
/// The high frequency magnetometer in the Coldlab - Z
const DCDB_QDMI_TelemetrySensor_impl_t TEL29{
    "/qic/coldlab/magnetometer/Z", "nT", std::chrono::duration<float>(0.00002)};

/// The pressure in the Warmlab
const DCDB_QDMI_TelemetrySensor_impl_t TEL30{"/qic/warmlab/pressure", "Pa",
                                             std::chrono::duration<float>(60)};
/// The pressure in the Coldlab
const DCDB_QDMI_TelemetrySensor_impl_t TEL31{"/qic/coldlab/pressure", "Pa",
                                             std::chrono::duration<float>(60)};

/// The maximum parameter of the high frequency microphone in the Coldlab
const DCDB_QDMI_TelemetrySensor_impl_t TEL32{
    "/qic/coldlab/MR1/microphone_max", "dB", std::chrono::duration<float>(1)};
/// The minimum parameter of the high frequency microphone in the Coldlab
const DCDB_QDMI_TelemetrySensor_impl_t TEL33{
    "/qic/coldlab/MR1/microphone_min", "dB", std::chrono::duration<float>(1)};

/// The maximum parameter of the high frequency seismometer in the Coldlab - X
const DCDB_QDMI_TelemetrySensor_impl_t TEL34{
    "/qic/coldlab/seismometer/max/X", "ug", std::chrono::duration<float>(1)};
/// The maximum parameter of the high frequency seismometer in the Coldlab - Y
const DCDB_QDMI_TelemetrySensor_impl_t TEL35{
    "/qic/coldlab/seismometer/max/Y", "ug", std::chrono::duration<float>(1)};
/// The maximum parameter of the high frequency seismometer in the Coldlab - Z
const DCDB_QDMI_TelemetrySensor_impl_t TEL36{
    "/qic/coldlab/seismometer/max/Z", "ug", std::chrono::duration<float>(1)};
/// The minimum parameter of the high frequency seismometer in the Coldlab - X
const DCDB_QDMI_TelemetrySensor_impl_t TEL37{
    "/qic/coldlab/seismometer/min/X", "ug", std::chrono::duration<float>(1)};
/// The minimum parameter of the high frequency seismometer in the Coldlab - Y
const DCDB_QDMI_TelemetrySensor_impl_t TEL38{
    "/qic/coldlab/seismometer/min/Y", "ug", std::chrono::duration<float>(1)};
/// The minimum parameter of the high frequency seismometer in the Coldlab - Z
const DCDB_QDMI_TelemetrySensor_impl_t TEL39{
    "/qic/coldlab/seismometer/min/Z", "ug", std::chrono::duration<float>(1)};

/// The maximum parameter of the high frequency magnetometer in the Coldlab - X
const DCDB_QDMI_TelemetrySensor_impl_t TEL40{
    "/qic/coldlab/magnetometer/stats/max/X", "nT",
    std::chrono::duration<float>(1)};
/// The maximum parameter of the high frequency magnetometer in the Coldlab - Y
const DCDB_QDMI_TelemetrySensor_impl_t TEL41{
    "/qic/coldlab/magnetometer/stats/max/Y", "nT",
    std::chrono::duration<float>(1)};
/// The maximum parameter of the high frequency magnetometer in the Coldlab - Z
const DCDB_QDMI_TelemetrySensor_impl_t TEL42{
    "/qic/coldlab/magnetometer/stats/max/Z", "nT",
    std::chrono::duration<float>(1)};
/// The minimum parameter of the high frequency magnetometer in the Coldlab - X
const DCDB_QDMI_TelemetrySensor_impl_t TEL43{
    "/qic/coldlab/magnetometer/stats/min/X", "nT",
    std::chrono::duration<float>(1)};
/// The minimum parameter of the high frequency magnetometer in the Coldlab - Y
const DCDB_QDMI_TelemetrySensor_impl_t TEL44{
    "/qic/coldlab/magnetometer/stats/min/Y", "nT",
    std::chrono::duration<float>(1)};
/// The minimum parameter of the high frequency magnetometer in the Coldlab - Z
const DCDB_QDMI_TelemetrySensor_impl_t TEL45{
    "/qic/coldlab/magnetometer/stats/min/Z", "nT",
    std::chrono::duration<float>(1)};

/// The magnetometer in the Warmlab - X
const DCDB_QDMI_TelemetrySensor_impl_t TEL46{
    "/qic/warmlab/magnetometer/X", "nT", std::chrono::duration<float>(60)};
/// The magnetometer in the Warmlab - Y
const DCDB_QDMI_TelemetrySensor_impl_t TEL47{
    "/qic/warmlab/magnetometer/Y", "nT", std::chrono::duration<float>(60)};
/// The magnetometer in the Warmlab - Z
const DCDB_QDMI_TelemetrySensor_impl_t TEL48{
    "/qic/warmlab/magnetometer/Z", "nT", std::chrono::duration<float>(60)};

/// The humidity in the Coldlab
const DCDB_QDMI_TelemetrySensor_impl_t TEL49{
    "/qic/coldlab/humidity", "humidity", std::chrono::duration<float>(60)};
/// The humidity in the Warmlab
const DCDB_QDMI_TelemetrySensor_impl_t TEL50{
    "/qic/warmlab/humidity", "humidity", std::chrono::duration<float>(60)};

/// The temperature in the Warmlab
const DCDB_QDMI_TelemetrySensor_impl_t TEL51{"/qic/warmlab/temperature", "K",
                                             std::chrono::duration<float>(60)};
/// The temperature in the Coldlab
const DCDB_QDMI_TelemetrySensor_impl_t TEL52{"/qic/coldlab/temperature", "K",
                                             std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 28FC45720E000071
const DCDB_QDMI_TelemetrySensor_impl_t TEL53{
    "/qic/coldlab/temperature/28FC45720E000071", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 280105730E00002F
const DCDB_QDMI_TelemetrySensor_impl_t TEL54{
    "/qic/coldlab/temperature/280105730E00002F", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 280D73740E000025
const DCDB_QDMI_TelemetrySensor_impl_t TEL55{
    "/qic/coldlab/temperature/280D73740E000025", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 28154A740E0000A8
const DCDB_QDMI_TelemetrySensor_impl_t TEL56{
    "/qic/coldlab/temperature/28154A740E0000A8", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 28169F720E000074
const DCDB_QDMI_TelemetrySensor_impl_t TEL57{
    "/qic/coldlab/temperature/28169F720E000074", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 281843740E000011
const DCDB_QDMI_TelemetrySensor_impl_t TEL58{
    "/qic/coldlab/temperature/281843740E000011", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 281AF5730E00002A
const DCDB_QDMI_TelemetrySensor_impl_t TEL59{
    "/qic/coldlab/temperature/281AF5730E00002A", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 282E54720E00006A
const DCDB_QDMI_TelemetrySensor_impl_t TEL60{
    "/qic/coldlab/temperature/282E54720E00006A", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 2836E1730E000082
const DCDB_QDMI_TelemetrySensor_impl_t TEL61{
    "/qic/coldlab/temperature/2836E1730E000082", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 28428F740E0000F3
const DCDB_QDMI_TelemetrySensor_impl_t TEL62{
    "/qic/coldlab/temperature/28428F740E0000F3", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 28464D720E000087
const DCDB_QDMI_TelemetrySensor_impl_t TEL63{
    "/qic/coldlab/temperature/28464D720E000087", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 28479D720E0000EE
const DCDB_QDMI_TelemetrySensor_impl_t TEL64{
    "/qic/coldlab/temperature/28479D720E0000EE", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 284C8D720E00006A
const DCDB_QDMI_TelemetrySensor_impl_t TEL65{
    "/qic/coldlab/temperature/284C8D720E00006A", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 285502740E00000A
const DCDB_QDMI_TelemetrySensor_impl_t TEL66{
    "/qic/coldlab/temperature/285502740E00000A", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 285598730E000086
const DCDB_QDMI_TelemetrySensor_impl_t TEL67{
    "/qic/coldlab/temperature/285598730E000086", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 2858E4720E00000F
const DCDB_QDMI_TelemetrySensor_impl_t TEL68{
    "/qic/coldlab/temperature/2858E4720E00000F", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 285920740E00000C
const DCDB_QDMI_TelemetrySensor_impl_t TEL69{
    "/qic/coldlab/temperature/285920740E00000C", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 2859DB730E00005C
const DCDB_QDMI_TelemetrySensor_impl_t TEL70{
    "/qic/coldlab/temperature/2859DB730E00005C", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 286230720E00006C
const DCDB_QDMI_TelemetrySensor_impl_t TEL71{
    "/qic/coldlab/temperature/286230720E00006C", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 287805730E000021
const DCDB_QDMI_TelemetrySensor_impl_t TEL72{
    "/qic/coldlab/temperature/287805730E000021", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 287815740E0000DB
const DCDB_QDMI_TelemetrySensor_impl_t TEL73{
    "/qic/coldlab/temperature/287815740E0000DB", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 287A8E730E0000F4
const DCDB_QDMI_TelemetrySensor_impl_t TEL74{
    "/qic/coldlab/temperature/287A8E730E0000F4", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 288605730E000040
const DCDB_QDMI_TelemetrySensor_impl_t TEL75{
    "/qic/coldlab/temperature/288605730E000040", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 288CC2740E00007A
const DCDB_QDMI_TelemetrySensor_impl_t TEL76{
    "/qic/coldlab/temperature/288CC2740E00007A", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 289233740E000059
const DCDB_QDMI_TelemetrySensor_impl_t TEL77{
    "/qic/coldlab/temperature/289233740E000059", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 2892C0720E0000B8
const DCDB_QDMI_TelemetrySensor_impl_t TEL78{
    "/qic/coldlab/temperature/2892C0720E0000B8", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 289A65730E000077
const DCDB_QDMI_TelemetrySensor_impl_t TEL79{
    "/qic/coldlab/temperature/289A65730E000077", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 28B31C740E00004F
const DCDB_QDMI_TelemetrySensor_impl_t TEL80{
    "/qic/coldlab/temperature/28B31C740E00004F", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 28B8BB740E0000D5
const DCDB_QDMI_TelemetrySensor_impl_t TEL81{
    "/qic/coldlab/temperature/28B8BB740E0000D5", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 28BC52740E000020
const DCDB_QDMI_TelemetrySensor_impl_t TEL82{
    "/qic/coldlab/temperature/28BC52740E000020", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 28BCF6730E00008A
const DCDB_QDMI_TelemetrySensor_impl_t TEL83{
    "/qic/coldlab/temperature/28BCF6730E00008A", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 28D0A6740E000027
const DCDB_QDMI_TelemetrySensor_impl_t TEL84{
    "/qic/coldlab/temperature/28D0A6740E000027", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 28DA45730E0000FA
const DCDB_QDMI_TelemetrySensor_impl_t TEL85{
    "/qic/coldlab/temperature/28DA45730E0000FA", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 28EA6D740E000057
const DCDB_QDMI_TelemetrySensor_impl_t TEL86{
    "/qic/coldlab/temperature/28EA6D740E000057", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 28EB91730E000061
const DCDB_QDMI_TelemetrySensor_impl_t TEL87{
    "/qic/coldlab/temperature/28EB91730E000061", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 28EDGE3730E00003D
const DCDB_QDMI_TelemetrySensor_impl_t TEL88{
    "/qic/coldlab/temperature/28EDE3730E00003D", "K",
    std::chrono::duration<float>(60)};
/// The temperature in the Coldlab - 28F192730E0000BB
const DCDB_QDMI_TelemetrySensor_impl_t TEL89{
    "/qic/coldlab/temperature/28F192730E0000BB", "K",
    std::chrono::duration<float>(60)};

/// All the telemetry sensor array
constexpr std::array<const DCDB_QDMI_TelemetrySensor_impl_t *, 90>
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
        &TEL81, &TEL82, &TEL83, &TEL84, &TEL85, &TEL86, &TEL87, &TEL88, &TEL89};
