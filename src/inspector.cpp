/**
 * Copyright (c) 2017  Raúl Nozal <raul.nozal@unican.es>
 * This file is part of clbalancer which is released under MIT License.
 * See file LICENSE for full license details.
 */
#include "inspector.hpp"

namespace clb {

void
Inspector::printActionTypeDuration(ActionType action, size_t d)
{
  switch (action) {
    case ActionType::init:
      cout << " init: " << d << " us.\n";
      break;
    case ActionType::useDiscovery:
      cout << " useDiscovery: " << d << " us.\n";
      break;
    case ActionType::initDiscovery:
      cout << " initDiscovery: " << d << " us.\n";
      break;
    case ActionType::initContext:
      cout << " initContext: " << d << " us.\n";
      break;
    case ActionType::initQueue:
      cout << " initQueue: " << d << " us.\n";
      break;
    case ActionType::initBuffers:
      cout << " initBuffers: " << d << " us.\n";
      break;
    case ActionType::initKernel:
      cout << " initKernel: " << d << " us.\n";
      break;
    case ActionType::writeBuffersDummy:
      cout << " writeBuffersDummy: " << d << " us.\n";
      break;
    case ActionType::writeBuffers:
      cout << " writeBuffers: " << d << " us.\n";
      break;
    case ActionType::deviceStart:
      cout << " deviceStart: " << d << " us.\n";
      break;
    case ActionType::schedulerStart:
      cout << " schedulerStart: " << d << " us.\n";
      break;
    case ActionType::deviceReady:
      cout << " deviceReady: " << d << " us.\n";
      break;
    case ActionType::deviceRun:
      cout << " deviceRun: " << d << " us.\n";
      break;
    case ActionType::completeWork:
      cout << " completeWork: " << d << " us.\n";
      break;
    case ActionType::deviceEnd:
      cout << " deviceEnd: " << d << " us.\n";
      break;
    case ActionType::schedulerEnd:
      cout << " schedulerEnd: " << d << " us.\n";
      break;
  }
}

} // namespace clb
