# knx - Component for esphome
## Esphome Component that allow comunication using Knx - Twisted Pair
This component is written for ESPHome to enable communication via KNX TP(Twisted Pair).
This is based on [KnxTpUart](https://github.com/majuss/KnxTpUart) and has been modified to compile on ESP32/ESP8266, following ESPHome coding style.
### Knx component configuration:


*  **id (Required** , ID): Specifies the ID used for the KNX component.
*  **uart_id (Required**, ID): Specifies the ID of the UART hub.
*  **use_address (Required**, string): Defines the KNX device address. The format is group.subgroup.address (e.g., 10.22.10).
*  **listen_group_address (Required**, Array[string]): An array of addresses that the component will listen to.
*  **serial_timeout** (Optional, int): Sets the serial read timeout in milliseconds. The default is 1000 ms.
*  **lambda (Required**):  Required for receiving KNX events. The KNX event will have one of the addresses specified in the `listen_group_address` entries.


Usage example :
```yaml

  external_components:
    - source:
        type: git
        ref: main
        url: https://github.com/fxmike08/custom-esphome
      components: [ knx ]
      refresh: 0s

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
