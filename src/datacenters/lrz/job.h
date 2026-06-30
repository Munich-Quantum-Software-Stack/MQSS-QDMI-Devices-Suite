#include "lrz_qdmi/device.h"
#include "session.h"
#include <mqss/job.h>

/**
 * @brief Implementation of the LRZ_QDMI_Device_Job structure.
 * @details This structure can, e.g., be used to store the job id.
 */
struct LRZ_QDMI_Device_Job_impl_d {
  LRZ_QDMI_Device_Session session = nullptr;
  QDMI_Job_Status status = QDMI_JOB_STATUS_SUBMITTED;

  std::string circuit;
  QDMI_Program_Format circuit_format = QDMI_PROGRAM_FORMAT_MAX;
  std::string resource;
  unsigned int shots;

  std::string uuid;

  std::unique_ptr<mqss::client::JobResult> job_result;
  mqss::client::CircuitJobRequest jobRequest;

  std::string histogram_keys;
  std::vector<size_t> histogram_values;
};
