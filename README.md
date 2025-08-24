# udphmd-openvr

# README English Edition
A virtual OpenVR HMD driver.  
Send **position (x,y,z)** and **rotation quaternion** over **UDP**.  
Forked from [r57zone/OpenVR-OpenTrack](https://github.com/r57zone/OpenVR-OpenTrack).

---

## Quick Start

### 1. Compile Driver
Using Visual Studio to compile is recommended.

### 2. Install Driver
1. Copy the entire `udphmd` folder to  
   `YourSteamPath\steamapps\common\SteamVR\drivers`.
2. Restart SteamVR.

### 3. Send Data
Send **56-byte binary** to **UDP 8730** (default):

| Offset | Type      | Field                 |
|--------|-----------|-----------------------|
| 0-23   | double[3] | Position X Y Z (m)    |
| 24-55  | double[4] | Quaternion W X Y Z    |

---

## Configuration
Edit `udphmd\resources\settings\default.vrsettings`.

See [r57zone/OpenVR-OpenTrack](https://github.com/r57zone/OpenVR-OpenTrack) for full details.

| Name | Description |
|------|-------------|
| Port | UDP server port |
| DistanceBetweenEyes | Stereo image distance |
| DistortionK1, DistortionK2 | Lens distortion |
| ScreenOffsetX | Horizontal shift |
| ZoomHeight, ZoomWidth | Image scale |
| FOV | Field of view (°) |
| ipd | Inter-pupillary distance |
| FPS | Refresh rate |
| renderWidth,renderHeight | Eye buffer resolution |
| windowWidth,windowHeight | Window size |
| windowX,windowY | Window position |
| DebugMode | Debug mode. Lock to 30 FPS |

---

## License

| Component        | License       |
|------------------|---------------|
| UDPHMD-OpenVR    | CC0-1.0       |
| OpenVR SDK       | BSD-3-Clause  |
| OpenVR-OpenTrack | CC0           |

## Acknowledgments
Special thanks to [r57zone](https://github.com/r57zone) for the [OpenVR-OpenTrack](https://github.com/r57zone/OpenVR-OpenTrack) project, which provided a solid foundation for this driver.

# README 中文版

这是一个虚拟 OpenVR 头显驱动，你可以通过 **UDP** 直接发送 **位置 (x,y,z)** 和 **旋转四元数**。  
基于 [r57zone/OpenVR-OpenTrack](https://github.com/r57zone/OpenVR-OpenTrack) 二次开发。

---

## 快速上手

### 1.编译驱动
推荐使用 Visual Studio 编译

### 2. 安装驱动
1. 将整个 `udphmd` 文件夹复制到  
   `你的Steam路径\steamapps\common\SteamVR\drivers`  
2. 重启 SteamVR。

### 3. 发送数据
向默认 **UDP 8730 端口** 发送 **56 字节二进制** 数据：

| 偏移 | 类型  | 字段 |
|-----|------|------|
| 0-23 | double[3] | 位置 X Y Z |
| 24-55| double[4] | 四元数 W X Y Z |

---

## 配置说明
配置文件 `udphmd\resources\settings\default.vrsettings`

更多配置细节请参考 [r57zone/OpenVR-OpenTrack](https://github.com/r57zone/OpenVR-OpenTrack)。

| 参数名 | 含义 |
|-------|-----|
| Port | UDP 监听端口 |
| DistanceBetweenEyes | 双眼图像间距 |
| DistortionK1, DistortionK2 | 镜头畸变系数 |
| ScreenOffsetX | 水平图像偏移 |
| ZoomWidth, ZoomHeight | 缩放系数 |
| FOV | 视场角（度）|
| ipd | 瞳距 |
| FPS | 刷新率 |
| renderWidth, renderHeight | 单眼渲染分辨率 |
| windowWidth, renderHeight | 窗口大小 |
| windowX, windowY | 窗口位置 |
| DebugMode | 调试模式，会锁定 30 FPS |

---

## 许可证

| 组件              | 许可证        |
|-------------------|---------------|
| 本项目新代码       | CC0-1.0       |
| OpenVR SDK        | BSD-3-Clause  |
| OpenVR-OpenTrack  | CC0           |

## 致谢
特别感谢 [r57zone](https://github.com/r57zone) 提供的 [OpenVR-OpenTrack](https://github.com/r57zone/OpenVR-OpenTrack) 项目，为本驱动奠定了坚实基础。
