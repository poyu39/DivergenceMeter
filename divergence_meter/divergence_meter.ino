#include <WiFiNINA.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

#include "pin_map.h"
#include "config.h"

#define LEFT_DOT 10
#define RIGHT_DOT 11

void set_tubes_num(int tube, int num);
void set_tubes_pin();
void set_date_mode();

// NTP 伺服器參數
const long utc_offset_sec = 3600 * timezone;

int loop_counter = 0;       // 2ms
uint8_t sec_c = 0;
uint8_t min_c = 0;

uint8_t tube_index = 0;
uint8_t nums[8] = {0, 0, LEFT_DOT, 0, 0, LEFT_DOT, 0, 0};
uint8_t temp = 0;

uint8_t mode = 0;

// 建立 WiFi 和 NTPClient 物件
WiFiUDP ntp_udp;
NTPClient time_client(ntp_udp, "tock.stdtime.gov.tw", utc_offset_sec);

unsigned long epoch_time = 0;

void setup() {
    set_tubes_pin();
    pinMode(BTN_PIN, INPUT_PULLUP);
    
    // 初始化序列埠
    Serial.begin(9600);
    
    // 連接 WiFi
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi");
    
    // 啟動 NTPClient
    time_client.begin();
    time_client.update();
    epoch_time = time_client.getEpochTime();
    setTime(epoch_time);
}

void loop() {
    if (digitalRead(BTN_PIN) == LOW) {
        mode = (mode + 1) % 2;
        delay(500);
    }

    // 時間模式
    if (mode == 0) {
        if (loop_counter % 500 == 0) {
            nums[0] = hour() / 10;
            nums[1] = hour() % 10;
            nums[3] = minute() / 10;
            nums[4] = minute() % 10;
            nums[6] = second() / 10;
            nums[7] = second() % 10;
        }
        // 每秒切換小數點
        if (nums[7] != temp) {
            nums[2] = (nums[2] == LEFT_DOT) ? RIGHT_DOT : LEFT_DOT;
            nums[5] = (nums[5] == LEFT_DOT) ? RIGHT_DOT : LEFT_DOT;
            temp = nums[7];
            sec_c++;
            if (sec_c == 60) {
                sec_c = 0;
                min_c++;
            }
        }
        // 避免 Cathode Poisoning
        if (min_c == 60) {
            for (uint8_t i = 0; i < 12; i++) {
                for (uint8_t j = 0; j < 8; j++) {
                    set_tubes_num(j, i);
                    delay(100);
                }
            }
            min_c = 0;
            // 校正時間
            time_client.update();
            epoch_time = time_client.getEpochTime();
            setTime(epoch_time);
        }
    }
    
    // 日期模式
    if (mode == 1) {
        set_date_mode();
    }
    

    // 更新
    set_tubes_num(tube_index, nums[tube_index]);
    tube_index = (tube_index + 1) % 8;
    loop_counter = (loop_counter + 1) % 1000;
    delay(2);
}


void set_tubes_num(uint8_t tube, uint8_t num) {
    // 腳位錯誤
    // 0  1  2  3  4  5  6  7  8  9
    // 1  0  9  8  7  6  5  4  3  2
    if (num >= 2 && num <= 9) num = 11 - num;
    if (num == 0 or num == 1) num = 1 - num;
    digitalWrite(TUBE_SEL_0, tube & 0x01);
    digitalWrite(TUBE_SEL_1, tube & 0x02);
    digitalWrite(TUBE_SEL_2, tube & 0x04);
    digitalWrite(TUBE_NUM_0, num & 0x01);
    digitalWrite(TUBE_NUM_1, num & 0x02);
    digitalWrite(TUBE_NUM_2, num & 0x04);
    digitalWrite(TUBE_NUM_3, num & 0x08);
}

void set_tubes_pin() {
    pinMode(TUBE_NUM_0, OUTPUT);
    pinMode(TUBE_NUM_1, OUTPUT);
    pinMode(TUBE_NUM_2, OUTPUT);
    pinMode(TUBE_NUM_3, OUTPUT);
    pinMode(TUBE_SEL_0, OUTPUT);
    pinMode(TUBE_SEL_1, OUTPUT);
    pinMode(TUBE_SEL_2, OUTPUT);
}

void set_date_mode() {
    nums[0] = year() / 1000;
    nums[1] = (year() % 1000) / 100;
    nums[2] = (year() % 100) / 10;
    nums[3] = year() % 10;
    nums[4] = month() / 10;
    nums[5] = month() % 10;
    nums[6] = day() / 10;
    nums[7] = day() % 10;
}