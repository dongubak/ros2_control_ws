# ROS2 Control 복습

---

## 1. 왜 ROS2 Control인가?

- 하드웨어가 달라도 상위 소프트웨어(Nav2, MoveIt)는 같은 코드로 동작
- **하드웨어 플러그인만 교체**하면 나머지는 그대로

---

## 2. 3개 레이어

```
[상위 소프트웨어]  Nav2, teleop, MoveIt
        ↕  ROS 토픽 (/cmd_vel 등)
[Controller]      diff_drive, joint_trajectory 등
        ↕  command/state interface (공유 메모리)
[Controller Manager]  제어 루프 실행, 중간 관리자
        ↕  read/write
[Hardware Interface]  mock, Gazebo, 실제 드라이버
```

---

## 3. 핵심 개념: interface

| 종류 | 방향 | 의미 |
|---|---|---|
| `command_interface` | 컨트롤러 → 하드웨어 | 명령 쓰기 (속도, 위치, 힘) |
| `state_interface` | 하드웨어 → 컨트롤러 | 상태 읽기 (위치, 속도, 힘) |

- **ROS 토픽이 아님** — controller_manager 내부 공유 메모리
- `command_interface`: 한 컨트롤러만 `[claimed]` 가능 (독점)
- `state_interface`: 여러 컨트롤러가 동시에 읽기 가능

---

## 4. 제어 루프 (update_rate Hz마다 반복)

```
read()   → 하드웨어에서 state_interface 읽기
update() → 각 컨트롤러 계산 실행
write()  → command_interface 값을 하드웨어에 쓰기
```

---

## 5. URDF 선언 구조

```xml
<ros2_control name="..." type="system">
    <hardware>
        <plugin>mock_components/GenericSystem</plugin>
        <param name="calculate_dynamics">true</param>
    </hardware>
    <joint name="base_right_wheel_joint">
        <command_interface name="velocity" />  ← 속도 명령 가능
        <state_interface name="position" />    ← 위치 읽기 가능
        <state_interface name="velocity" />    ← 속도 읽기 가능
    </joint>
</ros2_control>
```

- `calculate_dynamics: true` → 속도를 적분해서 위치 자동 계산

---

## 6. yaml 구조 규칙

```yaml
controller_manager:        # controller_manager 노드 파라미터
  ros__parameters:
    update_rate: 50
    joint_state_broadcaster:
      type: joint_state_broadcaster/JointStateBroadcaster   # 등록
    diff_drive_controller:
      type: diff_drive_controller/DiffDriveController       # 등록

diff_drive_controller:     # 컨트롤러 세부 파라미터 (최상위 레벨)
  ros__parameters:
    left_wheel_names: ["base_left_wheel_joint"]
    right_wheel_names: ["base_right_wheel_joint"]
    wheel_separation: 0.45   # ← 오타 주의! seperation❌ separation✅
    wheel_radius: 0.1
    enable_odom_tf: true
```

---

## 7. 주요 컨트롤러

| 컨트롤러 | 역할 |
|---|---|
| `joint_state_broadcaster` | state_interface → `/joint_states` 발행. **항상 필요** |
| `diff_drive_controller` | `/cmd_vel` → 바퀴 속도 command_interface + `/odom` 발행 |
| `joint_trajectory_controller` | MoveIt용. 로봇 팔 경로 제어 |
| `imu_sensor_broadcaster` | IMU state_interface → `/imu/data` 발행 |

---

## 8. /cmd_vel 흐름 요약

```
teleop → /cmd_vel 토픽
  → diff_drive_controller (운동학 계산 → 좌/우 바퀴 속도)
  → command_interface (공유 메모리)
  → [controller_manager write()]
  → mock 하드웨어 (속도 적분 → 위치 갱신)
  → state_interface 갱신
  → diff_drive_controller read() → /odom + TF(odom→base_footprint)
  → joint_state_broadcaster → /joint_states
  → robot_state_publisher → TF(base_footprint→base_link→바퀴)
  → RViz
```

---

## 9. spawner 흐름 요약

```
spawner 실행
  → controller_manager 서비스 응답 대기
  → load_controller   (플러그인 로드, 상태: unconfigured)
  → configure_controller (interface claim, 상태: inactive)
  → switch_controller  (제어 루프 등록, 상태: active)
```

---

## 10. Humble 특이사항

```xml
<!-- controller_manager 노드에 remap 필수 -->
<remap from="/controller_manager/robot_description" to="/robot_description"/>
```

robot_state_publisher는 `/robot_description`에 발행하지만
Humble의 controller_manager는 `/controller_manager/robot_description`을 구독하기 때문.

---

## 11. 유용한 CLI

```bash
ros2 control list_controllers           # 활성화된 컨트롤러 확인
ros2 control list_hardware_interfaces   # interface 상태 확인 (claimed 여부)
ros2 control list_controller_types      # 설치된 플러그인 목록
```

---

## 12. 파일 구조 요약

| 파일 | 역할 |
|---|---|
| `urdf/mobile_base.ros2_control.xacro` | 하드웨어 인터페이스 선언 |
| `config/my_robot_controller.yaml` | 컨트롤러 등록 및 파라미터 |
| `launch/rsp.launch.py` | robot_state_publisher 실행 |
| `launch/my_robot.launch.xml` | 전체 시스템 한 번에 실행 |
