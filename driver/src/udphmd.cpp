//============ Copyright (c) Valve Corporation, All rights reserved. ============
//=============== Changed by r57zone (https://github.com/r57zone) ===============
//=========== Further changed by mmqltrl (https://github.com/mmqltrl) ===========

#include <openvr_driver.h>
#include "driverlog.h"

#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>

#if defined(_WINDOWS)
// #include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "WSock32.Lib")
#endif

using namespace vr;

#if defined(_WIN32)
#define HMD_DLL_EXPORT extern "C" __declspec(dllexport)
#define HMD_DLL_IMPORT extern "C" __declspec(dllimport)
#elif defined(__GNUC__) || defined(COMPILER_GCC) || defined(__APPLE__)
#define HMD_DLL_EXPORT extern "C" __attribute__((visibility("default")))
#define HMD_DLL_IMPORT extern "C"
#else
#error "Unsupported Platform."
#endif

inline HmdQuaternion_t Get_HmdQuaternion(double w, double x, double y, double z)
{
	HmdQuaternion_t quat = {w, x, y, z};
	return quat;
}

// HMDPoseData vars
double rW = 0, rX = 0, rY = 0, rZ = 0;
double pX = 0, pY = 0, pZ = 0;
struct HMDPoseDataS
{
	double X;
	double Y;
	double Z;
	double rW; // Quaternion rotation
	double rX;
	double rY;
	double rZ;
} HMDPoseData;

// WinSock
SOCKET socketS;
int bytes_read;
struct sockaddr_in from;
int fromlen;
bool SocketActivated = false;

std::thread *pSocketThread = NULL;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

float ToRadians(float degrees)
{
	return degrees * 3.14159265358979323846 / 180;
}

void WinSockReadFunc()
{
	while (SocketActivated)
	{
		memset(&HMDPoseData, 0, sizeof(HMDPoseData));
		bytes_read = recvfrom(socketS, (char *)(&HMDPoseData), sizeof(HMDPoseData), 0, (sockaddr *)&from, &fromlen);

		if (bytes_read > 0)
		{
			rW = HMDPoseData.rW;
			rX = HMDPoseData.rX;
			rY = HMDPoseData.rY;
			rZ = HMDPoseData.rZ;
			pX = HMDPoseData.X;
			pY = HMDPoseData.Y;
			pZ = HMDPoseData.Z;
		}
		else
			Sleep(1);
	}
}

class CSampleDeviceDriver : public vr::ITrackedDeviceServerDriver, public vr::IVRDisplayComponent
{
public:
	CSampleDeviceDriver()
	{
		m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
		m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;

		DriverLog("Using settings values");
		m_flIPD = vr::VRSettings()->GetFloat(k_pch_SteamVR_Section, k_pch_SteamVR_IPD_Float);

		char buf[1024];
		EVRSettingsError err = VRSettingsError_None;
		vr::VRSettings()->GetString("udphmd", "serialNumber", buf, sizeof(buf), &err);
		m_sSerialNumber = buf;
		if (err != VRSettingsError_None)
		{
			DriverLog("Failed to get serialNumber! Headset window will not display!");
		}

		vr::VRSettings()->GetString("udphmd", "modelNumber", buf, sizeof(buf));
		m_sModelNumber = buf;

		m_nWindowX = vr::VRSettings()->GetInt32("udphmd", "windowX");
		m_nWindowY = vr::VRSettings()->GetInt32("udphmd", "windowY");
		m_nWindowWidth = vr::VRSettings()->GetInt32("udphmd", "windowWidth");
		m_nWindowHeight = vr::VRSettings()->GetInt32("udphmd", "windowHeight");
		m_nRenderWidth = vr::VRSettings()->GetInt32("udphmd", "renderWidth");
		m_nRenderHeight = vr::VRSettings()->GetInt32("udphmd", "renderHeight");
		m_flSecondsFromVsyncToPhotons = vr::VRSettings()->GetFloat("udphmd", "secondsFromVsyncToPhotons");
		m_flFPS = vr::VRSettings()->GetFloat("udphmd", "FPS");

		m_fDistortionK1 = vr::VRSettings()->GetFloat("udphmd", "DistortionK1");
		m_fDistortionK2 = vr::VRSettings()->GetFloat("udphmd", "DistortionK2");
		m_fZoomWidth = vr::VRSettings()->GetFloat("udphmd", "ZoomWidth");
		m_fZoomHeight = vr::VRSettings()->GetFloat("udphmd", "ZoomHeight");
		m_fFOV = ToRadians(vr::VRSettings()->GetFloat("udphmd", "FOV"));
		m_nDistanceBetweenEyes = vr::VRSettings()->GetFloat("udphmd", "DistanceBetweenEyes");
		m_nScreenOffsetX = vr::VRSettings()->GetFloat("udphmd", "ScreenOffsetX");
		m_bDebugMode = vr::VRSettings()->GetBool("udphmd", "DebugMode");
	}

	virtual ~CSampleDeviceDriver()
	{
	}

	virtual EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId)
	{
		m_unObjectId = unObjectId;
		m_ulPropertyContainer = vr::VRProperties()->TrackedDeviceToPropertyContainer(m_unObjectId);

		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_ModelNumber_String, m_sModelNumber.c_str());
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_RenderModelName_String, m_sModelNumber.c_str());

		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_SerialNumber_String, m_sSerialNumber.c_str());
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_ManufacturerName_String, "UDPHMD");
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_TrackingFirmwareVersion_String, "1.0");
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_HardwareRevision_String, "1.0");
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_DeviceIsWireless_Bool, false);
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_DeviceIsCharging_Bool, false);
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_CanUnifyCoordinateSystemWithHmd_Bool, true);
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_ContainsProximitySensor_Bool, false);
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_DeviceCanPowerOff_Bool, false);
		vr::VRProperties()->SetInt32Property(m_ulPropertyContainer, vr::Prop_DeviceClass_Int32, vr::TrackedDeviceClass_HMD);
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_HasCamera_Bool, false);
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_Firmware_ForceUpdateRequired_Bool, false);
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_RegisteredDeviceType_String, "HMD");
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_NeverTracked_Bool, false);
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_Identifiable_Bool, false);

		vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_UserIpdMeters_Float, m_flIPD);
		vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_UserHeadToEyeDepthMeters_Float, 0.f);
		vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_DisplayFrequency_Float, m_flFPS);
		vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_SecondsFromVsyncToPhotons_Float, m_flSecondsFromVsyncToPhotons);

		// return a constant that's not 0 (invalid) or 1 (reserved for Oculus)
		vr::VRProperties()->SetUint64Property(m_ulPropertyContainer, Prop_CurrentUniverseId_Uint64, 2);

		// avoid "not fullscreen" warnings from vrmonitor
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_IsOnDesktop_Bool, false);

		// Debug mode activate Windowed Mode (borderless fullscreen), lock to 30 FPS
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_DisplayDebugMode_Bool, m_bDebugMode);

		// Icons can be configured in code or automatically configured by an external file "drivername\resources\driver.vrresources".
		// Icon properties NOT configured in code (post Activate) are then auto-configured by the optional presence of a driver's "drivername\resources\driver.vrresources".
		// In this manner a driver can configure their icons in a flexible data driven fashion by using an external file.
		//
		// The structure of the driver.vrresources file allows a driver to specialize their icons based on their HW.
		// Keys matching the value in "Prop_ModelNumber_String" are considered first, since the driver may have model specific icons.
		// An absence of a matching "Prop_ModelNumber_String" then considers the ETrackedDeviceClass ("HMD", "Controller", "GenericTracker", "TrackingReference")
		// since the driver may have specialized icons based on those device class names.
		//
		// An absence of either then falls back to the "system.vrresources" where generic device class icons are then supplied.
		//
		// Please refer to "bin\drivers\sample\resources\driver.vrresources" which contains this sample configuration.
		//
		// "Alias" is a reserved key and specifies chaining to another json block.
		//
		// In this sample configuration file (overly complex FOR EXAMPLE PURPOSES ONLY)....
		//
		// "Model-v2.0" chains through the alias to "Model-v1.0" which chains through the alias to "Model-v Defaults".
		//
		// Keys NOT found in "Model-v2.0" would then chase through the "Alias" to be resolved in "Model-v1.0" and either resolve their or continue through the alias.
		// Thus "Prop_NamedIconPathDeviceAlertLow_String" in each model's block represent a specialization specific for that "model".
		// Keys in "Model-v Defaults" are an example of mapping to the same states, and here all map to "Prop_NamedIconPathDeviceOff_String".
		//
		/*bool bSetupIconUsingExternalResourceFile = true;
		if ( !bSetupIconUsingExternalResourceFile )
		{
			// Setup properties directly in code.
			// Path values are of the form {drivername}\icons\some_icon_filename.png
			vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceOff_String, "{sample}/icons/headset_sample_status_off.png" );
			vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceSearching_String, "{sample}/icons/headset_sample_status_searching.gif" );
			vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceSearchingAlert_String, "{sample}/icons/headset_sample_status_searching_alert.gif" );
			vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceReady_String, "{sample}/icons/headset_sample_status_ready.png" );
			vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceReadyAlert_String, "{sample}/icons/headset_sample_status_ready_alert.png" );
			vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceNotReady_String, "{sample}/icons/headset_sample_status_error.png" );
			vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceStandby_String, "{sample}/icons/headset_sample_status_standby.png" );
			vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceAlertLow_String, "{sample}/icons/headset_sample_status_ready_low.png" );
		}*/

		return VRInitError_None;
	}

	virtual void Deactivate()
	{
		m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
	}

	virtual void EnterStandby()
	{
	}

	void *GetComponent(const char *pchComponentNameAndVersion)
	{
		if (!_stricmp(pchComponentNameAndVersion, vr::IVRDisplayComponent_Version))
		{
			return (vr::IVRDisplayComponent *)this;
		}

		// override this to add a component to a driver
		return NULL;
	}

	virtual void PowerOff()
	{
	}

	/** debug request from a client */
	virtual void DebugRequest(const char *pchRequest, char *pchResponseBuffer, uint32_t unResponseBufferSize)
	{
		if (unResponseBufferSize >= 1)
			pchResponseBuffer[0] = 0;
	}

	virtual void GetWindowBounds(int32_t *pnX, int32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight)
	{
		*pnX = m_nWindowX;
		*pnY = m_nWindowY;
		*pnWidth = m_nWindowWidth;
		*pnHeight = m_nWindowHeight;
	}

	virtual bool IsDisplayOnDesktop()
	{
		return true;
	}

	virtual bool IsDisplayRealDisplay()
	{
		if (m_nWindowX == 0 && m_nWindowY == 0)
			return false;
		else
			return true; // Support working on extended display
	}

	virtual void GetRecommendedRenderTargetSize(uint32_t *pnWidth, uint32_t *pnHeight)
	{
		*pnWidth = m_nRenderWidth;
		*pnHeight = m_nRenderHeight;
	}

	virtual void GetEyeOutputViewport(EVREye eEye, uint32_t *pnX, uint32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight)
	{
		*pnY = m_nScreenOffsetX;
		*pnWidth = m_nWindowWidth / 2;
		*pnHeight = m_nWindowHeight;

		if (eEye == Eye_Left)
		{
			*pnX = m_nDistanceBetweenEyes;
		}
		else
		{
			*pnX = (m_nWindowWidth / 2) - m_nDistanceBetweenEyes;
		}
	}

	virtual void GetProjectionRaw(EVREye eEye, float *pfLeft, float *pfRight, float *pfTop, float *pfBottom)
	{
		*pfLeft = -m_fFOV;
		*pfRight = m_fFOV;
		*pfTop = -m_fFOV;
		*pfBottom = m_fFOV;
	}

	virtual DistortionCoordinates_t ComputeDistortion(EVREye eEye, float fU, float fV)
	{
		DistortionCoordinates_t coordinates;

		// Distortion for lens implementation from https://github.com/HelenXR/openvr_survivor/blob/master/src/head_mount_display_device.cc
		float hX;
		float hY;
		double rr;
		double r2;
		double theta;

		rr = sqrt((fU - 0.5f) * (fU - 0.5f) + (fV - 0.5f) * (fV - 0.5f));
		r2 = rr * (1 + m_fDistortionK1 * (rr * rr) + m_fDistortionK2 * (rr * rr * rr * rr));
		theta = atan2(fU - 0.5f, fV - 0.5f);
		hX = sin(theta) * r2 * m_fZoomWidth;
		hY = cos(theta) * r2 * m_fZoomHeight;

		coordinates.rfBlue[0] = hX + 0.5f;
		coordinates.rfBlue[1] = hY + 0.5f;
		coordinates.rfGreen[0] = hX + 0.5f;
		coordinates.rfGreen[1] = hY + 0.5f;
		coordinates.rfRed[0] = hX + 0.5f;
		coordinates.rfRed[1] = hY + 0.5f;

		return coordinates;
	}

	virtual DriverPose_t GetPose()
	{
		DriverPose_t pose = {0};

		if (SocketActivated)
		{
			pose.poseIsValid = true;
			pose.result = TrackingResult_Running_OK;
			pose.deviceIsConnected = true;
		}
		else
		{
			pose.poseIsValid = false;
			pose.result = TrackingResult_Uninitialized;
			pose.deviceIsConnected = false;
		}

		pose.qWorldFromDriverRotation = Get_HmdQuaternion(1, 0, 0, 0);
		pose.qDriverFromHeadRotation = Get_HmdQuaternion(1, 0, 0, 0);
		pose.shouldApplyHeadModel = true;
		pose.poseTimeOffset = 0;

		// Set head tracking rotation
		pose.qRotation = Get_HmdQuaternion(rW, rX, rY, rZ);

		// Set position tracking
		pose.vecPosition[0] = pX * 0.01;
		pose.vecPosition[1] = pZ * 0.01;
		pose.vecPosition[2] = pY * 0.01;

		return pose;
	}

	void RunFrame()
	{
		// In a real driver, this should happen from some pose tracking thread.
		// The RunFrame interval is unspecified and can be very irregular if some other
		// driver blocks it for some periodic task.
		if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid)
		{
			vr::VRServerDriverHost()->TrackedDevicePoseUpdated(m_unObjectId, GetPose(), sizeof(DriverPose_t));
		}
	}

	std::string GetSerialNumber() const { return m_sSerialNumber; }

private:
	vr::TrackedDeviceIndex_t m_unObjectId;
	vr::PropertyContainerHandle_t m_ulPropertyContainer;

	std::string m_sSerialNumber;
	std::string m_sModelNumber;

	int32_t m_nWindowX;
	int32_t m_nWindowY;
	int32_t m_nWindowWidth;
	int32_t m_nWindowHeight;
	int32_t m_nRenderWidth;
	int32_t m_nRenderHeight;
	float m_flSecondsFromVsyncToPhotons;
	float m_flFPS;
	float m_flIPD;

	float m_fDistortionK1;
	float m_fDistortionK2;
	float m_fZoomWidth;
	float m_fZoomHeight;
	float m_fFOV;
	int32_t m_nDistanceBetweenEyes;
	int32_t m_nScreenOffsetX;
	bool m_bDebugMode;
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CServerDriver_Sample : public IServerTrackedDeviceProvider
{
public:
	virtual EVRInitError Init(vr::IVRDriverContext *pDriverContext);
	virtual void Cleanup();
	virtual const char *const *GetInterfaceVersions() { return vr::k_InterfaceVersions; }
	virtual void RunFrame();
	virtual bool ShouldBlockStandbyMode() { return false; }
	virtual void EnterStandby() {}
	virtual void LeaveStandby() {}

private:
	CSampleDeviceDriver *m_pNullHmdLatest = nullptr;
};

CServerDriver_Sample g_serverDriverNull;

EVRInitError CServerDriver_Sample::Init(vr::IVRDriverContext *pDriverContext)
{
	VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
	InitDriverLog(vr::VRDriverLog());

	WSADATA wsaData;
	int iResult;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult == 0)
	{
		struct sockaddr_in local;
		fromlen = sizeof(from);
		local.sin_family = AF_INET;
		int sock_port = vr::VRSettings()->GetInt32("udphmd", "Port");
		DriverLog(("Creating socket on port " + std::to_string(sock_port)).c_str());
		local.sin_port = htons(sock_port);
		local.sin_addr.s_addr = INADDR_ANY;

		socketS = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		u_long nonblocking_enabled = true;
		ioctlsocket(socketS, FIONBIO, &nonblocking_enabled);

		if (socketS != INVALID_SOCKET)
		{
			iResult = bind(socketS, (sockaddr *)&local, sizeof(local));

			if (iResult != SOCKET_ERROR)
			{
				SocketActivated = true;

				socklen_t len = sizeof(local);
				if (getsockname(socketS, (sockaddr *)&local, &len) != 0)
				{
					DriverLog("Cannot call getsockname");
				}
				else
				{
					unsigned short real_port = ntohs(local.sin_port);
					DriverLog(("Socket activated on port " + std::to_string(real_port)).c_str());
				}

				pSocketThread = new std::thread(WinSockReadFunc);
			}
			else
			{
				DriverLog("Cannot create socket: SOCKET_ERROR");
				WSACleanup();
				SocketActivated = false;
			}
		}
		else
		{
			DriverLog("Cannot create socket: INVALID_SOCKET");
			WSACleanup();
			SocketActivated = false;
		}
	}
	else
	{
		DriverLog("Cannot create socket");
		WSACleanup();
		SocketActivated = false;
	}

	m_pNullHmdLatest = new CSampleDeviceDriver();
	vr::VRServerDriverHost()->TrackedDeviceAdded(m_pNullHmdLatest->GetSerialNumber().c_str(), vr::TrackedDeviceClass_HMD, m_pNullHmdLatest);

	return VRInitError_None;
}

void CServerDriver_Sample::Cleanup()
{
	if (SocketActivated)
	{
		SocketActivated = false;
		if (pSocketThread)
		{
			pSocketThread->join();
			delete pSocketThread;
			pSocketThread = nullptr;
		}
		closesocket(socketS);
		WSACleanup();
	}
	CleanupDriverLog();
	delete m_pNullHmdLatest;
	m_pNullHmdLatest = NULL;
}

void CServerDriver_Sample::RunFrame()
{
	if (m_pNullHmdLatest)
	{
		m_pNullHmdLatest->RunFrame();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
HMD_DLL_EXPORT void *HmdDriverFactory(const char *pInterfaceName, int *pReturnCode)
{
	if (0 == strcmp(IServerTrackedDeviceProvider_Version, pInterfaceName))
	{
		return &g_serverDriverNull;
	}

	if (pReturnCode)
		*pReturnCode = VRInitError_Init_InterfaceNotFound;

	return NULL;
}
