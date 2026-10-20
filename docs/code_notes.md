# 代码笔记

关键函数、参数、数据结构与背后的原理。

## 1. 参数速查

| 参数 | 值 | 含义 |
|---|---|---|
| TIM2 时钟 | 72 MHz | APB1 分频后，定时器时钟仍为 72 MHz（硬件自动 ×2） |
| `TIM_Prescaler` | 71 | 72 MHz ÷ 72 = 1 MHz，1 个计数 = 1 μs |
| `TIM_Period` | 19999 | 20000 个计数 = 20 ms = 50 Hz |
| `TIM_Pulse`（CCR） | 脉宽(μs) | 1000 / 1500 / 2000 = 一侧 / 中位 / 另一侧 |
| TIM3 `PSC` / `ARR` | 7199 / 99 | 10 kHz × 100 = 10 ms |
| UART | 115200 8N1 | PA9 = TX，PA10 = RX |

**角度 → 脉宽**

```
pulse_us = 500 + angle × 2000 / 180
```

90° → 1500 μs，0° → 500 μs，180° → 2500 μs。

## 2. 数据结构

### `Axis_t`（定义在 `config.h`）

```c
typedef struct
{
    int16_t current;   /* 当前角度，由 TIM3 中断逐步修改 */
    int16_t target;    /* 目标角度，由串口指令写入 */
    int16_t minDeg;    /* 安全下限 */
    int16_t maxDeg;    /* 安全上限 */
} Axis_t;

extern Axis_t g_axis[AXIS_COUNT + 1];
```

下表：**下标 1 = 偏航，下标 2 = 俯仰，下标 0 空置**。
让轴编号从 1 开始，正好和 `TIM2_CH1` / `TIM2_CH2` 一一对应，少一层换算。

## 3. 关键函数

### `Servo_Init(void)`

初始化 TIM2 的两个通道输出 50 Hz PWM。

- **两个时钟要开**：`RCC_APB2Periph_GPIOA`（APB2）+ `RCC_APB1Periph_TIM2`（APB1）
- **两个引脚一起配**：`GPIO_Pin_0 | GPIO_Pin_1`，模式 `GPIO_Mode_AF_PP`
- **时基只配一次**：两通道共用 `CNT` 和 `ARR`
- **通道各配一次**：`TIM_OC1Init` / `TIM_OC2Init`，且各自 `PreloadConfig`

### `Servo_AngleToPulse(int16_t deg)`

```c
us = (uint32_t)SERVO_MIN_US
   + (uint32_t)deg * (SERVO_MAX_US - SERVO_MIN_US) / SERVO_ANGLE_SPAN;
```

**必须用 32 位中间变量**：`180 × 2000 = 360000`，超过 `uint16_t` 上限 65535。
临界点在 `deg ≥ 33`——超过就溢出回绕，表现是"角度越大位置越离谱"，
而小角度一切正常，极难排查。

### `Servo_SetAngle(uint8_t axis, int16_t deg)`

立即设置某轴角度。三步：

1. 按轴取 `minDeg` / `maxDeg`，把 `deg` 夹进范围（限位）
2. 换算成脉宽
3. 按轴选 `TIM_SetCompare1`（偏航）或 `TIM_SetCompare2`（俯仰）

这是**唯一的限位入口**。任何绕过它直接写 CCR 的代码都会失去保护。

### `Servo_MoveTo(uint8_t axis, int16_t deg)`

只写 `g_axis[axis].target`，**不产生任何运动**，函数体内没有延时。

这是"非阻塞"的实现关键：调用后立刻返回，真正的移动交给 `Servo_Update`。
串口中断里调用它是安全的（不会长时间占用中断）。

### `Servo_Update(void)`

由 TIM3 中断每 10 ms 调用一次，遍历所有轴：

```c
if (g_axis[i].current < g_axis[i].target)      g_axis[i].current++;
else if (g_axis[i].current > g_axis[i].target) g_axis[i].current--;

Servo_SetAngle(i, g_axis[i].current);
```

每轴每步 1°，所以转速 ≈ **100°/秒**。改 TIM3 周期即可改速度，无需改这个函数。

### `USART1_IRQHandler(void)`

每收到 1 字节进一次，职责只有两件：

1. 收到 `\r` 或 `\n` → 把缓冲区当一帧交给 `UART_ParseCommand`，然后清零
2. 否则存入缓冲区；缓冲区满（32 字节）则丢弃整行重新开始

读 `USART_ReceiveData()` 就是读 DR，**会自动清 RXNE**，不需要手动 `ClearITPendingBit`。
中断里不做延时、不做长时间等待。

### `UART_ParseCommand(char *cmd)`

| 首字符 | 含义 |
|---|---|
| `A` / `a` | 偏航 Yaw |
| `B` / `b` | 俯仰 Pitch |
| 其他 | 回 `ERR` |

`atoi(&cmd[1])` 取数字，检查 0~180 后调用 `Servo_MoveTo`，回 `OK`。

注意 `atoi` 对非数字串返回 0，所以 `Aabc` 会被当成 `A0`（再被限位钳到 10°）。
不会崩溃，但结果不直观——如果要严格，需要额外校验每个字符都是数字。

### `Delay_us` / `Delay_ms`（delay.c）

基于 SysTick，72 MHz 下 `LOAD = 72 × us`，`CTRL = 0x00000005`
（bit0 使能 + bit2 时钟源选 HCLK）。

**上限**：`SysTick->LOAD` 是 24 位寄存器，最大 16777215，
所以单次 `Delay_us` 不能超过约 **233 ms**；更长的延时用 `Delay_ms` 循环调 `Delay_us(1000)`。

## 4. 原理补充

### PWM1 模式怎么产生波形

`CNT < CCR` 时输出有效电平（高），`CNT ≥ CCR` 时输出无效电平（低）。
因为 1 个计数 = 1 μs，所以 **CCR 的数值直接等于高电平的微秒数**，不需要任何换算。

### 为什么周期必须是 20 ms

位置型舵机内部按 20 ms 周期解码脉宽。改频率会改变"脉宽—角度"的对应关系，
多数模拟舵机只认 50 Hz，所以 `ARR` 不建议随意修改。

### 为什么舵机需要持续信号

舵机每个周期都重新从脉冲宽度里读取目标角度，内部没有"记住上次目标"。
信号中断后目标消失，电机失去力矩，表现为松力、被外力带走，部分型号会抖动。

### 为什么两个轴共用一个定时器

`TIM2` 的 4 个通道**共用同一个 `CNT` 和 `ARR`**，所以周期天然完全同步；
而每个通道有独立的 `CCR`，脉宽互不影响。这正好是舵机需要的关系。
用两个定时器也能做，但会引入不必要的频率/相位差异，且代码更繁琐。
