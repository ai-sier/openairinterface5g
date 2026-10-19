add_executable(test_asn1_msg
  test_asn1_msg.cpp
  ${OPENAIR2_DIR}/LAYER2/nr_pdcp/asn1_utils.c
  ${OPENAIR2_DIR}/LAYER2/nr_rlc/asn1_utils.c)
target_link_libraries(test_asn1_msg PRIVATE nr_rrc SECURITY minimal_lib GTest::gtest UTIL)
add_dependencies(tests test_asn1_msg)
add_test(NAME test_asn1_msg
    COMMAND ./test_asn1_msg)

add_executable(test_nr_meas_periodical
  test_nr_meas_periodical.cpp
  ${OPENAIR2_DIR}/RRC/NR_UE/nr_meas_periodic.c
  ${OPENAIR2_DIR}/LAYER2/nr_pdcp/asn1_utils.c
  ${OPENAIR2_DIR}/LAYER2/nr_rlc/asn1_utils.c)
target_link_libraries(test_nr_meas_periodical PRIVATE nr_rrc SECURITY minimal_lib GTest::gtest UTIL)
add_dependencies(tests test_nr_meas_periodical)
add_test(NAME test_nr_meas_periodical
    COMMAND ./test_nr_meas_periodical)
