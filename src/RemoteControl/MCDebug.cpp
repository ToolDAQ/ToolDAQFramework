#include <zmq.hpp>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
//#include <snappy.h>
#include <zstd.h>
//#include <msgpack.hpp>

int main(){

 // zmq::context_t * context = args->context;

 std::string multicastaddress="239.192.1.1"; ///< Multicast address to send beacons
 int multicastport=5000; ///< Port to use for multicast beacons
  
  struct sockaddr_in addr;
  int addrlen, sock, cnt;
  struct ip_mreq mreq;
  char message[512];
  

  //std::string test="{\"sys_load\": \"7328/31104/53088\", \"brb_mag_x\": 0.041534423828125, \"brb_mag_y\": -0.469635009765625, \"brb_mag_z\": -0.2945556640625, \"brb_temp_0\": 20.125, \"brb_temp_1\": 17.125, \"brb_temp_2\": 15.5, \"pmt0_ctrl0\": 65534, \"pmt0_v5val\": 4.98542785644531, \"pmt1_ctrl0\": 65534, \"pmt1_v5val\": 4.940260887146, \"pmt2_ctrl0\": 65534, \"pmt2_v5val\": 4.95857191085815, \"pmt3_ctrl0\": 65534, \"pmt3_v5val\": 4.950026512146, \"pmt4_ctrl0\": 65534, \"pmt4_v5val\": 4.98298597335815, \"pmt5_ctrl0\": 65534, \"pmt5_v5val\": 4.948805809021, \"pmt6_ctrl0\": 65534, \"pmt6_v5val\": 4.936598777771, \"pmt7_ctrl0\": 65534, \"pmt7_v5val\": 4.946364402771, \"pmt8_ctrl0\": 65534, \"pmt8_v5val\": 4.937819480896, \"pmt9_ctrl0\": 65534, \"pmt9_v5val\": 4.942702293396, \"sys_memory\": \"1809285120/1021296640/237568/155648\", \"brb_bus_p_0\": 1.46749997138977, \"brb_bus_p_1\": 0.827499985694885, \"brb_bus_p_2\": 0.924999952316284, \"brb_bus_p_3\": 0.0775000005960464, \"brb_bus_p_4\": 0.0700000002980232, \"brb_bus_p_5\": 16.9074993133545, \"brb_bus_p_6\": 3.02250003814697, \"brb_bus_v_0\": 6.03600025177002, \"brb_bus_v_1\": 4.01000022888184, \"brb_bus_v_2\": 1.79500007629395, \"brb_bus_v_3\": 4.9830002784729, \"brb_bus_v_4\": 3.29700016975403, \"brb_bus_v_5\": 12.1920003890991, \"brb_bus_v_6\": 3.27600026130676, \"pmt10_ctrl0\": 65534, \"pmt10_v5val\": 4.947585105896, \"pmt11_ctrl0\": 65534, \"pmt11_v5val\": 4.98176527023315, \"pmt12_ctrl0\": 65534, \"pmt12_v5val\": 4.95735120773315, \"pmt13_ctrl0\": 65534, \"pmt13_v5val\": 4.952467918396, \"pmt14_ctrl0\": 65534, \"pmt14_v5val\": 4.95613050460815, \"pmt15_ctrl0\": 65534, \"pmt15_v5val\": 4.97932386398315, \"pmt16_ctrl0\": 65534, \"pmt16_v5val\": 4.925612449646, \"pmt17_ctrl0\": 65534, \"pmt17_v5val\": 4.98664855957031, \"pmt18_ctrl0\": 65534, \"pmt18_v5val\": 4.945143699646, \"brb_hum_temp\": 15.1174926757812, \"brb_humidity\": 39.874267578125, \"brb_pressure\": 876.358154296875, \"device_state\": \"Idle\", \"pmt0_hit_cnt\": 3302, \"pmt0_mcutemp\": 13, \"pmt0_status0\": 32768, \"pmt0_status1\": 3, \"pmt1_hit_cnt\": 3248, \"pmt1_mcutemp\": 12, \"pmt1_status0\": 32768, \"pmt1_status1\": 3, \"pmt2_hit_cnt\": 3211, \"pmt2_mcutemp\": 16, \"pmt2_status0\": 32768, \"pmt2_status1\": 3, \"pmt3_hit_cnt\": 3238, \"pmt3_mcutemp\": 13, \"pmt3_status0\": 32768, \"pmt3_status1\": 3, \"pmt4_hit_cnt\": 3077, \"pmt4_mcutemp\": 13, \"pmt4_status0\": 32768, \"pmt4_status1\": 3, \"pmt5_hit_cnt\": 2925, \"pmt5_mcutemp\": 13, \"pmt5_status0\": 32768, \"pmt5_status1\": 3, \"pmt6_hit_cnt\": 2524, \"pmt6_mcutemp\": 12, \"pmt6_status0\": 32768, \"pmt6_status1\": 3, \"pmt7_hit_cnt\": 3094, \"pmt7_mcutemp\": 14, \"pmt7_status0\": 32768, \"pmt7_status1\": 3, \"pmt8_hit_cnt\": 3027, \"pmt8_mcutemp\": 13, \"pmt8_status0\": 32768, \"pmt8_status1\": 3, \"pmt9_hit_cnt\": 3130, \"pmt9_mcutemp\": 14, \"pmt9_status0\": 32768, \"pmt9_status1\": 3, \"brb_fpga_temp\": 26.6507129669189, \"brb_shunt_v_0\": 0.00729249976575375, \"brb_shunt_v_1\": 0.00618749996647239, \"brb_shunt_v_2\": 0.0775749981403351, \"brb_shunt_v_3\": 0.0159374997019768, \"brb_shunt_v_4\": 0.00108499999623746, \"brb_shunt_v_5\": 0.0416224971413612, \"brb_shunt_v_6\": 0.0276999995112419, \"pmt0_hvcurval\": 1.25490200519562, \"pmt0_hvvolnom\": 1056.00830078125, \"pmt0_hvvolref\": 2.44190120697021, \"pmt0_hvvolval\": 1030.89953613281, \"pmt0_ramupspd\": 29.9992370605469, \"pmt0_triptime\": 2, \"pmt10_hit_cnt\": 3064, \"pmt10_mcutemp\": 14, \"pmt10_status0\": 32768, \"pmt10_status1\": 3, \"pmt11_hit_cnt\": 2780, \"pmt11_mcutemp\": 11, \"pmt11_status0\": 32768, \"pmt11_status1\": 3, \"pmt12_hit_cnt\": 3226, \"pmt12_mcutemp\": 13, \"pmt12_status0\": 32768, \"pmt12_status1\": 3, \"pmt13_hit_cnt\": 2908, \"pmt13_mcutemp\": 14, \"pmt13_status0\": 32768, \"pmt13_status1\": 3, \"pmt14_hit_cnt\": 2974, \"pmt14_mcutemp\": 11, \"pmt14_status0\": 32768, \"pmt14_status1\": 3, \"pmt15_hit_cnt\": 3259, \"pmt15_mcutemp\": 14, \"pmt15_status0\": 32768, \"pmt15_status1\": 3, \"pmt16_hit_cnt\": 3006, \"pmt16_mcutemp\": 12, \"pmt16_status0\": 32768, \"pmt16_status1\": 3, \"pmt17_hit_cnt\": 2894, \"pmt17_mcutemp\": 12, \"pmt17_status0\": 32768, \"pmt17_status1\": 3,\"pmt18_hit_cnt\": 3177, \"pmt18_mcutemp\": 12, \"pmt18_status0\": 32768, \"pmt18_status1\": 3, \"pmt19_hit_cnt\": 0, \"pmt1_hvcurval\": 1.14991986751556, \"pmt1_hvvolnom\": 957.99951171875, \"pmt1_hvvolref\": 2.44411373138428, \"pmt1_hvvolval\": 947.035949707031, \"pmt1_ramupspd\": 29.9992370605469, \"pmt1_triptime\": 2, \"pmt2_hvcurval\": 1.22560465335846, \"pmt2_hvvolnom\": 1026.00134277344, \"pmt2_hvvolref\": 2.43236446380615, \"pmt2_hvvolval\": 1010.75762939453, \"pmt2_ramupspd\": 29.9992370605469, \"pmt2_triptime\": 2, \"pmt3_hvcurval\": 1.16700994968414, \"pmt3_hvvolnom\": 991.004821777344, \"pmt3_hvvolref\": 2.43968868255615, \"pmt3_hvvolval\": 971.938659667969, \"pmt3_ramupspd\": 29.9992370605469, \"pmt3_triptime\": 2, \"pmt4_hvcurval\": 1.11085677146912, \"pmt4_hvvolnom\": 921.995849609375, \"pmt4_hvvolref\": 2.44045162200928, \"pmt4_hvvolval\": 920.302124023438, \"pmt4_ramupspd\": 29.9992370605469, \"pmt4_triptime\": 2, \"pmt5_hvcurval\": 1.25978481769562, \"pmt5_hvvolnom\": 1073.99865722656, \"pmt5_hvvolref\": 2.44045162200928, \"pmt5_hvvolval\": 1048.11169433594, \"pmt5_ramupspd\": 29.9992370605469, \"pmt5_triptime\": 2, \"pmt6_hvcurval\": 1.17677581310272, \"pmt6_hvvolnom\": 989.997680664062, \"pmt6_hvvolref\": 2.44045162200928, \"pmt6_hvvolval\": 974.135986328125, \"pmt6_ramupspd\": 29.9992370605469, \"pmt6_triptime\": 2, \"pmt7_hvcurval\": 1.1963073015213, \"pmt7_hvvolnom\": 998.008728027344, \"pmt7_hvvolref\": 2.43900203704834, \"pmt7_hvvolval\": 986.221130371094, \"pmt7_ramupspd\": 29.9992370605469, \"pmt7_triptime\": 2, \"pmt8_hvcurval\": 1.1938658952713, \"pmt8_hvvolnom\": 1013.00067138672, \"pmt8_hvvolref\": 2.44045162200928, \"pmt8_hvvolval\": 984.023803710938, \"pmt8_ramupspd\": 29.9992370605469, \"pmt8_triptime\": 2, \"pmt9_hvcurval\": 1.24757766723633, \"pmt9_hvvolnom\": 1049.00439453125, \"pmt9_hvvolref\": 2.43968868255615, \"pmt9_hvvolval\": 1031.63195800781, \"pmt9_ramupspd\": 29.9992370605469, \"pmt9_triptime\": 2, \"brb_clock_freq\": 124993208, \"brb_press_temp\": 14.1937503814697, \"pmt0_hv2volval\": 1041.56555175781, \"pmt0_hvcurrmax\": 6, \"pmt0_hvvolmarg\": 100, \"pmt0_mcufw_ver\": \"3.5.0\", \"pmt10_hvcurval\": 1.1963073015213, \"pmt10_hvvolnom\": 1011.00939941406, \"pmt10_hvvolref\": 2.43678951263428, \"pmt10_hvvolval\": 992.08056640625, \"pmt10_ramupspd\": 29.9992370605469, \"pmt10_triptime\": 2, \"pmt11_hvcurval\": 1.18654155731201, \"pmt11_hvvolnom\": 985.9921875, \"pmt11_hvvolref\": 2.43823909759521, \"pmt11_hvvolval\": 979.995422363281, \"pmt11_ramupspd\": 29.9992370605469, \"pmt11_triptime\": 2, \"pmt12_hvcurval\": 1.1938658952713, \"pmt12_hvvolnom\": 994.003234863281, \"pmt12_hvvolref\": 2.43602657318115, \"pmt12_hvvolval\": 989.517028808594, \"pmt12_ramupspd\": 29.9992370605469, \"pmt12_triptime\": 2, \"pmt13_hvcurval\": 1.20119023323059, \"pmt13_hvvolnom\": 1000, \"pmt13_hvvolref\": 2.43533992767334, \"pmt13_hvvolval\": 993.54541015625, \"pmt13_ramupspd\": 29.9992370605469, \"pmt13_triptime\": 2, \"pmt14_hvcurval\": 1.1914244890213, \"pmt14_hvvolnom\": 998.992919921875, \"pmt14_hvvolref\": 2.43236446380615, \"pmt14_hvvolval\": 986.953552246094, \"pmt14_ramupspd\": 29.9992370605469, \"pmt14_triptime\": 2, \"pmt15_hvcurval\": 1.25490200519562, \"pmt15_hvvolnom\": 1050.01147460938, \"pmt15_hvvolref\": 2.44113826751709, \"pmt15_hvvolval\": 1037.49145507812, \"pmt15_ramupspd\": 29.9992370605469, \"pmt15_triptime\": 2, \"pmt16_hvcurval\": 1.21095597743988, \"pmt16_hvvolnom\": 998.008728027344, \"pmt16_hvvolref\": 2.44556355476379, \"pmt16_hvvolval\": 995.376525878906,\"pmt16_ramupspd\": 29.9992370605469, \"pmt16_triptime\": 2, \"pmt17_hvcurval\": 1.25246047973633, \"pmt17_hvvolnom\": 1050.01147460938, \"pmt17_hvvolref\": 2.44045162200928, \"pmt17_hvvolval\": 1036.75903320312, \"pmt17_ramupspd\": 29.9992370605469, \"pmt17_triptime\": 2, \"pmt18_hvcurval\": 1.30373084545135, \"pmt18_hvvolnom\": 1081.00256347656, \"pmt18_hvvolref\": 2.43968868255615, \"pmt18_hvvolval\": 1079.97253417969, \"pmt18_ramupspd\": 29.9992370605469, \"pmt18_triptime\": 2, \"pmt1_hv2volval\": 959.327087402344, \"pmt1_hvcurrmax\": 6, \"pmt1_hvvolmarg\": 100, \"pmt1_mcufw_ver\": \"3.5.0\", \"pmt2_hv2volval\": 1018.08197021484, \"pmt2_hvcurrmax\": 6, \"pmt2_hvvolmarg\": 100, \"pmt2_mcufw_ver\": \"3.5.0\", \"pmt3_hv2volval\": 969.214904785156, \"pmt3_hvcurrmax\": 6, \"pmt3_hvvolmarg\": 100, \"pmt3_mcufw_ver\": \"3.5.0\", \"pmt4_hv2volval\": 916.159301757812, \"pmt4_hvcurrmax\": 6, \"pmt4_hvvolmarg\": 100, \"pmt4_mcufw_ver\": \"3.5.0\", \"pmt5_hv2volval\": 1049.14172363281, \"pmt5_hvcurrmax\": 6, \"pmt5_hvvolmarg\": 100, \"pmt5_mcufw_ver\": \"3.6.0\", \"pmt6_hv2volval\": 974.364868164062, \"pmt6_hvcurrmax\": 6, \"pmt6_hvvolmarg\": 100, \"pmt6_mcufw_ver\": \"3.5.0\", \"pmt7_hv2volval\": 997.184692382812, \"pmt7_hvcurrmax\": 6, \"pmt7_hvvolmarg\": 100, \"pmt7_mcufw_ver\": \"3.5.0\", \"pmt8_hv2volval\": 989.562805175781, \"pmt8_hvcurrmax\": 6, \"pmt8_hvvolmarg\": 100, \"pmt8_mcufw_ver\": \"3.5.0\", \"pmt9_hv2volval\": 1037.17102050781, \"pmt9_hvcurrmax\": 6, \"pmt9_hvvolmarg\": 100, \"pmt9_mcufw_ver\": \"3.5.0\", \"ntp_sync_status\": 1, \"pmt0_ramdownspd\": 200, \"pmt10_hv2volval\": 992.378112792969, \"pmt10_hvcurrmax\": 6, \"pmt10_hvvolmarg\": 100, \"pmt10_mcufw_ver\": \"3.5.0\", \"pmt11_hv2volval\": 981.57470703125, \"pmt11_hvcurrmax\": 6, \"pmt11_hvvolmarg\": 100, \"pmt11_mcufw_ver\":\"3.5.0\", \"pmt12_hv2volval\": 995.628295898438, \"pmt12_hvcurrmax\": 6, \"pmt12_hvvolmarg\": 100, \"pmt12_mcufw_ver\": \"3.5.0\", \"pmt13_hv2volval\": 993.408081054688, \"pmt13_hvcurrmax\": 6, \"pmt13_hvvolmarg\": 100, \"pmt13_mcufw_ver\": \"3.5.0\", \"pmt14_hv2volval\": 987.800415039062, \"pmt14_hvcurrmax\": 6, \"pmt14_hvvolmarg\": 100, \"pmt14_mcufw_ver\": \"3.5.0\", \"pmt15_hv2volval\": 1041.95471191406, \"pmt15_hvcurrmax\": 6, \"pmt15_hvvolmarg\": 100, \"pmt15_mcufw_ver\": \"3.6.0\", \"pmt16_hv2volval\": 1005.40167236328, \"pmt16_hvcurrmax\": 6, \"pmt16_hvvolmarg\": 100, \"pmt16_mcufw_ver\": \"3.5.0\", \"pmt17_hv2volval\": 1039.73449707031, \"pmt17_hvcurrmax\": 6, \"pmt17_hvvolmarg\": 100, \"pmt17_mcufw_ver\": \"3.5.0\", \"pmt18_hv2volval\": 1080.40747070312, \"pmt18_hvcurrmax\": 6, \"pmt18_hvvolmarg\": 100, \"pmt18_mcufw_ver\": \"3.5.0\", \"pmt1_ramdownspd\": 200, \"pmt2_ramdownspd\": 200, \"pmt3_ramdownspd\": 200, \"pmt4_ramdownspd\": 200, \"pmt5_ramdownspd\": 200, \"pmt6_ramdownspd\": 200, \"pmt7_ramdownspd\": 200, \"pmt8_ramdownspd\": 200, \"pmt9_ramdownspd\": 200, \"brb_leds_enabled\": 0, \"net_eth1_rxbytes\": 7941957526, \"net_eth1_txbytes\": 322325633335, \"net_timestamp_ms\": 1744215325902, \"pmt10_ramdownspd\": 200, \"pmt11_ramdownspd\": 200, \"pmt12_ramdownspd\": 200, \"pmt13_ramdownspd\": 200, \"pmt14_ramdownspd\": 200, \"pmt15_ramdownspd\": 200, \"pmt16_ramdownspd\": 200, \"pmt17_ramdownspd\": 200, \"pmt18_ramdownspd\": 200, \"brb_trigger_flags\": 9, \"modbus_crc_errors\": 0, \"brb_coarse_counter\": 1336957442424, \"brb_last_timestamp\": 1744215325, \"last_run_sent_high\": 7, \"modbus_no_response\": 15892, \"net_eth1_rxpackets\": 137996799, \"net_eth1_txpackets\": 228354232, \"ntp_report_message\": \"RefID=898a1245;Stratum=3;Leap=0;RefTime=1744215267.528483405;CurrCorrS=-1.24431e-05;LastOffsetS=8.22122e-06;RMSOffsetS=1.47568e-05;FreqOffsPPM=-3.67574;ResidFreqPPM=0.0024126;SkewPPM=0.0748111;RootDlyS=0.000980721;RootDispS=0.00109125;LastUpdIntv=64.4361;Sync=1\", \"brb_chan_0_pedestal\": 0, \"brb_chan_1_pedestal\": -20, \"brb_chan_2_pedestal\": 12, \"brb_chan_3_pedestal\": 0, \"brb_chan_4_pedestal\": 2, \"brb_chan_5_pedestal\": -9, \"brb_chan_6_pedestal\": 15, \"brb_chan_7_pedestal\": 0, \"brb_chan_8_pedestal\": -2, \"brb_chan_9_pedestal\": -11, \"last_run_bytes_sent\": 10382348336, \"last_run_queue_high\": 1, \"pmt0_modbus_crc_err\": 0, \"pmt1_modbus_crc_err\": 0, \"pmt2_modbus_crc_err\": 0, \"pmt3_modbus_crc_err\": 0, \"pmt4_modbus_crc_err\": 0, \"pmt5_modbus_crc_err\": 0, \"pmt6_modbus_crc_err\": 0, \"pmt7_modbus_crc_err\": 0, \"pmt8_modbus_crc_err\": 0, \"pmt9_modbus_crc_err\": 0, \"brb_chan_0_threshold\": 20, \"brb_chan_10_pedestal\": -6, \"brb_chan_11_pedestal\": 2, \"brb_chan_12_pedestal\": 15, \"brb_chan_13_pedestal\": 2, \"brb_chan_14_pedestal\": -3, \"brb_chan_15_pedestal\": -3, \"brb_chan_16_pedestal\": 10, \"brb_chan_17_pedestal\": 4, \"brb_chan_18_pedestal\": 0, \"brb_chan_19_pedestal\": -13, \"brb_chan_1_threshold\": 20,\"brb_chan_2_threshold\": 20, \"brb_chan_3_threshold\": 20, \"brb_chan_4_threshold\": 20, \"brb_chan_5_threshold\": 20, \"brb_chan_6_threshold\": 20, \"brb_chan_7_threshold\": 20, \"brb_chan_8_threshold\": 20, \"brb_chan_9_threshold\": 20, \"last_run_frames_ackd\": 1038344, \"last_run_frames_sent\": 1038344, \"last_run_missed_acks\": 0, \"modbus_commands_sent\": 3070072, \"net_eth1_rxmulticast\": 75, \"ntp_report_timestamp\": 1744215320450, \"pmt10_modbus_crc_err\": 0, \"pmt11_modbus_crc_err\": 0, \"pmt12_modbus_crc_err\": 0, \"pmt13_modbus_crc_err\": 0, \"pmt14_modbus_crc_err\": 0, \"pmt15_modbus_crc_err\": 0, \"pmt16_modbus_crc_err\": 0, \"pmt17_modbus_crc_err\": 0, \"pmt18_modbus_crc_err\": 0, \"brb_chan_10_threshold\": 20, \"brb_chan_11_threshold\": 20, \"brb_chan_12_threshold\": 20, \"brb_chan_13_threshold\": 20, \"brb_chan_14_threshold\": 20, \"brb_chan_15_threshold\": 20, \"brb_chan_16_threshold\": 20, \"brb_chan_17_threshold\": 20, \"brb_chan_18_threshold\": 20, \"brb_chan_19_threshold\": 20, \"brb_clk_cleaner_status\": 2, \"last_run_bytes_dropped\": 0, \"last_run_card_mismatch\": 0, \"last_run_start_time_ms\": 1744204629510, \"modbus_error_responses\": 4587, \"last_run_frames_dropped\": 0, \"brb_hit_count_interval_s\": 5, \"last_run_resend_attempts\": 0, \"last_run_queue_bytes_high\": 207705, \"last_run_frames_drop_unacked\": 0, \"last_run_frames_dropped_oversized\": 0}";
// for(unsigned int i=0; i<8;i++){
//  test+=test;
//}
//test="{\"msg_id\":2850, \"msg_time\":\"2025-04-09T20:12:57.195519Z\", \"msg_type\":\"Service Discovery\", \"msg_value\":\"RBU234 Node Daemon\", \"remote_port\":24000, \"status\":\"Online\", \"status_query\":1, \"uuid\":\"f508b4c6-ee8f-4d76-bc03-4af80470671c\"}";
//std::string test2="";
//std::string test3="";
//std::string test4="";
//std::string test5="";
//test5.resize(test.length());
//test4.resize(test.length());

 // snappy::Compress(test.data(), test.length(), &test2);
 // snappy::Uncompress(test2.data(), test2.length(), &test3);

//std::cout<<test<<" : "<<test.length()<<std::endl<<test2<<" : "<<test2.length()<<std::endl<<test3<<" : "<<test3.length()<<std::endl;
//std::cout<<(test==test)<<" : "<<test.length()<<std::endl<<" : "<<test2.length()<<std::endl<<(test3==test)<<" : "<<test3.length()<<std::endl;


//std::cout<<"zstd size="<<ZSTD_compress( test4.data(), test4.length(), test.data(), test.length(), 1)<<std::endl;
 
//ZSTD_decompress(test5.data(), test5.length(), test4.data(), test4.length());

//std::cout<<test4<<std::endl<<test3<<test3.length()<<std::endl;
//std::cout<<(test5==test)<<":"<<test5.length()<<std::endl;


  // set up socket //
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  int a =1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &a, sizeof(int));
  //fcntl(sock, F_SETFL, O_NONBLOCK); 
  if (sock < 0) {
    perror("socket");
    exit(1);
  }
  bzero((char *)&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(multicastport);
  addrlen = sizeof(addr);
  
  // receive //
  if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {        
    perror("bind");
    printf("Failed to bind to multicast listen socket");
    exit(1);
  }    
  mreq.imr_multiaddr.s_addr = inet_addr(multicastaddress.c_str());         
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);         
  if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,&mreq, sizeof(mreq)) < 0) {
    perror("setsockopt mreq");
    printf("Failed to goin multicast group listen thread");
    exit(1);
  }         
  
  
  //////////////////////////////
  
  zmq::pollitem_t items [] = {
    { NULL, sock, ZMQ_POLLIN, 0 },
  
  };
 
  while(true){
    
    zmq::poll (&items [0], 1, 1000);
    
    if ((items [0].revents & ZMQ_POLLIN)) {
      
      
      cnt = recvfrom(sock, message, sizeof(message), 0, (struct sockaddr *) &addr, (socklen_t*) &addrlen);
      if ((cnt > 0) && (message[0]=='{') ) {
	//perror("recvfrom");
	// exit(1);
	//	break;
	//} 
	//else if (cnt > 0){
    printf("%s: message = \"%s\"\n", inet_ntoa(addr.sin_addr), message);
	
	//if(message[0]!='[') break;
	
  }
}
  }
}

   