#include <M5StickCPlus.h>
#include <Unit_Sonic.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// インスタンス（クラス（設計図）から作成された具体的なオブジェクト）を作成
SONIC_IO sensor; // 超音波測距センサのクラスSONIC_IOからインスタンスsensorを作成
WiFiClient espClient; // Wi-Fi通信を管理するためのクラスWiFiClientからインスタンスespClientを作成
PubSubClient client(espClient); // MQTTプロトコルを使用してメッセージを送受信するためのクラスPubSubClientからclientインスタンスを作成

// 時間管理用の変数
unsigned long previousMillis = 0; // 前の時刻（32bit符号なし整数）
const long interval = 30000; // 測定間隔

// WiFi設定
const char* ssid = ""; // SSID
const char* password = ""; // パスワード

// MQTT設定
const char* mqtt_server = ""; // サーバーアドレス
const int mqtt_port = 1883; // ポート番号
const char* channel_token = ""; // チャンネルトークン
const char* topic = ""; // トピック名

// 関数プロトタイプ宣言
void setupWifi();
void reConnect();

String msg;

void setup() {
  M5.begin();
  M5.Lcd.setRotation(3); // 画面の回転設定

  // WiFi接続の初期化
  setupWifi();

  // MQTT接続の初期化
  client.setServer(mqtt_server, mqtt_port);
  client.connect("IoT_manhole", channel_token, "");

  // 超音波測距センサの初期化
  sensor.begin(32, 33);
}

void loop() {
  M5.update(); // デバイスの状態を更新

  // MQTTの再接続処理（接続が切れた場合）
  if (!client.connected()) {
    reConnect();
  }

  client.loop(); // MQTTクライアントのループ処理

  // 現在の時間を取得
  unsigned long currentMillis = millis();

  // 測定間隔が経過した場合の処理
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    static float distance = 0;
    distance = sensor.getDistance(); // 超音波センサで距離を測定

    // 測定値が有効範囲内である場合
    if ((distance < 4000) && (distance > 20)) {
      M5.Lcd.fillScreen(BLACK); // 画面を黒でクリア
      M5.Lcd.setCursor(0, 27); // カーソル位置を設定
      M5.Lcd.printf("%.2fmm", distance); // 測定値を表示

      // JSON形式のメッセージを作成
      char msg[200];
      sprintf(msg, "{\"data\": %.2f, \"write\": true}", distance);

      // MQTTトピックにメッセージを送信
      client.publish(topic, msg);
    }
  }
}

void setupWifi() {
  delay(10);
  M5.Lcd.printf("Connecting to %s", ssid); // 接続中のWiFi SSIDを表示

  // WiFi接続の開始
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // 接続が完了するまで500msごとにドットを表示
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    M5.Lcd.print(".");
  }

  M5.Lcd.printf("\nSuccess\n"); // 接続成功メッセージを表示
}

void reConnect() {
  // MQTTサーバーに再接続するまで繰り返し試行
  while (!client.connected()) {
    M5.Lcd.print("Attempting MQTT connection...");

    // クライアントIDを生成（ランダムな16進数を追加）
    String clientId = "IoT_manhole_";
    clientId += String(random(0xffff), HEX);

    // 接続成功時の処理
    if (client.connect(clientId.c_str(), channel_token, "")) {
      M5.Lcd.printf("\nSuccess\n");
      client.publish(topic, "Hello world"); // テストメッセージを送信
      client.subscribe(topic);               // トピックを購読
    } 
    // 接続失敗時の処理
    else {
      M5.Lcd.print("failed, rc=");
      M5.Lcd.print(client.state()); // エラーコードを表示
      M5.Lcd.println("try again in 5 seconds");
      delay(5000); // 5秒待機
    }
  }
}
