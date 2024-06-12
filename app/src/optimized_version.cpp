#include <iostream>
#include <memory>
#include "test_sdk.h"
#include <thread>
#include <unistd.h>


CrChar path[45] = "/home/rafael2ms/Dev/SonySDK1v11_Examples/app"; // warnings when declared as CrChar*
CrChar prefix[8] = "ILXLR1_";
CrInt32 startNumber = 1;
CrInt8u *cam_id = 0;
SCRSDK::ICrCameraObjectInfo *pCam = nullptr;
MyDeviceCallback *cb = new MyDeviceCallback();
SCRSDK::CrDeviceHandle hDev = 0;

bool InitSDK();
bool EnumerateCameras();
bool ConnectCamera(SCRSDK::CrSdkControlMode mode);
SCRSDK::CrError setSavePath();

int main()
{
    // Init the SDK
    if (!InitSDK())
    {
        std::cerr << "Cam init failed" << std::endl;
        return 0;
    }

    // Enumurate cameras and get the ID of the first one
    if (!EnumerateCameras())
    {
        std::cerr << "Invalid camera ID provided." << std::endl;
        return 0;
    }

    // mode to connect
    SCRSDK::CrSdkControlMode mode = SCRSDK::CrSdkControlMode_Remote;
    // and we connect
    if(!ConnectCamera(mode))
    {
        std::cerr << "conenction failed." << std::endl;
        return 0;
    }
    
    //save path
    setSavePath();
 



    /*  -------------- Pag 51
        After the Connect() function, ICrCameraObjectInfo can be freed. There is no need to wait for
        OnConnected() or the OnError() callback function. It means you can delete the
        ICrEnumCameraObjectInfo object returned from the EnumCameraObjects() function
    */

    // pEnum->Release(); // After this point, pobj or any data derived from it might be invalid!
    std::cout << "Connecting..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "Done!\n\n"
              << std::endl;

    // std::cin >> s;

    for (int k = 0; k < 1; k++)
    {
        // Get the Camera Properties (page 59)
        SCRSDK::CrDeviceProperty *pProperties = nullptr;
        CrInt32 numofProperties;
        SCRSDK::CrError err_getdevice = SCRSDK::GetDeviceProperties(hDev, &pProperties, &numofProperties);

        if (err_getdevice != SCRSDK::CrError_None)
        {
            std::cout << "Properties are NOT returned successfully." << std::endl;
            break;
        }
        std::cout << "Properties are returned successfully." << std::endl;

        if (pProperties == nullptr)
        {
            break;
        }
        std::cout << "The property list is received successfully." << std::endl;
        std::cout << "There are " << numofProperties << " properties" << std::endl;

        for (CrInt32 n = 0; n < numofProperties; n++)
        {
            std::cout << "Properties[" << n << "] is: " << GetDevPropertyName(pProperties[n].GetCode()) << std::endl;

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
                std::cout << "CrDeviceProperty_FNumber is: " << currentvalue << std::endl;
                std::cout << "Countofelement is: " << countofelement << std::endl;

                void *rawValues = pProperties[n].GetValues();
                if (rawValues == nullptr || countofelement == 0)
                {
                    std::cout << "break" << std::endl;
                    // break;
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

            if (pProperties[n].GetCode() == SCRSDK::CrDeviceProperty_SdkControlMode)
            {
                CrInt16u currentvalue = static_cast<CrInt16u>(pProperties[n].GetCurrentValue());
                CrInt32u countofelement = pProperties[n].GetValueSize() / sizeof(CrInt16u);
                // CrInt16u *poptions = static_cast<CrInt16u*>(pProperties[n].GetValues());
                std::cout << "CrDeviceProperty_SdkControlMode is: " << currentvalue << std::endl;
                std::cout << "Countofelement is: " << countofelement << std::endl;

                void *rawValues = pProperties[n].GetValues();
                if (rawValues == nullptr || countofelement == 0)
                {
                    std::cout << "break" << std::endl;
                    // break;
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

        SCRSDK::CrError err_release = SCRSDK::ReleaseDeviceProperties(hDev, pProperties);
        if (err_release != SCRSDK::CrError_None)
        {
            std::cout << "Properties are NOT released successfully." << std::endl;
            break;
        }
    }

    for (size_t i = 0; i < 1; i++)
    {
        std::cout << "================ GetLiveViewImage ==============" << std::endl;
        SCRSDK::CrImageInfo *pInfo = new SCRSDK::CrImageInfo();
        SCRSDK::GetLiveViewImageInfo(hDev, pInfo);
        SCRSDK::CrImageDataBlock *pLiveViewImage = new SCRSDK::CrImageDataBlock();
        pLiveViewImage->SetSize(pInfo->GetBufferSize());
        CrInt8u *recvBuffer = new CrInt8u[pInfo->GetBufferSize()];
        pLiveViewImage->SetData(recvBuffer);
        SCRSDK::GetLiveViewImage(hDev, pLiveViewImage);

        // GetImageData() returns the data pointer and GetImageSize() returns the data size.
        CrInt32u size = pLiveViewImage->GetImageSize();
        CrInt8u *pdata = pLiveViewImage->GetImageData();

        std::cout << "size: " << size << std::endl;
        // break;
    }

    for (size_t i = 0; i < 1; i++)
    {
        std::cout << "================ Triggering ==============" << std::endl;
        SCRSDK::SendCommand(hDev, SCRSDK::CrCommandId_Release, SCRSDK::CrCommandParam_Down);
        std::cout << "x-- Down --x" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        // sleep(1);
        SCRSDK::SendCommand(hDev, SCRSDK::CrCommandId_Release, SCRSDK::CrCommandParam_Up);
        std::cout << "x-- Up --x" << std::endl;
    }

    // std::this_thread::sleep_for(std::chrono::seconds(10));

    // If the connection is successful and you're done with the camera, clean up:
    // Assuming Disconnect is a method you need to call
    SCRSDK::Disconnect(hDev);
    // delete cb;  // Always delete your dynamically allocated memory4

    pCam->Release(); // Release the camera object info

    SCRSDK::Release(); // Clean up the camera info object

    return 0;
}

bool InitSDK()
{
    SCRSDK::Release();
    if (!SCRSDK::Init(0))
    {
        std::cout << "Sony SDK Not Initialized" << std::endl;
        return false;
    }
    std::cout << "Sony SDK Initialized" << std::endl;
    return true;
}

// this fucntion, 1. enumurate cameras, catch any erros, if no erros and found a camera or more, return the first camera id
bool EnumerateCameras()
{
    std::cout << "Enumerate!" << std::endl;
    SCRSDK::ICrEnumCameraObjectInfo *pEnum = nullptr;
    SCRSDK::CrError err = SCRSDK::EnumCameraObjects(&pEnum);

    if (err != SCRSDK::CrError_None || pEnum == nullptr)
    {
        std::cout << "No cameras found or error occurred." << std::endl;
        return false;
    }

    CrInt32u cntOfCamera = pEnum->GetCount();
    if (cntOfCamera == 0)
    {
        std::cout << "No cameras found." << std::endl;
        pEnum->Release();
        return false;
    }

    const SCRSDK::ICrCameraObjectInfo *pobj = pEnum->GetCameraObjectInfo(0);
    if (pobj == nullptr)
    {
        pEnum->Release();
        return false;
    }

    std::cout << "Camera 1: Model = " << pobj->GetModel()
              << ", ID = " << pobj->GetId()
              << ", GetConnectionTypeName = " << pobj->GetConnectionTypeName()
              << ", GetPairingNecessity = " << pobj->GetPairingNecessity() << std::endl;

    // guarantee that it won't be lost after releasing
    cam_id = pobj->GetId();
    pEnum->Release(); // After this point, pobj or any data derived from it might be invalid!

    return true;
}

//This function creates an usb object (and) connect to the camera using its object.
bool ConnectCamera(SCRSDK::CrSdkControlMode mode)
{
    SCRSDK::CrError err = SCRSDK::CreateCameraObjectInfoUSBConnection(&pCam, SCRSDK::CrCameraDeviceModel_ILX_LR1, cam_id);
    if (err != SCRSDK::CrError_None)
    {
        std::cerr << "Failed to create camera object info, error code: " << err << std::endl;
        return false;
    }
    std::cout << "USB Object Created!" << std::endl;

    if (pCam == nullptr)
    {
        std::cerr << "Failed in addressing camera object, error code: " << err << std::endl;
        return false;
    }
    
    // SCRSDK::CrSdkControlMode_ContentsTransfer or SCRSDK::CrSdkControlMode_Remote
    err = SCRSDK::Connect(pCam, cb, &hDev, mode); 

    if (err != SCRSDK::CrError_None)
    {
        delete cb; // Clean up callback if connection failed
        std::cout << "Failed to connect to the camera, error code: " << err << std::endl;
        return false; // Return an error code to signify failure
    }

    if (hDev == 0)
    {
        delete cb; // Clean up callback if connection failed
        std::cout << "Failed in addressing Handle, error code: " << err << std::endl;
        return false; // Return an error code to signify failure
    }
    return true;
}

SCRSDK::CrError setSavePath()
{
    SCRSDK::CrError err = SCRSDK::SetSaveInfo(hDev, path, prefix, startNumber);
    if (err != SCRSDK::CrError_None)
    {
        std::cerr << "Failed to set save info, error code: " << err << std::endl;
    }
    return err;
}