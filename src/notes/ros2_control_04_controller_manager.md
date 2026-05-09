# ROS2 Control - 04. Controller Manager와 yaml 설정

## Controller Manager란?

Controller Manager는 ROS2 Control의 **중앙 허브**입니다.

> 비유: 공항의 관제탑과 같습니다.
> 비행기(컨트롤러)가 이륙/착륙(활성화/비활성화)을 요청하면 관제탑(controller_manager)이 허가하고 조율합니다.
> 활주로(하드웨어 인터페이스)도 관제탑이 관리합니다.

```
역할 1: 하드웨어 플러그인 로드 및 interface 초기화
역할 2: 컨트롤러 로드 / 설정 / 활성화 / 비활성화
역할 3: 제어 루프 실행 (read → update → write)
```

---

## yaml 파일 구조

### 전체 예시

```yaml
# 1부: controller_manager 자체 설정 + 컨트롤러 등록
controller_manager:
  ros__parameters:
    update_rate: 50                          # 제어 루프 주기 (Hz)

    joint_state_broadcaster:                 # 컨트롤러 이름 (임의 지정)
      type: joint_state_broadcaster/JointStateBroadcaster

    diff_drive_controller:                   # 컨트롤러 이름 (임의 지정)
      type: diff_drive_controller/DiffDriveController


# 2부: 각 컨트롤러의 세부 파라미터
diff_drive_controller:
  ros__parameters:
    left_wheel_names: ["base_left_wheel_joint"]
    right_wheel_names: ["base_right_wheel_joint"]
    wheel_separation: 0.45
    wheel_radius: 0.1
    ...
```

### 구조 규칙

```
yaml 파일
├── controller_manager.ros__parameters  ← controller_manager 노드의 파라미터
│   ├── update_rate                     ← 제어 루프 속도
│   ├── [컨트롤러이름].type             ← 어떤 플러그인을 로드할지 등록
│   └── ...
│
└── [컨트롤러이름].ros__parameters      ← 해당 컨트롤러의 세부 설정
    ├── 파라미터1
    └── 파라미터2
```

> 중요: `controller_manager` 아래에는 **등록**만 하고, 세부 파라미터는 **최상위 레벨**에 별도로 작성합니다.

---

## update_rate vs publish_rate

| 파라미터 | 위치 | 의미 |
|---|---|---|
| `update_rate` | `controller_manager` 아래 | 제어 루프 전체 속도 (모든 컨트롤러 공통) |
| `publish_rate` | 각 컨트롤러 아래 | 해당 컨트롤러가 토픽을 발행하는 속도 |

```
update_rate: 50Hz  →  50ms마다 read/update/write 실행
publish_rate: 50Hz →  50ms마다 /joint_states, /odom 등 발행
```

- `publish_rate`는 `update_rate`보다 낮거나 같게 설정
- 예: update_rate=100Hz, publish_rate=50Hz → 제어는 100번/초, 발행은 50번/초

---

## diff_drive_controller 주요 파라미터

```yaml
diff_drive_controller:
  ros__parameters:
    # 바퀴 조인트 이름 (URDF의 joint name과 일치해야 함)
    left_wheel_names: ["base_left_wheel_joint"]
    right_wheel_names: ["base_right_wheel_joint"]

    # 로봇 치수 (실제 로봇 측정값 입력)
    wheel_separation: 0.45    # 좌우 바퀴 중심 간격 (m)
    wheel_radius: 0.1         # 바퀴 반지름 (m)

    # 오도메트리 프레임
    odom_frame_id: "odom"           # 절대 위치 기준 프레임
    base_frame_id: "base_footprint" # 로봇 기준 프레임

    # 오도메트리 TF 발행 여부
    enable_odom_tf: true      # true: odom→base_footprint TF 자동 발행

    # 발행 주기
    publish_rate: 50.0

    # 속도 제한
    linear.x.max_velocity: 1.0
    linear.x.min_velocity: -1.0
    angular.z.max_velocity: 1.0
    angular.z.min_velocity: -1.0
```

### wheel_separation 계산 방법

```
      [왼쪽 바퀴]     [오른쪽 바퀴]
           |←── wheel_separation ──→|
           |         0.45m          |
```

URDF에서 바퀴 조인트의 y 좌표를 보면 알 수 있습니다:
- `base_left_wheel_joint`: xyz="... 0.225 ..."
- `base_right_wheel_joint`: xyz="... -0.225 ..."
- wheel_separation = 0.225 + 0.225 = **0.45m**

---

## Controller Manager가 초기화되는 과정

```
1. ros2_control_node 실행
         ↓
2. robot_description 수신 (토픽 또는 파라미터로)
   ← 이게 안 되면 초기화가 멈춤 (Humble remap 문제)
         ↓
3. URDF 파싱 → 하드웨어 플러그인 로드
         ↓
4. command/state interface 등록
         ↓
5. 서비스 활성화 (/controller_manager/list_controllers 등)
         ↓
6. spawner가 서비스를 통해 컨트롤러 로드 요청 가능
```

---

## Humble에서 robot_description 수신 문제

ROS2 Humble에서는 controller_manager가 구독하는 토픽 이름이 다릅니다.

| | 토픽 이름 |
|---|---|
| robot_state_publisher 발행 | `/robot_description` |
| controller_manager 구독 (Humble) | `/controller_manager/robot_description` |

해결: launch 파일에서 remap 추가

```xml
<node pkg="controller_manager" exec="ros2_control_node">
    <param from="$(find-pkg-share my_robot_bringup)/config/my_robot_controller.yaml" />
    <remap from="/controller_manager/robot_description" to="/robot_description"/>
</node>
```

> Jazzy(Ubuntu 24.04)에서는 이 문제가 없습니다.

---

## spawner란?

spawner는 controller_manager에게 컨트롤러 로드를 **요청하는 도구**입니다.
내부적으로 서비스 호출로 동작합니다.

```bash
ros2 run controller_manager spawner joint_state_broadcaster
```

```
spawner → /controller_manager/load_controller      서비스 호출
        → /controller_manager/configure_controller 서비스 호출
        → /controller_manager/switch_controller    서비스 호출 (활성화)
```

launch 파일에서는 노드로 실행:

```xml
<node pkg="controller_manager" exec="spawner" args="joint_state_broadcaster" />
<node pkg="controller_manager" exec="spawner" args="diff_drive_controller" />
```
