#include "session.h"
#include "lrz.h"

void LRZ_QDMI_Device_Session_impl_d::setSites() {
  if (!sites.size())
    sites.clear();

  for (size_t index = 0; index < (*resource).getQubitCount(); index++) {
    sites.push_back(new LRZ_QDMI_Site_impl_d(index));
  }
}

std::vector<LRZ_QDMI_Site> LRZ_QDMI_Device_Session_impl_d::setSupportedSites(
    std::vector<std::vector<unsigned int>> supportedQubits) {
  std::vector<LRZ_QDMI_Site> supportedSites;
  for (auto qubitPair : supportedQubits) {
    for (auto qubit : qubitPair) {
      supportedSites.emplace_back((sites).at(qubit));
    }
  }
  return supportedSites;
}

void LRZ_QDMI_Device_Session_impl_d::setOperations() {
  if (!operations.size())
    operations.clear();
  size_t index = 0;
  for (const auto &gate : resource->getNativeGateset()) {
    operations.push_back(new LRZ_QDMI_Operation_impl_d(
        gate.getName(), gate.getQubitNumber(), gate.getParameterNumber(),
        setSupportedSites(gate.getSupportedQubits())));
  }
}

void LRZ_QDMI_Device_Session_impl_d::setCouplingMap() {
  if (!couplingMap.size())
    couplingMap.clear();
  for (auto qubitPair : (*resource).getCouplingMap()) {
    for (auto qubit : qubitPair) {
      couplingMap.emplace_back((sites).at(qubit));
    }
  }
}

void LRZ_QDMI_Device_Session_impl_d::setResource(
    const std::string &newResourceName) {
  resourceName = newResourceName;
  resource = client.getResourceInfo(resourceName);
  setSites();
  setOperations();
  setCouplingMap();
}

std::vector<std::string> LRZ_QDMI_Device_Session_impl_d::getAllResourceNames() {
  std::vector<std::string> resouceNames;
  std::vector<mqss::client::Resource> resources = client.getAllResources();
  for (mqss::client::Resource resourceInfo : resources) {
    resouceNames.push_back(resourceInfo.getName());
  }
  return resouceNames;
}
