# SmartFinger Desktop Control

本目录是电脑本地控制页。

## 作用

1. 浏览器直连 ESP32 串口
2. 自动把电脑当前时间同步给设备
3. “极速抢票”使用窄幅随机高速节奏，手动开始和停止
4. “档位点击”控制 `Grade 1-5` 持续循环点击
5. “定时触发”倒计时或按指定日期时间启动 `Grade 6`
6. 设置 `Grade 1-5` 的自定义固定频率
7. 查看设备状态和串口日志

连接顺序固定为：先连接串口，再选择“立即执行”或“定时执行”。设备没有返回 `STATE` 前，执行区域保持锁定，避免误以为命令已经生效。

## 启动

Web Serial 需要 `localhost` 或安全上下文，不能直接双击 `file://` 打开。

推荐在项目根目录运行：

```bash
python3 -m http.server 4173 -d desktop-control
```

然后在浏览器打开：

```text
http://localhost:4173
```

也可以直接双击 `launcher/SmartFinger.app`。它会自动启动本地服务并打开控制页，适合不想碰命令行的使用场景。

## 浏览器要求

推荐：

1. Google Chrome
2. Microsoft Edge

Safari 目前不支持 Web Serial。

## 串口占用

烧录固件前先点击页面上的“断开串口”。页面在连接失败、设备拔出、刷新或关闭时也会主动释放串口。
