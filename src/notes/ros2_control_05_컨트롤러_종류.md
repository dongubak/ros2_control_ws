# ROS2 Control - 05. 컨트롤러 종류

## 컨트롤러란?

컨트롤러는 **상위 명령을 받아 하드웨어 인터페이스 값으로 변환하는 소프트웨어 모듈**입니다.

```
[외부 명령 (토픽/액션)] → [컨트롤러] → [command_interface]
                              ↑
                     [state_interface] (피드백)
```

설치된 컨트롤러 목록 확인:
```bash
ros2 control list_controller_types
```

---

## 브로드캐스터 (Broadcaster)

명령을 내리는 것이 아니라 **상태를 외부에 알리는** 컨트롤러입니다.

### joint_state_broadcaster

```
state_interface → joint_state_broadcaster → /joint_states 토픽
```

- 모든 조인트의 위치/속도/힘 값을 `/joint_states`로 발행
- `robot_state_publisher`가 이 토픽을 구독하여 TF를 계산
- RViz에서 로봇 모델이 움직이는 데 필수

```yaml
joint_state_broadcaster:
  ros__parameters:
    type: joint_state_broadcaster/JointStateBroadcaster
```

### imu_sensor_broadcaster

```
IMU state_interface → imu_sensor_broadcaster → /imu/data 토픽
```

- IMU 센서 데이터를 `sensor_msgs/Imu` 메시지로 발행

### force_torque_sensor_broadcaster

- 6축 힘/토크 센서 데이터 발행

---

## 모바일 로봇 컨트롤러

### diff_drive_controller (이 프로젝트에서 사용)

차동 구동 방식(좌우 바퀴 독립 제어) 모바일 로봇용 컨트롤러

```
/cmd_vel (Twist) → diff_drive_controller → 좌/우 바퀴 velocity command_interface
                           ↓
                     /odom 발행 + odom→base_footprint TF 발행
```

- 입력: `/diff_drive_controller/cmd_vel` (geometry_msgs/TwistStamped 또는 Twist)
- 출력: `/diff_drive_controller/odom`, TF

### ackermann_steering_controller

자동차형 조향(앞바퀴 조향 + 뒷바퀴 구동) 로봇용

```
/cmd_vel → ackermann_controller → 조향각 position + 구동 velocity command_interface
```

### mecanum_drive_controller

4개의 메카넘 바퀴를 가진 전방향 이동 로봇용

### tricycle_controller

삼륜 로봇용 (앞바퀴 1개 조향/구동, 뒷바퀴 2개 고정)

---

## 관절 제어 컨트롤러

### joint_trajectory_controller

로봇 팔처럼 **여러 관절을 경로(trajectory)에 따라** 제어합니다.

```
/joint_trajectory (액션) → joint_trajectory_controller → 각 관절 position/velocity command_interface
```

- MoveIt이 이 컨트롤러를 통해 로봇 팔을 제어
- 시간에 따른 경로 보간(interpolation) 지원

### forward_command_controller

가장 단순한 컨트롤러. 받은 명령 값을 그대로 인터페이스에 전달합니다.

```
/commands 토픽 → forward_command_controller → command_interface (그대로 전달)
```

### position_controllers / JointGroupPositionController

여러 조인트의 목표 위치를 동시에 설정합니다.

### velocity_controllers / JointGroupVelocityController

여러 조인트의 목표 속도를 동시에 설정합니다.

### effort_controllers / JointGroupEffortController

여러 조인트의 목표 힘/토크를 동시에 설정합니다.

---

## 체이너블 컨트롤러 (Chainable Controller)

컨트롤러끼리 연결할 수 있는 특수한 컨트롤러입니다.

### pid_controller

PID 제어를 수행하는 컨트롤러. 다른 컨트롤러의 앞단에 연결합니다.

```
[목표값 토픽] → [pid_controller] → reference_interface → [velocity_controller] → command_interface
```

비유: pid_controller는 "얼마나 빠르게 움직여야 하는지 계산"하고,
velocity_controller는 "그 속도대로 모터를 돌려"라고 명령하는 역할 분리

### passthrough_controller

입력을 그대로 다음 컨트롤러로 전달. 디버깅이나 테스트에 사용.

---

## 컨트롤러 선택 기준

| 로봇 종류 | 추천 컨트롤러 |
|---|---|
| 차동 구동 모바일 로봇 | `diff_drive_controller` |
| 자동차형 로봇 | `ackermann_steering_controller` |
| 메카넘 바퀴 로봇 | `mecanum_drive_controller` |
| 로봇 팔 (경로 제어) | `joint_trajectory_controller` |
| 단순 위치 제어 | `position_controllers/JointGroupPositionController` |
| 단순 속도 제어 | `velocity_controllers/JointGroupVelocityController` |
| 조인트 상태 발행 | `joint_state_broadcaster` (항상 필요) |
| IMU 데이터 발행 | `imu_sensor_broadcaster` |

> `joint_state_broadcaster`는 로봇 종류에 관계없이 **항상 함께 사용**합니다.
> `/joint_states` 토픽이 없으면 robot_state_publisher가 TF를 계산할 수 없습니다.
