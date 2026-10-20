# 调试记录

按时间记录遇到的问题：现象 → 原因 → 解决 → 验证。
越靠前的越新。

---

## 2026-10-20 · GitHub 推送失败：CONNECT tunnel failed, response 502

**现象**

`git push` 报错：

```
fatal: unable to access 'https://github.com/...': CONNECT tunnel failed, response 502
```

**排查过程**

| 测试 | 结果 |
|---|---|
| 直连 `github.com` | 20 s 完全超时 |
| 经代理访问 `www.baidu.com` | 0.10 s，HTTP 200 |
| 经代理访问 `api.github.com` | 0.48 s，HTTP 200 |
| 经代理访问 `github.com` | 25 s 只收完一半，超时 |
| `ssh -T git@github.com` | **认证成功** |

**原因**

本机代理（`127.0.0.1:12944`）本身正常，`api.github.com` 也通，
但**对 `github.com` 的 HTTPS CONNECT 隧道转不出去**，所以 git 走 HTTPS 必然失败。
与仓库配置、账号权限无关。

**解决**

改用 SSH 地址：

```bash
git remote set-url origin git@github.com:DOURENCHI/STM32-pan-tilt-gimbal.git
git push -u origin main
```

**验证**

- `git ls-remote origin` 返回正确 commit
- GitHub API 显示根目录 6 项，`private: false`

**经验**

这台机器推 GitHub **一律用 SSH**，不要再试 HTTPS + token。
SSH 走独立通道，不受 `http_proxy` 环境变量影响。

---

## 2026-10-20 · 下载成功但舵机完全不动

**现象**

Keil 显示下载完成，但两个舵机毫无反应：没有声音，用手转也没有阻力变化。

**原因**

`Options for Target` → `Debug` → `Settings` → `Flash Download` 里
**没有勾选 `Reset and Run`**。下载完成后芯片停在复位状态，`main()` 从未执行。

**解决**

勾选 `Reset and Run`，重新下载（F8）。

**验证**

舵机立即执行上电动作：先回中位，再按程序移动。

**经验**

"下载成功"只代表 Flash 写入完成，不代表程序在跑。
判断方法：看 PA0 平均电压，约 0.25 V 说明有 PWM 输出。

---

## 2026-10-20 · 链接失败：Undefined symbol main

**现象**

```
.\Objects\Gimbal.axf: Error: L6218E: Undefined symbol main (referred from __rtentry2.o).
".\Objects\Gimbal.axf" - 1 Error(s), 0 Warning(s).
Target not created.
```

**原因**

把课程的**函数片段**整段贴进 `main.c` 后，文件里只有 `Servo_Init`、
`Servo_SetAngle`、`Gimbal_MoveTo` 等函数，**没有任何 `main()`**。
链接器找不到程序入口。

**解决**

在 `main.c` 末尾补上：

```c
int main(void)
{
    Servo_Init();
    /* ... 初始化与主循环 ... */
}
```

**验证**

`Build Output` 出现 `Program Size: Code=... RO-data=...`，`0 Error(s)`，
`Objects` 下生成 `Gimbal.axf`。

**经验**

`L6218E: Undefined symbol X` 一律理解为"**X 没有实现**"：
查是否漏写函数、漏把 `.c` 加进工程、函数名拼错。与路径、权限无关。

---

## 2026-10-20 · 链接失败：Undefined symbol Delay_ms（表现为"下载失败"）

**现象**

下载时提示：

```
Error: Flash Download failed - Could not load file '...\Objects\Gimbal.axf'
```

但 `Build Output` 里其实是：

```
linking...
.\Objects\Gimbal.axf: Error: L6218E: Undefined symbol Delay_ms (referred from main.o).
Target not created.
```

**原因**

`main.c` 调用了 `Delay_ms()`，但工程里没有 `delay.c`（也没有 `delay.h`）。
编译阶段（生成 `.o`）全部通过，**错误只出现在链接阶段**，
所以界面看起来像"下载失败"，实际是"根本没有生成可下载的文件"。

**解决**

新增 `delay.c` / `delay.h`（SysTick 实现），加入 Keil 的 `User` 组，
并在 `main.c` 顶部 `#include "delay.h"`。

**验证**

`Objects` 下出现 `Gimbal.axf`（约 300 KB），下载后程序正常运行。

**经验**

看到 `Could not load file xxx.axf`，**第一件事是看 `Build Output` 末尾有没有 `Target not created`**：

- 有 → 链接失败，`.axf` 没生成 → 查符号缺失
- 没有但文件确实不存在 → 才考虑路径、输出目录配置问题

---

## 2026-10-20 · 误判记录：把链接错误当成中文路径问题

**经过**

最早看到 `Could not load file ...\Objects\Gimbal.axf`，路径里有 5 段中文
（`工程\方向与规划\项目\二维云台\二维云台`），直接判断为"中文路径导致 Keil 出问题"。

**结果**

把整个工程迁移到 `D:\STM32_Project\Gimbal` 后，**报错完全一样**。
真正原因是 `Undefined symbol Delay_ms`，与路径无关。

**教训**

Keil 报"找不到 `.axf`"时，**先确认 `.axf` 到底有没有生成**，
再判断是"文件没生成"还是"生成了但找不到"。这两种情况的处理方式完全不同，
猜错方向会浪费大量时间。

**保留的改进**

虽然不是本次主因，工程仍放在纯英文短路径下——这是好习惯，
可以避免另一类真实存在的兼容性问题（路径过长、非 ASCII 字符解析失败）。
