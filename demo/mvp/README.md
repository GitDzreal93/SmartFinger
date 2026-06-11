# SmartFinger MVP

当前目标：打通 0.96 寸 I2C OLED，用三脚旋钮调节 `0-5` 档，并用舵机带动导电海绵做机械点击。

## 接线

| OLED | ESP32-C3 SuperMini |
|---|---|
| `GND` | `GND` |
| `VCC` | `3V3` |
| `SCL` | `GPIO9` |
| `SDA` | `GPIO8` |

## 旋钮接线

三脚旋钮只有 `A/C/B`，没有按压开关。

| 旋钮 | ESP32-C3 SuperMini |
|---|---|
| `A` | `GPIO4` |
| `C` | `GND` |
| `B` | `GPIO5` |

## 舵机接线

以 SG90 类小舵机为例：

| 舵机 | ESP32-C3 SuperMini |
|---|---|
| 红线 | `5V` / `VBUS` |
| 棕线/黑线 | `GND` |
| 黄线/橙线 | `GPIO3` |

不要把舵机红线接 `3V3`。如果 OLED 闪烁、ESP32 重启、舵机乱抖，说明 USB 供电不够，改用外部 5V 给舵机供电，并把外部电源 GND 和 ESP32 GND 接在一起。

## 操作

- 旋钮左右转：切换 0-5 档。
- `Grade 0`：停止点击。
- `Grade 1-5`：舵机自动按对应频率上下点击。
- OLED 显示当前状态、档位和理论频率。

## 档位

| 档位 | 状态 | 按下时长 | 抬起间隔 | 理论频率 |
|---:|---:|---:|---:|
| 0 | 停止 | 0ms | 0ms | 0 次/秒 |
| 1 | 运行 | 100ms | 250ms | 约 3 次/秒 |
| 2 | 运行 | 90ms | 160ms | 约 4 次/秒 |
| 3 | 运行 | 80ms | 120ms | 约 5 次/秒 |
| 4 | 运行 | 70ms | 90ms | 约 6 次/秒 |
| 5 | 运行 | 60ms | 70ms | 约 8 次/秒 |

舵机动作角度在代码里：

```cpp
SERVO_UP_US = 900;
SERVO_DOWN_US = 1500;
```

如果舵机方向反了，或按下太深/太浅，优先改这两个值。

## PlatformIO 烧录

```bash
cd mvp
pio run
pio run --target upload
pio device monitor
```

烧录后 OLED 应显示：

```text
SmartFinger
Mode: STOP
Grade: 0
Rate: 0/s
I2C SDA8 SCL9
```

如果 OLED 不亮，先检查 `GND/VCC`、`SCL/SDA` 是否接反。这个模块常见 I2C 地址是 `0x3C`，少数是 `0x3D`。
