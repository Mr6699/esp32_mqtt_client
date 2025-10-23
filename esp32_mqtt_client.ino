/*
 * ESP32 MQTT客户端示例
 * 本程序演示如何使用ESP32连接到MQTT服务器，发布和订阅消息
 */

#include <WiFi.h>
#include <PubSubClient.h>

// WiFi配置
const char* ssid = "XXXXX";
const char* password = "XXXXXXX";

// MQTT服务器配置
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883; // 默认MQTT端口
const char* mqtt_user = ""; // 如果需要
const char* mqtt_password = ""; // 如果需要
char client_id[30]; // 客户端ID数组
// 创建WiFi客户端和MQTT客户端对象
WiFiClient espClient;
PubSubClient client(espClient);

// 主题定义
const char* pub_topic = "esp32/data";
const char* sub_topic = "esp32/commands";

// 定时器变量
unsigned long lastMsg = 0;
#define MSG_INTERVAL 2000 // 消息发布间隔(毫秒)

// MQTT回调函数，用于处理接收到的消息
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("收到消息 [");
  Serial.print(topic);
  Serial.print("]: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  
  // 处理接收到的命令
  if (String(topic) == sub_topic) {
    // 这里可以根据接收到的命令执行相应的操作
    Serial.print("处理命令: ");
    Serial.println(messageTemp);
  }
}

// 连接WiFi函数
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("连接到WiFi网络: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_15dBm);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi已连接");
  Serial.println("IP地址: ");
  Serial.println(WiFi.localIP());
}

// MQTT重连函数
void reconnect() {
  // 循环直到重新连接成功
  while (!client.connected()) {
    Serial.print("尝试MQTT连接...");
    Serial.print("服务器: ");
    Serial.print(mqtt_server);
    Serial.print(":");
    Serial.print(mqtt_port);
    Serial.print(", 客户端ID: ");
    Serial.println(client_id);
    
    // 设置连接参数，增加超时时间和保活时间
    client.setKeepAlive(60); // 设置保活时间为60秒
   
    
    // 尝试连接
    if (client.connect(client_id, mqtt_user, mqtt_password)) {
      Serial.println("已连接");
      // 连接成功后订阅主题
      client.subscribe(sub_topic);
    } else {
      int error_code = client.state();
      Serial.print("连接失败，错误代码 = ");
      Serial.print(error_code);
      
      // 详细解释错误代码
      switch(error_code) {
        case -4: Serial.println(": 连接被拒绝 - 服务器不可用"); break;
        case -5: Serial.println(": 连接被拒绝 - 不支持的协议版本");
                 Serial.println("  解决方法: 确认MQTT服务器支持的协议版本，可能需要更新PubSubClient库");
                 Serial.println("  1. 检查MQTT服务器配置，确认支持MQTT v3.1或v3.1.1");
                 Serial.println("  2. 尝试使用不同的MQTT服务器，如public.emqx.io");
                 Serial.println("  3. 更新PubSubClient库到最新版本");
                 break;
        case -6: Serial.println(": 连接被拒绝 - 标识符被拒绝"); break;
        case -7: Serial.println(": 连接被拒绝 - 服务器不可用"); break;
        case -8: Serial.println(": 连接被拒绝 - 错误的用户名或密码"); break;
        case -9: Serial.println(": 连接被拒绝 - 未授权"); break;
        default: Serial.println(": 未知错误");
      }
      
      Serial.println("，5秒后重试");
      // 等待5秒后重试
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // 等待串口连接
  while (!Serial) {
    ; // 对于基于USB的串口，如果没有串口监视器连接，可能会卡住
  }
  
  // 生成唯一客户端ID
  sprintf(client_id, "ESP32Client-%04X", random(0xFFFF));
  Serial.print("客户端ID: ");
  Serial.println(client_id);
  
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  // 检查MQTT连接状态，如果断开则重新连接
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  // 定期发布消息
  unsigned long currentMillis = millis();
  if (currentMillis - lastMsg > MSG_INTERVAL) {
    lastMsg = currentMillis;
    
    // 生成示例数据
    float temperature = random(200, 300) / 10.0; // 模拟温度数据
    float humidity = random(400, 700) / 10.0;    // 模拟湿度数据
    
    // 格式化消息
    char msg[100];
    sprintf(msg, "{\"temperature\":%.1f,\"humidity\":%.1f,\"device_id\":\"%s\"}", temperature, humidity, client_id);
    
    Serial.print("发布消息: ");
    Serial.println(msg);
    client.publish(pub_topic, msg);
  }
}