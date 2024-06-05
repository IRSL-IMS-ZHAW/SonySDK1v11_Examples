#include <iostream>
#include <memory>
#include "test_sdk.h"
#include <thread>

CrChar path[] = "/path/to/images/";
CrChar prefix[] = "ILXLR1_";

int main()
{
    std::cout << "Sony" << std::endl;
    SCRSDK::Init();

    SCRSDK::ICrCameraObjectInfo *pCam = CreateUSBObject();
    std::cout << pCam << std::endl;

    if (pCam == nullptr)
    {
        std::cout << "Failed to create camera object." << std::endl;
        return 0;
    }
    SCRSDK::CrDeviceHandle handle = ConnectCamera(pCam);
    std::cout << handle << std::endl;

    if (handle == 0)
    { // Assuming 0 is an invalid handle
        std::cout << "Failed to connect to the camera." << std::endl;
        return 0;
    }
    std::cout << "Camera connected successfully." << std::endl;

    // Store image folder (page 70)
    SCRSDK::SetSaveInfo(handle, path, prefix, 001);

    // Use the handle for further operations
    int mode = 1;
    switch (mode)
    {
    case 1:
    { // Press the shutter release button for one second
        SCRSDK::SendCommand(handle, SCRSDK::CrCommandId_Release, SCRSDK::CrCommandParam_Down);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        SCRSDK::SendCommand(handle, SCRSDK::CrCommandId_Release, SCRSDK::CrCommandParam_Up);
        break;
    }
    case 2:
    { // Get a Live View Image (page 66)
        SCRSDK::CrImageInfo *pInfo = new SCRSDK::CrImageInfo();
        SCRSDK::GetLiveViewImageInfo(handle, pInfo);
        SCRSDK::CrImageDataBlock *pLiveViewImage = new SCRSDK::CrImageDataBlock();
        pLiveViewImage->SetSize(pInfo->GetBufferSize());
        CrInt8u *recvBuffer = new CrInt8u[pInfo->GetBufferSize()];
        pLiveViewImage->SetData(recvBuffer);
        SCRSDK::GetLiveViewImage(handle, pLiveViewImage);

        // GetImageData() returns the data pointer and GetImageSize() returns the data size.
        CrInt32u size = pLiveViewImage->GetImageSize();
        CrInt8u *pdata = pLiveViewImage->GetImageData();
        break;
    }
    case 3:
    {
        // Get the Camera Properties (page 59)
        SCRSDK::CrDeviceProperty *pProperties = nullptr;
        CrInt32 numofProperties;
        SCRSDK::CrError err_getdevice = SCRSDK::GetDeviceProperties(handle, &pProperties, &numofProperties);

        if (err_getdevice != SCRSDK::CrError_None)
        {
            std::cout << "Properties are NOT returned successfully." << std::endl;
            break;
        }
        std::cout << "Properties are returned successfully." << std::endl;
        if (!pProperties)
        {
            break;
        }
        std::cout << "The property list is received successfully." << std::endl;

        for (CrInt32 n = 0; n < numofProperties; n++)
        {
            // When enableFlag is Disable, Setter/Getter API is invalid (not guaranteed)
            SCRSDK::CrPropertyEnableFlag flag = pProperties[n].GetPropertyEnableFlag();

            SCRSDK::CrDataType type = pProperties[n].GetValueType();
            int dataLen = sizeof(CrInt64u); // Maximum length
            if (type & SCRSDK::CrDataType_UInt8)
            {
                dataLen = sizeof(CrInt8u);
            }
            else if (type & SCRSDK::CrDataType_UInt16)
            {
                dataLen = sizeof(CrInt16u);
            }
            int numofValue = pProperties[n].GetValueSize() / dataLen;

            /* Assuming that we want to get n different properties at once.
            switch (pProperties[n].GetCode()) {
                case SCRSDK::CrDeviceProperty_FNumber:
            */

            // For testing:
            if (pProperties[n].GetCode() == SCRSDK::CrDeviceProperty_FNumber)
            {
                CrInt16u currentvalue = static_cast<CrInt16u>(pProperties[n].GetCurrentValue());
                CrInt32u countofelement = pProperties[n].GetValueSize() / sizeof(CrInt16u);
                // CrInt16u *poptions = static_cast<CrInt16u*>(pProperties[n].GetValues());
                void *rawValues = pProperties[n].GetValues();
                if (!(rawValues != nullptr && countofelement > 0))
                {
                    break;
                }
                CrInt16u *poptions = static_cast<CrInt16u *>(rawValues);
                // if (countofelement) {  // Do I need to check this again?
                CrInt16u *elements = new CrInt16u[countofelement];
                for (CrInt32u n = 0; n < countofelement; n++)
                {
                    elements[n] = poptions[n]; // Originally was *poptions++
                }
                delete[] elements;
            }
        }

        SCRSDK::CrError err_release = SCRSDK::ReleaseDeviceProperties(handle, pProperties);
        if (err_release != SCRSDK::CrError_None)
        {
            std::cout << "Properties are NOT released successfully." << std::endl;
            break;
        }
    }
    case 4:
    {
        /*
        The property code (CrDeviceProperty) is defined in enum CrDevicePropertyCode in CrDeviceProperty.h.
        For example, CrDeviceProperty_FNumber is defined as 0x0100.
        The value type is CrDataType_UInt16.
        The current value is defined as a 64bit variable, but in this case only the highest 16bit is valid.
        */

        // Before call "SetProperty" -> Get the Camera Properties (page 59)
        SCRSDK::CrDeviceProperty prop;
        prop.SetCode(SCRSDK::CrDevicePropertyCode::CrDeviceProperty_FNumber); // Specify the code of CrDataType_STR device property you want to update
        prop.SetValueType(SCRSDK::CrDataType::CrDataType_UInt16Array);        // Specify the type of CrDataType_STR device property you want to update
        // prop.SetCurrentValue((CrInt64u)newValue);

        /*
        CrInt64u currentValue = pProperties[1].GetCurrentValue();
        CrInt16u meaningfulValue = static_cast<CrInt16u>(currentValue >> 48); // Assuming the meaningful part is the highest 16 bits
        CrInt16u newValue = meaningfulValue * 1.1; // Increase by 10%
        CrInt64u newValueToSet = static_cast<CrInt64u>(newValue) << 48; // Place the new value in the correct position

        // If CrDataType Array, only value that exist in the values pointer can be set.
        prop.SetCurrentValue(newValueToSet);
        */

        SCRSDK::CrError err_setdevice = SCRSDK::SetDeviceProperty(handle, &prop);

        if (err_setdevice == SCRSDK::CrError_None)
        {
            std::cout << "Property updated successfully." << std::endl;
        }
        else
        {
            std::cout << "Failed to update property." << std::endl;
        }
    }
    default:
        std::cout << "Looking forward to the Weekend";
        break;
    }

    // Disconnect and clean up before exiting
    SCRSDK::Disconnect(handle);

    // Clean up SDK resources
    SCRSDK::Release();
    return 0;
}
