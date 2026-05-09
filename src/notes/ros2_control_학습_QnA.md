# ROS2 Control 학습 Q&A

강의를 들으면서 생긴 질문들을 정리한 문서입니다.

---

## Q1. mobile_base.ros2_control.xacro 파일의 각 태그는 무슨 의미인가?

**파일 위치**: `my_robot_description/urdf/mobile_base.ros2_control.xacro`

### `<ros2_control>` 태그
- `name`: 이 인터페이스의 고유 이름 (controller_manager가 식별에 사용)
- `type="system"`: 여러 조인트를 하나의 하드웨어 단위로 묶어 제어함을 의미

### `<hardware>` 태그
- `plugin: mock_components/GenericSystem`: 실제 하드웨어 없이 시뮬레이션하는 가상 플러그인
- `calculate_dynamics: true`: 속도 명령으로부터 위치를 자동 계산(적분)
  - 실제 로봇에서는 이 플러그인 대신 실제 드라이버 플러그인을 사용

### `<joint>` 태그
- `command_interface name="velocity"`: 속도 명령을 이 조인트에 내릴 수 있음 (제어 입력)
- `state_interface name="position"`: 조인트의 현재 위치(각도)를 읽을 수 있음 (피드백)
- `state_interface name="velocity"`: 조인트의 현재 속도를 읽을 수 있음 (피드백)

---

## Q2. Joint State Broadcaster란 무엇인가?

하드웨어 인터페이스의 **state_interface**(위치, 속도 등)를 읽어서 `/joint_states` 토픽으로 발행(publish)하는 기본 컨트롤러입니다.

```
[하드웨어/시뮬레이터]
       ↓  state_interface (position, velocity)
[Joint State Broadcaster]
       ↓  /joint_states 토픽 발행
[RViz, robot_state_publisher, Nav2, MoveIt 등]
```

### 왜 필요한가?
- `robot_state_publisher`는 `/joint_states`를 구독해서 TF를 계산
- RViz에서 로봇 모델이 움직이는 것, Nav2/MoveIt이 현재 조인트 상태를 아는 것 모두 이 토픽에 의존
- **실제 제어를 하는 것이 아니라, 상태를 외부에 알려주는 역할만 함**

---

## Q3. my_robot_controller.yaml 파일의 구조와 URDF와의 연결 지점은?

**파일 위치**: `my_robot_bringup/config/my_robot_controller.yaml`

### ros2_control의 3개 레이어

```
[URDF / xacro]          ← 하드웨어가 무엇인지, 어떤 인터페이스를 가졌는지 선언
      ↕
[controller_manager]    ← 하드웨어와 컨트롤러를 연결하는 중간 관리자
      ↕
[Controller]            ← 실제로 명령을 내리거나 상태를 읽는 소프트웨어 모듈
```

### URDF와의 연결 지점
- **xacro**: "어떤 인터페이스가 있는지" 선언 → 콘센트 설치
- **yaml**: "어떤 컨트롤러가 그 인터페이스를 사용할지" 지정 → 플러그 꽂기
- **controller_manager**: 런타임에 이 둘을 연결

### yaml 파일 구조

```yaml
controller_manager:
  ros__parameters:
    update_rate: 50                    # 컨트롤러 루프를 초당 50회 실행

    joint_state_broadcaster:           # 컨트롤러 이름 (임의로 지정 가능)
      type: joint_state_broadcaster/JointStateBroadcaster  # 실제 플러그인
```

### ros2_control GitHub에서 참조 가능한 주요 플러그인

| 플러그인 | 용도 |
|---|---|
| `joint_state_broadcaster/JointStateBroadcaster` | 조인트 상태 발행 |
| `diff_drive_controller/DiffDriveController` | 바퀴 속도 제어 (차동 구동) |
| `joint_trajectory_controller/JointTrajectoryController` | 팔 로봇 경로 제어 |

---

## Q4. Controller Manager가 노드로 만들어진다는 것, 클로즈드 루프, update_rate vs publish_rate

### controller_manager는 ROS2 노드다

`controller_manager`는 일반적인 ROS2 노드로 실행됩니다.
launch 파일에서 아래처럼 노드로 띄우게 됩니다:

```python
Node(
    package="controller_manager",
    executable="ros2_control_node",
    parameters=[robot_description, controller_params_file],
)
```

즉, `my_robot_controller.yaml`의 파라미터들은 이 노드에 전달되는 ROS2 파라미터입니다.

---

### 클로즈드 루프 (Closed Loop Control)

ros2_control은 **클로즈드 루프(폐루프)** 제어 구조입니다:

```
[명령 입력] → [Controller] → [command_interface] → [하드웨어]
                   ↑                                      ↓
              [비교/보정]  ←  [state_interface]  ←  [센서 피드백]
```

- **오픈 루프**: 명령만 내리고 결과를 확인하지 않음
- **클로즈드 루프**: 현재 상태(피드백)를 읽어서 목표값과 비교하며 지속적으로 보정

xacro에서 선언한 `state_interface`(위치, 속도)가 바로 이 피드백 경로입니다.

---

### update_rate vs publish_rate

| 파라미터 | 위치 | 의미 |
|---|---|---|
| `update_rate` | `controller_manager` 아래 | 제어 루프 전체가 실행되는 주기 (Hz) |
| `publish_rate` | 각 컨트롤러 파라미터 아래 | 해당 컨트롤러가 토픽을 발행하는 주기 (Hz) |

```yaml
controller_manager:
  ros__parameters:
    update_rate: 50          # 제어 루프: 초당 50회 (모든 컨트롤러 공통)

joint_state_broadcaster:
  ros__parameters:
    publish_rate: 50.0       # /joint_states 토픽 발행: 초당 50회
```

- `update_rate`: 하드웨어에서 상태를 읽고, 컨트롤러를 계산하고, 명령을 쓰는 **전체 루프 속도**
- `publish_rate`: 그 중에서 토픽으로 **외부에 알리는 속도** (update_rate보다 낮게 설정 가능)
- 보통 `publish_rate ≤ update_rate` 로 설정

---

## Q5. XML launch 파일에서 xacro 주석의 `:` 때문에 오류가 발생하는 이유

### 오류 내용

```
Failed to convert '...' using yaml rules: yaml.safe_load() failed
mapping values are not allowed here
```

### 원인

XML launch 파일에서 `$(command 'xacro ...')` 로 xacro 출력을 파라미터로 전달할 때,
launch 시스템이 그 값을 **YAML로도 파싱**을 시도합니다.
xacro 주석 안에 `태그: 설명` 형식의 `:` 가 있으면 YAML의 key-value 문법으로 해석되어 오류 발생.

### 해결책 - Python launch 파일 사용

`xacro.process_file()` 로 직접 파싱하면 YAML 파싱 단계를 거치지 않아 주석의 `:` 와 무관합니다.

```python
import xacro
robot_description = xacro.process_file(xacro_file).toxml()

Node(
    package="robot_state_publisher",
    executable="robot_state_publisher",
    parameters=[{"robot_description": robot_description}],
)
```

---

## Q6. controller_manager spawner 실행 시 서비스를 찾지 못하는 문제 (Humble)

### 오류 내용

```
waiting for service /controller_manager/list_controllers to become available...
Could not contact service /controller_manager/list_controllers
```

### 원인 - Humble remap 문제

controller_manager가 초기화되려면 robot_description을 받아야 합니다.

| 노드 | 토픽 |
|---|---|
| robot_state_publisher | `/robot_description` 에 **발행** |
| controller_manager (Humble) | `/controller_manager/robot_description` 을 **구독** |

두 토픽이 달라서 controller_manager가 robot_description을 못 받고 초기화가 안 됨
→ 서비스가 열리지 않음 → spawner가 서비스를 찾지 못함

> Jazzy에서는 이 문제가 없음. Humble 전용 이슈.

### 임시 해결책 - 실행 시 remap 추가

```bash
ros2 run controller_manager ros2_control_node \
  --ros-args \
  --params-file /path/to/my_robot_controller.yaml \
  --remap /controller_manager/robot_description:=/robot_description
```

### 영구 해결책 - launch 파일에 remap 추가

```xml
<node pkg="controller_manager" exec="ros2_control_node">
    <param ..../>
    <remap from="/controller_manager/robot_description" to="/robot_description"/>
</node>
```

---

## Q7. controller_manager 실행 시 FIFO RT scheduling WARN은 무엇인가?

### 오류 내용

```
[WARN] Could not enable FIFO RT scheduling policy: with error number <1>(Operation not permitted)
```

### 원인

controller_manager는 제어 루프를 정확한 주기로 실행하기 위해 Linux의 **FIFO 실시간(RT) 스케줄링**을 요청합니다.
일반 사용자 권한으로는 이 권한이 없어서 발생하는 경고입니다.

### 영향

**동작에는 문제없음.** 개발/학습 환경에서는 무시해도 됩니다.
실제 로봇에서 정밀한 제어가 필요할 때 RT 커널 설정으로 해결합니다.

---

## Q8. diff_drive_controller yaml 설정과 전체 실행 순서

### my_robot_controller.yaml 최종 구조

```yaml
controller_manager:
  ros__parameters:
    update_rate: 50

    joint_state_broadcaster:
      type: joint_state_broadcaster/JointStateBroadcaster

    diff_drive_controller:
      type: diff_drive_controller/DiffDriveController


diff_drive_controller:
  ros__parameters:
    left_wheel_names: ["base_left_wheel_joint"]
    right_wheel_names: ["base_right_wheel_joint"]

    wheel_separation: 0.45      # 바퀴 간격 (m)
    wheel_radius: 0.1           # 바퀴 반지름 (m)

    odom_frame_id: "odom"
    base_frame_id: "base_footprint"

    pose_covariance_diagonal: [0.001, 0.001, 0.001, 0.001, 0.001, 0.01]
    twist_covariance_diagonal: [0.001, 0.001, 0.001, 0.001, 0.001, 0.01]

    enable_odom_tf: true
    publish_rate: 50.0

    linear.x.max_velocity: 1.0
    linear.x.min_velocity: -1.0
    angular.z.max_velocity: 1.0
    angular.z.min_velocity: -1.0
```

> 주의: `wheel_seperation` (오타) → `wheel_separation` (올바름). 오타 시 값이 0으로 남아 초기화 오류 발생.

### controller_manager yaml 구조 규칙

- `controller_manager.ros__parameters` 아래: 컨트롤러 이름과 `type` 등록 (controller_manager가 어떤 컨트롤러를 로드할지 선언)
- 최상위 레벨에 컨트롤러 이름으로 별도 섹션: 해당 컨트롤러의 세부 파라미터 설정

### 전체 실행 순서 (터미널 4개)

```
터미널 1: ros2 launch my_robot_description rsp.launch.py
          → robot_state_publisher 실행, /robot_description 발행

터미널 2: ros2 run controller_manager ros2_control_node \
            --ros-args \
            --params-file .../my_robot_controller.yaml \
            --remap /controller_manager/robot_description:=/robot_description
          → controller_manager 실행 (Humble remap 필요)
ros2 run controller_manager ros2_control_node \
> --ros-args \
> --params-file ~/ros2_control_ws/src/my_robot_bringup/config/my_robot_controller.yaml 
터미널 3: ros2 run controller_manager spawner joint_state_broadcaster
          → joint_state_broadcaster 로드 및 활성화

터미널 4: ros2 run controller_manager spawner diff_drive_controller
          → diff_drive_controller 로드 및 활성화
```

### 결과 확인

- `joint_state_broadcaster`: `/joint_states` 토픽 발행 → RViz에서 로봇 모델 표시
- `diff_drive_controller`: `/cmd_vel` 구독, `/odom` 발행 준비 완료

---

## Q9. cmd_vel 입력부터 RViz에서 로봇이 움직이기까지의 전체 흐름

### 실행 환경

```
teleop_twist_keyboard  →  /diff_drive_controller/cmd_vel  (TwistStamped, remapped)
```

### 거시적 흐름 (6단계)

```
[1] teleop_twist_keyboard
    키보드 입력 → TwistStamped 발행 → /diff_drive_controller/cmd_vel

[2] diff_drive_controller (컨트롤러)
    cmd_vel 수신 → 좌/우 바퀴 목표 속도 계산
    → command_interface에 속도값 쓰기

[3] mock_components/GenericSystem (가상 하드웨어)
    command_interface에서 속도값 읽기
    calculate_dynamics: true → 속도를 시간에 대해 적분 → 위치 계산
    → state_interface(position, velocity) 갱신

[4-A] diff_drive_controller (피드백 읽기)
    state_interface에서 바퀴 위치/속도 읽기
    → 오도메트리(이동 거리, 방향) 계산
    → /odom 토픽 발행
    → enable_odom_tf: true → odom → base_footprint TF 발행

[4-B] joint_state_broadcaster
    state_interface에서 모든 조인트 위치/속도 읽기
    → /joint_states 토픽 발행

[5] robot_state_publisher
    /joint_states 구독
    → URDF의 조인트 구조 참고하여 나머지 TF 계산
    → base_footprint → base_link → 바퀴들 TF 발행

[6] RViz
    TF 트리 전체 구독
    → 로봇 모델을 올바른 위치/방향으로 렌더링
```

### 핵심 요약

| 구간 | 전달 수단 |
|---|---|
| 사용자 → 컨트롤러 | `/cmd_vel` 토픽 |
| 컨트롤러 → 하드웨어 | `command_interface` (ros2_control 내부 메모리) |
| 하드웨어 → 컨트롤러 | `state_interface` (ros2_control 내부 메모리) |
| 컨트롤러 → 외부 | `/odom` 토픽, TF |
| 조인트 상태 → 외부 | `/joint_states` 토픽, TF |

`command_interface` / `state_interface` 는 ROS 토픽이 아니라 **ros2_control 내부의 공유 메모리**입니다.
실제 하드웨어에서는 이 인터페이스가 드라이버를 통해 모터/엔코더와 연결됩니다.

---

## Q10. Controller Manager란 무엇인가?

### 한 줄 정의

controller_manager는 **하드웨어(플러그인)와 컨트롤러(플러그인)를 연결하고 관리하는 중간 관리자 노드**입니다.

### 위치와 역할

```
[URDF/xacro]
    ↓ robot_description 파라미터
[controller_manager 노드]
    ├── 하드웨어 플러그인 로드 (예: mock_components/GenericSystem)
    ├── command_interface / state_interface 등록 및 관리
    ├── 컨트롤러 플러그인 로드/활성화/비활성화
    └── 제어 루프 실행 (update_rate Hz)
```

### 주요 책임 3가지

| 책임 | 설명 |
|---|---|
| 하드웨어 관리 | URDF에 선언된 하드웨어 플러그인을 로드하고 interface를 초기화 |
| 컨트롤러 관리 | spawner의 요청을 받아 컨트롤러를 로드/설정/활성화/비활성화 |
| 제어 루프 실행 | `update_rate` 주기로 read → update → write 사이클 반복 |

### 제어 루프 사이클 (매 tick마다 반복)

```
read()   → 하드웨어에서 state_interface 값 읽기 (엔코더 등)
update() → 각 컨트롤러의 계산 실행 (PID 등)
write()  → command_interface 값을 하드웨어에 쓰기 (모터 명령 등)
```

### spawner와의 관계

`spawner`는 controller_manager에게 컨트롤러 로드를 **요청하는 도구**입니다.
서비스 호출로 동작하기 때문에, controller_manager가 완전히 초기화된 후에만 사용 가능합니다.

```
ros2 run controller_manager spawner joint_state_broadcaster
    → /controller_manager/load_controller 서비스 호출
    → /controller_manager/configure_controller 서비스 호출
    → /controller_manager/switch_controller 서비스 호출 (활성화)
```

### 실행 방법

```bash
ros2 run controller_manager ros2_control_node \
  --ros-args \
  --params-file /path/to/my_robot_controller.yaml \
  --remap /controller_manager/robot_description:=/robot_description
```

- `robot_description` 파라미터(또는 토픽)를 받아야 하드웨어 플러그인을 초기화할 수 있음
- 초기화 전에는 서비스가 열리지 않아 spawner가 접근 불가

### launch 파일에서의 구성

```xml
<node pkg="controller_manager" exec="ros2_control_node">
    <param from="$(find-pkg-share my_robot_bringup)/config/my_robot_controller.yaml" />
    <remap from="/controller_manager/robot_description" to="/robot_description"/>
</node>
```

---

## Q11. ros2 control CLI 명령어들

### list_controllers — 현재 로드된 컨트롤러 목록

```bash
ros2 control list_controllers
```

```
diff_drive_controller   diff_drive_controller/DiffDriveController      active
joint_state_broadcaster joint_state_broadcaster/JointStateBroadcaster  active
```

- 컨트롤러 이름 / 플러그인 타입 / 상태(active, inactive, unconfigured) 표시
- `active`: 제어 루프에서 실행 중인 상태

---

### list_controller_types — 설치된 컨트롤러 플러그인 목록

```bash
ros2 control list_controller_types
```

시스템에 설치된 모든 컨트롤러 플러그인과 기반 인터페이스 타입을 표시합니다.

| 타입 | 의미 |
|---|---|
| `controller_interface::ControllerInterface` | 일반 컨트롤러 |
| `controller_interface::ChainableControllerInterface` | 체이너블 컨트롤러 (컨트롤러끼리 연결 가능) |

주요 컨트롤러:

| 플러그인 | 용도 |
|---|---|
| `diff_drive_controller/DiffDriveController` | 차동 구동 로봇 |
| `joint_state_broadcaster/JointStateBroadcaster` | 조인트 상태 발행 |
| `joint_trajectory_controller/JointTrajectoryController` | 팔 로봇 경로 제어 |
| `velocity_controllers/JointGroupVelocityController` | 조인트 그룹 속도 제어 |
| `position_controllers/JointGroupPositionController` | 조인트 그룹 위치 제어 |
| `effort_controllers/JointGroupEffortController` | 조인트 그룹 힘/토크 제어 |
| `imu_sensor_broadcaster/IMUSensorBroadcaster` | IMU 센서 데이터 발행 |
| `ackermann_steering_controller/AckermannSteeringController` | 아커만 조향 (자동차형) |
| `pid_controller/PidController` | PID 제어기 |

---

### list_hardware_interfaces — 등록된 인터페이스 목록

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

xacro에서 선언한 인터페이스들이 실제로 등록된 것을 확인할 수 있습니다.

**command interface 상태 태그:**

| 태그 | 의미 |
|---|---|
| `[available]` | 하드웨어가 초기화되어 사용 가능한 상태 |
| `[claimed]` | 특정 컨트롤러가 이미 독점 사용 중 |
| `[unavailable]` | 하드웨어 미초기화 또는 비활성 상태 |

> `command_interface`는 한 번에 하나의 컨트롤러만 `[claimed]` 가능합니다. (여러 컨트롤러가 같은 조인트를 동시에 제어하는 것을 방지)
> `state_interface`는 여러 컨트롤러가 동시에 읽기 가능합니다.

---
