# ROS2 Control - 02. 아키텍처

## command_interface와 state_interface

ROS2 Control의 핵심은 **인터페이스**입니다.
인터페이스는 하드웨어와 컨트롤러가 데이터를 주고받는 **표준화된 창구**입니다.

### command_interface (명령 창구)

> "이 창구에 값을 쓰면 하드웨어가 그대로 실행합니다"

- 컨트롤러 → 하드웨어 방향
- 예: 바퀴에 "초당 1.0 라디안으로 회전해라"

### state_interface (상태 창구)

> "이 창구를 읽으면 하드웨어의 현재 상태를 알 수 있습니다"

- 하드웨어 → 컨트롤러 방향
- 예: "현재 바퀴 위치는 3.14 라디안이다"

### 인터페이스 종류

| 인터페이스 | 의미 | 예시 |
|---|---|---|
| `velocity` | 속도 | 바퀴 RPM, 관절 각속도 |
| `position` | 위치(각도) | 관절 각도, 바퀴 누적 회전량 |
| `effort` | 힘/토크 | 모터 전류, 관절 토크 |
| `acceleration` | 가속도 | 관절 각가속도 |

---

## 인터페이스는 ROS 토픽이 아니다

중요한 개념입니다.

```
❌ 잘못된 이해: /cmd_vel → 하드웨어
✅ 올바른 이해: /cmd_vel → 컨트롤러 → command_interface → 하드웨어
```

- **ROS 토픽**: 네트워크를 통해 노드 간에 데이터 전달 (느림, 유연함)
- **인터페이스**: controller_manager 내부의 **공유 메모리** (빠름, 실시간 제어에 적합)

인터페이스는 제어 루프(50Hz, 100Hz 등)마다 직접 읽고 쓰기 때문에 토픽보다 훨씬 빠릅니다.

---

## 제어 루프 (Control Loop)

controller_manager는 `update_rate`에 설정된 주기로 아래 3단계를 반복합니다.

```
┌──────────────────────────────────────────┐
│              매 tick마다 반복              │
│                                          │
│  1. read()                               │
│     하드웨어에서 state_interface 값 읽기  │
│     (엔코더 위치, 속도 등)                │
│                 ↓                        │
│  2. update()                             │
│     각 컨트롤러의 계산 실행               │
│     (PID 계산, 속도→바퀴 변환 등)         │
│                 ↓                        │
│  3. write()                              │
│     command_interface 값을 하드웨어에 쓰기│
│     (모터에 목표 속도 전달)               │
└──────────────────────────────────────────┘
```

이 루프가 **클로즈드 루프(Closed Loop)** 제어의 핵심입니다.
현재 상태(state)를 읽어서 목표값과 비교하고, 차이를 보정하는 것을 계속 반복합니다.

---

## Hardware Interface 플러그인

URDF/xacro에서 `<plugin>` 태그로 선언합니다.

### mock_components/GenericSystem (학습용)

```xml
<hardware>
    <plugin>mock_components/GenericSystem</plugin>
    <param name="calculate_dynamics">true</param>
</hardware>
```

- 실제 하드웨어 없이 소프트웨어만으로 동작
- `calculate_dynamics: true` → 속도 명령을 시간에 대해 적분하여 위치 자동 계산
- 실제 물리 법칙은 적용되지 않음 (관성, 마찰 없음)

### 실제 하드웨어 플러그인 구조

```
[ros2_control 표준 인터페이스]
        ↕  (플러그인이 구현)
[제조사 드라이버 / SDK]
        ↕
[실제 모터 / 센서 하드웨어]
```

하드웨어 플러그인은 `hardware_interface::SystemInterface`를 상속받아
`read()`, `write()` 함수를 구현합니다.

---

## 컨트롤러 플러그인

컨트롤러는 `controller_interface::ControllerInterface`를 상속받아 구현됩니다.

### 일반 컨트롤러 (ControllerInterface)

독립적으로 동작하며, 토픽/액션으로 입력을 받고 인터페이스를 통해 하드웨어를 제어합니다.

```
/cmd_vel 토픽 → [diff_drive_controller] → command_interface
                        ↑
               state_interface (피드백)
```

### 체이너블 컨트롤러 (ChainableControllerInterface)

컨트롤러끼리 연결할 수 있는 특수한 컨트롤러입니다.
출력이 토픽이 아닌 다른 컨트롤러의 입력으로 바로 연결됩니다.

```
[상위 컨트롤러] → reference_interface → [하위 컨트롤러] → command_interface
```

예: `pid_controller` (체이너블) → `velocity_controller`

---

## 전체 구조 요약

```
[teleop / Nav2 / MoveIt]
         │ ROS 토픽 (/cmd_vel 등)
         ▼
   [Controller]  ←─────────────────────────────────┐
    diff_drive                                      │
    joint_traj   ←── reference_interface (체이너블) │
    pid 등                                          │
         │ command_interface (공유 메모리)           │
         ▼                                          │
[Controller Manager]                               │
    read/update/write 루프                          │
         │ state_interface (공유 메모리) ────────────┘
         ▼
[Hardware Interface]
    mock / Gazebo / 실제 드라이버
         │
         ▼
    [실제 하드웨어]
```
