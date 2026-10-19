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

#include <gtest/gtest.h>
extern "C" {
#include "openair2/RRC/NR/MESSAGES/asn1_msg.h"
#include "openair2/RRC/NR_UE/nr_meas_periodic.h"
#include "common/utils/nr/nr_common.h"
#include "common/ran_context.h"
#include "common/utils/LOG/log.h"
#include "NR_UL-DCCH-Message.h"
RAN_CONTEXT_t RC;
}

TEST(meas_periodical, rsrp_range)
{
  EXPECT_EQ(nr_rsrp_to_l3_range(-156.5), 0);
  EXPECT_EQ(nr_rsrp_to_l3_range(-156.0), 1);
  EXPECT_EQ(nr_rsrp_to_l3_range(-155.5), 1);
  EXPECT_EQ(nr_rsrp_to_l3_range(-31.0), 126);
  EXPECT_EQ(nr_rsrp_to_l3_range(-20.0), 126);
}

TEST(meas_periodical, rsrq_range)
{
  EXPECT_EQ(nr_rsrq_to_l3_range(-43.5), 0);
  EXPECT_EQ(nr_rsrq_to_l3_range(-43.0), 1);
  EXPECT_EQ(nr_rsrq_to_l3_range(-42.5), 2);
  EXPECT_EQ(nr_rsrq_to_l3_range(19.9), 126);
  EXPECT_EQ(nr_rsrq_to_l3_range(20.0), 127);
}

TEST(meas_periodical, filter_coefficient)
{
  const int k[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 13, 15, 17, 19};
  for (long fc = 0; fc < 15; fc++)
    EXPECT_NEAR(nr_l3_filter_coefficient(&fc), 1.0 / pow(2, k[fc] / 4.0), 1e-9);
  EXPECT_NEAR(nr_l3_filter_coefficient(NULL), 0.5, 1e-9);
  long bad = 15;
  EXPECT_NEAR(nr_l3_filter_coefficient(&bad), 0.5, 1e-9);
}

TEST(meas_periodical, beam_consolidation)
{
  nr_l3_beam_t beams[64] = {};
  double rsrp, rsrq;
  bool rsrq_valid;
  EXPECT_FALSE(nr_l3_consolidate_beams(beams, 64, 0, false, 0, &rsrp, &rsrq, &rsrq_valid));

  beams[3] = {true, -90, -10.0, true};
  EXPECT_TRUE(nr_l3_consolidate_beams(beams, 64, 0, false, 0, &rsrp, &rsrq, &rsrq_valid));
  EXPECT_EQ(rsrp, -90);
  EXPECT_TRUE(rsrq_valid);
  EXPECT_DOUBLE_EQ(rsrq, -10.0);

  beams[5] = {true, -80, -12.0, true};
  beams[7] = {true, -100, -15.0, true};
  // no averaging configured: highest beam
  EXPECT_TRUE(nr_l3_consolidate_beams(beams, 64, 0, false, 0, &rsrp, &rsrq, &rsrq_valid));
  EXPECT_EQ(rsrp, -80);
  // average of the beams at or above -95 dBm, at most 2: the beam at -100 is excluded
  EXPECT_TRUE(nr_l3_consolidate_beams(beams, 64, 2, true, -95, &rsrp, &rsrq, &rsrq_valid));
  double expected = 10 * log10((pow(10, -8.0) + pow(10, -9.0)) / 2);
  EXPECT_NEAR(rsrp, expected, 1e-9);
  // no beam reaches the threshold: highest beam
  EXPECT_TRUE(nr_l3_consolidate_beams(beams, 64, 2, true, -70, &rsrp, &rsrq, &rsrq_valid));
  EXPECT_EQ(rsrp, -80);
  // RSRQ not valid on the selected beam
  nr_l3_beam_t one[1] = {{true, -70, 0.0, false}};
  EXPECT_TRUE(nr_l3_consolidate_beams(one, 1, 0, false, 0, &rsrp, &rsrq, &rsrq_valid));
  EXPECT_FALSE(rsrq_valid);
}

TEST(meas_periodical, encode_decode)
{
  uint8_t buf[256];
  int size = do_nrMeasurementReport_periodical(1, 0, 60, 30, true, buf, sizeof(buf));
  ASSERT_GT(size, 0);
  NR_UL_DCCH_Message_t *msg = NULL;
  asn_dec_rval_t r = uper_decode(NULL, &asn_DEF_NR_UL_DCCH_Message, (void **)&msg, buf, size, 0, 0);
  ASSERT_EQ(r.code, RC_OK);
  ASSERT_EQ(msg->message.present, NR_UL_DCCH_MessageType_PR_c1);
  NR_MeasurementReport_IEs_t *ie = msg->message.choice.c1->choice.measurementReport->criticalExtensions.choice.measurementReport;
  EXPECT_EQ(ie->measResults.measId, 1);
  ASSERT_EQ(ie->measResults.measResultServingMOList.list.count, 1);
  NR_MeasResultServMO_t *serv = ie->measResults.measResultServingMOList.list.array[0];
  EXPECT_EQ(serv->servCellId, 0);
  NR_MeasResultNR_t *nr = &serv->measResultServingCell;
  ASSERT_NE(nr->measResult.cellResults.resultsSSB_Cell, nullptr);
  ASSERT_NE(nr->measResult.cellResults.resultsSSB_Cell->rsrp, nullptr);
  EXPECT_EQ(*nr->measResult.cellResults.resultsSSB_Cell->rsrp, 60);
  ASSERT_NE(nr->measResult.cellResults.resultsSSB_Cell->rsrq, nullptr);
  EXPECT_EQ(*nr->measResult.cellResults.resultsSSB_Cell->rsrq, 30);
  EXPECT_EQ(nr->measResult.cellResults.resultsSSB_Cell->sinr, nullptr);
  EXPECT_EQ(nr->measResult.rsIndexResults, nullptr);
  EXPECT_EQ(ie->measResults.measResultNeighCells, nullptr);
  ASN_STRUCT_FREE(asn_DEF_NR_UL_DCCH_Message, msg);

  size = do_nrMeasurementReport_periodical(2, 0, 60, 0, false, buf, sizeof(buf));
  ASSERT_GT(size, 0);
  msg = NULL;
  r = uper_decode(NULL, &asn_DEF_NR_UL_DCCH_Message, (void **)&msg, buf, size, 0, 0);
  ASSERT_EQ(r.code, RC_OK);
  ie = msg->message.choice.c1->choice.measurementReport->criticalExtensions.choice.measurementReport;
  EXPECT_EQ(ie->measResults.measResultServingMOList.list.array[0]->measResultServingCell.measResult.cellResults.resultsSSB_Cell->rsrq,
            nullptr);
  ASN_STRUCT_FREE(asn_DEF_NR_UL_DCCH_Message, msg);
}

// advance 10 ms steps until the timer expires; returns the number of steps
static int run_until_expiry(nr_meas_periodic_t *e, int max_steps)
{
  for (int i = 1; i <= max_steps; i++)
    if (nr_meas_periodic_tick(e))
      return i;
  return -1;
}

TEST(meas_periodical, r8_sends_eight_then_stops)
{
  nr_meas_periodic_t list[NR_MEAS_PERIODIC_MAX] = {};
  ASSERT_TRUE(nr_meas_periodic_start(list, 1, 1, 8, 10240));
  nr_meas_periodic_t *e = &list[1];
  EXPECT_TRUE(e->active);
  EXPECT_EQ(e->reports_sent, 0);
  for (int n = 1; n <= 7; n++) {
    EXPECT_TRUE(nr_meas_periodic_report_sent(e)) << n;
    EXPECT_EQ(e->reports_sent, n);
    EXPECT_EQ(run_until_expiry(e, 2000), 1024);
  }
  EXPECT_FALSE(nr_meas_periodic_report_sent(e));
  EXPECT_EQ(e->reports_sent, 8);
  EXPECT_FALSE(e->active);
  EXPECT_FALSE(nr_meas_periodic_tick(e));
  EXPECT_EQ(run_until_expiry(e, 4000), -1);
}

TEST(meas_periodical, r1_does_not_start_timer)
{
  nr_meas_periodic_t list[NR_MEAS_PERIODIC_MAX] = {};
  ASSERT_TRUE(nr_meas_periodic_start(list, 2, 1, 1, 1024));
  EXPECT_FALSE(nr_meas_periodic_report_sent(&list[2]));
  EXPECT_FALSE(list[2].active);
  EXPECT_EQ(run_until_expiry(&list[2], 1000), -1);
}

TEST(meas_periodical, infinity_continues)
{
  nr_meas_periodic_t list[NR_MEAS_PERIODIC_MAX] = {};
  ASSERT_TRUE(nr_meas_periodic_start(list, 3, 1, NR_MEAS_PERIODIC_INFINITE, 240));
  for (int n = 1; n <= 100; n++) {
    EXPECT_TRUE(nr_meas_periodic_report_sent(&list[3]));
    EXPECT_EQ(run_until_expiry(&list[3], 100), 24);
  }
  EXPECT_TRUE(list[3].active);
  EXPECT_EQ(list[3].reports_sent, 100);
}

TEST(meas_periodical, elapsed_frames_counts_active_only)
{
  nr_meas_periodic_t list[NR_MEAS_PERIODIC_MAX] = {};
  ASSERT_TRUE(nr_meas_periodic_start(list, 1, 1, 8, 10240));
  EXPECT_EQ(list[1].elapsed_frames, 0u);
  nr_meas_periodic_report_sent(&list[1]);
  for (int i = 0; i < 5; i++)
    nr_meas_periodic_tick(&list[1]);
  EXPECT_EQ(list[1].elapsed_frames, 5u);
  EXPECT_FALSE(nr_meas_periodic_tick(&list[4])); // inactive entry
  EXPECT_EQ(list[4].elapsed_frames, 0u);
}

TEST(meas_periodical, reset_and_idle)
{
  nr_meas_periodic_t list[NR_MEAS_PERIODIC_MAX] = {};
  ASSERT_TRUE(nr_meas_periodic_start(list, 1, 1, 8, 1024));
  ASSERT_TRUE(nr_meas_periodic_start(list, 5, 1, 8, 1024));
  nr_meas_periodic_report_sent(&list[1]);
  EXPECT_TRUE(nr_meas_periodic_reset(list, 1));
  EXPECT_FALSE(list[1].active);
  EXPECT_EQ(list[1].reports_sent, 0);
  EXPECT_FALSE(nr_meas_periodic_reset(list, 1)); // nothing was active
  EXPECT_FALSE(nr_meas_periodic_reset(list, NR_MEAS_PERIODIC_MAX));
  EXPECT_EQ(nr_meas_periodic_reset_all(list), 1);
  EXPECT_FALSE(list[5].active);
  EXPECT_EQ(nr_meas_periodic_reset_all(list), 0);
}

TEST(meas_periodical, invalid_start)
{
  nr_meas_periodic_t list[NR_MEAS_PERIODIC_MAX] = {};
  EXPECT_FALSE(nr_meas_periodic_start(list, NR_MEAS_PERIODIC_MAX, 1, 8, 1024));
  EXPECT_FALSE(nr_meas_periodic_start(list, 1, 1, 8, 0));
  EXPECT_FALSE(list[1].active);
}

int main(int argc, char **argv)
{
  logInit();
  g_log->log_component[NR_RRC].level = OAILOG_ERR;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
