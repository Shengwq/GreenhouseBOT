// 引入必要的库文件
#include <Arduino.h>      // Arduino核心库
#include <PubSubClient.h> // MQTT客户端库（发布订阅客户端）
#include <ArduinoJson.h>  // JSON处理库
#include <WiFi.h>
#include "bsp_wifi.h"
/*----------------------------------------------
 云平台数据格式说明（下发示例）：
{
  "TLED": {
    "v": 1   // 1=开灯，0=关灯
  }
}
-----------------------------------------------*/

const char *ssid = "0903";                      // WiFi网络名称（SSID）
const char *password = "0903swqlpx";            // WiFi密码

const char* mqttServer = "5ca310aa4a.st1.iotda-device.cn-south-1.myhuaweicloud.com"; // 请替换为你的 MQTT 服务器地址
const int   mqttPort = 1883;                 // 请替换为你的 MQTT 端口（建议使用 TLS/SSL）
const char* clientId = "681095820e587e2293116397_dev1_0_0_2025042909";     // 请替换为你的 MQTT 客户端 ID
const char* mqttUser = "681095820e587e2293116397_dev1";     // 请替换为你的 MQTT 用户名
const char* mqttPassword = "c89c30fc4576cb62e6492237f4f996242bde92955ed316d451241195d6e35f15"; // 请替换为你的 MQTT 密码

// ================== 全局变量 ==================
int msgId = 1;      // 消息序列号（循环使用）
float temp = 22.60; // 模拟温度值（单位：℃）
int humi = 80;      // 模拟湿度值（单位：%）
int TLED_state = 0; // LED状态（0=关，1=开）

// 网络客户端对象
WiFiClient espClient;           // WiFi客户端实例
PubSubClient client(espClient); // MQTT客户端实例

//主题
const char *pubTopic = "$oc/devices/681095820e587e2293116397_dev1/sys/properties/report";      //设备上报属性（发布主题）
const char *subTopic = "$oc/devices/681095820e587e2293116397_dev1/sys/messages/down";      //订阅主题（接收平台下发的指令）
/*----------------------------------------------
 功能：建立MQTT连接并订阅主题
 说明：
 1. 持续尝试连接直到成功
 2. 使用设备认证信息进行连接
 3. 成功连接后订阅控制主题
-----------------------------------------------*/
void mqttConnect()
{
  while (!client.connected())
  {
    Serial.println("尝试连接MQTT服务器...");

    // 使用设备ID、产品ID和TOKEN进行认证
    if (client.connect(clientId, mqttUser, mqttPassword))
    {
      Serial.println("MQTT连接成功");
      client.subscribe(subTopic); // 订阅控制主题
      Serial.println("已订阅控制主题：" + String(subTopic));
    }
    else
    {
      Serial.print("连接失败，5秒后重试...");
      Serial.print(client.state());
      delay(5000); // 等待5秒后重试
    }
  }
}

/*----------------------------------------------
 功能：构建并发送设备数据到云平台
 数据结构说明：
 {
   "id": 消息序列号,
   "dp": {
     "temp": [{"v": 温度值}],
     "humi": [{"v": 湿度值}],
     "TLED": [{"v": LED状态}]
   }
 }
-----------------------------------------------*/
void postData()
{
  JsonDocument doc; // 创建JSON文档对象

  // 处理消息序列号溢出（最大65535）
  if (msgId > 65535)
    msgId = 1;
  doc["id"] = msgId++; // 设置消息ID并自增

  // 构建数据点（Data Points）
  JsonObject dp = doc["dp"].to<JsonObject>();

  // 温度数据点（数组结构）
  JsonArray tempArr = dp["temp"].to<JsonArray>();
  JsonObject tempObj = tempArr.add<JsonObject>();
  tempObj["v"] = temp; // 设置温度值

  // 湿度数据点
  JsonArray humiArr = dp["humi"].to<JsonArray>();
  JsonObject humiObj = humiArr.add<JsonObject>();
  humiObj["v"] = humi; // 设置湿度值

  // LED状态数据点
  JsonArray ledArr = dp["TLED"].to<JsonArray>();
  JsonObject ledObj = ledArr.add<JsonObject>();
  ledObj["v"] = TLED_state; // 设置LED状态

  // 序列化JSON并发送
  char payload[256];           // 数据缓冲区
  serializeJson(doc, payload); // 将JSON对象序列化为字符串

  if (client.publish(pubTopic, payload))
  {
    Serial.println("数据发送成功：");
    Serial.println(payload); // 打印发送内容
  }
  else
  {
    Serial.println("数据发送失败！");
  }
}

/*----------------------------------------------
 功能：MQTT消息回调函数
 参数：
   - topic: 消息来源主题
   - payload: 消息内容（字节数组）
   - length: 消息长度
 说明：收到控制指令时解析并执行相应操作
-----------------------------------------------*/
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  // 将字节数组转换为字符串
  String msg;
  for (int i = 0; i < length; i++)
  {
    msg += (char)payload[i];
  }
  Serial.println("收到消息：" + msg);

  // 解析JSON数据
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, msg);

  if (error)
  {
    Serial.print("JSON解析失败：");
    Serial.println(error.c_str());
    return;
  }

  // 检查是否包含TLED控制指令
  if (doc.containsKey("TLED"))
  {
    int newState = doc["TLED"]["v"];

    // 仅当状态改变时执行操作
    if (newState != TLED_state)
    {
      TLED_state = newState;
      // digitalWrite(TLED_PIN, TLED_state); // 控制LED
      Serial.println("LED状态已更新：" + String(TLED_state));

      postData(); // 立即上报新状态
    }
  }
}

/*----------------------------------------------
 初始化函数（仅运行一次）
-----------------------------------------------*/
void setup()
{
  Serial.begin(115200); // 初始化串口通信
  Serial.println("\n设备启动中...");
  wifi_connect(ssid, password);

  // 配置MQTT客户端
  client.setServer(mqttServer, mqttPort); // 设置服务器地址和端口
  client.setKeepAlive (60); //设置心跳时间
  client.setCallback(mqttCallback);         // 设置消息回调函数
  mqttConnect();                            // 首次连接MQTT
}

/*----------------------------------------------
 主循环函数（持续运行）
-----------------------------------------------*/
void loop()
{
  // 保持MQTT连接
  if (!client.connected())
  {
    mqttConnect();
  }
  client.loop(); // 处理MQTT消息

  // 定时上报数据（每3秒）
  static unsigned long lastPost = 0;
  if (millis() - lastPost >= 3000)
  {
    postData();
    lastPost = millis(); // 重置计时器
  }
}

