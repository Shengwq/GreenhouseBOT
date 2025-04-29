#include <WiFi.h> // WiFi连接库

void wifi_connect(const char *ssid, const char *password)
{
    // 连接WiFi网络
    Serial.print("正在连接WiFi: " + String(ssid));
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    uint8_t wait = 0;
    while (WiFi.status() != WL_CONNECTED && wait <= 50)
    {
        delay(200);
        wait++;
        Serial.print(".");
    }
    if (wait > 50)
    {
        Serial.println("WiFi Connect fail");
        wait = 0;
    }
    else
    {
        Serial.println("WiFi Connect success!");
        Serial.println("\r\nWiFi连接成功！IP地址：" + WiFi.localIP().toString());
    }
}
