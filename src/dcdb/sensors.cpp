
#include "sensors.h"
#include <dcdb/sensor.h>
#include <ostream>
#include <unistd.h>

std::list<DCDB::SensorDataStoreReading>
DCDB_QDMI_Environment_impl_t::query(DCDB::Connection *connection,
                                    uint64_t start, uint64_t end) {

  std::list<DCDB::SensorDataStoreReading> readings;
  DCDB::TimeStamp start_ts, end_ts;
  start_ts = DCDB::TimeStamp(start);
  end_ts = DCDB::TimeStamp(end);
  DCDB::Sensor s1(connection, id);

  s1.query(readings, start_ts, end_ts);
  return readings;
}