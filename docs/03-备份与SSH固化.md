# 3. 备份自己的设备，再考虑原厂 SSH 固化

## 备份

首先把备份保存到电脑私有目录，不放在公开源码或Release附件目录。至少保留完整GPT/misc、A/B boot与rootfs、fh_data、fh_conf、NV/protect/raw分区及本机 `wlan/e2p`，记录哈希和当前槽。需要恢复时使用**同一台设备**的备份。

登录SSH后只读盘点：

```sh
cat /proc/cmdline
cat /proc/partitions
for p in /sys/class/block/mmcblk0p*/uevent; do cat "$p"; done
mount
```

可以使用 `tools/backup/backup_stock.py` 从电脑通过SSH流式读取并校验，避免把大镜像堆到CPE的 `/tmp` 或数据分区。工具会交互询问密码、首次主机指纹，不把凭据存入仓库。

```powershell
python -m pip install paramiko
python tools\backup\backup_stock.py --host 192.168.8.1 --output E:\private-device-work\backup
```

默认备份关键分区、自己校准文件及分区映射；需要全盘时增加 `--full-disk`，电脑空间要足够。没有完成自己的备份前不要操作固化或刷机。

## 临时开启不等于固化

`/data/lg6851f-stock-ssh-bootstrap.sh` 可以持久保存，但脚本文件存在并不代表启动时会自动运行。这里核对的LG6851F原厂 `rcS` 是厂商定制启动链，并非标准OpenWrt遍历 `rc.d`；仅执行 `dropbear enable` 不足以证明固化成功。

若准备马上刷OpenWrt，可以完成备份后直接进入下一章；本项目OpenWrt已自带持久SSH，无需先修改原厂rootfs。若确实要长期保留“原厂系统+SSH”，再使用下面的离线固化工具。

## 原厂固化：仅离线修改自己的 rootfs

在Linux中操作自己导出的**128MiB、直接SquashFS格式**原厂镜像：

```sh
python3 tools/unlock/patch_stock_rootfs.py /private/rootfs_a.img --work-dir /private/stock-ssh-a
```

工具识别LG6851F原厂启动脚本，在原启动链末尾追加一个最长120秒的后台启动窗口，调用自己已经放在 `/data/` 的引导文件；保留原启动过程，不启用未知overlay，也不自动写设备或重启。输出 `rootfs-owner-ssh.img` 和哈希记录。脚本、密码哈希、主机密钥都属于操作人的私有资料，不作为公开成品提供。

把输出与其**对应的原厂槽位和原厂boot**配对，通过MTK工具仅刷该槽rootfs；其他分区不选。不可把原厂rootfs与OpenWrt boot混搭，也不能把A文件改名当B文件。具体槽位按自己备份记录决定。首次固化工具尚无本次新设备启动实测，所以保留另一原厂槽与恢复文件，刷入后确认一次正常重启仍能SSH，才算固化完成。

本教程不提供LG6121F的MTD解锁、UBI卷扩容或全A/B分区同步操作；这些不是此Pro设备的分区方案。

