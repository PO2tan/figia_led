//--------------------------------------------------------
//  フィギュアライト
//  @PO2tanVR
//  参考サイト:http://marchan.e5.valueserver.jp/cabin/comp/jbox/arc214/doc21405.html
//--------------------------------------------------------
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <stdio.h>
#include <string.h>
#include <Adafruit_NeoPixel.h>

//--------------------------------------------------------
//  プロトタイプ宣言
//--------------------------------------------------------
void CONNECT_WiFi(void);
void WEBSERVER_SETUP_DATAN(void);
void SEND_WEB(void);
void GET_DATA(void);
void DATA_SET(byte i, String data);

//--------------------------------------------------------
//  ネットワーク設定
//--------------------------------------------------------
String SSID = "MEtan_setting";				//|接続先のSSID,PASS
String PASS	= "metan2042";					//|

IPAddress IP(192, 168, 1, 32);				//|APモードにした時のネットワーク各種設定
IPAddress DNS(192, 168, 1, 1);			    //|
IPAddress GATEWAY(192, 168, 1, 1);			//|
IPAddress NETMASK(255, 255, 255, 0);		//|

int HOST_PORT = 80;			        		//通信ポート
ESP8266WebServer server(HOST_PORT);
WiFiClient client;

//--------------------------------------------------------
//  各種設定
//  https://www.marutsu.co.jp/pc/static/large_order/WS2812B_0124
//--------------------------------------------------------
#define MAX_BRIGHTNESS  128     //USBの最大供給電力を超えないよう設定
#define DIN_PIN         5
#define LED_COUNT       16
#define MIN_DELAY       3

Adafruit_NeoPixel pixels(LED_COUNT, DIN_PIN, NEO_GRB + NEO_KHZ800);

//--------------------------------------------------------
//  グローバル変数
//--------------------------------------------------------
bool one_color_mode = false;
byte speed = 5, brightness = 50;
unsigned long led_time = 0;
int step_num = 0;
String color = "000000", color_log = "";

String request_parameter[] = {
    "brightness",
    "speed",
    "onecolor",
    "rainbow",
    "color"
}; 

//--------------------------------------------------------
//  ページデータ
//--------------------------------------------------------
String PAGE_DATA = R"EOB(
<!DOCTYPE html>
<meta charset="utf-8" name="viewport" content="width=device-width">
<html>
    <head>
        <title>LED調光</title>
        <style type="text/css">
            body {
                text-align: center;
                background-color: #3A6EA5;
            }
            div {
                background-color: #008080;
            }
        </style>
        <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.4.1/jquery.min.js"></script>
    </head>
    <body>
        <div>
        <p>
            <h3>明るさ:<span id="brightness_value"></span></h3>
            <input id="brightness" type="range" min="1" max="100" step="1" value="_brightness_">
        </p>
        <p>
            <h3>流れる速度:<span id="speed_value"></span></h3>
            <input id="speed" type="range" min="1" max="10" step="1" value="_speed_">
        </p>
        <p>
            <h3>色</h3>
            <input id="onecolor" name="one_color" type="radio" _one_>単色
            <input id="rainbow" name="one_color" type="radio" _rainbow_>レインボー
            </br>
            <input id="color" type="color" value="#FFFFFF">
        </p>
        </div>
        <script>
            function onecolor_change(event) {send_data(event.target.id, event.currentTarget.value);}
            function color_change(event) {
                var data = event.currentTarget.value.toString().substring(1);
                send_data(event.target.id, data);
            }
            function send_data(object_name, data) {
                $.get("/data?" + object_name + "=" + data);
                console.log(object_name + ":" + data);
            }

            var obj_brightness = document.getElementById("brightness");
            var target_brightness = document.getElementById("brightness_value");
            target_brightness.innerHTML = obj_brightness.value;
            obj_brightness.oninput = function() {
                obj_brightness.value = this.value;
                target_brightness.innerHTML = this.value;
                send_data("brightness", this.value);
            }

            var obj_speed = document.getElementById("speed");
            var target_speed = document.getElementById("speed_value");
            target_speed.innerHTML = obj_speed.value;
            obj_speed.oninput = function() { 
                obj_speed.value = this.value;
                target_speed.innerHTML = this.value;
                send_data("speed", this.value);
            }

            var obj_one_color = document.getElementById("onecolor");
            obj_one_color.addEventListener("change", onecolor_change);

            var obj_reinbow = document.getElementById("rainbow");
            obj_reinbow.addEventListener("change", onecolor_change);

            var obj_color = document.getElementById("color");
            obj_color.addEventListener("change", color_change);
        </script>
    </body>
</html>
)EOB";

//--------------------------------------------------------
//  WIFI接続関数
//--------------------------------------------------------
void CONNECT_WiFi(void) {
	if(WiFi.status() != WL_CONNECTED) {
		WiFi.mode(WIFI_STA);        				//WiFiの動作モードの設定(子機)
        WiFi.config(IP, GATEWAY, NETMASK, DNS);                       //IPを固定
		WiFi.begin(SSID, PASS);						//アクセスポイントへ接続
        Serial.print("connecting");

		while (WiFi.status() != WL_CONNECTED) {
            Serial.print(".");
            delay(100);
        }
        Serial.println("sucsess!");
        Serial.println(WiFi.localIP().toString());
        Serial.println("----");
	}
}

//--------------------------------------------------------
//  WEBサーバーセットアップ
//--------------------------------------------------------
void WEBSERVER_SETUP_DATAN(void) {
  	server.begin();                			//webサーバーを起動しクライアントの接続を待機
  	server.on("/", SEND_WEB);
    server.on("/data", GET_DATA);
}

//--------------------------------------------------------
//  WEBページ
//--------------------------------------------------------
void SEND_WEB(void) {
    //各種値を書き換え
    if(one_color_mode == true) {
        PAGE_DATA.replace("_one_", "checked");
        PAGE_DATA.replace("_rainbow_", "");
    }
    else{
        PAGE_DATA.replace("_one_", "");
        PAGE_DATA.replace("_rainbow_", "checked");
    }
    PAGE_DATA.replace("_brightness_", String(brightness));
    PAGE_DATA.replace("_speed_", String(speed));

    server.send(200, "text/html", PAGE_DATA);
}

//--------------------------------------------------------
//  データ読み込み
//--------------------------------------------------------
void GET_DATA(void) {
    byte i, max;
    String buf;

    max = sizeof(request_parameter) / sizeof(String);
    for(i=0;i<max;i++) {
        buf = server.arg(request_parameter[i]);		//リクエストパラメータの値を取得
        if(buf != "") {
            DATA_SET(i, buf);
            break;
        }

    }

    server.send(200, "text/html", "OK");
}

//--------------------------------------------------------
//  データ反映
//  "brightness"
//  "speed"
//  "onecolor"
//  "rainbow"
//  "color"
//--------------------------------------------------------
void DATA_SET(byte i, String data) {
    Serial.println(request_parameter[i] + ":" + data);

    switch(i) {
        case 0:                 //brightness
            brightness = strtol(data.c_str(), NULL, 10);
            pixels.setBrightness(MAX_BRIGHTNESS - (100 - brightness));
            break;

        case 1:                 //speed
            speed = strtol(data.c_str(), NULL, 10);
            break;

        case 2:                 //onecolor
            if(data == "on") {one_color_mode = true;}
            break;

        case 3:                 //rainbow
            if(data == "on") {one_color_mode = false;}
            break;

        case 4:                 //color
            color = data;
            break;
    }
}

//--------------------------------------------------------
//  SETUP
//--------------------------------------------------------
void setup(void) {
    Serial.begin(9600);
    Serial.println("boot");
    Serial.println("----");

    pixels.begin();
    CONNECT_WiFi();
    WEBSERVER_SETUP_DATAN();
}

//--------------------------------------------------------
//  LOOP
//  https://blog.hrendoh.com/neopixel-ring-effects/
//--------------------------------------------------------
void loop (void) {
    int i, r, g, b, pixelHue;
    String buf;

    if(one_color_mode == true) {        //単色発光モード
        if(color != color_log) {
            pixels.clear();

            //ff ff ff
            buf = color.substring(0, 2);
            r = strtol(buf.c_str(), NULL, 16);

            buf = color.substring(2, 4);
            g = strtol(buf.c_str(), NULL, 16);

            buf = color.substring(4, 6);
            b = strtol(buf.c_str(), NULL, 16);

            for(i=0; i<pixels.numPixels(); i++) {
                pixels.setPixelColor(i, pixels.Color(r, g, b));
            }
            pixels.show();
            color_log = color;
        }
    }
    else {                              //ゲーミングモード
        if(millis() - led_time >= MIN_DELAY + (10 - speed)) {
            for(i=0; i<pixels.numPixels(); i++) {
                // ストリップの長さに沿って色相環（65536の範囲）を1回転させる量だけピクセルの色相をオフセットします。
                pixelHue = step_num + (i * 65536L / pixels.numPixels());//色を決めている?
                // ColorHSV関数に色相(0 to 65535)を渡し、その結果をgamma32()でガンマ補正します。
                pixels.setPixelColor(i, pixels.gamma32(pixels.ColorHSV(pixelHue)));
            }
            pixels.show();
            step_num += 256;
            led_time = millis();
            if(step_num == 65536) {step_num = 0;}
        }
    }
    server.handleClient();
}