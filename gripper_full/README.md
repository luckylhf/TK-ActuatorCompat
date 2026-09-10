# Inspire 灵巧手/夹爪混装兼容版 v0.1.4（2.0.7.3 正式交付版）

## 1. 作用和已验证范围

本版本保留 TG2.0-Pro v2.0.7.3 最新版 `libbodyctrl_component.so`，通过 `LD_PRELOAD` 在同一个 `bodyctrl_nodes` 进程中启动独立夹爪节点，并共享新版 BodyControl 已初始化的 Usb2CAN master 1 / channel 1。

本交付包可以配置为四种工作模式：左右双手、左手＋右夹爪、左夹爪＋右手、左右双夹爪。其中后三种模式会启动 v0.1.4 独立夹爪节点。v0.1.4 增加左右夹爪独立安装开关；未安装的夹爪侧不会创建 ROS 接口、发送 CAN 查询或发布虚假的通信丢失码 `33072`。

已经实机验证：

- 最新版 BodyControl 正常启动，机器人指示灯正常；
- 混装配置下最新版 BodyControl 与独立夹爪节点可同时启动；
- 左右夹爪可以打开、闭合；
- 左右状态话题可以持续发布；
- v0.1.1 起已修复状态/错误字段顺序。CAN 返回的前 16 位是错误位，后 16 位是运行状态；v0.1.0 会把正常全开、全闭错误显示成 `11/12`；
- v0.1.2 增加面向原始 2.0.7.3 包的完整夹爪资源；
- v0.1.3 调整为“新增文件由脚本安装、已有配置由用户参照样例手工修改”，并补充灵巧手/夹爪双向切换说明。
- v0.1.4 增加 `left_gripper_installed`、`right_gripper_installed`，正式支持单手单夹混装。

目标环境为 Ubuntu 22.04、ROS 2 Humble、TG2.0-Pro v2.0.7.3。当前固定使用左夹 ID 18、右夹 ID 28、Usb2CAN master 1、CAN channel 1。

## 2. ROS 2 接口

订阅接口：

| Topic | 类型 | 用途 |
|---|---|---|
| `/inspire_gripper/left_command` | `sensor_msgs/msg/JointState` | 左夹爪运动 |
| `/inspire_gripper/right_command` | `sensor_msgs/msg/JointState` | 右夹爪运动 |
| `/inspire_gripper/clear_error_command` | `std_msgs/msg/String` | 左、右或双侧清错 |
| `/inspire_gripper/set_catch_mode_command` | `bodyctrl_msgs/msg/CmdSetParam` | 单次/持续夹持模式 |
| `/inspire_gripper/fast_stop_command` | `bodyctrl_msgs/msg/CmdSetParam` | 快速停止 |
| `/inspire_gripper/set_max_openlen_command` | `bodyctrl_msgs/msg/CmdSetParam` | 最大物理开度 |
| `/inspire_gripper/set_min_openlen_command` | `bodyctrl_msgs/msg/CmdSetParam` | 最小物理开度 |

发布接口：

| Topic | 类型 | 内容 |
|---|---|---|
| `/inspire_gripper/left_status` | `sensor_msgs/msg/JointState` | 左夹位置和力反馈 |
| `/inspire_gripper/right_status` | `sensor_msgs/msg/JointState` | 右夹位置和力反馈 |
| `/inspire_gripper/left_error` | `std_msgs/msg/UInt32` | 左夹运行状态/错误合并码 |
| `/inspire_gripper/right_error` | `std_msgs/msg/UInt32` | 右夹运行状态/错误合并码 |

运动命令的 `position`、`velocity`、`effort` 输入范围为 0～1；`position=0` 表示打开，`position=1` 表示闭合。

只有在 `inspire_gripper.yaml` 中标记为已安装的夹爪侧，才会出现该侧的 command/status/error 三个话题。至少启用一侧夹爪时，公共的五个配置和清错话题存在；左右双手模式不创建任何夹爪话题。

## 3. 文件变更清单

### 3.1 必须新增

解压后新增整个目录：

```text
/home/ubuntu/ros2ws/install/body_control/gripper_full
```

目录内容：

```text
gripper_full/
├── README.md
├── install_new_files.sh
├── examples/
│   ├── body.launch.py.modified.example
│   ├── body_no_sbus.launch.py.modified.example
│   ├── config_ethercat.double_grippers.modified.example
│   ├── config_ethercat.double_hands.modified.example
│   ├── config_ethercat.mixed.modified.example
│   ├── config_ethercat.yaml.modified.example
│   ├── inspire_gripper.double_grippers.yaml.example
│   ├── inspire_gripper.double_hands.yaml.example
│   ├── inspire_gripper.left_hand_right_gripper.yaml.example
│   ├── inspire_gripper.left_gripper_right_hand.yaml.example
│   ├── inspire_hands.double_grippers.yaml.example
│   ├── inspire_hands.double_hands.yaml.example
│   ├── inspire_hands.left_hand_right_gripper.yaml.example
│   └── inspire_hands.left_gripper_right_hand.yaml.example
├── overlay/
│   ├── lib/
│   │   └── libdev_inspire_gripper_can.so
│   └── share/body_control/param/
│       └── inspire_gripper.yaml
└── node/
    ├── CMakeLists.txt
    ├── build.sh
    ├── gripper_config.hpp
    ├── gripper_protocol.hpp
    ├── inspire_gripper_full.cpp
    ├── package.xml
    ├── test_config.cpp
    └── test_protocol.cpp
```

运行 `build.sh` 后生成：

```text
/home/ubuntu/ros2ws/install/body_control/gripper_full/node/libinspire_gripper_full.so
```

压缩包带有旧可用包的原厂 `libdev_inspire_gripper_can.so`，但不带预编译的 `libinspire_gripper_full.so`；独立全功能节点必须在目标机器人上编译。

### 3.2 必须新增到 2.0.7.3 安装目录

`install_new_files.sh` 只负责把以下两个原始 2.0.7.3 中不存在的文件新增进去：

```text
/home/ubuntu/ros2ws/install/body_control/lib/libdev_inspire_gripper_can.so
/home/ubuntu/ros2ws/install/body_control/share/body_control/param/inspire_gripper.yaml
```

这两个文件在原始 2.0.7.3 本体包中不存在。它们作为完整夹爪资源一起部署；当前独立节点的实际 CAN 收发仍由 `libinspire_gripper_full.so` 完成。

脚本不会修改或覆盖任何已有配置。如果目标位置已经存在不同内容的同名文件，脚本会停止并要求人工检查。

### 3.3 必须由用户手工修改

以下现有文件不由脚本修改。先备份，然后参照 `examples/` 中的完整“修改后”样例手工编辑。

修改：

```text
/home/ubuntu/ros2ws/install/body_control/share/body_control/param/config_ethercat.yaml
```

同时手工修改：

```text
/home/ubuntu/ros2ws/install/body_control/share/body_control/param/inspire_hands.yaml
/home/ubuntu/ros2ws/install/body_control/share/body_control/param/inspire_gripper.yaml
```

四种模式必须严格按下表同步设置。表中的 `H-L/H-R` 分别代表 `left_hand_installed/right_hand_installed`，`G-L/G-R` 分别代表 `left_gripper_installed/right_gripper_installed`。

| 工作模式 | `enable_hands` | `enable_inspire_gripper` | H-L | H-R | G-L | G-R | 夹爪 `LD_PRELOAD` |
|---|---:|---:|---:|---:|---:|---:|---|
| 左右双手 | `true` | `false` | `true` | `true` | `false` | `false` | 保留 |
| 左手＋右夹爪 | `true` | `true` | `true` | `false` | `false` | `true` | 保留 |
| 左夹爪＋右手 | `true` | `true` | `false` | `true` | `true` | `false` | 保留 |
| 左右双夹爪 | `false` | `true` | `false` | `false` | `true` | `true` | 保留 |

`enable_hands` 是原厂灵巧手 Loader 的全局开关，不是左右侧别开关。实机已经确认：左右双夹爪时，如果 `enable_hands: true`，即使 `inspire_hands.yaml` 的左右开关都是 `false`，按 A 仍可能启动失败。因此双夹爪模式必须设置 `enable_hands: false`。

四种模式下，每一侧都必须恰好启用 hand 或 gripper 中的一种：同一侧不能同时为 `true`，也不能同时为 `false`。左右双手模式必须把两个 gripper 侧别开关都设为 `false`；预加载库会检测到没有启用夹爪侧并结束夹爪节点初始化，不创建夹爪节点、话题或 CAN 轮询。

同时手工修改两个启动文件：

```text
/home/ubuntu/ros2ws/install/body_control/share/body_control/launch/body.launch.py
/home/ubuntu/ros2ws/install/body_control/share/body_control/launch/body_no_sbus.launch.py
```

在两个文件的 `yamlConfigs` 中加入：

```python
{"inspire_gripper": os.path.join(package_share_directory, "param", "inspire_gripper.yaml")},
```

在两个文件的 BodyControl `ComposableNodeContainer` 中都加入：

```python
additional_env={
    'LD_PRELOAD': '/home/ubuntu/ros2ws/install/body_control/gripper_full/node/libinspire_gripper_full.so'
},
```

每个 launch 只能保留一个指向 `libinspire_gripper_full.so` 的夹爪 `LD_PRELOAD`。四种模式都保留该配置，以后切换模式不再修改 launch。

对应的完整参考文件：

```text
examples/config_ethercat.double_hands.modified.example
examples/config_ethercat.double_grippers.modified.example
examples/config_ethercat.yaml.modified.example
examples/config_ethercat.mixed.modified.example
examples/body.launch.py.modified.example
examples/body_no_sbus.launch.py.modified.example
examples/inspire_gripper.double_hands.yaml.example
examples/inspire_gripper.double_grippers.yaml.example
examples/inspire_gripper.left_hand_right_gripper.yaml.example
examples/inspire_gripper.left_gripper_right_hand.yaml.example
examples/inspire_hands.double_hands.yaml.example
examples/inspire_hands.double_grippers.yaml.example
examples/inspire_hands.left_hand_right_gripper.yaml.example
examples/inspire_hands.left_gripper_right_hand.yaml.example
```

这些文件仅供查看和 `diff`，不能由脚本直接覆盖 2.0.7.3 的现有文件。

### 3.4 必须保持不动

| 文件 | 要求 |
|---|---|
| `lib/libbodyctrl_component.so` | 保留最新版，绝不能换成旧版 |
| `lib/libucbp.so` | 保留最新版；本节点链接并调用它 |
| `share/ament_index/resource_index/rclcpp_components/body_control` | 必须继续指向最新版主体库 |

组件索引的正确内容：

```text
body_control::BodyControl;lib/libbodyctrl_component.so
```

### 3.5 各文件的实际作用

| 文件/配置 | 结论 |
|---|---|
| `lib/libdev_inspire_gripper_can.so` | 从旧可用包补入 2.0.7.3；作为原厂夹爪驱动资源保留 |
| `param/inspire_gripper.yaml` | 记录夹爪左右安装开关、ID 18/28 和 channel 1；独立节点会读取两个安装开关 |
| `param/inspire_hands.yaml` | 四种模式都按配置表设置原有的左右灵巧手安装开关 |
| `config_ethercat.yaml` | 全局启停原厂 hand Loader；双夹爪必须关闭 hand，双手和混装必须启用 hand |
| 两个 launch 的 `inspire_gripper` 参数行 | 把夹爪 YAML 路径传给 BodyControl，保持完整包结构 |
| 两个 launch 的 `LD_PRELOAD` | 四种模式都保留；至少一侧 gripper 为 `true` 时启动独立夹爪节点，两侧都为 `false` 时不创建节点 |

重要：2.0.7.3 的 `libbodyctrl_component.so` 本身不包含旧夹爪 Loader，因此仅补入原厂 `.so`、YAML 和开关仍不会自动产生夹爪话题。实际补齐功能的是 `LD_PRELOAD` 加载的 `libinspire_gripper_full.so`。独立节点固定使用 ID 18/28、master 1、channel 1，并从 `inspire_gripper.yaml` 读取左右夹爪是否安装。

## 4. 从零部署

### 4.1 停止旧实例

如果正在运行旧的 full 版本，先重启，重启后暂时不要按 A：

```bash
sudo reboot
```

重新登录后确认没有 BodyControl 容器：

```bash
pgrep -af 'component_container.*bodyctrl_nodes'
```

应无输出。这样可以避免覆盖正在被进程加载的旧 `.so`。

### 4.2 放置、校验和解压

把 `inspire_gripper_full_v0.1.4_mixed_final_v2073.tar.gz` 放到：

```text
/home/ubuntu/ros2ws/install/body_control
```

执行：

```bash
cd /home/ubuntu/ros2ws/install/body_control
sha256sum inspire_gripper_full_v0.1.4_mixed_final_v2073.tar.gz
tar -xzf inspire_gripper_full_v0.1.4_mixed_final_v2073.tar.gz
```

SHA256 必须与交付时提供的值一致。

### 4.3 编译和检查

```bash
cd /home/ubuntu/ros2ws/install/body_control/gripper_full/node
chmod +x build.sh
./build.sh
```

必须出现 `100% tests passed`，并生成 `libinspire_gripper_full.so`。继续执行：

```bash
grep '<version>' package.xml
ls -lh libinspire_gripper_full.so

nm -D libinspire_gripper_full.so |
grep GetU2CMaster

ldd libinspire_gripper_full.so |
grep 'not found'
```

预期：

- 版本为 `0.1.4`；
- `.so` 文件存在；
- `nm` 能看到 `BodyControl::GetU2CMaster` 对应符号；
- 最后一条 `ldd` 检查没有任何输出。

### 4.4 只安装新增文件

该脚本只新增原始 2.0.7.3 中没有的原厂驱动和夹爪 YAML，不修改任何已有文件：

```bash
cd /home/ubuntu/ros2ws/install/body_control/gripper_full
chmod +x install_new_files.sh
sudo ./install_new_files.sh
```

原厂驱动如果已经存在且校验一致，脚本会保留；不同则停止。夹爪 YAML 如果已经存在，脚本只检查 v0.1.4 的两个侧别开关，不覆盖用户选择。

如果机器已经安装过 v0.1.3，原有 `inspire_gripper.yaml` 没有这两个开关，脚本会主动停止。不要删除旧文件；在该文件第一行配置内容前手工加入下面两行，再根据实际末端组合修改真假值：

```yaml
left_gripper_installed: true
right_gripper_installed: true
```

检查新增结果：

```bash
BODY_CONTROL_ROOT=/home/ubuntu/ros2ws/install/body_control

ls -lh \
"$BODY_CONTROL_ROOT/lib/libdev_inspire_gripper_can.so" \
"$BODY_CONTROL_ROOT/share/body_control/param/inspire_gripper.yaml"

sha256sum \
"$BODY_CONTROL_ROOT/lib/libdev_inspire_gripper_can.so" \
"$BODY_CONTROL_ROOT/share/body_control/param/inspire_gripper.yaml"

```

预期：

- 两个新增文件都存在；
- 原厂驱动 SHA256 为 `3baac3dcc85f634f008d4da62a08c055900df60f043b456aa1b17ff8742ecaf9`；
- 未修改的双夹爪模板 YAML SHA256 为 `5b08e16ccb26dd3dc95e15d896bcb79a0a496e7e3d44aa288838ed28d4db9cd4`。混装时开关值改变，目标 YAML 的 SHA256 随之改变，不要求等于该值。

### 4.5 参照样例手工修改已有文件

先备份五个现有文件：

```bash
ROOT=/home/ubuntu/ros2ws/install/body_control
CONFIG="$ROOT/share/body_control/param/config_ethercat.yaml"
HAND_CONFIG="$ROOT/share/body_control/param/inspire_hands.yaml"
GRIPPER_CONFIG="$ROOT/share/body_control/param/inspire_gripper.yaml"
BODY_LAUNCH="$ROOT/share/body_control/launch/body.launch.py"
NO_SBUS_LAUNCH="$ROOT/share/body_control/launch/body_no_sbus.launch.py"

test -e "${CONFIG}.before_gripper_full_v014" ||
sudo cp -a "$CONFIG" "${CONFIG}.before_gripper_full_v014"

test -e "${HAND_CONFIG}.before_gripper_full_v014" ||
sudo cp -a "$HAND_CONFIG" "${HAND_CONFIG}.before_gripper_full_v014"

test -e "${GRIPPER_CONFIG}.before_gripper_full_v014" ||
sudo cp -a "$GRIPPER_CONFIG" "${GRIPPER_CONFIG}.before_gripper_full_v014"

test -e "${BODY_LAUNCH}.before_gripper_full_v014" ||
sudo cp -a "$BODY_LAUNCH" "${BODY_LAUNCH}.before_gripper_full_v014"

test -e "${NO_SBUS_LAUNCH}.before_gripper_full_v014" ||
sudo cp -a "$NO_SBUS_LAUNCH" "${NO_SBUS_LAUNCH}.before_gripper_full_v014"
```

使用 `sudoedit` 或自己熟悉的编辑器修改：

```bash
sudoedit "$CONFIG"
sudoedit "$HAND_CONFIG"
sudoedit "$GRIPPER_CONFIG"
sudoedit "$BODY_LAUNCH"
sudoedit "$NO_SBUS_LAUNCH"
```

先确定实际安装的末端，只选择下表中的一行。三个 YAML 必须使用同一行对应的三个示例，不能交叉组合。

| 工作模式 | `config_ethercat.yaml` 示例 | `inspire_hands.yaml` 示例 | `inspire_gripper.yaml` 示例 |
|---|---|---|---|
| 左右双手 | `config_ethercat.double_hands.modified.example` | `inspire_hands.double_hands.yaml.example` | `inspire_gripper.double_hands.yaml.example` |
| 左手＋右夹爪 | `config_ethercat.mixed.modified.example` | `inspire_hands.left_hand_right_gripper.yaml.example` | `inspire_gripper.left_hand_right_gripper.yaml.example` |
| 左夹爪＋右手 | `config_ethercat.mixed.modified.example` | `inspire_hands.left_gripper_right_hand.yaml.example` | `inspire_gripper.left_gripper_right_hand.yaml.example` |
| 左右双夹爪 | `config_ethercat.double_grippers.modified.example` | `inspire_hands.double_grippers.yaml.example` | `inspire_gripper.double_grippers.yaml.example` |

只参照示例中的相关开关进行手工修改，保留机器人现有文件中的其他配置。四种模式的最终开关值必须和第 3.3 节配置表完全一致。

两个 launch 的 `yamlConfigs` 都增加：

```python
{"inspire_gripper": os.path.join(package_share_directory, "param", "inspire_gripper.yaml")},
```

两个 launch 的 BodyControl `ComposableNodeContainer` 都增加：

```python
additional_env={
    'LD_PRELOAD': '/home/ubuntu/ros2ws/install/body_control/gripper_full/node/libinspire_gripper_full.so'
},
```

不要直接复制样例覆盖系统文件。下面以“左手＋右夹爪”为例查看系统文件与对应示例的差别：

```bash
cd /home/ubuntu/ros2ws/install/body_control/gripper_full

diff -u "$CONFIG" examples/config_ethercat.mixed.modified.example
diff -u "$BODY_LAUNCH" examples/body.launch.py.modified.example
diff -u "$NO_SBUS_LAUNCH" examples/body_no_sbus.launch.py.modified.example

diff -u "$HAND_CONFIG" examples/inspire_hands.left_hand_right_gripper.yaml.example
diff -u "$GRIPPER_CONFIG" examples/inspire_gripper.left_hand_right_gripper.yaml.example
```

选择其他模式时，把上述三个 YAML 示例名替换为表中同一行的文件。`config_ethercat.yaml.modified.example` 是早期保留的双夹爪参考，与 `config_ethercat.double_grippers.modified.example` 内容相同。

最后检查：

```bash
grep -nE '^enable_hands:|^enable_inspire_gripper:' "$CONFIG"
grep -nE '^(left|right)_hand_installed:' "$HAND_CONFIG"
grep -nE '^(left|right)_gripper_installed:' "$GRIPPER_CONFIG"

grep -nE 'inspire_gripper|LD_PRELOAD' \
"$BODY_LAUNCH" "$NO_SBUS_LAUNCH"

PYTHONPYCACHEPREFIX=/tmp/body_control_pycache \
python3 -m py_compile "$BODY_LAUNCH" "$NO_SBUS_LAUNCH"

cat "$ROOT/share/ament_index/resource_index/rclcpp_components/body_control"
```

预期：

- 六个开关与第 3.3 节所选模式的同一行完全一致；
- 双夹爪模式的 `enable_hands` 必须为 `false`，其余三个模式为 `true`；
- 双手模式的 `enable_inspire_gripper` 为 `false`，其余三个模式为 `true`；
- 每一侧恰好启用 hand 或 gripper 中的一个；
- 两个 launch 各有一个夹爪 YAML 项和一个 full 版 `LD_PRELOAD`；
- Python 语法检查无输出；
- 组件索引仍是 `body_control::BodyControl;lib/libbodyctrl_component.so`。

全部正确后重启：

```bash
sudo reboot
```

重新登录后按遥控器 A 启动 BodyControl。

## 5. 四种模式验收顺序

四种模式都先执行第 5.1 节。左右双手模式随后执行第 5.2 节的“双手模式”检查和第 5.3.1 节的灵巧手检查，跳过其余夹爪测试。混装模式同时检查实际手侧和实际夹爪侧；双夹爪模式跳过第 5.3.1 节。

所有运动测试前，确保实际安装夹爪的运动范围内没有人体、线缆和硬物。先使用低速度、低力度。任何一步出现指示灯红色、容器退出或异常动作，都停止后续测试。以下带“左夹”或“右夹”的动作，只执行实际安装夹爪的一侧，不向安装灵巧手的一侧发送夹爪命令。

### 5.1 检查进程、动态库和节点

```bash
PID=$(pgrep -f 'component_container.*bodyctrl_nodes' | head -1)
echo "BODYCTRL_PID=$PID"
```

作用：找到实际运行 BodyControl 的容器进程。预期 PID 非空。

```bash
sudo grep -E \
'libbodyctrl_component|libinspire_gripper_full' \
/proc/$PID/maps
```

作用：确认最新版主体库和预加载库在同一进程中。四种模式都应同时看到：

```text
lib/libbodyctrl_component.so
gripper_full/node/libinspire_gripper_full.so
```

```bash
ros2 node list |
grep -E '^/bodyctrl$|^/bodyctrl_nodes$|^/inspire_gripper_full$'
```

作用：检查 ROS 2 节点。四种模式都必须看到：

```text
/bodyctrl
/bodyctrl_nodes
```

左手＋右夹爪、左夹爪＋右手、左右双夹爪还必须看到：

```text
/inspire_gripper_full
```

左右双手模式的两个 gripper 侧别均为 `false`，因此不得出现 `/inspire_gripper_full`。此时预加载库仍在进程中，但夹爪线程读取配置后退出，不创建 ROS 节点和 CAN 轮询。

如果 PID 为空、缺少任意动态库、节点与当前模式不符或指示灯异常，不发送运动命令。

### 5.2 检查话题与侧别隔离

```bash
ros2 topic list |
grep '^/inspire_gripper/' |
sort
```

左右双手模式预期无输出，随后跳过本节剩余内容和所有夹爪运动测试。

单手单夹混装时预期看到 8 个夹爪话题：5 个公共控制话题，加上已安装夹爪侧的 command/status/error。左手＋右夹爪时应看到：

```text
/inspire_gripper/clear_error_command
/inspire_gripper/fast_stop_command
/inspire_gripper/right_command
/inspire_gripper/right_error
/inspire_gripper/right_status
/inspire_gripper/set_catch_mode_command
/inspire_gripper/set_max_openlen_command
/inspire_gripper/set_min_openlen_command
```

同时确认未安装的左夹爪话题不存在：

```bash
ros2 topic info -v /inspire_gripper/right_command
ros2 topic list | grep '^/inspire_gripper/left_'
```

第一条预期类型为 `sensor_msgs/msg/JointState`、有 1 个订阅者且节点名为 `inspire_gripper_full`；第二条预期无输出。

检查右夹爪反馈话题：

```bash
ros2 topic info -v /inspire_gripper/right_status
ros2 topic info -v /inspire_gripper/right_error
```

预期 status 类型为 `sensor_msgs/msg/JointState`，error 类型为 `std_msgs/msg/UInt32`，每个都有 1 个发布者。

左夹爪＋右手时结论完全对称：把上述命令中的 `right` 和 `left` 对调，且右夹爪三个话题必须不存在。双夹爪模式仍应看到原来的全部 11 个话题。

检查另外五个控制接口：

```bash
ros2 topic info -v /inspire_gripper/clear_error_command
ros2 topic info -v /inspire_gripper/set_catch_mode_command
ros2 topic info -v /inspire_gripper/fast_stop_command
ros2 topic info -v /inspire_gripper/set_max_openlen_command
ros2 topic info -v /inspire_gripper/set_min_openlen_command
```

预期每个都有 1 个订阅者。清错类型为 `std_msgs/msg/String`，其余四个为 `bodyctrl_msgs/msg/CmdSetParam`。

### 5.3 只读测试，不发送控制

```bash
timeout 5s ros2 topic echo --once /inspire_gripper/right_status
timeout 5s ros2 topic echo --once /inspire_gripper/right_error
```

上面以“左手＋右夹爪”为例，确认右夹爪能收到 CAN 返回。两条命令都应在 5 秒内输出一次消息。左夹爪＋右手时把 `right` 改为 `left`。

状态消息预期：

- `name` 与实际夹爪侧一致；
- `position` 是闭合百分比，完全打开约为 0，完全闭合约为 100，允许存在机械和标定误差；
- `velocity` 当前固定上报 0；
- `effort` 随负载变化，不把它是否严格处于 0～100 作为第一轮通过条件。

```bash
timeout 5s ros2 topic hz /inspire_gripper/right_status
```

作用：检查实际夹爪侧反馈持续发布。左夹爪＋右手时把 `right` 改为 `left`。单个 status 话题理论约 25 Hz（单夹无需轮询另一侧）；只要稳定持续收到即可。

### 5.3.1 确认灵巧手在线

```bash
ros2 topic list | grep '^/inspire_hand/' | sort
ros2 topic info -v /inspire_hand/ctrl
ros2 topic info -v /inspire_hand/state/left_hand
ros2 topic info -v /inspire_hand/state/right_hand
```

作用：确认原 BodyControl 的灵巧手 Loader 工作。双手和两种混装模式执行本节；双夹爪模式跳过。预期 `/inspire_hand/ctrl` 有订阅者，实际安装的灵巧手侧对应状态话题能收到消息。原厂 Loader 可能预先创建未安装侧的发布端，因此未安装手侧是否出现在 `topic info` 中不作为通过条件，也不要向该侧发送命令。

左手＋右夹爪时读取：

```bash
timeout 5s ros2 topic echo --once /inspire_hand/state/left_hand
```

左夹爪＋右手时读取：

```bash
timeout 5s ros2 topic echo --once /inspire_hand/state/right_hand
```

实际灵巧手状态命令应在 5 秒内收到 `sensor_msgs/msg/JointState`。这里只做只读检查；灵巧手运动仍使用原厂指令，不用夹爪指令控制。

左右双手时，左右状态都应读取一次：

```bash
timeout 5s ros2 topic echo --once /inspire_hand/state/left_hand
timeout 5s ros2 topic echo --once /inspire_hand/state/right_hand
```

两条命令都应在 5 秒内收到 `sensor_msgs/msg/JointState`，并且 `/inspire_gripper_full` 不存在。

再次确认容器和两个功能仍同时存在：

```bash
pgrep -af 'component_container.*bodyctrl_nodes'
ros2 node list | grep '^/inspire_gripper_full$'
ros2 topic list | grep '^/inspire_hand/'
```

三条检查均应有输出，机器人指示灯保持正常。

### 5.4 左夹闭合和打开

仅当左侧安装的是夹爪时执行本节；左侧安装灵巧手时整节跳过。

左夹低速、低力度闭合：

```bash
ros2 topic pub --once \
/inspire_gripper/left_command \
sensor_msgs/msg/JointState \
"{header: {frame_id: ''}, name: ['inspire_gripper_left'], position: [1.0], velocity: [0.1], effort: [0.1]}"

sleep 3

timeout 5s ros2 topic echo --once /inspire_gripper/left_status
timeout 5s ros2 topic echo --once /inspire_gripper/left_error
```

预期：左夹闭合，`position` 明显增大并接近 100。无物体且闭合到最小时通常为 `data: 2`；夹到物体停止时可能为 `data: 6`。

左夹低速、低力度打开：

```bash
ros2 topic pub --once \
/inspire_gripper/left_command \
sensor_msgs/msg/JointState \
"{header: {frame_id: ''}, name: ['inspire_gripper_left'], position: [0.0], velocity: [0.1], effort: [0.1]}"

sleep 3

timeout 5s ros2 topic echo --once /inspire_gripper/left_status
timeout 5s ros2 topic echo --once /inspire_gripper/left_error
```

预期：左夹打开，`position` 回到约 0，错误/状态话题应为 `data: 1`。

### 5.5 右夹闭合和打开

仅当右侧安装的是夹爪时执行本节；右侧安装灵巧手时整节跳过。

右夹低速、低力度闭合：

```bash
ros2 topic pub --once \
/inspire_gripper/right_command \
sensor_msgs/msg/JointState \
"{header: {frame_id: ''}, name: ['inspire_gripper_right'], position: [1.0], velocity: [0.1], effort: [0.1]}"

sleep 3

timeout 5s ros2 topic echo --once /inspire_gripper/right_status
timeout 5s ros2 topic echo --once /inspire_gripper/right_error
```

预期：右夹闭合，`position` 明显增大并接近 100。无物体且闭合到最小时通常为 `data: 2`；夹到物体停止时可能为 `data: 6`。

右夹低速、低力度打开：

```bash
ros2 topic pub --once \
/inspire_gripper/right_command \
sensor_msgs/msg/JointState \
"{header: {frame_id: ''}, name: ['inspire_gripper_right'], position: [0.0], velocity: [0.1], effort: [0.1]}"

sleep 3

timeout 5s ros2 topic echo --once /inspire_gripper/right_status
timeout 5s ros2 topic echo --once /inspire_gripper/right_error
```

预期：右夹打开，`position` 回到约 0，错误/状态话题应为 `data: 1`。

上述四次动作后再检查：

```bash
pgrep -af 'component_container.*bodyctrl_nodes'
ros2 node list | grep '^/inspire_gripper_full$'
```

预期容器和夹爪节点仍然存在。正常全开/全闭不能再固定显示为 `11/12`。

### 5.6 夹持模式

`CmdSetParam.name` 中左夹 ID 为 `18`，右夹 ID 为 `28`。混装验收的参数命令只能包含实际安装的夹爪 ID。下面以左夹爪为例；如果安装的是右夹爪，把 `18` 改为 `28`。

设为单次夹持模式：

```bash
ros2 topic pub --once \
/inspire_gripper/set_catch_mode_command \
bodyctrl_msgs/msg/CmdSetParam \
"{cmds: [{name: 18, param: 0.0}]}"
```

预期：命令本身不产生运动；下一次夹持在达到目标或夹到物体后停止。

设为持续夹持模式：

```bash
ros2 topic pub --once \
/inspire_gripper/set_catch_mode_command \
bodyctrl_msgs/msg/CmdSetParam \
"{cmds: [{name: 18, param: 1.0}]}"
```

预期：命令本身不产生运动；下一次夹住物体后，在夹持力下降时会继续夹紧以维持夹持力。该效果只能使用安全、柔性的测试物体验证。最后一条命令会把实际夹爪留在持续夹持模式；如需单次模式，再发送参数 0。

### 5.7 最大/最小开度

参数是夹爪物理开度百分比，和运动命令的“闭合百分比”方向相反。下面用左夹做可见测试，并在每次测试后立即恢复。如果实际安装的是右夹，把全部 topic 中的 `left` 改为 `right`、名称改为 `inspire_gripper_right`、设备 ID `18` 改为 `28`。

把左夹最大物理开度临时设为 80%，然后发送完全打开命令：

```bash
ros2 topic pub --once \
/inspire_gripper/set_max_openlen_command \
bodyctrl_msgs/msg/CmdSetParam \
"{cmds: [{name: 18, param: 80.0}]}"

ros2 topic pub --once \
/inspire_gripper/left_command \
sensor_msgs/msg/JointState \
"{name: ['inspire_gripper_left'], position: [0.0], velocity: [0.1], effort: [0.1]}"

sleep 3
timeout 5s ros2 topic echo --once /inspire_gripper/left_status
```

预期：左夹不能开到原来的最大位置；由于 status 使用闭合百分比，`position` 预计在约 20 附近，而不是约 0。

立即恢复实际夹爪最大物理开度为 100%：

```bash
ros2 topic pub --once \
/inspire_gripper/set_max_openlen_command \
bodyctrl_msgs/msg/CmdSetParam \
"{cmds: [{name: 18, param: 100.0}]}"
```

把左夹最小物理开度临时设为 20%，然后发送完全闭合命令：

```bash
ros2 topic pub --once \
/inspire_gripper/set_min_openlen_command \
bodyctrl_msgs/msg/CmdSetParam \
"{cmds: [{name: 18, param: 20.0}]}"

ros2 topic pub --once \
/inspire_gripper/left_command \
sensor_msgs/msg/JointState \
"{name: ['inspire_gripper_left'], position: [1.0], velocity: [0.1], effort: [0.1]}"

sleep 3
timeout 5s ros2 topic echo --once /inspire_gripper/left_status
```

预期：左夹不能闭到原来的最小间隙；`position` 预计在约 80 附近，而不是约 100。

立即恢复实际夹爪最小物理开度为 0%：

```bash
ros2 topic pub --once \
/inspire_gripper/set_min_openlen_command \
bodyctrl_msgs/msg/CmdSetParam \
"{cmds: [{name: 18, param: 0.0}]}"
```

最后重新执行一次实际夹爪的打开、闭合，确认完整行程恢复。

### 5.8 快速停止和恢复

下面以左夹爪为例。如果实际安装的是右夹爪，按上一节的替换规则改为右侧后执行。

先让左夹处于打开位置，然后低速闭合：

```bash
ros2 topic pub --once \
/inspire_gripper/left_command \
sensor_msgs/msg/JointState \
"{name: ['inspire_gripper_left'], position: [0.0], velocity: [0.1], effort: [0.1]}"

sleep 3

ros2 topic pub --once \
/inspire_gripper/left_command \
sensor_msgs/msg/JointState \
"{name: ['inspire_gripper_left'], position: [1.0], velocity: [0.05], effort: [0.1]}"

sleep 0.3

ros2 topic pub --once \
/inspire_gripper/fast_stop_command \
bodyctrl_msgs/msg/CmdSetParam \
"{cmds: [{name: 18, param: 1.0}]}"
```

预期：左夹在行程中停止。查看反馈：

```bash
timeout 5s ros2 topic echo --once /inspire_gripper/left_status
timeout 5s ros2 topic echo --once /inspire_gripper/left_error
```

`position` 应停在中间位置；运行状态通常为 `3`，但以实际停止动作和反馈持续存在为主要判断。

解除快速停止、执行一次清错并重新打开：

```bash
ros2 topic pub --once \
/inspire_gripper/fast_stop_command \
bodyctrl_msgs/msg/CmdSetParam \
"{cmds: [{name: 18, param: 0.0}]}"

ros2 topic pub --once \
/inspire_gripper/clear_error_command \
std_msgs/msg/String \
"{data: left}"

ros2 topic pub --once \
/inspire_gripper/left_command \
sensor_msgs/msg/JointState \
"{name: ['inspire_gripper_left'], position: [0.0], velocity: [0.1], effort: [0.1]}"
```

预期：左夹恢复运动并完全打开，随后 `left_error` 为 `1`。

### 5.9 清除错误

不需要也不允许为了测试而人为制造堵转、过温或过流。无故障时可以验证命令接口：

```bash
ros2 topic pub --once \
/inspire_gripper/clear_error_command \
std_msgs/msg/String \
"{data: all}"
```

预期：只有实际启用的夹爪侧收到清错请求，不应产生运动，BodyControl 和夹爪节点继续运行；混装模式还应保持灵巧手正常。发生可清除故障时可使用实际侧名称或 `all`；过温故障必须先等待温度下降。

### 5.10 最终持续性检查

```bash
timeout 10s ros2 topic hz /inspire_gripper/right_status

pgrep -af 'component_container.*bodyctrl_nodes'
ros2 node list | grep '^/inspire_gripper_full$'
```

上面以右夹爪为例；左夹爪时把 `right` 改为 `left`。预期：实际夹爪反馈持续稳定、容器和节点始终存在、机器人指示灯正常；混装模式还应保持实际灵巧手侧的话题正常。

## 6. 状态/错误合并码

`left_error` 和 `right_error` 虽然名称包含 error，但无故障时发布运行状态；设备错误位非零时才发布 11～16。

| 数值 | 含义 |
|---:|---|
| 0 | 无错误/无有效运行状态 |
| 1 | 张开到最大并停止 |
| 2 | 闭合到最小并停止 |
| 3 | 已停止 |
| 4 | 正在夹取 |
| 5 | 正在张开 |
| 6 | 夹到物体并停止 |
| 11 | 堵转故障 |
| 12 | 过温故障 |
| 13 | 过流故障 |
| 14 | 内部通信故障 |
| 15 | 未知设备故障 |
| 16 | 未识别的错误位 |
| 33072 | 设备通信丢失超过 2 秒 |

## 7. 在灵巧手和夹爪之间切换

为了以后反复更换硬件时影响最小，原厂夹爪驱动、夹爪 YAML、全功能源码、编译出的 `.so` 以及两个 launch 中的 `LD_PRELOAD` 都固定保留。完成首次部署后，四种模式之间切换只修改下面三个 YAML，不再修改 launch，也不反复复制或删除库文件：

```text
/home/ubuntu/ros2ws/install/body_control/share/body_control/param/config_ethercat.yaml
/home/ubuntu/ros2ws/install/body_control/share/body_control/param/inspire_hands.yaml
/home/ubuntu/ros2ws/install/body_control/share/body_control/param/inspire_gripper.yaml
```

任何硬件更换都必须在 BodyControl 停止且执行器断电后进行。修改完成并通过语法检查前不要按 A。

### 7.1 固定不再修改的 launch 配置

`body.launch.py` 和 `body_no_sbus.launch.py` 都始终保留：

```python
additional_env={
    'LD_PRELOAD': '/home/ubuntu/ros2ws/install/body_control/gripper_full/node/libinspire_gripper_full.so'
},
```

每个 launch 只能出现一次该配置。`yamlConfigs` 中的 `inspire_gripper` 参数行也始终保留。

左右双手模式虽然保留 `LD_PRELOAD`，但 `inspire_gripper.yaml` 的左右开关均为 `false`。预加载库检测到没有启用的夹爪侧后，不创建 `/inspire_gripper_full`、夹爪话题或 CAN 轮询。

### 7.2 四种模式配置表

根据断电后实际安装的末端，严格选择下表的一行：

| 工作模式 | `enable_hands` | `enable_inspire_gripper` | `left_hand_installed` | `right_hand_installed` | `left_gripper_installed` | `right_gripper_installed` |
|---|---:|---:|---:|---:|---:|---:|
| 左右双手 | `true` | `false` | `true` | `true` | `false` | `false` |
| 左手＋右夹爪 | `true` | `true` | `true` | `false` | `false` | `true` |
| 左夹爪＋右手 | `true` | `true` | `false` | `true` | `true` | `false` |
| 左右双夹爪 | `false` | `true` | `false` | `false` | `true` | `true` |

特别注意：左右双夹爪时必须使用 `enable_hands: false`。实机已经确认，仅把 `inspire_hands.yaml` 的左右开关改为 `false`、却保留 `enable_hands: true`，按 A 可能导致 BodyControl 启动失败。

四种模式的三个完整 YAML 示例都在 `examples/` 中，文件对应关系见第 4.5 节。示例只供核对相关开关，不直接覆盖机器人原配置。

### 7.3 修改后检查和重启

检查：

```bash
ROOT=/home/ubuntu/ros2ws/install/body_control
CONFIG="$ROOT/share/body_control/param/config_ethercat.yaml"
HAND_CONFIG="$ROOT/share/body_control/param/inspire_hands.yaml"
GRIPPER_CONFIG="$ROOT/share/body_control/param/inspire_gripper.yaml"
BODY_LAUNCH="$ROOT/share/body_control/launch/body.launch.py"
NO_SBUS_LAUNCH="$ROOT/share/body_control/launch/body_no_sbus.launch.py"

grep -nE '^enable_hands:|^enable_inspire_gripper:' "$CONFIG"
grep -nE '^(left|right)_hand_installed:' "$HAND_CONFIG"
grep -nE '^(left|right)_gripper_installed:' "$GRIPPER_CONFIG"
grep -n LD_PRELOAD "$BODY_LAUNCH" "$NO_SBUS_LAUNCH"

PYTHONPYCACHEPREFIX=/tmp/body_control_pycache \
python3 -m py_compile "$BODY_LAUNCH" "$NO_SBUS_LAUNCH"
```

预期六个开关和第 7.2 节所选行完全一致，两个 launch 各有且只有一个 full 版 `LD_PRELOAD`，Python 语法检查无输出。然后：

```bash
sudo reboot
```

重新登录后按 A，并按第 5 节验证：

- 左右双手：有灵巧手接口，没有 `/inspire_gripper_full` 和夹爪话题；
- 两种混装：灵巧手接口和 `/inspire_gripper_full` 同时存在，只有实际夹爪侧出现三个侧别话题；
- 左右双夹爪：有 `/inspire_gripper_full` 和左右夹爪话题，BodyControl 正常启动，不要求出现灵巧手接口。

这些开关只在启动时读取，任何模式切换后都必须重启，不能通过 `ros2 param set` 动态切换。

## 8. 软件配置回滚

如果启动失败、指示灯异常或节点退出，不继续发送命令。先重启且不要按 A，然后恢复配置和两个启动文件：

```bash
BODY_CONTROL_ROOT=/home/ubuntu/ros2ws/install/body_control
CONFIG_FILE="$BODY_CONTROL_ROOT/share/body_control/param/config_ethercat.yaml"
HAND_CONFIG_FILE="$BODY_CONTROL_ROOT/share/body_control/param/inspire_hands.yaml"
GRIPPER_CONFIG_FILE="$BODY_CONTROL_ROOT/share/body_control/param/inspire_gripper.yaml"
BODY_LAUNCH="$BODY_CONTROL_ROOT/share/body_control/launch/body.launch.py"
NO_SBUS_LAUNCH="$BODY_CONTROL_ROOT/share/body_control/launch/body_no_sbus.launch.py"

sudo cp -a \
"${CONFIG_FILE}.before_gripper_full_v014" \
"$CONFIG_FILE"

sudo cp -a \
"${HAND_CONFIG_FILE}.before_gripper_full_v014" \
"$HAND_CONFIG_FILE"

sudo cp -a \
"${GRIPPER_CONFIG_FILE}.before_gripper_full_v014" \
"$GRIPPER_CONFIG_FILE"

sudo cp -a \
"${BODY_LAUNCH}.before_gripper_full_v014" \
"$BODY_LAUNCH"

sudo cp -a \
"${NO_SBUS_LAUNCH}.before_gripper_full_v014" \
"$NO_SBUS_LAUNCH"

PYTHONPYCACHEPREFIX=/tmp/body_control_pycache \
python3 -m py_compile "$BODY_LAUNCH" "$NO_SBUS_LAUNCH"

sudo reboot
```

回滚只恢复安装前的配置和 launch。新增的原厂驱动、YAML 和 `gripper_full` 目录继续保留，便于以后重新切回夹爪；只要 launch 不再预加载全功能库，它们就不会运行。不要替换最新版 `libbodyctrl_component.so`，不要改 ament 组件索引。

## 9. 实现边界

- 使用最新版 BodyControl 的 `GetU2CMaster()` ABI，并依赖当前对象布局偏移 `0x4B0 / 0xE8`；
- 使用当前 `libucbp.so` 导出的 `SendRawMessage` 和 `RegisterCanRawMessageCallback`；
- 接收回调以列表方式追加，不覆盖 BodyControl 现有回调；
- 双夹模式下单个状态或错误话题理论约 12.5 Hz，单夹模式下约 25 Hz；
- 只轮询 `inspire_gripper.yaml` 中标记为已安装的夹爪侧；已安装侧连续 2 秒没有有效响应时发布 `33072`；
- 左右安装开关在节点启动时读取，修改后必须重启；
- 当前未提供 ROS 参数来动态修改设备 ID、master 或 channel。
