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

正式版沿用 `demo/mvp_v2_mos` 的稳定参数：

| 档位 | 点击脉冲 | 释放间隔 | 理论频率 |
|---:|---:|---:|---:|
| 0 | 0ms | 0ms | 0 次/秒 |
| 1 | 70ms | 260ms | 约 3 次/秒 |
| 2 | 55ms | 195ms | 约 4 次/秒 |
| 3 | 45ms | 155ms | 约 5 次/秒 |
| 4 | 35ms | 105ms | 约 7 次/秒 |
| 5 | 25ms | 75ms | 约 10 次/秒 |

默认仍然是：

1. 上电 `Grade 0`
2. `GPIO3=LOW` 时停止
3. `GPIO3=HIGH` 时驱动 D4184 导通，点击头黑线被拉低

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

## 测试

```bash
node firmware/test/display_model.test.js
cd firmware && pio run
```

