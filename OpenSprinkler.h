/* OpenSprinkler Unified (AVR/RPI/BBB/LINUX/ESP8266) Firmware
 * Copyright (C) 2015 by Ray Wang (ray@opensprinkler.com)
 *
 * OpenSprinkler library header file
 * Feb 2015 @ OpenSprinkler.com
 *
 * This file is part of the OpenSprinkler library
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */


#ifndef _OPENSPRINKLER_H
#define _OPENSPRINKLER_H

#include "defines.h"
#include "utils.h"
#include "gpio.h"

#if defined(ARDUINO) && !defined(ESP8266) // headers for AVR
  #include "Arduino.h"
  #include <avr/eeprom.h>
  #include <Wire.h>
  #include "LiquidCrystal.h"
  #include "Time.h"
  #include "DS1307RTC.h"
  #include "EtherCard.h"
#elif defined(ESP8266) // headers for ESP8266
  #include <Wire.h>
  #include <FS.h>
  #include <RCSwitch.h>
	#include <stdlib.h>
	#include <UIPEthernet.h>
  #include "SSD1306Display.h"
  #include "i2crtc.h"
  #include "espconnect.h"
#else // headers for RPI/BBB/LINUX
  #include <time.h>
  #include <string.h>
  #include <unistd.h>
  #include <netdb.h>
  #include "etherport.h"

  #ifdef MQTT
  #include <mosquitto.h>
  #endif
#endif // end of headers
  
/** Non-volatile data */
struct NVConData {
  uint16_t sunrise_time;  // sunrise time (in minutes)
  uint16_t sunset_time;   // sunset time (in minutes)
  uint32_t rd_stop_time;  // rain delay stop time
  uint32_t external_ip;   // external ip
};

/** Station special attribute data */
struct StationSpecialData {
  byte type;
  byte data[STATION_SPECIAL_DATA_SIZE];
};

/** Station data structures - Must fit in STATION_SPECIAL_DATA_SIZE */
struct RFStationData {
  byte on[6];
  byte off[6];
  byte timing[4];
};

struct RFStationDataFull {
  byte on[8];
  byte off[8];
  byte timing[4];
  byte protocol[4];
};

struct RemoteStationData {
  byte ip[8];
  byte port[4];
  byte sid[2];
};

struct GPIOStationData {
  byte pin[2];
  byte active;
};

struct HTTPStationData {
  byte data[STATION_SPECIAL_DATA_SIZE];
};

/** Volatile controller status bits */
struct ConStatus {
  byte enabled:1;           // operation enable (when set, controller operation is enabled)
  byte rain_delayed:1;      // rain delay bit (when set, rain delay is applied)
  byte rain_sensed:1;       // rain sensor bit (when set, it indicates that rain is detected)
  byte program_busy:1;      // HIGH means a program is being executed currently
  byte has_curr_sense:1;    // HIGH means the controller has a current sensing pin
  byte has_sd:1;            // HIGH means a microSD card is detected
  byte safe_reboot:1;       // HIGH means a safe reboot has been marked
  byte has_hwmac:1;         // has hardware MAC chip
  byte req_ntpsync:1;       // request ntpsync
  byte req_network:1;       // request check network
  byte display_board:4;     // the board that is being displayed onto the lcd
  byte network_fails:2;     // number of network fails
  byte mas:8;               // master station index
  byte mas2:8;              // master2 station index
};

extern const char wtopts_filename[];
extern const char stns_filename[];
extern const char ifkey_filename[];
extern const byte op_max[];
extern const char op_json_names[];
#ifdef ESP8266
struct WiFiConfig {
  byte mode;
  String ssid;
  String pass;
};  
extern const char wifi_filename[];
#endif

class OpenSprinkler {
public:
  // data members
#if defined(ARDUINO) && !defined(ESP8266)
  static LiquidCrystal lcd; // 16x2 character LCD
#elif defined(ESP8266)
  static SSD1306Display lcd;// 128x64 OLED display
#else
  // todo: LCD define for RPI/BBB
#endif

#if defined(OSPI)
  static byte pin_sr_data;    // RPi shift register data pin
                              // to handle RPi rev. 1
#endif

  static NVConData nvdata;
  static ConStatus status;
  static ConStatus old_status;
  static byte nboards, nstations;
  static byte hw_type;// hardware type
  static byte hw_rev; // hardware minor
  
  static byte options[];  // option values, max, name, and flag

  static byte station_bits[];     // station activation bits. each byte corresponds to a board (8 stations)
                                  // first byte-> master controller, second byte-> ext. board 1, and so on

  // variables for time keeping
  static ulong sensor_lasttime;  // time when the last sensor reading is recorded
  static volatile ulong flowcount_time_ms;// time stamp when new flow sensor click is received (in milliseconds)
  static ulong flowcount_rt;     // flow count (for computing real-time flow rate)
  static ulong flowcount_log_start; // starting flow count (for logging)
  static ulong raindelay_start_time;  // time when the most recent rain delay started
  static byte  button_timeout;        // button timeout
  static ulong checkwt_lasttime;      // time when weather was checked
  static ulong checkwt_success_lasttime; // time when weather check was successful
  static ulong powerup_lasttime;      // time when controller is powered up most recently
  static byte  weather_update_flag; 
  // member functions
  // -- setup
  static void update_dev();   // update software for Linux instances
  static void reboot_dev();   // reboot the microcontroller
  static void begin();        // initialization, must call this function before calling other functions
  static byte start_network();  // initialize network with the given mac and port
  static byte start_ether();  // initialize ethernet with the given mac and port
  static void mqtt_publish(const char *topic, const char *payload);  // publish mqtt message
#if defined(ARDUINO)
  static bool load_hardware_mac(uint8_t*, bool wired=false);  // read hardware mac address
#endif
  static time_t now_tz();
  // -- station names and attributes
  static void get_station_name(byte sid, char buf[]); // get station name
  static void set_station_name(byte sid, char buf[]); // set station name
  static uint16_t parse_rfstation_code(RFStationData *data, ulong *on, ulong *off); // parse rf code into on/off/time sections
  static void switch_rfstation(RFStationData *data, bool turnon);  // switch rf station
  static void switch_remotestation(RemoteStationData *data, bool turnon); // switch remote station
  static void switch_gpiostation(GPIOStationData *data, bool turnon); // switch gpio station
  static void switch_httpstation(HTTPStationData *data, bool turnon); // switch http station
  static void station_attrib_bits_save(int addr, byte bits[]); // save station attribute bits to nvm
  static void station_attrib_bits_load(int addr, byte bits[]); // load station attribute bits from nvm
  static byte station_attrib_bits_read(int addr); // read one station attribte byte from nvm

  // -- options and data storeage
  static void nvdata_load();
  static void nvdata_save();

  static void options_setup();
  static void options_load();
  static void options_save(bool savewifi=false);

  static byte password_verify(char *pw);  // verify password
  
  // -- controller operation
  static void enable();           // enable controller operation
  static void disable();          // disable controller operation, all stations will be closed immediately
  static void raindelay_start();  // start raindelay
  static void raindelay_stop();   // stop rain delay
  static void rainsensor_status();// update rainsensor status
  static bool programswitch_status(ulong); // get program switch status
#if defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega1284__) || defined(ESP8266)
  static uint16_t read_current(); // read current sensing value
  static uint16_t baseline_current; // resting state current
#endif
  static int detect_exp();        // detect the number of expansion boards
  static byte weekday_today();    // returns index of today's weekday (Monday is 0)

  static byte set_station_bit(byte sid, byte value); // set station bit of one station (sid->station index, value->0/1)
  static void switch_special_station(byte sid, byte value); // swtich special station
  static void clear_all_station_bits(); // clear all station bits
  static void apply_all_station_bits(); // apply all station bits (activate/deactive values)

  // -- LCD functions
#if defined(ARDUINO) // LCD functions for Arduino
  #ifdef ESP8266
  static void lcd_print_pgm(PGM_P str); // ESP8266 does not allow PGM_P followed by PROGMEM
  static void lcd_print_line_clear_pgm(PGM_P str, byte line);
  #else
  static void lcd_print_pgm(PGM_P PROGMEM str);           // print a program memory string
  static void lcd_print_line_clear_pgm(PGM_P PROGMEM str, byte line);
  #endif
  static void lcd_print_time(time_t t);                  // print current time
  static void lcd_print_ip(const byte *ip, byte endian);    // print ip
  static void lcd_print_mac(const byte *mac);             // print mac
  static void lcd_print_station(byte line, char c);       // print station bits of the board selected by display_board
  static void lcd_print_version(byte v);                   // print version number

  // -- UI and buttons
  static byte button_read(byte waitmode); // Read button value. options for 'waitmodes' are:
                                          // BUTTON_WAIT_NONE, BUTTON_WAIT_RELEASE, BUTTON_WAIT_HOLD
                                          // return values are 'OR'ed with flags
                                          // check defines.h for details

  // -- UI functions --
  static void ui_set_options(int oid);    // ui for setting options (oid-> starting option index)
  static void lcd_set_brightness(byte value=1);
  static void lcd_set_contrast();

  #ifdef ESP8266
  static WiFiConfig wifi_config;
  static IOEXP *mainio, *drio;
  static IOEXP *expanders[];
  static RCSwitch rfswitch;
  static void detect_expanders();
  static void flash_screen();
  static void toggle_screen_led();
  static void set_screen_led(byte status);  
  static byte get_wifi_mode() {return wifi_config.mode;}
  static void config_ip();
  static void reset_to_ap();
  static byte state;
  #endif  
private:
  static void lcd_print_option(int i);  // print an option to the lcd
  static void lcd_print_2digit(int v);  // print a integer in 2 digits
  static void lcd_start();
  static byte button_read_busy(byte pin_butt, byte waitmode, byte butt, byte is_holding);
#if defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega1284__) || defined(ESP8266)
  static byte engage_booster;
#endif
#if defined(ESP8266)
  static void latch_boost();
  static void latch_open(byte sid);
  static void latch_close(byte sid);
  static void latch_setzonepin(byte sid, byte value);
  static void latch_setallzonepins(byte value);
  static void latch_apply_all_station_bits();
  static byte prev_station_bits[];
#endif
#endif // LCD functions
};

#endif  // _OPENSPRINKLER_H
