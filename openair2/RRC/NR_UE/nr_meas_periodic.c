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

#include "nr_meas_periodic.h"
#include "common/utils/LOG/log.h"

bool nr_meas_periodic_start(nr_meas_periodic_t *list, long measId, long reportConfigId, long reportAmount, int interval_ms)
{
  if (measId < 0 || measId >= NR_MEAS_PERIODIC_MAX) {
    LOG_E(NR_RRC, "periodical report: measId %ld out of range, not started\n", measId);
    return false;
  }
  if (interval_ms <= 0) {
    LOG_E(NR_RRC, "periodical report: invalid interval %d ms for measId %ld, not started\n", interval_ms, measId);
    return false;
  }
  nr_meas_periodic_t *e = &list[measId];
  e->active = true;
  e->measId = measId;
  e->reportConfigId = reportConfigId;
  e->reportAmount = reportAmount;
  e->reports_sent = 0;
  e->interval_ms = interval_ms;
  e->elapsed_frames = 0;
  nr_timer_setup(&e->timer, interval_ms, 10);
  return true;
}

bool nr_meas_periodic_report_sent(nr_meas_periodic_t *entry)
{
  entry->reports_sent++;
  bool infinite = entry->reportAmount == NR_MEAS_PERIODIC_INFINITE;
  if (infinite || entry->reports_sent < entry->reportAmount) {
    nr_timer_setup(&entry->timer, entry->interval_ms, 10);
    nr_timer_start(&entry->timer);
    return true;
  }
  nr_timer_stop(&entry->timer);
  entry->active = false;
  return false;
}

bool nr_meas_periodic_tick(nr_meas_periodic_t *entry)
{
  if (!entry->active)
    return false;
  entry->elapsed_frames++;
  return nr_timer_tick(&entry->timer);
}

static bool reset_entry(nr_meas_periodic_t *e)
{
  bool was_active = e->active;
  nr_timer_stop(&e->timer);
  e->active = false;
  e->reports_sent = 0;
  return was_active;
}

bool nr_meas_periodic_reset(nr_meas_periodic_t *list, long measId)
{
  if (measId < 0 || measId >= NR_MEAS_PERIODIC_MAX)
    return false;
  return reset_entry(&list[measId]);
}

int nr_meas_periodic_reset_all(nr_meas_periodic_t *list)
{
  int n = 0;
  for (int i = 0; i < NR_MEAS_PERIODIC_MAX; i++)
    n += reset_entry(&list[i]);
  return n;
}
