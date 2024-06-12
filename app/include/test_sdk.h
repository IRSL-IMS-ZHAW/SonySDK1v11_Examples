#ifndef MY_SONY_SDK_H
#define MY_SONY_SDK_H

/*
#include <CameraRemote_SDK.h>
#include <CrDeviceProperty.h>
#include <CrTypes.h>
#include <CrCommandData.h>
#include <CrError.h>
#include <ICrCameraObjectInfo.h>
#include <CrDefines.h>
#include <CrImageDataBlock.h>
#include <IDeviceCallback.h>
*/

#include <ConnectionInfo.h>
#include <MessageDefine.h>
#include <CameraDevice.h>
#include <PropertyValueTable.h>
#include <Text.h>

#include <iostream>

// function declaration
//bool InitSDK();
//void Terminate();
//void Disconnect(SCRSDK::CrDeviceHandle);
//bool ConnectCamera(SCRSDK::ICrCameraObjectInfo *pcamera);
//SCRSDK::ICrCameraObjectInfo* CreateUSBObject(CrInt8u* cam_id);
//CrInt8u* Enumerate();

class MyDeviceCallback : public SCRSDK::IDeviceCallback{
public:
    void OnConnected(SCRSDK::DeviceConnectionVersioin version) override;
    void OnDisconnected(CrInt32u error) override;
    void OnPropertyChanged() override;
    void OnPropertyChangedCodes(CrInt32u num, CrInt32u *codes) override;
    void OnLvPropertyChanged() override;
    void OnLvPropertyChangedCodes(CrInt32u num, CrInt32u *codes) override;
    void OnCompleteDownload(CrChar *filename, CrInt32u type) override;
    void OnNotifyContentsTransfer(CrInt32u notify, SCRSDK::CrContentHandle handle, CrChar *filename) override;
    void OnWarning(CrInt32u warning) override;
    void OnWarningExt(CrInt32u warning, CrInt32 param1, CrInt32 param2, CrInt32 param3) override;
    void OnError(CrInt32u error) override;
    // additional functions required by IDeviceCallback
};
std::string GetDevPropertyName(CrInt32u code);

/*
class MyDeviceCallback : public SCRSDK::IDeviceCallback {
    void OnConnected(SCRSDK::DeviceConnectionVersioin version) override;
    void OnDisconnected(CrInt32u error) override;
    void OnPropertyChanged() override;
    void OnPropertyChangedCodes(CrInt32u num, CrInt32u* codes) override;
    void OnLvPropertyChanged() override;
    void OnLvPropertyChangedCodes(CrInt32u num, CrInt32u* codes) override;
    void OnCompleteDownload(CrChar* filename, CrInt32u type) override ;
    void OnNotifyContentsTransfer(CrInt32u notify, SCRSDK::CrContentHandle handle, CrChar* filename) override;
    void OnWarning(CrInt32u warning) override ;
    void OnWarningExt(CrInt32u warning, CrInt32 param1, CrInt32 param2, CrInt32 param3) override;
    void OnError(CrInt32u error) override;
};
*/
#endif // MY_SONY_SDK_H
