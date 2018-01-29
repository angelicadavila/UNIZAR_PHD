/**
 * Copyright (c) 2017  Raúl Nozal <raul.nozal@unican.es>
 * This file is part of clbalancer which is released under MIT License.
 * See file LICENSE for full license details.
 */
#ifndef CLBALANCER_INSPECTOR_HPP
#define CLBALANCER_INSPECTOR_HPP 1

#include <iostream>

using std::cout;

namespace clb {

enum class ActionType
{
  initQueue = 0,
  initBuffers = 1,
  writeBuffersDummy = 2,
  initKernel = 3,
  writeBuffers = 4,
  deviceStart = 5,
  deviceReady = 6,
  deviceRun = 7,
  completeWork = 8,
  deviceEnd = 9,

  initDiscovery = 10,
  initContext = 11,
  useDiscovery = 12,
  init = 13,

  schedulerStart = 14,
  schedulerEnd = 15,
};

class Inspector
{
public:
  static void printActionTypeDuration(ActionType action, size_t d);
};

} // namespace clb

#endif /* CLBALANCER_INSPECTOR_HPP */
