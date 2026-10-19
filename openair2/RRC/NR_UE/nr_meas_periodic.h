/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/* \file nr_meas_periodic.h
 * \brief per-measId state of periodical measurement reporting (VarMeasReportList, TS 38.331 5.5.4)
 */

#ifndef NR_MEAS_PERIODIC_H
#define NR_MEAS_PERIODIC_H

#include <stdbool.h>
#include <stdint.h>
#include "common/utils/nr/nr_common.h"

/* maxNrofMeasId */
#define NR_MEAS_PERIODIC_MAX 64
/* reportAmount value used for "infinity" */
#define NR_MEAS_PERIODIC_INFINITE 0

typedef struct {
  bool active;
  long measId;
  long reportConfigId;
  long reportAmount; // 1, 2, 4, 8, 16, 32, 64 or NR_MEAS_PERIODIC_INFINITE
  int reports_sent;
  int interval_ms;
  uint32_t elapsed_frames; // frames since the start of reporting, only logged
  NR_timer_t timer;
} nr_meas_periodic_t;

/* Activate reporting for measId. Does not start the timer: the first report is sent at once by the caller. */
bool nr_meas_periodic_start(nr_meas_periodic_t *list, long measId, long reportConfigId, long reportAmount, int interval_ms);
/* Count a sent report. Returns true if more reports follow (timer started), false if reporting is finished (entry deactivated). */
bool nr_meas_periodic_report_sent(nr_meas_periodic_t *entry);
/* Advance one 10 ms step. Returns true if the report timer expired. */
bool nr_meas_periodic_tick(nr_meas_periodic_t *entry);
/* Deactivate the entry of measId, stop the timer and reset reports_sent. Returns true if it was active. */
bool nr_meas_periodic_reset(nr_meas_periodic_t *list, long measId);
/* Same for all entries. Returns the number of entries that were active. */
int nr_meas_periodic_reset_all(nr_meas_periodic_t *list);

#endif
