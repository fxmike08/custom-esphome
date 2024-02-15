// Author: Dulgheru Mihaita (Since 2022)
// Last modified: 05.05.2022

#include "knx_component.h"
#include "esphome/core/util.h"
#include "esphome/core/log.h"
#include "esphome/components/uart/uart_component.h"

namespace esphome {
namespace knx {

  int buffer[MAX_KNX_TELEGRAM_SIZE];
  void KnxComponent::loop() {
    KnxComponentserial_eventType eType = this->serial_event();
    //Evaluation of the received telegram -> only KNX telegrams are accepted
    if (eType == KNX_TELEGRAM) {
      KnxTelegram* telegram = this->get_received_telegram();
        ESP_LOGD(TAG, "Received event for group %s.", telegram->get_target_group().c_str());
        if (this->lambda_writer_.has_value())  // insert Labda function if available
          (*this->lambda_writer_)(*this);
    }
  }

  void KnxComponent::setup() {
    this->_source_area = String(this->use_address_.substr(0, this->use_address_.find('.')).c_str()).toInt();
    this->_source_line = String(this->use_address_.substr(this->use_address_.find('.') + 1, this->use_address_.length()).substr(0, this->use_address_.substr(this->use_address_.find('.') + 1, this->use_address_.length()).find('.')).c_str()).toInt();
    this->_source_member = String(this->use_address_.substr(this->use_address_.find_last_of('.') + 1, this->use_address_.length()).c_str()).toInt();
    this->_tg = new KnxTelegram();
    this->_tg_ptp = new KnxTelegram();
    this->_listen_to_broadcasts = false;
    
    this->uart_reset();
  }

  void KnxComponent::dump_config(){ 
    ESP_LOGCONFIG(TAG, " Knx use_address: %s", this->use_address_.c_str());
    for(int i = 0 ; i < this->_listen_group_address_count; i++){
      ESP_LOGCONFIG(TAG, " Knx is listening for group address: %d/%d/%d ",
        this->_listen_group_addresses[i][0], this->_listen_group_addresses[i][1], this->_listen_group_addresses[i][2]
      );
    }
  }

  void KnxComponent::set_use_address(const std::string &use_address) { this->use_address_ = use_address; }


  /* ============== ADAPTED ======================= */

  void KnxComponent::set_listen_to_broadcasts(bool listen) {
    this->_listen_to_broadcasts = listen;
  }

  void KnxComponent::uart_reset() {
    byte sendByte = 0x01;
    this->write(sendByte);
  }

  void KnxComponent::uart_state_request() {
    byte sendByte = 0x02;
    this->write(sendByte);
  }

  void KnxComponent::set_individual_address(int area, int line, int member) {
    this->_source_area = area;
    this->_source_line = line;
    this->_source_member = member;
  }

  KnxComponentserial_eventType KnxComponent::serial_event() {
    while (this->available() > 0) {
      this->check_errors();
      int incomingByte = this->peek();
      print_byte(incomingByte);

      if (this->is_knx_control_byte(incomingByte)) {
        ESP_LOGD(TAG, "We have KNX CONTROL BYTE");
        bool interested = this->read_knx_telegram();
        if (interested) {
          ESP_LOGD(TAG, "Event KNX_TELEGRAM");
          return KNX_TELEGRAM;
        }
        else {
          ESP_LOGD(TAG, "Event IRRELEVANT_KNX_TELEGRAM");
          return IRRELEVANT_KNX_TELEGRAM;
        }
      }
      else if (incomingByte == TPUART_RESET_INDICATION_BYTE) {
        this->serial_read();
        ESP_LOGD(TAG, "Event TPUART_RESET_INDICATION");
        return TPUART_RESET_INDICATION;
      }
      else {
        this->serial_read();
        ESP_LOGV(TAG, "UNKNOWN");
        return UNKNOWN;
      }
    }
    return UNKNOWN;
  }


  bool KnxComponent::is_knx_control_byte(int b) {
    return ( (b | B00101100) == B10111100 ); // Ignore repeat flag and priority flag
  }

  void KnxComponent::check_errors() {
    this->check_uart_settings(19200, 1, esphome::uart::UARTParityOptions::UART_CONFIG_PARITY_EVEN, 8);
  }

  void KnxComponent::print_byte(int incomingByte) {
    ESP_LOGV(TAG,"Incoming Byte: ");
    ESP_LOGV(TAG, "int: %i ", incomingByte);
    ESP_LOGV(TAG, " - ");
    ESP_LOGV(TAG, "hex: %x", incomingByte);
  }

  bool KnxComponent::read_knx_telegram() {
    // Receive header
    for (int i = 0; i < 6; i++) {
      this->_tg->set_buffer_byte(i, this->serial_read());
    }
    ESP_LOGV(TAG,"Payload Length: %d", this->_tg->get_payload_length());

    int bufpos = 6;
    for (int i = 0; i < this->_tg->get_payload_length(); i++) {
      _tg->set_buffer_byte(bufpos, this->serial_read());
      bufpos++;
    }

    // Checksum
    this->_tg->set_buffer_byte(bufpos, this->serial_read());

    // Verify if we are interested in this message - GroupAddress
    bool interested = this->_tg->is_target_group() && this->is_listening_to_group_address(this->_tg->get_target_main_group(), this->_tg->get_target_middle_group(), this->_tg->get_target_sub_group());

    // Physical address
    interested = interested || ((!this->_tg->is_target_group()) && this->_tg->get_target_area() == _source_area && this->_tg->get_target_line() == _source_line && this->_tg->get_target_member() == _source_member);

    // Broadcast (Programming Mode)
    interested = interested || (this->_listen_to_broadcasts && this->_tg->is_target_group() && this->_tg->get_target_main_group() == 0 && this->_tg->get_target_middle_group() == 0 && this->_tg->get_target_sub_group() == 0);

    if (interested) {
      this->send_ack();
    }
    else {
      this->send_not_addressed();
    }

    if (this->_tg->get_communication_type() == KNX_COMM_UCD) {
      ESP_LOGV(TAG,"UCD Telegram received");
    }
    else if (_tg->get_communication_type() == KNX_COMM_NCD) {
      ESP_LOGV(TAG,"NCD Telegram %d received", this->_tg->get_sequence_number());

      if (interested) {
        this->send_ncd_pos_confirm(this->_tg->get_sequence_number(), this->_tg->get_source_area(), this->_tg->get_source_line(), this->_tg->get_source_member());
      }
    }

    // Returns if we are interested in this diagram
    return interested;
  }

  KnxTelegram* KnxComponent::get_received_telegram() {
    return this->_tg;
  }

  // Command Write

  bool KnxComponent::group_write_bool(String address, bool value) {
    int valueAsInt = 0;
    if (value) {
      valueAsInt = B00000001;
    }

    this->create_knx_message_frame(2, KNX_COMMAND_WRITE, address, valueAsInt);
    return this->send_message();
  }

  bool KnxComponent::group_write_4bit_int(String address, int value) {
    int out_value = 0;
    if (value) {
      out_value = value & B00001111;
    }

    this->create_knx_message_frame(2, KNX_COMMAND_WRITE, address, out_value);
    return this->send_message();
  }

  bool KnxComponent::group_write_4Bit_dim(String address, bool direction, byte steps) {
    int value = 0;
    if (direction || steps) {
      value = (direction << 3) + (steps & B00000111);
    }

    this->create_knx_message_frame(2, KNX_COMMAND_WRITE, address, value);
    return this->send_message();
  }

  bool KnxComponent::group_write_1byte_int(String address, int value) {
    this->create_knx_message_frame(2, KNX_COMMAND_WRITE, address, 0);
    this->_tg->set_1byte_int_value(value);
    this->_tg->create_checksum();
    return this->send_message();
  }

  bool KnxComponent::group_write_2byte_int(String address, int value) {
    this->create_knx_message_frame(2, KNX_COMMAND_WRITE, address, 0);
    this->_tg->set_2byte_int_value(value);
    this->_tg->create_checksum();
    return this->send_message();
  }

  bool KnxComponent::group_write_2byte_float(String address, float value) {
    this->create_knx_message_frame(2, KNX_COMMAND_WRITE, address, 0);
    this->_tg->set_2byte_float_value(value);
    this->_tg->create_checksum();
    return this->send_message();
  }

  bool KnxComponent::group_write_3byte_time(String address, int weekday, int hour, int minute, int second) {
    this->create_knx_message_frame(2, KNX_COMMAND_WRITE, address, 0);
    this->_tg->set_3byte_time(weekday, hour, minute, second);
    this->_tg->create_checksum();
    return this->send_message();
  }

  bool KnxComponent::group_write_3byte_date(String address, int day, int month, int year) {
    this->create_knx_message_frame(2, KNX_COMMAND_WRITE, address, 0);
    this->_tg->set_3byte_date(day, month, year);
    this->_tg->create_checksum();
    return this->send_message();
  }

  bool KnxComponent::group_write_4byte_float(String address, float value) {
    this->create_knx_message_frame(2, KNX_COMMAND_WRITE, address, 0);
    this->_tg->set_4byte_float_value(value);
    this->_tg->create_checksum();
    return this->send_message();
  }

  bool KnxComponent::group_write_14byte_text(String address, String value) {
    this->create_knx_message_frame(2, KNX_COMMAND_WRITE, address, 0);
    this->_tg->set_14byte_value(value);
    this->_tg->create_checksum();
    return this->send_message();
  }

  // Command Answer

  bool KnxComponent::group_answer_bool(String address, bool value) {
    int valueAsInt = 0;
    if (value) {
      valueAsInt = B00000001;
    }

    create_knx_message_frame(2, KNX_COMMAND_ANSWER, address, valueAsInt);
    return this->send_message();
  }

  /*
    bool KnxComponent::groupAnswerBitInt(String address, int value) {
    int out_value = 0;
    if (value) {
      out_value = value & B00001111;
    }

    this->create_knx_message_frame(2, KNX_COMMAND_ANSWER, address, out_value);
    return this->send_message();
    }

    bool KnxComponent::group_answer_4bit_dim(String address, bool direction, byte steps) {
    int value = 0;
    if (direction || steps) {
      value = (direction << 3) + (steps & B00000111);
    }

    this->create_knx_message_frame(2, KNX_COMMAND_ANSWER, address, value);
    return this->send_message();
    }
  */

  bool KnxComponent::group_answer_1byte_int(String address, int value) {
    this->create_knx_message_frame(2, KNX_COMMAND_ANSWER, address, 0);
    this->_tg->set_1byte_int_value(value);
    this->_tg->create_checksum();
    return this->send_message();
  }

  bool KnxComponent::group_answer_2byte_int(String address, int value) {
    this->create_knx_message_frame(2, KNX_COMMAND_ANSWER, address, 0);
    this->_tg->set_2byte_int_value(value);
    this->_tg->create_checksum();
    return this->send_message();
  }

  bool KnxComponent::group_answer_2byte_float(String address, float value) {
    this->create_knx_message_frame(2, KNX_COMMAND_ANSWER, address, 0);
    this->_tg->set_2byte_float_value(value);
    this->_tg->create_checksum();
    return this->send_message();
  }

  bool KnxComponent::group_answer_3byte_time(String address, int weekday, int hour, int minute, int second) {
    this->create_knx_message_frame(2, KNX_COMMAND_ANSWER, address, 0);
    this->_tg->set_3byte_time(weekday, hour, minute, second);
    this->_tg->create_checksum();
    return this->send_message();
  }

  bool KnxComponent::group_answer_3byte_date(String address, int day, int month, int year) {
    this->create_knx_message_frame(2, KNX_COMMAND_ANSWER, address, 0);
    this->_tg->set_3byte_date(day, month, year);
    this->_tg->create_checksum();
    return this->send_message();
  }
  bool KnxComponent::group_answer_4byte_float(String address, float value) {
    this->create_knx_message_frame(2, KNX_COMMAND_ANSWER, address, 0);
    this->_tg->set_4byte_float_value(value);
    this->_tg->create_checksum();
    return this->send_message();
  }

  bool KnxComponent::group_answer_14byte_text(String address, String value) {
    this->create_knx_message_frame(2, KNX_COMMAND_ANSWER, address, 0);
    this->_tg->set_14byte_value(value);
    this->_tg->create_checksum();
    return this->send_message();
  }

  // Command Read

  bool KnxComponent::group_read(String address) {
    this->create_knx_message_frame(2, KNX_COMMAND_READ, address, 0);
    this->_tg->create_checksum();
    return this->send_message();
  }

  bool KnxComponent::individual_answer_address() {
    this->create_knx_message_frame(2, KNX_COMMAND_INDIVIDUAL_ADDR_RESPONSE, "0/0/0", 0);
    this->_tg->create_checksum();
    return this->send_message();
  }

  bool KnxComponent::individual_answer_mask_version(int area, int line, int member) {
    this->create_knx_message_frame_individual(4, KNX_COMMAND_MASK_VERSION_RESPONSE, String(area) + "/" + String(line) + "/" + String(member), 0);
    this->_tg->set_communication_type(KNX_COMM_NDP);
    this->_tg->set_buffer_byte(8, 0x07); // Mask version part 1 for BIM M 112
    this->_tg->set_buffer_byte(9, 0x01); // Mask version part 2 for BIM M 112
    this->_tg->create_checksum();
    return this->send_message();
  }

  bool KnxComponent::individual_answer_auth(int accessLevel, int sequenceNo, int area, int line, int member) {
    this->create_knx_message_frame_individual(3, KNX_COMMAND_ESCAPE, String(area) + "/" + String(line) + "/" + String(member), KNX_EXT_COMMAND_AUTH_RESPONSE);
    this->_tg->set_communication_type(KNX_COMM_NDP);
    this->_tg->set_sequence_number(sequenceNo);
    this->_tg->set_buffer_byte(8, accessLevel);
    this->_tg->create_checksum();
    return this->send_message();
  }

  void KnxComponent::create_knx_message_frame(int payloadlength, KnxCommandType command, String address, int firstDataByte) {
    int mainGroup = address.substring(0, address.indexOf('/')).toInt();
    int middleGroup = address.substring(address.indexOf('/') + 1, address.length()).substring(0, address.substring(address.indexOf('/') + 1, address.length()).indexOf('/')).toInt();
    int subGroup = address.substring(address.lastIndexOf('/') + 1, address.length()).toInt();
    this->_tg->clear();
    this->_tg->set_source_address(_source_area, _source_line, _source_member);
    this->_tg->set_target_group_address(mainGroup, middleGroup, subGroup);
    this->_tg->set_first_data_byte(firstDataByte);
    this->_tg->set_command(command);
    this->_tg->set_payload_length(payloadlength);
    this->_tg->create_checksum();
  }

  void KnxComponent::create_knx_message_frame_individual(int payloadlength, KnxCommandType command, String address, int firstDataByte) {
    int area = address.substring(0, address.indexOf('/')).toInt();
    int line = address.substring(address.indexOf('/') + 1, address.length()).substring(0, address.substring(address.indexOf('/') + 1, address.length()).indexOf('/')).toInt();
    int member = address.substring(address.lastIndexOf('/') + 1, address.length()).toInt();
    this->_tg->clear();
    this->_tg->set_source_address(_source_area, _source_line, _source_member);
    this->_tg->set_target_individual_address(area, line, member);
    this->_tg->set_first_data_byte(firstDataByte);
    this->_tg->set_command(command);
    this->_tg->set_payload_length(payloadlength);
    this->_tg->create_checksum();
  }

  bool KnxComponent::send_ncd_pos_confirm(int sequenceNo, int area, int line, int member) {
    this->_tg_ptp->clear();
    this->_tg_ptp->set_source_address(_source_area, _source_line, _source_member);
    this->_tg_ptp->set_target_individual_address(area, line, member);
    this->_tg_ptp->set_sequence_number(sequenceNo);
    this->_tg_ptp->set_communication_type(KNX_COMM_NCD);
    this->_tg_ptp->set_control_data(KNX_CONTROLDATA_POS_CONFIRM);
    this->_tg_ptp->set_payload_length(1);
    this->_tg_ptp->create_checksum();


    int messageSize = this->_tg_ptp->get_total_length();

    uint8_t sendbuf[2];
    for (int i = 0; i < messageSize; i++) {
      if (i == (messageSize - 1)) {
        sendbuf[0] = TPUART_DATA_END;
      }
      else {
        sendbuf[0] = TPUART_DATA_START_CONTINUE;
      }

      sendbuf[0] |= i;
      sendbuf[1] = this->_tg_ptp->get_buffer_byte(i);

      this->write_array(sendbuf, 2);
    }


    int confirmation;
    while (true) {
      confirmation = serial_read();
      if (confirmation == B10001011) {
        return true; // Sent successfully
      }
      else if (confirmation == B00001011) {
        return false;
      }
      else if (confirmation == -1) {
        // Read timeout
        return false;
      }
    }

    return false;
  }

  bool KnxComponent::send_message() {
    int messageSize = this->_tg->get_total_length();

    uint8_t sendbuf[2];
    for (int i = 0; i < messageSize; i++) {
      if (i == (messageSize - 1)) {
        sendbuf[0] = TPUART_DATA_END;
      }
      else {
        sendbuf[0] = TPUART_DATA_START_CONTINUE;
      }

      sendbuf[0] |= i;
      sendbuf[1] = this->_tg->get_buffer_byte(i);

      this->write_array(sendbuf, 2);
    }


    int confirmation;
    while (true) {
      confirmation = this->serial_read();
      if (confirmation == B10001011) {
        delay (SERIAL_WRITE_DELAY_MS);
        return true; // Sent successfully
      }
      else if (confirmation == B00001011) {
        delay (SERIAL_WRITE_DELAY_MS);
        return false;
      }
      else if (confirmation == -1) {
        // Read timeout
        delay (SERIAL_WRITE_DELAY_MS);
        return false;
      }
    }

    return false;
  }

  void KnxComponent::send_ack() {
    byte sendByte = B00010001;
    this->write(sendByte);
    delay(SERIAL_WRITE_DELAY_MS);
  }

  void KnxComponent::send_not_addressed() {
    byte sendByte = B00010000;
    this->write(sendByte);
    delay(SERIAL_WRITE_DELAY_MS);
  }

  int KnxComponent::serial_read() {
    unsigned long startTime = millis();
    while (! ( this->available() > 0)) {
      delay(1);
    }

    int inByte = this->read();
    this->check_errors();
    this->print_byte(inByte);

    return inByte;
  }

  void KnxComponent::add_listen_group_address(String address) {
    if (_listen_group_address_count >= MAX_LISTEN_GROUP_ADDRESSES) {
      ESP_LOGW(TAG, "Already listening to MAX_LISTEN_GROUP_ADDRESSES, cannot listen to another.");
      return;
    }
    this->_listen_group_addresses[this->_listen_group_address_count][0] = address.substring(0, address.indexOf('/')).toInt();
    ;
    this->_listen_group_addresses[this->_listen_group_address_count][1] = address.substring(address.indexOf('/') + 1, address.length()).substring(0, address.substring(address.indexOf('/') + 1, address.length()).indexOf('/')).toInt();
    this->_listen_group_addresses[this->_listen_group_address_count][2] = address.substring(address.lastIndexOf('/') + 1, address.length()).toInt();

    this->_listen_group_address_count++;
  }

  bool KnxComponent::is_listening_to_group_address(int main, int middle, int sub) {
    for (int i = 0; i < this->_listen_group_address_count; i++) {
      if ( (_listen_group_addresses[i][0] == main)
          && (_listen_group_addresses[i][1] == middle)
          && (_listen_group_addresses[i][2] == sub)) {
        return true;
      }
    }

    return false;
  }

}  // namespace knx
}  // namespace esphome
