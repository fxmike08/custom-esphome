# knx - Component for esphome
## Esphome Component that allow comunication using Knx - Twisted Pair
This component is written for esphome in order to comunicate via KNX TP.
This is based on [KnxTpUart](https://github.com/majuss/KnxTpUart) library and modified for compiling on ESP32/ESP8266 according with esphome coding style.


Usage example :
```yaml

  external_components:
    - source:
        type: git
        url: https://github.com/fxmike08/knx

...
  uart:
    - id: uart_bus
      tx_pin: 1 #carefull with strapping pins
      rx_pin: 3 #carefull with strapping pins
      baud_rate: 19200
      parity: even
      debug:
        direction: BOTH
        dummy_receiver: false
        after:
          delimiter: "\n"
        sequence:
          - lambda: UARTDebug::log_binary(direction, bytes, 32);

...

  knx:
    id: knxd
    uart_id: uart_bus
    use_address: 10.10.1
    listen_group_address: ["0/0/3", "0/0/1"]
    lambda: |-
      KnxTelegram* telegram = knx.get_received_telegram();
      ESP_LOGD("KNX", "group: %s",telegram->get_target_group());
      if(telegram->get_target_group() == "0/0/3" ){
        if(telegram->get_bool()){
           ESP_LOGD("KNX", "KNX ON");
        } else {
           ESP_LOGD("KNX", "KNX OFF");
        }
      }
      return;

...

  api:
    services:
      - service: knx_write_group_bool
        variables:
          address: string
          value: bool
        then: 
          - lambda: |-
              id(knxd).group_write_bool(address.c_str(), value);

...
  - platform: gpio
    name: $friendly_devicename Relay 2
    id: relay_2
    pin:
      number: 19
    on_turn_on:
      - lambda: |-
          id(knxd).group_write_bool("0/0/3", true);
```

**If you like this project, consider buying me a beer 🍺 <a href="https://paypal.me/fxmike08" target="_blank"><img src="https://img.shields.io/static/v1?logo=paypal&label=&message=donate&color=slategrey"></a>**
