# SmartFinger MVP V2 MOS

目标：用 ESP32-C3 SuperMini + D4184 MOS 模块控制 1 代吸盘点击头，并用旋钮调节连续点击频率档位。

这版不接舵机。点击头贴在电容屏上，ESP32 按档位输出脉冲，D4184 负责把点击头黑线拉到低电平，从而触发一次点击。

## 点击头说明书要点

从你发的说明书看，这个点击头的控制逻辑是：

1. 红线接电源正极，支持 `3.3-5V`。
2. 黑线接控制端，输出低电平时工作。
3. 一个脉冲点击一次；连续脉冲就是连续点击。
4. 如果持续导通，会变成一直按住，不适合做高频连点。

所以用 D4184 时，ESP32 不直接给点击头供电流，只控制 MOS 开关：

```text
5V/VBUS -> 点击头红线
点击头黑线 -> D4184 LOAD
D4184 导通时：点击头黑线被拉到 GND，触发点击
```

## MVP V2 点击头组件

| 组件 | 数量 | 作用 |
|---|---:|---|
| ESP32-C3 SuperMini | 1 | 主控 |
| 0.96 寸 OLED I2C | 1 | 显示档位和状态 |
| 三脚旋钮 `A/C/B` | 1 | 调节 `Grade 0-5` |
| D4184 MOS 模块 | 1 | 把点击头黑线拉低 |
| 1 代吸盘点击头 | 1 | 电容屏点击执行器 |
| 面包板 + 杜邦线 | 若干 | 免焊连接 |

## OLED 接线

| OLED | ESP32-C3 |
|---|---|
| `GND` | `GND` |
| `VCC` | `3V3` |
| `SCL` | `GPIO9` |
| `SDA` | `GPIO8` |

## 旋钮接线

| 三脚旋钮 | ESP32-C3 |
|---|---|
| `A` | `GPIO4` |
| `C` | `GND` |
| `B` | `GPIO5` |

## D4184 控制侧接线

你的 D4184 模块绿色小端子丝印是 `GND/PWM`：

| D4184 绿色端子 | ESP32-C3 |
|---|---|
| `GND` | `GND` |
| `PWM` | `GPIO3` |

`Grade 0` 时 `GPIO3=LOW`，MOS 关闭。`Grade 1-5` 时 `GPIO3` 按档位输出高电平脉冲，每个高电平脉冲让 D4184 导通一次。

## D4184 负载侧 + 点击头接线

你的 D4184 蓝色三位端子背面丝印是：

```text
+   LOAD   -
```

点击头这样接：

```text
ESP32 5V/VBUS -> D4184 蓝色端子 +
ESP32 GND     -> D4184 蓝色端子 -

点击头红线 -> ESP32 5V/VBUS
点击头黑线 -> D4184 蓝色端子 LOAD
```

如果你想先空载验证，可以先用 LED 替代点击头：

```text
ESP32 5V/VBUS -> 330Ω-1kΩ 电阻 -> LED 正极
LED 负极       -> D4184 蓝色端子 LOAD
```

LED 能随档位闪烁后，再换成点击头。

## 不用 D4184 的直连备选

说明书写的是黑线接单片机 IO、输出低电平工作，所以理论上也可以这样接：

```text
点击头红线 -> ESP32 3V3
点击头黑线 -> ESP32 GPIO3
```

但直连时 GPIO3 的逻辑要改成低电平触发，并且 GPIO 要承担点击头电流。当前 `mvp_v2_mos/src/main.cpp` 默认是给 D4184 用的：`GPIO3=HIGH` 表示点击。直连时把代码顶部改成：

```cpp
constexpr int TAP_ACTIVE_LEVEL = LOW;
constexpr int TAP_IDLE_LEVEL = HIGH;
```

## 总接线图

```text
OLED:
GND -> ESP32 GND
VCC -> ESP32 3V3
SCL -> ESP32 GPIO9
SDA -> ESP32 GPIO8

旋钮:
A -> ESP32 GPIO4
C -> ESP32 GND
B -> ESP32 GPIO5

D4184 控制侧:
PWM -> ESP32 GPIO3
GND -> ESP32 GND

D4184 负载侧:
+    -> ESP32 5V/VBUS
-    -> ESP32 GND
LOAD -> 点击头黑线

点击头:
红线 -> ESP32 5V/VBUS
黑线 -> D4184 LOAD
```

## 档位

| 档位 | 点击脉冲 | 释放间隔 | 理论频率 |
|---:|---:|---:|---:|
| 0 | 0ms | 0ms | 0 次/秒 |
| 1 | 70ms | 260ms | 约 3 次/秒 |
| 2 | 55ms | 195ms | 约 4 次/秒 |
| 3 | 45ms | 155ms | 约 5 次/秒 |
| 4 | 35ms | 105ms | 约 7 次/秒 |
| 5 | 25ms | 75ms | 约 10 次/秒 |

预期现象：

```text
Grade 0：停止点击
Grade 1：慢速连点
Grade 2-5：逐渐变快
```

建议第一次贴屏测试从 `Grade 1` 开始。如果屏幕识别不稳定，可以把代码里的 `tapMs` 稍微加大，比如 Grade 1 从 `70ms` 调到 `90ms`。

## 烧录

```bash
cd mvp_v2_mos
pio run
pio run --target upload --upload-port /dev/cu.usbmodem1301
```
