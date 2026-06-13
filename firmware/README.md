# SmartFinger Firmware

`firmware/` 是正式工程目录，基于已经验证稳定的 `demo/mvp_v2_mos` 重构而来。

目标：

1. 保持当前点击头连点行为不变。
2. 把 GPIO、档位参数、点击调度、旋钮输入、OLED 显示拆成独立模块。
3. 给后续增加按键、模式切换、BLE 或参数保存预留清晰边界。

## 目录结构

```text
firmware/
  include/
    App.h
    AppConfig.h
    ClickController.h
    ClickTypes.h
    EncoderInput.h
    StatusDisplay.h
  src/
    App.cpp
    ClickController.cpp
    EncoderInput.cpp
    StatusDisplay.cpp
    main.cpp
  test/
    display_model.test.js
  tools/
    display_model.js
```

## 当前行为

正式版沿用 `demo/mvp_v2_mos` 的稳定参数，并增加一个高仿真第 6 档：

| 档位 | 点击脉冲 | 释放间隔 | 理论频率 |
|---:|---:|---:|---:|
| 0 | 0ms | 0ms | 0 次/秒 |
| 1 | 70ms | 260ms | 约 3 次/秒 |
| 2 | 55ms | 195ms | 约 4 次/秒 |
| 3 | 45ms | 155ms | 约 5 次/秒 |
| 4 | 35ms | 105ms | 约 7 次/秒 |
| 5 | 25ms | 75ms | 约 10 次/秒 |
| 6 | 自适应 | 自适应 | 5 秒高仿真模式 |

默认仍然是：

1. 上电 `Grade 0`
2. `GPIO3=LOW` 时停止
3. `GPIO3=HIGH` 时驱动 D4184 导通，点击头黑线被拉低

## Grade 6 高仿真模式

`Grade 6` 会启动一次固定 `5` 秒的高仿真点击流程：

1. 以微秒级时间轴作为零点开始运行。
2. 前 `10%` 时间保持爆发节奏，后续按疲劳曲线逐步放慢，到结尾约比初始慢 `12%`。
3. 每个周期有 `4%` 概率插入 `80-120ms` 的长停顿。
4. 普通周期会在疲劳基准上叠加高斯随机波动，但总周期不会低于 `28ms`。
5. 按下时长以 `18ms` 为中心波动，最低不低于 `12ms`。
6. 如果按下过长导致松开时间不足 `12ms`，系统会自动缩短按下时间，优先保证松开可识别。
7. 到达 `5` 秒终点后，系统会强制执行一次松开并停止，不会在 `Grade 6` 上自动重复启动。

每次 `Grade 6` 跑完后，串口会输出实际耗时、点击次数和平均 CPS。

## 定时启动

现在可以通过串口给 `Grade 6` 设时间，让它自己倒计时并自动触发。

可用命令：

```text
TIME HH:MM:SS   设置软件时钟
AT HH:MM:SS     设定某个时刻自动启动 Grade 6
IN 5            设定 5 秒后自动启动 Grade 6
CANCEL          取消已设定的自动启动
STATUS          查看当前时钟和定时状态
```

推荐用法：

```text
TIME 22:05:00
AT 22:05:10
```

或者直接相对倒计时：

```text
IN 3.5
```

等待期间 OLED 会显示：

1. `Mode: ARMED`
2. `Left: 剩余秒数`
3. `Step: WAIT`

到时后会自动进入 `Grade 6` 的 5 秒高仿真点击流程。

## 电脑 GUI 控制

现在可以不再依赖旋钮，直接用电脑本地控制页操作。

控制页目录：

```text
desktop-control/
```

一键启动器目录：

```text
launcher/SmartFinger.app
```

启动方式：

```bash
python3 -m http.server 4173 -d desktop-control
```

然后在浏览器打开：

```text
http://localhost:4173
```

也可以直接双击 `launcher/SmartFinger.app`。

控制页支持：

1. 浏览器直连 ESP32 串口
2. 自动同步电脑当前时间
3. “立即执行”只控制 `Grade 1-5`，持续循环直到手动停止
4. “定时执行”只控制 `Grade 6`，到点后自动运行 5 秒
5. 自定义固定频率 `tap_ms / rest_ms`
6. 倒计时启动和指定时刻启动
7. 实时查看 `STATE` 状态和串口日志

新增串口命令：

```text
SELECT <1-5>
START
STOP
PROFILE <tap_ms> <rest_ms>
PROFILE CLEAR
STATUS
```

## 模块职责

`AppConfig.h`

集中管理引脚、OLED 参数、档位时序和电平定义。

`ClickController`

只负责点击状态机，不关心旋钮和 OLED。

`EncoderInput`

只负责读取旋钮增减方向，输出 `-1 / 0 / 1`。

`StatusDisplay`

只负责 OLED 显示。如果 OLED 初始化失败，不影响点击逻辑继续运行。

`App`

负责把输入、控制器和显示模块编排在一起。

## 烧录

```bash
cd firmware
pio run
pio run --target upload --upload-port /dev/cu.usbmodem1301
```

固件已启用 ESP32-C3 原生 USB CDC，网页通过 Web Serial 发送的命令会由固件的 `Serial` 接收。烧录前请先在控制页点击“断开串口”；连接失败时页面也会自动释放串口。

## 测试

```bash
node firmware/test/display_model.test.js
node desktop-control/test/control-model.test.mjs
node --check desktop-control/app.js
cd firmware && pio run
```
