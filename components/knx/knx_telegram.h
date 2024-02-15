// File: knx_telegram.h
// Author: Daniel Kleine-Albers (Since 2012)
// Modified: Thorsten Gehrig (Since 2014)
// Modified: Michael Werski (Since 2014)
// Modified: Katja Blankenheim (Since 2014)
// Modified: Mag Gyver (Since 2016)
// Modified: Dulgheru Mihaita (Since 2022)

// Last modified: 05.05.2022

#ifndef KnxTelegram_h
#define KnxTelegram_h

#include "Arduino.h"

#define MAX_KNX_TELEGRAM_SIZE 23
#define KNX_TELEGRAM_HEADER_SIZE 6

// KNX priorities
enum KnxPriorityType {
  KNX_PRIORITY_SYSTEM = B00,
  KNX_PRIORITY_ALARM = B10,
  KNX_PRIORITY_HIGH = B01,
  KNX_PRIORITY_NORMAL = B11
};

// KNX commands / APCI Coding
enum KnxCommandType {
  KNX_COMMAND_READ = B0000,
  KNX_COMMAND_WRITE = B0010,
  KNX_COMMAND_ANSWER = B0001,
  KNX_COMMAND_INDIVIDUAL_ADDR_WRITE = B0011,
  KNX_COMMAND_INDIVIDUAL_ADDR_REQUEST = B0100,
  KNX_COMMAND_INDIVIDUAL_ADDR_RESPONSE = B0101,
  KNX_COMMAND_MASK_VERSION_READ = B1100,
  KNX_COMMAND_MASK_VERSION_RESPONSE = B1101,
  KNX_COMMAND_RESTART = B1110,
  KNX_COMMAND_ESCAPE = B1111
};

// Extended (escaped) KNX commands
enum KnxExtendedCommandType {
  KNX_EXT_COMMAND_AUTH_REQUEST = B010001,
  KNX_EXT_COMMAND_AUTH_RESPONSE = B010010
};

// KNX Transport Layer Communication Type
enum KnxCommunicationType {
  KNX_COMM_UDP = B00, // Unnumbered Data Packet
  KNX_COMM_NDP = B01, // Numbered Data Packet
  KNX_COMM_UCD = B10, // Unnumbered Control Data
  KNX_COMM_NCD = B11  // Numbered Control Data
};

// KNX Control Data (for UCD / NCD packets)
enum KnxControlDataType {
  KNX_CONTROLDATA_CONNECT = B00,      // UCD
  KNX_CONTROLDATA_DISCONNECT = B01,   // UCD
  KNX_CONTROLDATA_POS_CONFIRM = B10,  // NCD
  KNX_CONTROLDATA_NEG_CONFIRM = B11   // NCD
};

class KnxTelegram {
  public:
    KnxTelegram();

    void clear();
    void set_buffer_byte(int index, int content);
    int get_buffer_byte(int index);
    void set_payload_length(int size);
    int get_payload_length();
    void set_repeated(bool repeat);
    bool is_repeated();
    void set_priority(KnxPriorityType prio);
    KnxPriorityType get_priority();
    void set_source_address(int area, int line, int member);
    int get_source_area();
    int get_source_line();
    int get_source_member();
    void set_target_group_address(int main, int middle, int sub);
    void set_target_individual_address(int area, int line, int member);
    bool is_target_group();
    String get_target_group();
    int get_target_main_group();
    int get_target_middle_group();
    int get_target_sub_group();
    int get_target_area();
    int get_target_line();
    int get_target_member();
    void set_routing_counter(int counter);
    int get_routing_counter();
    void set_command(KnxCommandType command);
    KnxCommandType get_command();

    void set_first_data_byte(int data);
    int get_first_data_byte();
    bool get_bool();

    int get_4bit_int_value();
    bool get_4bit_direction_value();
    byte get_4bit_steps_value();

    void set_1byte_int_value(int value);
    int get_1byte_int_value();

    void set_2byte_int_value(int value);
    int get_2byte_int_value();
    void set_2byte_float_value(float value);
    float get_2byte_float_value();

    void set_3byte_time(int weekday, int hour, int minute, int second);
    int get_3byte_weekday_value();
    int get_3byte_hour_value();
    int get_3byte_minute_value();
    int get_3byte_second_value();
    void set_3byte_date(int day, int month, int year);
    int get_3byte_day_value();
    int get_3byte_month_value();
    int get_3byte_year_value();

    void set_4byte_float_value(float value);
    float get_4byte_float_value();

    void set_14byte_value(String value);
    String get_14byte_value();

    void create_checksum();
    bool verify_checksum();
    int get_checksum();
    void print();
    int get_total_length();
    KnxCommunicationType get_communication_type();
    void set_communication_type(KnxCommunicationType);
    int get_sequence_number();
    void set_sequence_number(int);
    KnxControlDataType get_control_data();
    void set_control_data(KnxControlDataType);

  private:
    int buffer[MAX_KNX_TELEGRAM_SIZE];
    int calculate_checksum();

};

#endif