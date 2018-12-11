// Copyright (C) 2015-2017 ETH Zurich and University of Bologna
// All rights reserved.
//
// This software may be modified and distributed under the terms
// of the BSD license.  See the LICENSE file for details.
//
// Germain Haugou (germain.haugou@gmail.com)
// Fabian Schuiki (fschuiki@iis.ee.ethz.ch)

#include "archi/pulp.h"

  .section .text
reset_handler:
  la x2, stack
  jal x1, main
  j loop

  .section .vectors, "ax"
  .option norvc;
  .org 0x0
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler
  jal x0, default_handler

  .org 0x80
  jal x0, reset_handler

  .org 0x84
loop:
  j loop

default_handler:
  mret
