# 2 wheeler DCC 2.0 Qt QML Project



This is the Qt QML GUI project of i.MX95 DCC 2.0 demo.



## Configuration

Certain behavior of the GUI is configuralbe via `/etc/dcc/app.conf`. Sample config of supported hardware can be found under `board` directory.




### [Key]

```ini
[Key]
enabled=true
```

"Key" feature shows a distant bubble when a key device is around. To enable "Key", set `enabled=true` in `[Key]` section. 

KW45 LOC board with Channel Sounding Initiator firmware must present and connected to one of USB ports of i.MX95 EVK.



### [LLM]

```ini
[LLM]
enabled=false
```

"LLM" feature enables a Large Language Model running locally, enables voice command and voice chat. To enable "LLM", set `enabled=true` in `[LLM]` section.

LLM is not installed by default on standard firmware. Contact NXP for additional info.



### [IMU]

```ini
[IMU]
enabled=false
chaseView=true
```

 "IMU" feature enables attitude control of the 3D motorcycle model with an external IMU. To enable "IMU", set `enabled=true` in `[IMU]` section. Additionally, the yaw axis can be locked by setting `chaseView` to `true` to prevent yaw drifting in case no secondary yaw sensor source is provided (aka no magnetometer).



## Build Dependency

- Qt 6.9.3

- libgpio and libgpiocxx

- libiio

- eigen3



## Build Steps

- First, generate yocto toolchain from BSP release:
  
  ```shell
  bitbake -c do_populate_sdk imx-image-full
  ```

- Install and source the sdk into current environment:
  
  ```shell
  ./fsl-imx-xwayland-glibc-x86_64-imx-image-full-armv8a-imx95-19x19-lpddr5-evk-toolchain-6.18-whinlatter.sh
  source /opt/fsl-imx-xwayland-glibc-x86_64-imx-image-full-armv8a-imx95-19x19-lpddr5-evk-toolchain-6.18-whinlatter/environment-setup-armv8a-poky-linux
  ```

- Extract source code:
  
  Download source code release package and extract the source code.

- Build the Qt GUI:
  
  ```shell
  cd imx95-dcc-qt
  mkdir build && cd build
  cmake ..
  make
  ```
