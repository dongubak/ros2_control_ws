# ROS2 Control - 06. 실행 방법

## 전체 실행 흐름

```
1. robot_state_publisher 실행    ← URDF 로드, /robot_description 발행
2. controller_manager 실행       ← 하드웨어 초기화, 제어 루프 시작
3. spawner로 컨트롤러 활성화     ← joint_state_broadcaster, diff_drive_controller
4. (선택) RViz 실행              ← 시각화
```

---

## 방법 1: 터미널 직접 실행 (학습/디버깅용)

### 터미널 1: robot_state_publisher

```bash
ros2 launch my_robot_description rsp.launch.py
```

`rsp.launch.py` 내용:
```python
import xacro
robot_description = xacro.process_file(xacro_file).toxml()

Node(
    package="robot_state_publisher",
    executable="robot_state_publisher",
    parameters=[{"robot_description": robot_description}],
)
```

> XML launch 파일에서 xacro를 직접 실행하면 xacro 주석의 `:` 문자가
> YAML 파싱 오류를 일으킬 수 있습니다. Python launch 파일을 권장합니다.

---

### 터미널 2: controller_manager

```bash
ros2 run controller_manager ros2_control_node \
  --ros-args \
  --params-file /path/to/my_robot_controller.yaml \
  --remap /controller_manager/robot_description:=/robot_description
```

> `--remap` 옵션은 ROS2 **Humble에서만** 필요합니다.

---

### 터미널 3: joint_state_broadcaster 활성화

```bash
ros2 run controller_manager spawner joint_state_broadcaster
```

성공 시:
```
[spawner]: Loaded joint_state_broadcaster
[spawner]: Configured and activated joint_state_broadcaster
```

---

### 터미널 4: diff_drive_controller 활성화

```bash
ros2 run controller_manager spawner diff_drive_controller
```

---

### 터미널 5: teleop (선택)

```bash
ros2 run teleop_twist_keyboard teleop_twist_keyboard \
  --ros-args \
  -r /cmd_vel:=/diff_drive_controller/cmd_vel \
  -p stamped:=true
```

> diff_drive_controller는 기본적으로 `TwistStamped` 메시지를 구독합니다.
> `-p stamped:=true`를 붙여야 타임스탬프가 포함된 메시지를 발행합니다.

---

## 방법 2: launch 파일로 한 번에 실행 (실제 사용)

### my_robot.launch.xml

```xml
<launch>
    <let name="rviz_config_path"
        value="$(find-pkg-share my_robot_description)/rviz/urdf_config.rviz" />

    <!-- robot_state_publisher -->
    <include file="$(find-pkg-share my_robot_description)/launch/rsp.launch.py"/>

    <!-- controller_manager (Humble: remap 필요) -->
    <node pkg="controller_manager" exec="ros2_control_node">
        <param from="$(find-pkg-share my_robot_bringup)/config/my_robot_controller.yaml" />
        <remap from="/controller_manager/robot_description" to="/robot_description"/>
    </node>

    <!-- 컨트롤러 활성화 -->
    <node pkg="controller_manager" exec="spawner" args="joint_state_broadcaster" />
    <node pkg="controller_manager" exec="spawner" args="diff_drive_controller" />

    <!-- RViz -->
    <node pkg="rviz2" exec="rviz2" output="screen"
        args="-d $(var rviz_config_path)" />
</launch>
```

실행:
```bash
ros2 launch my_robot_bringup my_robot.launch.xml
```

---

## 유용한 CLI 명령어

### 컨트롤러 상태 확인

```bash
ros2 control list_controllers
```
```
diff_drive_controller   diff_drive_controller/DiffDriveController      active
joint_state_broadcaster joint_state_broadcaster/JointStateBroadcaster  active
```

### 하드웨어 인터페이스 확인

```bash
ros2 control list_hardware_interfaces
```
```
command interfaces
    base_left_wheel_joint/velocity  [available] [claimed]
    base_right_wheel_joint/velocity [available] [claimed]
state interfaces
    base_left_wheel_joint/position
    base_left_wheel_joint/velocity
    base_right_wheel_joint/position
    base_right_wheel_joint/velocity
```

- `[available]`: 사용 가능
- `[claimed]`: 컨트롤러가 독점 사용 중
- `[unavailable]`: 사용 불가 (하드웨어 미초기화)

### 설치된 컨트롤러 플러그인 목록

```bash
ros2 control list_controller_types
```

### 컨트롤러 수동 비활성화/활성화

```bash
ros2 control set_controller_state diff_drive_controller inactive
ros2 control set_controller_state diff_drive_controller active
```

---

## 문제 해결 체크리스트

| 증상 | 원인 | 해결 |
|---|---|---|
| spawner가 서비스 대기 중 | controller_manager 초기화 안 됨 | robot_description 토픽 remap 확인 |
| `wheel_separation must be > 0` | yaml 오타 (`seperation`) | `separation`으로 수정 |
| `type param was not defined` | yaml에 컨트롤러 type 미등록 | controller_manager 아래에 type 추가 |
| launch 파일 YAML 파싱 오류 | xacro 주석에 `:` 포함 | Python launch 파일 사용 |
| RViz에서 로봇 안 움직임 | joint_state_broadcaster 미활성화 | spawner로 활성화 확인 |
| cmd_vel 명령이 안 먹힘 | 토픽 이름 불일치 | remap으로 토픽 이름 맞추기 |

---

## 토픽 구조 요약

```
/robot_description          ← robot_state_publisher가 발행
/joint_states               ← joint_state_broadcaster가 발행
/diff_drive_controller/cmd_vel  ← teleop/Nav2가 발행, diff_drive_controller가 구독
/diff_drive_controller/odom ← diff_drive_controller가 발행
/tf                         ← robot_state_publisher + diff_drive_controller가 발행
```
