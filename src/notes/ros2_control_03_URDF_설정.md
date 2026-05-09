# ROS2 Control - 03. URDF 설정

## URDF에서 ros2_control을 선언하는 이유

URDF는 원래 로봇의 **외형과 물리적 구조**를 기술하는 파일입니다.
여기에 `<ros2_control>` 태그를 추가하면 "이 로봇의 하드웨어 인터페이스는 이렇게 생겼다"고
controller_manager에게 알려줄 수 있습니다.

> 비유: URDF가 건물 설계도라면, `<ros2_control>` 태그는 "전기 콘센트 배치도"입니다.
> 어디에 어떤 콘센트(인터페이스)가 있는지 선언하는 것입니다.

---

## 파일 구조

xacro를 사용하는 경우 별도 파일로 분리하는 것이 일반적입니다.

```
my_robot.urdf.xacro
    └── <xacro:include> mobile_base.ros2_control.xacro
```

`mobile_base.ros2_control.xacro`에 ros2_control 관련 내용만 따로 작성합니다.

---

## 전체 구조

```xml
<ros2_control name="MobileBaseHardwareInterface" type="system">

    <hardware>
        <plugin>mock_components/GenericSystem</plugin>
        <param name="calculate_dynamics">true</param>
    </hardware>

    <joint name="base_right_wheel_joint">
        <command_interface name="velocity" />
        <state_interface name="position" />
        <state_interface name="velocity" />
    </joint>

    <joint name="base_left_wheel_joint">
        <command_interface name="velocity" />
        <state_interface name="position" />
        <state_interface name="velocity" />
    </joint>

</ros2_control>
```

---

## 태그별 설명

### `<ros2_control>` 태그

```xml
<ros2_control name="MobileBaseHardwareInterface" type="system">
```

| 속성 | 의미 |
|---|---|
| `name` | 이 하드웨어 인터페이스의 고유 이름. controller_manager가 식별에 사용 |
| `type` | 하드웨어의 종류 |

**type 종류:**

| type | 의미 | 사용 예 |
|---|---|---|
| `system` | 여러 조인트를 하나의 하드웨어로 묶어 제어 | 모바일 베이스, 로봇 팔 전체 |
| `actuator` | 조인트 하나에 대응하는 단일 액추에이터 | 단일 모터 |
| `sensor` | 명령 없이 상태만 읽는 센서 | IMU, 힘/토크 센서 |

---

### `<hardware>` 태그

```xml
<hardware>
    <plugin>mock_components/GenericSystem</plugin>
    <param name="calculate_dynamics">true</param>
</hardware>
```

실제로 어떤 소프트웨어(플러그인)가 하드웨어를 담당할지 지정합니다.

| 플러그인 | 사용 환경 |
|---|---|
| `mock_components/GenericSystem` | 학습/개발 (하드웨어 없음) |
| `gazebo_ros2_control/GazeboSystem` | Gazebo 시뮬레이션 |
| 직접 구현한 플러그인 | 실제 로봇 |

**`calculate_dynamics: true`**

mock_components가 속도 명령을 받았을 때, 시간에 따라 적분하여 위치를 계산합니다.
이 옵션이 없으면 위치값이 항상 0으로 유지됩니다.

```
velocity = 1.0 rad/s
시간 경과: 2초
position = 1.0 × 2 = 2.0 rad  ← calculate_dynamics가 이걸 해줌
```

---

### `<joint>` 태그

```xml
<joint name="base_right_wheel_joint">
    <command_interface name="velocity" />
    <state_interface name="position" />
    <state_interface name="velocity" />
</joint>
```

URDF에 정의된 조인트와 **같은 이름**을 사용해야 합니다.
이 조인트가 어떤 인터페이스를 제공하는지 선언합니다.

**command_interface**: 이 조인트에 **명령을 내릴 수 있는 창구**

```xml
<command_interface name="velocity" />   <!-- 속도 명령 가능 -->
<command_interface name="position" />   <!-- 위치 명령 가능 -->
<command_interface name="effort" />     <!-- 힘/토크 명령 가능 -->
```

**state_interface**: 이 조인트의 **상태를 읽을 수 있는 창구**

```xml
<state_interface name="position" />     <!-- 현재 위치 읽기 가능 -->
<state_interface name="velocity" />     <!-- 현재 속도 읽기 가능 -->
<state_interface name="effort" />       <!-- 현재 토크 읽기 가능 -->
```

---

## 실제 로봇 예시 비교

### 모바일 베이스 (이 프로젝트)

```xml
<joint name="base_right_wheel_joint">
    <command_interface name="velocity" />   <!-- 속도로 제어 -->
    <state_interface name="position" />
    <state_interface name="velocity" />
</joint>
```

바퀴는 속도 명령으로 제어하고, 위치(누적 회전량)와 속도를 피드백으로 읽습니다.

### 로봇 팔 (위치 제어)

```xml
<joint name="shoulder_joint">
    <command_interface name="position" />   <!-- 각도로 제어 -->
    <state_interface name="position" />
    <state_interface name="velocity" />
    <state_interface name="effort" />
</joint>
```

관절 각도를 목표값으로 제어하고, 현재 각도/속도/토크를 피드백으로 읽습니다.

---

## 선언과 실제 사용의 관계

URDF에서 선언한 인터페이스는 **사용 가능한 인터페이스를 등록**하는 것입니다.
실제로 어떤 컨트롤러가 어떤 인터페이스를 사용할지는 yaml 파일에서 지정합니다.

```
URDF 선언:  base_right_wheel_joint/velocity  [command] ← "이런 창구가 있다"
yaml 설정:  diff_drive_controller가 이 창구를 사용          ← "이 창구를 쓰겠다"
실행 시:    [available] → [claimed]                         ← "창구 사용 중"
```

확인 방법:
```bash
ros2 control list_hardware_interfaces
```
