# TK-ActuatorCompat

天工行者无疆（Walker TienKung Pro）机器人因时灵巧手 / EG2-4C 电动夹爪兼容包。

当前版本为 `v0.1.4`，面向 TG2.0-Pro `v2.0.7.3`，支持以下四种末端组合：

- 左右双灵巧手
- 左灵巧手＋右夹爪
- 左夹爪＋右灵巧手
- 左右双夹爪

## 适用环境

- Ubuntu 22.04
- ROS 2 Humble
- TG2.0-Pro v2.0.7.3
- 因时 EG2-4C 系列电动夹爪

当前夹爪配置固定使用左夹 ID 18、右夹 ID 28、Usb2CAN master 1、CAN channel 1。

## 仓库内容

```text
.
├── gripper_full/    # 节点源码、安装脚本、配置样例和详细部署说明
├── docs/            # 首次使用、协议、硬件及测试文档
├── inspire_gripper_full_v0.1.4_mixed_final_v2073.tar.gz
└── SHA256SUMS
```

## 使用入口

1. 先阅读 [`docs/须知.txt`](docs/%E9%A1%BB%E7%9F%A5.txt) 和 [`docs/DISCLAIMER.md`](docs/DISCLAIMER.md)。
2. 查看 [`gripper_full/README.md`](gripper_full/README.md) 中的完整部署、配置、编译与验收步骤。
3. 部署前校验压缩包：

   ```bash
   sha256sum -c SHA256SUMS
   ```

> 请勿直接用样例文件覆盖机器人现有配置。应先备份，再按实际末端组合逐项手工修改。

## 已验证范围

- 最新版 BodyControl 与独立夹爪节点可同时启动
- 左右夹爪可打开、闭合
- 左右夹爪状态话题可持续发布
- 可按安装开关隔离未安装一侧的 ROS 接口与 CAN 查询

## 安全提示

机器人和电动夹爪存在夹持、碰撞等机械风险。请由受过培训的人员在可控环境中操作，首次测试时限制速度和力度，并确保急停与物理防护措施可用。

