# 烽火 Pro LG6851F：短信解锁、原厂 SSH 固化与 OpenWrt

面向 **LG6851F Pro / MediaTek T830（MT6990）/ eMMC / 2.4G + 5G**。这里整理了从原厂系统短信注入获取 SSH，到备份、原厂 SSH 固化、安装 OpenWrt 和日常升级的流程，以及本次适配使用的完整工程源码。

**本仓库的解锁路线只有短信注入。** 不收录其他解锁方式。非烽火Pro2024、NAND/UBI 设备不能使用本项目分区镜像。

## V2.10 固件与工具下载

[本版 Release](https://github.com/DnnDaa/LG6851F-Pro/releases/tag/V2.10) · [完整更新说明](docs/releases/V2.10.md) · [历来更新记录](CHANGELOG.md)

基于 **systemfix-94 / Linux 6.18.44 旧分支**，同时提供 A/B 两槽。B 包保留 A 槽原厂系统与共享 SSH 文件；A 包仅适用于明确准备覆盖 A 槽的设备。网页包用于同槽升级，不直接负责跨槽首次安装。

| 文件 | 下载 |
|---|---|
| A 槽 网页升级 | [lg6851f-pro-V2.10-public-A-sysupgrade.tar.gz](https://github.com/DnnDaa/LG6851F-Pro/releases/download/V2.10/lg6851f-pro-V2.10-public-A-sysupgrade.tar.gz) |
| A 槽 MTK Download Only | [lg6851f-pro-V2.10-public-A-mtk.zip](https://github.com/DnnDaa/LG6851F-Pro/releases/download/V2.10/lg6851f-pro-V2.10-public-A-mtk.zip) |
| B 槽 网页升级 | [lg6851f-pro-V2.10-public-B-sysupgrade.tar.gz](https://github.com/DnnDaa/LG6851F-Pro/releases/download/V2.10/lg6851f-pro-V2.10-public-B-sysupgrade.tar.gz) |
| B 槽 MTK Download Only | [lg6851f-pro-V2.10-public-B-mtk.zip](https://github.com/DnnDaa/LG6851F-Pro/releases/download/V2.10/lg6851f-pro-V2.10-public-B-mtk.zip) |
| 完整工程/改动源码及精确运行层 | [source.tar.gz](https://github.com/DnnDaa/LG6851F-Pro/releases/download/V2.10/lg6851f-pro-V2.10-source.tar.gz) |
| 校验清单 | [SHA256SUMS.txt](https://github.com/DnnDaa/LG6851F-Pro/releases/download/V2.10/SHA256SUMS.txt) |
| Windows MTK 工具/驱动 | [历史工具包](https://github.com/DnnDaa/LG6851F-Pro/releases/download/lg6851f-public-20261015/lg6851f-windows-flash-tools.zip) |

V2.10 累积修复：AP/客户端与 BSS 槽位复用、TurboACC 和独立 NAT 模式、HNAT 重复统计开销、动态蜂窝 wan6 与 LAN IPv6、共享快照、概览局部异常隔离、中文及响应式布局。保留原厂 Wi-Fi7/MLO 和 HNAT/PPE/WED、BBR/CAKE、蜂窝、风扇、五灯、OpenClash/Mihomo、Nikki。公开版去除个人配置，首启从本机只读取得校准。

本轮完成签名/根层/模块/槽位/包一致性检查；公开 A/B 组合尚未重新刷入冷启动、跨设备及长期验收，不保证固定 CPU 百分比。精确重打包及重编译边界见 [源码说明](docs/releases/V2.10-source.md)。历史版本见 [v1.0.2](docs/releases/v1.0.2.md)，当时行为不应当成 V2.10 现状。

## 从这里开始

1. [购买后核对型号、批次与文件](docs/01-设备与文件.md)
2. [短信注入获得 SSH](docs/02-短信注入SSH.md)
3. [备份自己的设备与原厂 SSH 固化](docs/03-备份与SSH固化.md)
4. [首次刷入 OpenWrt A 槽](docs/04-首次刷机与回退.md)
5. [网页升级、首次设置与常见问题](docs/05-升级与使用.md)
6. [源码结构、编译与版本边界](docs/06-源码与编译.md)
7. [发布文件与校验](docs/07-发布与校验.md)


基础版本 [openwrt-mt6990-lg6851f](https://github.com/13179415360/openwrt-mt6990-lg6851f)
什么是新的：

- 移植了原厂固件的所有硬件加速HNAT/WED功能（蜂窝《=》WIFI；蜂窝《=》有线；有线《=》无线）并加入了 WIFI《=》WIFI（同BISSID）硬件加速。
- 两个频段的无线设置独立保存；5G 保留 BE/160MHz、固定信道44的配置能力。实际信道、功率仍由硬件、驱动和当地监管约束决定。
- CPU频率显示及挡位设定（省电/均衡/性能），性能挡位下可拉满2200MHZ，coremark跑分单核7985/四核30167
- 风扇控制：安静/均衡/强劲/自定义，实际转速和当前PWM展示
- 五个物理灯位控制（包含熄灭/开启/闪烁），4G与5G有绿蓝红三色可定义
- 状态看板、蜂窝状态与小区。
- Nikki 及所需内核功能、软件包、本地源与 iperf3；512MiB swap 保留。
- CCCI/WED 周期休眠使用空闲等待，保留原监控与恢复行为。
- 保留信号刷新缓存修复、原生频段/小区锁定读回、开机恢复和升级 overlay 刷新机制。

**最新功能以源码和发布清单为准。** 源自本机的此前版本已有用户使用反馈；本次公开版另做了个人配置清理和本机校准读取，完成离线内容检查，尚未做跨设备刷入和长期稳定性验证。提供的完整源码和编译入口在此次整理中没有重新执行从零全量编译，不把离线打包称为跨设备验证。

## 公开版与自用版的区别

公开版不包含原设备的 Wi-Fi 密码、固定 MAC 配置、SSH 主机密钥、订阅、NV/校准备份或维修日志。驱动启动前从**刷机设备自己的** `fh_data` 分区只读取得 `wlan/e2p`，放在RAM供驱动使用，不写厂商分区。缺失或布局不匹配会停止无线启动并记录原因，不用别人的校准文件兜底。

首次安装建议电脑接 LAN。管理地址通常为 `192.168.1.1`，先设置 root 密码；公开版首次生成随机 Wi-Fi 密码，可在无线页面查看并自行设置。新设备不会沿用作者的 SSID、密码或锁小区目标；保留配置升级的设备会保留自己的设置。

公共默认Wi-Fi MAC根据本机eMMC标识及校准内容生成稳定的本地单播地址，两频段不同；不承诺与铭牌的原厂MAC一致，也不会输出或上传eMMC标识。

## 源码与文件

这个目录本身是 OpenWrt 工程根目录。`target/`、`package/`、`toolchain/`、`tools/` 等保留当前工程；`vendor-source/` 是实际用于6.18移植的原厂驱动源码，`drivers/` 为新增CPU/电源灯驱动，`files/` 是最新公开运行层，`feeds-snapshot/` 固定实际使用的 feeds 内容。

固件、Windows MTK 工具与USB驱动放到 GitHub **Releases**，源码放仓库。旁边的本地 `release-assets/` 不是设备备份目录。不要把编译产生的私钥或自己操作后生成的 `private-device-work/` 上传。

## 致谢与许可

基础工程参考 [openwrt-mt6990-lg6851f](https://github.com/13179415360/openwrt-mt6990-lg6851f)，加速参考 [ImmortalWrt MT798x](https://github.com/chasey-dev/immortalwrt-mt798x-rebase) 与相关 MediaTek 实现。短信注入来源见教程中的 Codming 原文链接。

保留 OpenWrt、Linux、各软件包和原厂源码中的原有版权/许可声明；本项目没有把整套第三方代码、固件和Windows工具重新许可为同一种许可证。新增脚本注明GPL-2.0-only。公开MTK测试签名材料单独校对了既有公开来源，不含设备个人私钥或生产私钥，详见 [签名材料说明](docs/06-源码与编译.md#签名材料)。

