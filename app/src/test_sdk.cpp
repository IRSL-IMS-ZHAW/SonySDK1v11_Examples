#include "test_sdk.h"

//============================================================
bool Init()
{
    std::cout << "Init" << std::endl;
    bool ret = SCRSDK::Init(0);
    if (!ret)
    {
        std::cout << "Camera not connected." << std::endl;
        // code to handle the error
        return false;
    }
    std::cout << "Camera connected." << std::endl;
    return true;
}
//============================================================
void Terminate()
{
    SCRSDK::Release();
}
//============================================================
void Disconnect(SCRSDK::CrDeviceHandle handle)
{
    SCRSDK::Disconnect(handle);
}
//============================================================
//FROM: class MyDeviceCallback : public SCRSDK::IDeviceCallback

void MyDeviceCallback::OnConnected(SCRSDK::DeviceConnectionVersioin version)
{
    SCRSDK::DeviceConnectionVersioin ver = version;
    std::cout << "Camera connected with version: " << version << std::endl;
    // Program can use the device handle.
}

void MyDeviceCallback::OnDisconnected(CrInt32u error)
{
    // Implementation of OnDisconnected
    std::cout << "Camera disconnected, error code: " << error << std::endl;
}

void MyDeviceCallback::OnPropertyChanged()
{
    // Implementation of OnPropertyChanged
    //std::cout << "A property changed." << std::endl;
}

void MyDeviceCallback::OnPropertyChangedCodes(CrInt32u num, CrInt32u *codes)
{
    // Implementation of OnPropertyChangedCodes
    //std::cout << "Properties changed, number of codes: " << num << std::endl;
    // Optionally, loop through the codes array and print each code
}

void MyDeviceCallback::OnLvPropertyChanged()
{
    // Implementation of OnLvPropertyChanged
    std::cout << "Live view property changed." << std::endl;
}

void MyDeviceCallback::OnLvPropertyChangedCodes(CrInt32u num, CrInt32u *codes)
{
    // Implementation of OnLvPropertyChangedCodes
    std::cout << "Live view properties changed, number of codes: " << num << std::endl;
}

void MyDeviceCallback::OnCompleteDownload(CrChar *filename, CrInt32u type)
{
    // Implementation of OnCompleteDownload
    std::cout << "Download complete, filename: " << filename << ", type: " << type << std::endl;
}

void MyDeviceCallback::OnNotifyContentsTransfer(CrInt32u notify, SCRSDK::CrContentHandle handle, CrChar *filename)
{
    // Implementation of OnNotifyContentsTransfer
    std::cout << "Contents transfer notified, filename: " << filename << std::endl;
}

void MyDeviceCallback::OnWarning(CrInt32u warning)
{
    // Implementation of OnWarning
    std::cout << "Warning received, code: " << warning << std::endl;
}

void MyDeviceCallback::OnWarningExt(CrInt32u warning, CrInt32 param1, CrInt32 param2, CrInt32 param3)
{
    // Implementation of OnWarningExt
    std::cout << "Extended warning received, code: " << warning << std::endl;
}

void MyDeviceCallback::OnError(CrInt32u error)
{
    // Implementation of OnError
    std::cout << "Error occurred, code: " << error << std::endl;
}
// additional functions required by IDeviceCallback

//============================================================
bool ConnectCamera(SCRSDK::ICrCameraObjectInfo *pcamera)
{
    MyDeviceCallback *cb = new MyDeviceCallback();
    SCRSDK::CrDeviceHandle hDev = -1;
    SCRSDK::CrError err = SCRSDK::Connect(pcamera, cb, &hDev); //SCRSDK::CrSdkControlMode_ContentsTransfer

    if (err == SCRSDK::CrError_None)
    {
        //delete cb; // Clean up callback if connection failed
        return hDev; // Return the handle if connection was successful
    }
    else
    {
        delete cb; // Clean up callback if connection failed
        return 0;  // Return an invalid handle to signify failure
    }
}
//============================================================
SCRSDK::ICrCameraObjectInfo *CreateUSBObject(CrInt8u* cam_id)
{
    if (cam_id == nullptr) {
        std::cerr << "Invalid camera ID provided." << std::endl;
        return nullptr;
    }
    //CrChar serialNum[(SCRSDK::USB_SERIAL_LENGTH + 1)] = {0}; // +1 is Null-terminate
    //memcpy(serialNum, L"D518801E84CE", sizeof(serialNum));   // <-- check what is this value for us
    SCRSDK::ICrCameraObjectInfo *pCam = nullptr;
    SCRSDK::CrError err = SCRSDK::CreateCameraObjectInfoUSBConnection(&pCam, SCRSDK::CrCameraDeviceModel_ILX_LR1, cam_id); // ILX-LR1
    if (err == SCRSDK::CrError_None && pCam != nullptr)
    {
        return pCam;
    }
    else
    {
        std::cerr << "Failed to create camera object info, error code: " << err << std::endl;
        return nullptr;
    }
}
//============================================================
CrInt8u* Old_Enumerate() {
    std::cout << "Enumerate!" << std::endl;
    SCRSDK::ICrEnumCameraObjectInfo* pEnum = nullptr;
    SCRSDK::CrError err = SCRSDK::EnumCameraObjects(&pEnum);
    if (err != SCRSDK::CrError_None || pEnum == nullptr) {
        std::cout << "No cameras found or error occurred." << std::endl;
        return nullptr;
    }

    CrInt32u cntOfCamera = pEnum->GetCount();
    if (cntOfCamera == 0) {
        std::cout << "No cameras found." << std::endl;
        pEnum->Release();
        return nullptr;
    }

    // Only processing the first camera for simplicity
    const SCRSDK::ICrCameraObjectInfo* pobj = pEnum->GetCameraObjectInfo(0);
    if (pobj == nullptr) {
        pEnum->Release();
        return nullptr;
    }

    // Print details of the first camera
    std::cout << "Camera 1: Model = " << pobj->GetModel()
              << ", ID = " << pobj->GetId() << std::endl;

    // Store the ID before releasing resources
    CrInt8u* cameraId = pobj->GetId();

    // Release the enumerator object
    pEnum->Release();

    // Return the camera ID
    return cameraId;
}

//namespace SCRSDK {
std::string GetDevPropertyName(CrInt32u code) {
    static std::map<CrInt32u, std::string> propertyNames = {
        { 0, "CrDeviceProperty_Undefined    "},
        { 1, "CrDeviceProperty_S1"},
        { 2, "CrDeviceProperty_AEL"},
        { 3, "CrDeviceProperty_FEL"},
        { 4, "CrDeviceProperty_Reserved1"},
        { 5, "CrDeviceProperty_AWBL"},

        { 256, "CrDeviceProperty_FNumber    "},
        { 257, "CrDeviceProperty_ExposureBiasCompensation"},
        { 258, "CrDeviceProperty_FlashCompensation"},
        { 259, "CrDeviceProperty_ShutterSpeed"},
        { 260, "CrDeviceProperty_IsoSensitivity"},
        { 261, "CrDeviceProperty_ExposureProgramMode"},
        { 262, "CrDeviceProperty_FileType"},
        { 263, "CrDeviceProperty_StillImageQuality"},
        { 264, "CrDeviceProperty_WhiteBalance"},
        { 265, "CrDeviceProperty_FocusMode"},
        { 266, "CrDeviceProperty_MeteringMode"},
        { 267, "CrDeviceProperty_FlashMode"},
        { 268, "CrDeviceProperty_WirelessFlash"},
        { 269, "CrDeviceProperty_RedEyeReduction"},
        { 270, "CrDeviceProperty_DriveMode"},
        { 271, "CrDeviceProperty_DRO"},
        { 272, "CrDeviceProperty_ImageSize"},
        { 273, "CrDeviceProperty_AspectRatio"},
        { 274, "CrDeviceProperty_PictureEffect"},
        { 275, "CrDeviceProperty_FocusArea"},
        { 276, "CrDeviceProperty_reserved4"},
        { 277, "CrDeviceProperty_Colortemp"},
        { 278, "CrDeviceProperty_ColorTuningAB"},
        { 279, "CrDeviceProperty_ColorTuningGM"},
        { 280, "CrDeviceProperty_LiveViewDisplayEffect"},
        { 281, "CrDeviceProperty_StillImageStoreDestination"},
        { 282, "CrDeviceProperty_PriorityKeySettings"},
        { 283, "CrDeviceProperty_AFTrackingSensitivity"},
        { 284, "CrDeviceProperty_reserved6"},
        { 285, "CrDeviceProperty_Focus_Magnifier_Setting"},
        { 286, "CrDeviceProperty_DateTime_Settings"},
        { 287, "CrDeviceProperty_NearFar"},
        { 288, "CrDeviceProperty_reserved7"},
        { 289, "CrDeviceProperty_AF_Area_Position"},
        { 290, "CrDeviceProperty_reserved8"},
        { 291, "CrDeviceProperty_reserved9"},
        { 292, "CrDeviceProperty_Zoom_Scale"},
        { 293, "CrDeviceProperty_Zoom_Setting"},
        { 294, "CrDeviceProperty_Zoom_Operation"},
        { 295, "CrDeviceProperty_Movie_File_Format"},
        { 296, "CrDeviceProperty_Movie_Recording_Setting"},
        { 297, "CrDeviceProperty_Movie_Recording_FrameRateSetting"},
        { 298, "CrDeviceProperty_CompressionFileFormatStill"},
        { 299, "CrDeviceProperty_MediaSLOT1_FileType"},
        { 300, "CrDeviceProperty_MediaSLOT2_FileType"},
        { 301, "CrDeviceProperty_MediaSLOT1_ImageQuality"},
        { 302, "CrDeviceProperty_MediaSLOT2_ImageQuality"},
        { 303, "CrDeviceProperty_MediaSLOT1_ImageSize"},
        { 304, "CrDeviceProperty_MediaSLOT2_ImageSize"},
        { 305, "CrDeviceProperty_RAW_FileCompressionType"},
        { 306, "CrDeviceProperty_MediaSLOT1_RAW_FileCompressionType"},
        { 307, "CrDeviceProperty_MediaSLOT2_RAW_FileCompressionType"},
        { 308, "CrDeviceProperty_ZoomAndFocusPosition_Save"},
        { 309, "CrDeviceProperty_ZoomAndFocusPosition_Load"},
        { 310, "CrDeviceProperty_IrisModeSetting"},
        { 311, "CrDeviceProperty_ShutterModeSetting"},
        { 312, "CrDeviceProperty_GainControlSetting"},
        { 313, "CrDeviceProperty_GainBaseIsoSensitivity"},
        { 314, "CrDeviceProperty_GainBaseSensitivity"},
        { 315, "CrDeviceProperty_ExposureIndex"},
        { 316, "CrDeviceProperty_BaseLookValue"},
        { 317, "CrDeviceProperty_PlaybackMedia"},
        { 318, "CrDeviceProperty_DispModeSetting"},
        { 319, "CrDeviceProperty_DispMode"},
        { 320, "CrDeviceProperty_TouchOperation"},
        { 321, "CrDeviceProperty_SelectFinder"},
        { 322, "CrDeviceProperty_AutoPowerOffTemperature"},
        { 323, "CrDeviceProperty_BodyKeyLock"},
        { 324, "CrDeviceProperty_ImageID_Num_Setting"},
        { 325, "CrDeviceProperty_ImageID_Num"},
        { 326, "CrDeviceProperty_ImageID_String"},
        { 327, "CrDeviceProperty_ExposureCtrlType"},
        { 328, "CrDeviceProperty_MonitorLUTSetting"},
        { 329, "CrDeviceProperty_FocalDistanceInMeter"},
        { 330, "CrDeviceProperty_FocalDistanceInFeet"},
        { 331, "CrDeviceProperty_FocalDistanceUnitSetting"},
        { 332, "CrDeviceProperty_DigitalZoomScale"},
        { 333, "CrDeviceProperty_ZoomDistance"},
        { 334, "CrDeviceProperty_ZoomDistanceUnitSetting"},
        { 335, "CrDeviceProperty_ShutterModeStatus"},
        { 336, "CrDeviceProperty_ShutterSlow"},
        { 337, "CrDeviceProperty_ShutterSlowFrames"},
        { 338, "CrDeviceProperty_Movie_Recording_ResolutionForMain"},
        { 339, "CrDeviceProperty_Movie_Recording_ResolutionForProxy"},
        { 340, "CrDeviceProperty_Movie_Recording_FrameRateProxySetting"},
        { 341, "CrDeviceProperty_BatteryRemainDisplayUnit"},
        { 342, "CrDeviceProperty_PowerSource"},
        { 343, "CrDeviceProperty_MovieShootingMode"},
        { 344, "CrDeviceProperty_MovieShootingModeColorGamut"},
        { 345, "CrDeviceProperty_MovieShootingModeTargetDisplay"},
        { 346, "CrDeviceProperty_DepthOfFieldAdjustmentMode"},
        { 347, "CrDeviceProperty_ColortempStep"},
        { 348, "CrDeviceProperty_WhiteBalanceModeSetting"},
        { 349, "CrDeviceProperty_WhiteBalanceTint"},
        { 350, "CrDeviceProperty_WhiteBalanceTintStep"},
        { 351, "CrDeviceProperty_Focus_Operation"},
        { 352, "CrDeviceProperty_ShutterECSSetting"},
        { 353, "CrDeviceProperty_ShutterECSNumber"},
        { 354, "CrDeviceProperty_ShutterECSNumberStep"},
        { 355, "CrDeviceProperty_ShutterECSFrequency"},
        { 356, "CrDeviceProperty_RecorderControlProxySetting"},
        { 357, "CrDeviceProperty_ButtonAssignmentAssignable1"},
        { 358, "CrDeviceProperty_ButtonAssignmentAssignable2"},
        { 359, "CrDeviceProperty_ButtonAssignmentAssignable3"},
        { 360, "CrDeviceProperty_ButtonAssignmentAssignable4"},
        { 361, "CrDeviceProperty_ButtonAssignmentAssignable5"},
        { 362, "CrDeviceProperty_ButtonAssignmentAssignable6"},
        { 363, "CrDeviceProperty_ButtonAssignmentAssignable7"},
        { 364, "CrDeviceProperty_ButtonAssignmentAssignable8"},
        { 365, "CrDeviceProperty_ButtonAssignmentAssignable9"},
        { 366, "CrDeviceProperty_ButtonAssignmentLensAssignable1"},
        { 367, "CrDeviceProperty_AssignableButton1"},
        { 368, "CrDeviceProperty_AssignableButton2"},
        { 369, "CrDeviceProperty_AssignableButton3"},
        { 370, "CrDeviceProperty_AssignableButton4"},
        { 371, "CrDeviceProperty_AssignableButton5"},
        { 372, "CrDeviceProperty_AssignableButton6"},
        { 373, "CrDeviceProperty_AssignableButton7"},
        { 374, "CrDeviceProperty_AssignableButton8"},
        { 375, "CrDeviceProperty_AssignableButton9"},
        { 376, "CrDeviceProperty_LensAssignableButton1"},
        { 377, "CrDeviceProperty_FocusModeSetting"},
        { 378, "CrDeviceProperty_ShutterAngle"},
        { 379, "CrDeviceProperty_ShutterSetting"},
        { 380, "CrDeviceProperty_ShutterMode"},
        { 381, "CrDeviceProperty_ShutterSpeedValue"},
        { 382, "CrDeviceProperty_NDFilter"},
        { 383, "CrDeviceProperty_NDFilterModeSetting"},
        { 384, "CrDeviceProperty_NDFilterValue"},
        { 385, "CrDeviceProperty_GainUnitSetting"},
        { 386, "CrDeviceProperty_GaindBValue"},
        { 387, "CrDeviceProperty_AWB"},
        { 388, "CrDeviceProperty_SceneFileIndex"},
        { 389, "CrDeviceProperty_MoviePlayButton"},
        { 390, "CrDeviceProperty_MoviePlayPauseButton"},
        { 391, "CrDeviceProperty_MoviePlayStopButton"},
        { 392, "CrDeviceProperty_MovieForwardButton"},
        { 393, "CrDeviceProperty_MovieRewindButton"},
        { 394, "CrDeviceProperty_MovieNextButton"},
        { 395, "CrDeviceProperty_MoviePrevButton"},
        { 396, "CrDeviceProperty_MovieRecReviewButton"},
        { 397, "CrDeviceProperty_SubjectRecognitionAF"},
        { 398, "CrDeviceProperty_AFTransitionSpeed"},
        { 399, "CrDeviceProperty_AFSubjShiftSens"},
        { 400, "CrDeviceProperty_AFAssist"},
        { 401, "CrDeviceProperty_NDFilterSwitchingSetting"},
        { 402, "CrDeviceProperty_FunctionOfRemoteTouchOperation"},
        { 403, "CrDeviceProperty_RemoteTouchOperation"},
        { 404, "CrDeviceProperty_FollowFocusPositionSetting"},
        { 405, "CrDeviceProperty_FocusBracketShotNumber"},
        { 406, "CrDeviceProperty_FocusBracketFocusRange"},
        { 407, "CrDeviceProperty_ExtendedInterfaceMode"},
        { 408, "CrDeviceProperty_SQRecordingFrameRateSetting"},
        { 409, "CrDeviceProperty_SQFrameRate"},
        { 410, "CrDeviceProperty_SQRecordingSetting"},
        { 411, "CrDeviceProperty_AudioRecording"},
        { 412, "CrDeviceProperty_AudioInputMasterLevel"},
        { 413, "CrDeviceProperty_TimeCodePreset"},
        { 414, "CrDeviceProperty_TimeCodeFormat"},
        { 415, "CrDeviceProperty_TimeCodeRun"},
        { 416, "CrDeviceProperty_TimeCodeMake"},
        { 417, "CrDeviceProperty_UserBitPreset"},
        { 418, "CrDeviceProperty_UserBitTimeRec"},
        { 419, "CrDeviceProperty_ImageStabilizationSteadyShot"},
        { 420, "CrDeviceProperty_Movie_ImageStabilizationSteadyShot"},
        { 421, "CrDeviceProperty_SilentMode"},
        { 422, "CrDeviceProperty_SilentModeApertureDriveInAF"},
        { 423, "CrDeviceProperty_SilentModeShutterWhenPowerOff"},
        { 424, "CrDeviceProperty_SilentModeAutoPixelMapping"},
        { 425, "CrDeviceProperty_ShutterType"},
        { 426, "CrDeviceProperty_PictureProfile"},
        { 427, "CrDeviceProperty_PictureProfile_BlackLevel"},
        { 428, "CrDeviceProperty_PictureProfile_Gamma"},
        { 429, "CrDeviceProperty_PictureProfile_BlackGammaRange"},
        { 430, "CrDeviceProperty_PictureProfile_BlackGammaLevel"},
        { 431, "CrDeviceProperty_PictureProfile_KneeMode"},
        { 432, "CrDeviceProperty_PictureProfile_KneeAutoSet_MaxPoint"},
        { 433, "CrDeviceProperty_PictureProfile_KneeAutoSet_Sensitivity"},
        { 434, "CrDeviceProperty_PictureProfile_KneeManualSet_Point"},
        { 435, "CrDeviceProperty_PictureProfile_KneeManualSet_Slope"},
        { 436, "CrDeviceProperty_PictureProfile_ColorMode"},
        { 437, "CrDeviceProperty_PictureProfile_Saturation"},
        { 438, "CrDeviceProperty_PictureProfile_ColorPhase"},
        { 439, "CrDeviceProperty_PictureProfile_ColorDepthRed"},
        { 440, "CrDeviceProperty_PictureProfile_ColorDepthGreen"},
        { 441, "CrDeviceProperty_PictureProfile_ColorDepthBlue"},
        { 442, "CrDeviceProperty_PictureProfile_ColorDepthCyan"},
        { 443, "CrDeviceProperty_PictureProfile_ColorDepthMagenta"},
        { 444, "CrDeviceProperty_PictureProfile_ColorDepthYellow"},
        { 445, "CrDeviceProperty_PictureProfile_DetailLevel"},
        { 446, "CrDeviceProperty_PictureProfile_DetailAdjustMode"},
        { 447, "CrDeviceProperty_PictureProfile_DetailAdjustVHBalance"},
        { 448, "CrDeviceProperty_PictureProfile_DetailAdjustBWBalance"},
        { 449, "CrDeviceProperty_PictureProfile_DetailAdjustLimit"},
        { 450, "CrDeviceProperty_PictureProfile_DetailAdjustCrispening"},
        { 451, "CrDeviceProperty_PictureProfile_DetailAdjustHiLightDetail"},
        { 452, "CrDeviceProperty_PictureProfile_Copy"},
        { 453, "CrDeviceProperty_CreativeLook"},
        { 454, "CrDeviceProperty_CreativeLook_Contrast"},
        { 455, "CrDeviceProperty_CreativeLook_Highlights"},
        { 456, "CrDeviceProperty_CreativeLook_Shadows"},
        { 457, "CrDeviceProperty_CreativeLook_Fade"},
        { 458, "CrDeviceProperty_CreativeLook_Saturation"},
        { 459, "CrDeviceProperty_CreativeLook_Sharpness"},
        { 460, "CrDeviceProperty_CreativeLook_SharpnessRange"},
        { 461, "CrDeviceProperty_CreativeLook_Clarity"},
        { 462, "CrDeviceProperty_CreativeLook_CustomLook"},
        { 463, "CrDeviceProperty_Movie_ProxyFileFormat"},
        { 464, "CrDeviceProperty_ProxyRecordingSetting"},
        { 465, "CrDeviceProperty_FunctionOfTouchOperation"},
        { 466, "CrDeviceProperty_HighResolutionShutterSpeedSetting"},
        { 467, "CrDeviceProperty_DeleteUserBaseLook"},
        { 468, "CrDeviceProperty_SelectUserBaseLookToEdit"},
        { 469, "CrDeviceProperty_SelectUserBaseLookToSetInPPLUT"},
        { 470, "CrDeviceProperty_UserBaseLookInput"},
        { 471, "CrDeviceProperty_UserBaseLookAELevelOffset"},
        { 472, "CrDeviceProperty_BaseISOSwitchEI"},
        { 473, "CrDeviceProperty_FlickerLessShooting"},
        { 474, "CrDeviceProperty_reserved50"},
        { 475, "CrDeviceProperty_PlaybackVolumeSettings"},
        { 476, "CrDeviceProperty_AutoReview"},
        { 477, "CrDeviceProperty_AudioSignals"},
        { 478, "CrDeviceProperty_HDMIResolutionStillPlay"},
        { 479, "CrDeviceProperty_Movie_HDMIOutputRecMedia"},
        { 480, "CrDeviceProperty_Movie_HDMIOutputResolution"},
        { 481, "CrDeviceProperty_Movie_HDMIOutput4KSetting"},
        { 482, "CrDeviceProperty_Movie_HDMIOutputRAW"},
        { 483, "CrDeviceProperty_Movie_HDMIOutputRawSetting"},
        { 484, "CrDeviceProperty_reserved55"},
        { 485, "CrDeviceProperty_Movie_HDMIOutputTimeCode"},
        { 486, "CrDeviceProperty_Movie_HDMIOutputRecControl"},
        { 487, "CrDeviceProperty_reserved56"},
        { 488, "CrDeviceProperty_MonitoringOutputDisplayHDMI"},
        { 489, "CrDeviceProperty_Movie_HDMIOutputAudioCH"},
        { 490, "CrDeviceProperty_Movie_IntervalRec_IntervalTime"},
        { 491, "CrDeviceProperty_Movie_IntervalRec_FrameRateSetting"},
        { 492, "CrDeviceProperty_Movie_IntervalRec_RecordingSetting"},
        { 493, "CrDeviceProperty_EframingScaleAuto"},
        { 494, "CrDeviceProperty_EframingSpeedAuto"},
        { 495, "CrDeviceProperty_EframingModeAuto"},
        { 496, "CrDeviceProperty_EframingRecordingImageCrop"},
        { 497, "CrDeviceProperty_EframingHDMICrop"},
        { 498, "CrDeviceProperty_CameraEframing"},
        { 499, "CrDeviceProperty_USBPowerSupply"},
        { 500, "CrDeviceProperty_LongExposureNR"},
        { 501, "CrDeviceProperty_HighIsoNR"},
        { 502, "CrDeviceProperty_HLGStillImage"},
        { 503, "CrDeviceProperty_ColorSpace"},
        { 504, "CrDeviceProperty_BracketOrder"},
        { 505, "CrDeviceProperty_FocusBracketOrder"},
        { 506, "CrDeviceProperty_FocusBracketExposureLock1stImg"},
        { 507, "CrDeviceProperty_FocusBracketIntervalUntilNextShot"},
        { 508, "CrDeviceProperty_IntervalRec_ShootingStartTime"},
        { 509, "CrDeviceProperty_IntervalRec_ShootingInterval"},
        { 510, "CrDeviceProperty_IntervalRec_ShootIntervalPriority"},
        { 511, "CrDeviceProperty_IntervalRec_NumberOfShots"},
        { 512, "CrDeviceProperty_IntervalRec_AETrackingSensitivity"},
        { 513, "CrDeviceProperty_IntervalRec_ShutterType"},
        { 514, "CrDeviceProperty_reserved60"},
        { 515, "CrDeviceProperty_WindNoiseReduct"},
        { 516, "CrDeviceProperty_RecordingSelfTimer"},
        { 517, "CrDeviceProperty_RecordingSelfTimerCountTime"},
        { 518, "CrDeviceProperty_RecordingSelfTimerContinuous"},
        { 519, "CrDeviceProperty_RecordingSelfTimerStatus"},
        { 520, "CrDeviceProperty_BulbTimerSetting"},
        { 521, "CrDeviceProperty_BulbExposureTimeSetting"},
        { 522, "CrDeviceProperty_AutoSlowShutter"},
        { 523, "CrDeviceProperty_IsoAutoMinShutterSpeedMode"},
        { 524, "CrDeviceProperty_IsoAutoMinShutterSpeedManual"},
        { 525, "CrDeviceProperty_IsoAutoMinShutterSpeedPreset"},
        { 526, "CrDeviceProperty_FocusPositionSetting"},
        { 527, "CrDeviceProperty_SoftSkinEffect"},
        { 528, "CrDeviceProperty_PrioritySetInAF_S"},
        { 529, "CrDeviceProperty_PrioritySetInAF_C"},
        { 530, "CrDeviceProperty_FocusMagnificationTime"},
        { 531, "CrDeviceProperty_SubjectRecognitionInAF"},
        { 532, "CrDeviceProperty_RecognitionTarget"},
        { 533, "CrDeviceProperty_RightLeftEyeSelect"},
        { 534, "CrDeviceProperty_SelectFTPServer"},
        { 535, "CrDeviceProperty_SelectFTPServerID"},
        { 536, "CrDeviceProperty_FTP_Function"},
        { 537, "CrDeviceProperty_FTP_AutoTransfer"},
        { 538, "CrDeviceProperty_FTP_AutoTransferTarget"},
        { 539, "CrDeviceProperty_Movie_FTP_AutoTransferTarget"},
        { 540, "CrDeviceProperty_FTP_TransferTarget"},
        { 541, "CrDeviceProperty_Movie_FTP_TransferTarget"},
        { 542, "CrDeviceProperty_FTP_PowerSave"},
        { 543, "CrDeviceProperty_ButtonAssignmentAssignable10"},
        { 544, "CrDeviceProperty_ButtonAssignmentAssignable11"},
        { 545, "CrDeviceProperty_reserved14"},
        { 546, "CrDeviceProperty_reserved15"},
        { 547, "CrDeviceProperty_reserved16"},
        { 548, "CrDeviceProperty_reserved17"},
        { 549, "CrDeviceProperty_AssignableButton10"},
        { 550, "CrDeviceProperty_AssignableButton11"},
        { 551, "CrDeviceProperty_reserved37"},
        { 552, "CrDeviceProperty_reserved38"},
        { 553, "CrDeviceProperty_reserved39"},
        { 554, "CrDeviceProperty_reserved40"},
        { 555, "CrDeviceProperty_NDFilterUnitSetting"},
        { 556, "CrDeviceProperty_NDFilterOpticalDensityValue"},
        { 557, "CrDeviceProperty_TNumber"},
        { 558, "CrDeviceProperty_IrisDisplayUnit"},
        { 559, "CrDeviceProperty_Movie_ImageStabilizationLevel"},
        { 560, "CrDeviceProperty_ImageStabilizationSteadyShotAdjust"},
        { 561, "CrDeviceProperty_ImageStabilizationSteadyShotFocalLength"},

        { 1280, "CrDeviceProperty_S2"},
        { 1281, "CrDeviceProperty_reserved10"},
        { 1282, "CrDeviceProperty_reserved11"},
        { 1283, "CrDeviceProperty_reserved12"},
        { 1284, "CrDeviceProperty_reserved13"},
        { 1285, "CrDeviceProperty_Interval_Rec_Mode"},
        { 1286, "CrDeviceProperty_Still_Image_Trans_Size"},
        { 1287, "CrDeviceProperty_RAW_J_PC_Save_Image"},
        { 1288, "CrDeviceProperty_LiveView_Image_Quality"},
        { 1289, "CrDeviceProperty_CustomWB_Capture_Standby"},
        { 1290, "CrDeviceProperty_CustomWB_Capture_Standby_Cancel"},
        { 1291, "CrDeviceProperty_CustomWB_Capture"},
        { 1292, "CrDeviceProperty_Remocon_Zoom_Speed_Type"},

        { 1792, "CrDeviceProperty_GetOnly"},
        { 1793, "CrDeviceProperty_SnapshotInfo"},
        { 1794, "CrDeviceProperty_BatteryRemain"},
        { 1795, "CrDeviceProperty_BatteryLevel"},
        { 1796, "CrDeviceProperty_EstimatePictureSize"},
        { 1797, "CrDeviceProperty_RecordingState"},
        { 1798, "CrDeviceProperty_LiveViewStatus"},
        { 1799, "CrDeviceProperty_FocusIndication"},
        { 1800, "CrDeviceProperty_MediaSLOT1_Status"},
        { 1801, "CrDeviceProperty_MediaSLOT1_RemainingNumber"},
        { 1802, "CrDeviceProperty_MediaSLOT1_RemainingTime"},
        { 1803, "CrDeviceProperty_MediaSLOT1_FormatEnableStatus"},
        { 1804, "CrDeviceProperty_reserved20"},
        { 1805, "CrDeviceProperty_MediaSLOT2_Status"},
        { 1806, "CrDeviceProperty_MediaSLOT2_FormatEnableStatus"},
        { 1807, "CrDeviceProperty_MediaSLOT2_RemainingNumber"},
        { 1808, "CrDeviceProperty_MediaSLOT2_RemainingTime"},
        { 1809, "CrDeviceProperty_reserved22"},
        { 1810, "CrDeviceProperty_Media_FormatProgressRate"},
        { 1811, "CrDeviceProperty_FTP_ConnectionStatus"},
        { 1812, "CrDeviceProperty_FTP_ConnectionErrorInfo"},
        { 1813, "CrDeviceProperty_LiveView_Area"},
        { 1814, "CrDeviceProperty_reserved26"},
        { 1815, "CrDeviceProperty_reserved27"},
        { 1816, "CrDeviceProperty_Interval_Rec_Status"},
        { 1817, "CrDeviceProperty_CustomWB_Execution_State"},
        { 1818, "CrDeviceProperty_CustomWB_Capturable_Area"},
        { 1819, "CrDeviceProperty_CustomWB_Capture_Frame_Size"},
        { 1820, "CrDeviceProperty_CustomWB_Capture_Operation"},
        { 1821, "CrDeviceProperty_reserved32"},
        { 1822, "CrDeviceProperty_Zoom_Operation_Status"},
        { 1823, "CrDeviceProperty_Zoom_Bar_Information"},
        { 1824, "CrDeviceProperty_Zoom_Type_Status"},
        { 1825, "CrDeviceProperty_MediaSLOT1_QuickFormatEnableStatus"},
        { 1826, "CrDeviceProperty_MediaSLOT2_QuickFormatEnableStatus"},
        { 1827, "CrDeviceProperty_Cancel_Media_FormatEnableStatus"},
        { 1828, "CrDeviceProperty_Zoom_Speed_Range"},
        { 1829, "CrDeviceProperty_SdkControlMode"},
        { 1830, "CrDeviceProperty_ContentsTransferStatus"},
        { 1831, "CrDeviceProperty_ContentsTransferCancelEnableStatus"},
        { 1832, "CrDeviceProperty_ContentsTransferProgress"},
        { 1833, "CrDeviceProperty_IsoCurrentSensitivity"},
        { 1834, "CrDeviceProperty_CameraSetting_SaveOperationEnableStatus"},
        { 1835, "CrDeviceProperty_CameraSetting_ReadOperationEnableStatus"},
        { 1836, "CrDeviceProperty_CameraSetting_SaveRead_State"},
        { 1837, "CrDeviceProperty_CameraSettingsResetEnableStatus"},
        { 1838, "CrDeviceProperty_APS_C_or_Full_SwitchingSetting"},
        { 1839, "CrDeviceProperty_APS_C_or_Full_SwitchingEnableStatus"},
        { 1840, "CrDeviceProperty_DispModeCandidate"},
        { 1841, "CrDeviceProperty_ShutterSpeedCurrentValue"},
        { 1842, "CrDeviceProperty_Focus_Speed_Range"},
        { 1843, "CrDeviceProperty_NDFilterMode"},
        { 1844, "CrDeviceProperty_MoviePlayingSpeed"},
        { 1845, "CrDeviceProperty_MediaSLOT1Player"},
        { 1846, "CrDeviceProperty_MediaSLOT2Player"},
        { 1847, "CrDeviceProperty_BatteryRemainingInMinutes"},
        { 1848, "CrDeviceProperty_BatteryRemainingInVoltage"},
        { 1849, "CrDeviceProperty_DCVoltage"},
        { 1850, "CrDeviceProperty_MoviePlayingState"},
        { 1851, "CrDeviceProperty_FocusTouchSpotStatus"},
        { 1852, "CrDeviceProperty_FocusTrackingStatus"},
        { 1853, "CrDeviceProperty_DepthOfFieldAdjustmentInterlockingMode"},
        { 1854, "CrDeviceProperty_RecorderClipName"},
        { 1855, "CrDeviceProperty_RecorderControlMainSetting"},
        { 1856, "CrDeviceProperty_RecorderStartMain"},
        { 1857, "CrDeviceProperty_RecorderStartProxy"},
        { 1858, "CrDeviceProperty_RecorderMainStatus"},
        { 1859, "CrDeviceProperty_RecorderProxyStatus"},
        { 1860, "CrDeviceProperty_RecorderExtRawStatus"},
        { 1861, "CrDeviceProperty_RecorderSaveDestination"},
        { 1862, "CrDeviceProperty_AssignableButtonIndicator1"},
        { 1863, "CrDeviceProperty_AssignableButtonIndicator2"},
        { 1864, "CrDeviceProperty_AssignableButtonIndicator3"},
        { 1865, "CrDeviceProperty_AssignableButtonIndicator4"},
        { 1866, "CrDeviceProperty_AssignableButtonIndicator5"},
        { 1867, "CrDeviceProperty_AssignableButtonIndicator6"},
        { 1868, "CrDeviceProperty_AssignableButtonIndicator7"},
        { 1869, "CrDeviceProperty_AssignableButtonIndicator8"},
        { 1870, "CrDeviceProperty_AssignableButtonIndicator9"},
        { 1871, "CrDeviceProperty_LensAssignableButtonIndicator1"},
        { 1872, "CrDeviceProperty_GaindBCurrentValue"},
        { 1873, "CrDeviceProperty_SoftwareVersion"},
        { 1874, "CrDeviceProperty_CurrentSceneFileEdited"},
        { 1875, "CrDeviceProperty_MovieRecButtonToggleEnableStatus"},
        { 1876, "CrDeviceProperty_RemoteTouchOperationEnableStatus"},
        { 1877, "CrDeviceProperty_CancelRemoteTouchOperationEnableStatus"},
        { 1878, "CrDeviceProperty_LensInformationEnableStatus"},
        { 1879, "CrDeviceProperty_FollowFocusPositionCurrentValue"},
        { 1880, "CrDeviceProperty_FocusBracketShootingStatus"},
        { 1881, "CrDeviceProperty_PixelMappingEnableStatus"},
        { 1882, "CrDeviceProperty_TimeCodePresetResetEnableStatus"},
        { 1883, "CrDeviceProperty_UserBitPresetResetEnableStatus"},
        { 1884, "CrDeviceProperty_SensorCleaningEnableStatus"},
        { 1885, "CrDeviceProperty_PictureProfileResetEnableStatus"},
        { 1886, "CrDeviceProperty_CreativeLookResetEnableStatus"},
        { 1887, "CrDeviceProperty_LensVersionNumber"},
        { 1888, "CrDeviceProperty_DeviceOverheatingState"},
        { 1889, "CrDeviceProperty_Movie_IntervalRec_CountDownIntervalTime"},
        { 1890, "CrDeviceProperty_Movie_IntervalRec_RecordingDuration"},
        { 1891, "CrDeviceProperty_HighResolutionShutterSpeed"},
        { 1892, "CrDeviceProperty_BaseLookImportOperationEnableStatus"},
        { 1893, "CrDeviceProperty_LensModelName"},
        { 1894, "CrDeviceProperty_FocusPositionCurrentValue"},
        { 1895, "CrDeviceProperty_FocusDrivingStatus"},
        { 1896, "CrDeviceProperty_FlickerScanStatus"},
        { 1897, "CrDeviceProperty_FlickerScanEnableStatus"},
        { 1898, "CrDeviceProperty_FTPServerSettingVersion"},
        { 1899, "CrDeviceProperty_FTPServerSettingOperationEnableStatus"},
        { 1900, "CrDeviceProperty_FTPTransferSetting_SaveOperationEnableStatus"},
        { 1901, "CrDeviceProperty_FTPTransferSetting_ReadOperationEnableStatus"},
        { 1902, "CrDeviceProperty_FTPTransferSetting_SaveRead_State"},
        { 1903, "CrDeviceProperty_FTPJobListDataVersion"},
        { 1904, "CrDeviceProperty_CameraShakeStatus"},
        { 1905, "CrDeviceProperty_UpdateBodyStatus"},
        { 1906, "CrDeviceProperty_reserved35"},
        { 1907, "CrDeviceProperty_MediaSLOT1_WritingState"},
        { 1908, "CrDeviceProperty_MediaSLOT2_WritingState"},
        { 1909, "CrDeviceProperty_reserved36"},
        { 1910, "CrDeviceProperty_MediaSLOT1_RecordingAvailableType"},
        { 1911, "CrDeviceProperty_MediaSLOT2_RecordingAvailableType"},
        { 1912, "CrDeviceProperty_MediaSLOT3_RecordingAvailableType"},
        { 1913, "CrDeviceProperty_CameraOperatingMode"},
        { 1914, "CrDeviceProperty_PlaybackViewMode"},
        { 1915, "CrDeviceProperty_AssignableButtonIndicator10"},
        { 1916, "CrDeviceProperty_AssignableButtonIndicator11"},
        { 1917, "CrDeviceProperty_reserved41"},
        { 1918, "CrDeviceProperty_reserved42"},
        { 1919, "CrDeviceProperty_reserved43"},
        { 1920, "CrDeviceProperty_reserved44"},
        { 1921, "CrDeviceProperty_MediaSLOT3_Status"},
        { 1922, "CrDeviceProperty_reserved45"},
        { 1923, "CrDeviceProperty_MediaSLOT3_RemainingTime"},
        { 1924, "CrDeviceProperty_reserved46"},
        { 1925, "CrDeviceProperty_reserved47"},
        { 1926, "CrDeviceProperty_MonitoringDeliveringStatus"},
        { 1927, "CrDeviceProperty_MonitoringIsDelivering"},
        { 1928, "CrDeviceProperty_MonitoringSettingVersion"},
        { 1929, "CrDeviceProperty_MonitoringDeliveryTypeSupportInfo"},
        
        { 4096, "CrDeviceProperty_MaxVal "},

    };

    auto it = propertyNames.find(code);
    if (it != propertyNames.end()) {
        return it->second;
    }
    return "Unknown Property"; // Return this if the code is not found
}
