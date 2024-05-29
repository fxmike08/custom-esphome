// Author: Dulgheru Mihaita (Since 2022)
// Last modified: 05.05.2022

#pragma once

#include "esphome.h"
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "knx_telegram.h"

using namespace std;
static const char *const TAG = "knx"; 

namespace esphome {
namespace knx {

static const int MAX_LISTEN_GROUP_ADDRESSES=15;
static const int SERIAL_WRITE_DELAY_MS=100;
static const byte TPUART_DATA_START_CONTINUE=B10000000;
static const byte TPUART_DATA_END=B01000000;
// Services from TPUART
static const byte  TPUART_RESET_INDICATION_BYTE=B11;

enum KnxComponentserial_eventType {
  TPUART_RESET_INDICATION,
  KNX_TELEGRAM,
  IRRELEVANT_KNX_TELEGRAM,
  UNKNOWN
};

// Needed for lambda expression
class KnxComponent;
using lambda_writer_t = std::function<void(KnxComponent &)>;

class KnxComponent : public Component, public uart::UARTDevice {
  
  public:
    KnxComponent(uart::UARTComponent *uart) : uart::UARTDevice(uart) {
      _listen_group_address_count = 0;
    };
    void loop() override;
    void setup() override;
    void dump_config() override;
    void set_use_address(const std::string &use_address);
    void set_serial_timeout(const uint32_t &serial_timeout);

    // KNXTpUART - adapted
    void uart_reset();
    void uart_state_request();
    KnxComponentserial_eventType serial_event();
    KnxTelegram* get_received_telegram();

    void set_individual_address(int, int, int);

    void send_ack();
    void send_not_addressed();

    bool group_write_bool(String, bool);
    bool group_write_4bit_int(String, int);
    bool group_write_4Bit_dim(String, bool, byte);
    bool group_write_1byte_int(String, int);
    bool group_write_2byte_int(String, int);
    bool group_write_2byte_float(String, float);
    bool group_write_3byte_time(String, int, int, int, int);
    bool group_write_3byte_date(String, int, int, int);
    bool group_write_4byte_float(String, float);
    bool group_write_14byte_text(String, String);

    bool group_answer_bool(String, bool);
    /*
      bool group_answer_4bit_int(String, int);
      bool group_answer_4bit_dim(String, bool, byte);
    */
    bool group_answer_1byte_int(String, int);
    bool group_answer_2byte_int(String, int);
    bool group_answer_2byte_float(String, float);
    bool group_answer_3byte_time(String, int, int, int, int);
    bool group_answer_3byte_date(String, int, int, int);
    bool group_answer_4byte_float(String, float);
    bool group_answer_14byte_text(String, String);

    bool group_read(String);

    void add_listen_group_address(String);
    bool is_listening_to_group_address(int, int, int);

    bool individual_answer_address();
    bool individual_answer_mask_version(int, int, int);
    bool individual_answer_auth(int, int, int, int, int);

    void set_listen_to_broadcasts(bool);
    // Needed for lambda expression
    void set_lambda_writer(lambda_writer_t &&writer) { this->lambda_writer_ = writer; };


  protected:
    std::string use_address_;
    uint32_t serial_timeout_;
    // KNXTpUART - adapted
    KnxTelegram* _tg;       // for normal communication
    KnxTelegram* _tg_ptp;   // for PTP sequence confirmation
    int _source_area;
    int _source_line;
    int _source_member;
    int _listen_group_addresses[MAX_LISTEN_GROUP_ADDRESSES][3];
    int _listen_group_address_count;
    bool _listen_to_broadcasts;

    bool is_knx_control_byte(int);
    void check_errors();
    void print_byte(int);
    bool read_knx_telegram();
    void create_knx_message_frame(int, KnxCommandType, String, int);
    void create_knx_message_frame_individual(int, KnxCommandType, String, int);
    bool send_message();
    bool send_ncd_pos_confirm(int, int, int, int);
    int serial_read();
    optional<lambda_writer_t> lambda_writer_{};

};
}  // namespace knx
}  // namespace esphome
