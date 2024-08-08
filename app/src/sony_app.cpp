/*
 * File: sony_app.cpp
 * Author: Rafael Marques
 * Last Updated: August 8, 2024
 * Description: This app utilizes Sony SDK features to implement useful functions such as "LiveView", "Sequential Shooting", and "Streaming". 
 *              For more information, visit the "read me" file under the ./app/src directory.
 * Version: 1.0
 */

#include <iostream>
#include <memory>
#include "sony_app_lib.h"
#include <thread>
#include <unistd.h>

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <gst/app/gstappsrc.h> // Include for appsrc functions
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <fstream>
#include <termios.h>


class RemoteSDKController
{
private:
    SCRSDK::ICrEnumCameraObjectInfo *pEnum;
    CrInt8u *cam_id;

public:
    RemoteSDKController() : pEnum(nullptr), cam_id(0) {
    }
    SCRSDK::ICrEnumCameraObjectInfo* getEnumCameraObjectInfo() const {
        return pEnum;
    }

    static void init()
    {
        SCRSDK::Release();
        std::cout << "Sony" << std::endl;
        SCRSDK::Init();
    }

    bool enumerate()
    {
        std::cout << "Enumerate!" << std::endl;
        //pEnum = nullptr;
        SCRSDK::CrError err = SCRSDK::EnumCameraObjects(&pEnum);
        if (err != SCRSDK::CrError_None || pEnum == nullptr)
        {
            std::cout << "No cameras found or error occurred." << std::endl;
            return 0;
        }

        CrInt32u cntOfCamera = pEnum->GetCount();
        if (cntOfCamera == 0)
        {
            std::cout << "No cameras found." << std::endl;
            pEnum->Release();
            return 0;
        }

        const SCRSDK::ICrCameraObjectInfo *pobj = pEnum->GetCameraObjectInfo(0);
        if (pobj == nullptr)
        {
            pEnum->Release();
            return 0;
        }

        std::cout << "Camera 1: Model = " << pobj->GetModel()
                  << ", ID = " << pobj->GetId()
                  << ", GetConnectionTypeName = " << pobj->GetConnectionTypeName() << std::endl;

        // guarantee that it won't be lost after releasing
        cam_id = pobj->GetId();
        
        return 1;
    }

    CrInt8u* getCameraId() const {
        return cam_id;
    }

    void releaseEnum(){
        pEnum->Release(); // After this point, pobj or any data derived from it might be invalid!
    }

    void releaseSDK(){
        SCRSDK::Release();
    }
};

class CameraGstream{
private:
    SCRSDK::CrDeviceHandle hDev;
    
    GstElement *appsrc;
    GstElement *pipeline, *source, *decoder, *sink;
    GMainLoop *main_loop;
    guint length;
    gpointer user_data;
    //auto pInfo = std::make_unique<SCRSDK::CrImageInfo>();

public:
    
    CameraGstream() : main_loop(nullptr), hDev(0), pipeline(nullptr), source(nullptr), decoder(nullptr), sink(nullptr) {
        printf("CameraGstream constructed: %p\n", this);
    }

    ~CameraGstream() {
        printf("CameraGstream destructed: %p\n", this);
    }

    void SetDeviceHandle(SCRSDK::CrDeviceHandle newHandle) {
        //std::lock_guard<std::mutex> lock(mtx);
        hDev = newHandle;
    }

    SCRSDK::CrDeviceHandle GetDeviceHandle() {
        //std::lock_guard<std::mutex> lock(mtx);
        return hDev;
    }

    static gboolean timeout(GstRTSPServer *server)
    {
        GstRTSPSessionPool *pool = gst_rtsp_server_get_session_pool(server);
        gst_rtsp_session_pool_cleanup(pool);
        g_object_unref(pool);
        return TRUE;
    }
    struct CallbackData {
        CameraGstream* self;
        SCRSDK::CrDeviceHandle hDev;
    };

    static void media_configure_callback(GstRTSPMediaFactory *factory, GstRTSPMedia *media, gpointer user_data)
    {
        printf("========================\n");
        printf("media_configure_callback\n");
        
        CameraGstream *self = static_cast<CameraGstream*>(user_data);  // Cast user_data back to CameraGstream instance

        GstElement *element = gst_rtsp_media_get_element(media);
        self->appsrc = gst_bin_get_by_name_recurse_up(GST_BIN(element), "mysrc");

        if (!self->appsrc)
        {
            std::cerr << "Failed to retrieve 'appsrc' element.\n";
            return;
        }

        printf("[media_configure_callback] GetDeviceHandle = %ld\n", self->GetDeviceHandle());
        printf("[media_configure_callback] Instance address = %p\n", (void*)self);

        CallbackData* callbackData = new CallbackData{self, self->hDev};

        g_signal_connect(self->appsrc, "need-data", G_CALLBACK(need_data), callbackData);

        //appsrc_ready = true;

        gst_object_unref(element); // Unref the element, not appsrc

        printf("Exiting media_configure_callback\n");
        printf("========================\n");
    }

    void InitGstream(bool rtsp)
    {
        printf("InitGstream\n");
        //hDev = ext_hDev;

        if (hDev == 0) {
            printf("[InitGstream] Error: hDev\n");
            return;
        }
        printf("hDev = %ld\n", hDev);
        printf("GetDeviceHandle = %ld\n", GetDeviceHandle());
        
        
        gst_init(nullptr, nullptr);

        if (rtsp)
        {
            // Create the main pipeline
            pipeline = gst_pipeline_new("rtsp-pipeline");

            main_loop = g_main_loop_new(nullptr, FALSE);
            GstRTSPServer *server = gst_rtsp_server_new();
            gst_rtsp_server_set_service(server, "8554");

            GstRTSPMountPoints *mount_points = gst_rtsp_server_get_mount_points(server);
            GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();

            // RTSP initialization remains mostly the same but correct the caps in the launch string:
            gst_rtsp_media_factory_set_launch(factory,
                                            "( appsrc name=mysrc is-live=true block=true format=TIME "
                                            "caps=image/jpeg,width=640,height=480 ! "
                                            "jpegparse ! rtpjpegpay name=pay0 pt=96 )");

            gst_rtsp_media_factory_set_shared(factory, TRUE);
            gst_rtsp_mount_points_add_factory(mount_points, "/test", factory);
            g_object_unref(mount_points);

            g_signal_connect(factory, "media-configure", G_CALLBACK(media_configure_callback), this);
            printf("GetDeviceHandle = %ld\n", GetDeviceHandle());
            gst_rtsp_server_attach(server, nullptr);

            //g_timeout_add_seconds(2, (GSourceFunc)timeout, server);

            g_print("Stream ready at rtsp://127.0.0.1:8554/test\n");
        }
        else
        {
            pipeline = gst_pipeline_new("video-pipeline");
            source = gst_element_factory_make("appsrc", "jpeg-source");
            decoder = gst_element_factory_make("jpegdec", "jpeg-decoder");
            sink = gst_element_factory_make("autovideosink", "video-output");

            if (!pipeline || !source || !decoder || !sink)
            {
                std::cerr << "GStreamer elements could not be created." << std::endl;
                exit(1);
            }

            gst_bin_add_many(GST_BIN(pipeline), source, decoder, sink, NULL);
            gst_element_link_many(source, decoder, sink, NULL);

            // Set up the appsrc
            g_object_set(source, "format", GST_FORMAT_TIME, NULL);
            GstCaps *caps = gst_caps_new_simple("image/jpeg", "width", G_TYPE_INT, 1920, "height", G_TYPE_INT, 1080, NULL);
            gst_app_src_set_caps(GST_APP_SRC(source), caps);
            gst_caps_unref(caps);

            gst_element_set_state(pipeline, GST_STATE_PLAYING);
        }

        printf("Quiting InitGstream\n");
    }


    int pushToGstream() //
    {   
        //printf("[pushToGstream]\n");
        int retFlag = 0;
        std::unique_ptr<SCRSDK::CrImageInfo> pInfo(new SCRSDK::CrImageInfo());

        //printf("hDev = %ld\n", hDev);
        //printf("GetDeviceHandle = %ld\n", GetDeviceHandle());

        SCRSDK::CrError err1 = SCRSDK::GetLiveViewImageInfo(hDev, pInfo.get());
        //printf("A\n");
        if (err1 != SCRSDK::CrError_None)
        {
            printf("Error 1: %d \n", err1);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            {
                retFlag = 1;
                return 1;
            };
        }
        std::unique_ptr<SCRSDK::CrImageDataBlock> pLiveViewImage(new SCRSDK::CrImageDataBlock());
        pLiveViewImage->SetSize(pInfo->GetBufferSize());
        std::unique_ptr<CrInt8u[]> recvBuffer(new CrInt8u[pInfo->GetBufferSize()]);
        pLiveViewImage->SetData(recvBuffer.get());
        SCRSDK::CrError err2 = SCRSDK::GetLiveViewImage(hDev, pLiveViewImage.get());
        
        if (err2 != SCRSDK::CrError_None)
        {
            printf("Error 2: %d \n", err2);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            {
                retFlag = 2;
                return 2;
            };
        }
        // ============================================================
        //  void pushFrameToGstreamer(SCRSDK::CrImageDataBlock* pLiveViewImage) {
        // ============================================================

        /* THIS COMES FROM THE FAST VERSION */
        if (!GST_IS_ELEMENT(source))
        {
            std::cerr << "Invalid source element." << std::endl;
            {
                retFlag = 3;
                return 3;
            };
        }

        // Function to push JPEG data into GStreamer pipeline
        GstBuffer *buffer = gst_buffer_new_allocate(NULL, pLiveViewImage->GetImageSize(), NULL);
        gst_buffer_fill(buffer, 0, pLiveViewImage->GetImageData(), pLiveViewImage->GetImageSize());

        // THIS COMES FROM THE SLOW  VERSION
        // Setting PTS and DURATION for proper playback
        static guint64 pts = 0;
        GST_BUFFER_PTS(buffer) = pts;
        GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 30); // 30fps
        pts += GST_BUFFER_DURATION(buffer);

        GstFlowReturn ret;
        g_signal_emit_by_name(source, "push-buffer", buffer, &ret);
        gst_buffer_unref(buffer);

        if (ret != GST_FLOW_OK)
        {
            std::cerr << "Error pushing buffer to GStreamer pipeline." << std::endl;
        }
        return 0;
    }

    static void need_data(gpointer user_data)
    {
        CallbackData* data = static_cast<CallbackData*>(user_data);
        if (!data || !data->self) {
            std::cerr << "Invalid data passed to need_data\n";
            return;
        }

        printf("[need_data] hDev = %ld\n", (long) data->hDev);
        
        // Now call pushToGstream on the instance of CameraGstream, not statically
        data->self->pushToGstream();
        // Consider handling the deletion of data here if it's the last use
    }

    void DisconnectGstream(){
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(pipeline));
    }
    
    void InitializeMainLoop() {
        if (main_loop == nullptr) {
            printf("InitializeMainLoop\n");
            main_loop = g_main_loop_new(nullptr, FALSE);
        }
    }

    void LoopRun(){
        if (main_loop == nullptr) {
            printf("[LoopRun] Error!\n");
            return;
        }
        g_main_loop_run(main_loop);
    }

    void LoopQuit(){
        if (main_loop == nullptr) {
            return;
        }
        g_main_loop_quit(main_loop);
        g_main_loop_unref(main_loop);
        main_loop = nullptr;
    }
};

class Camera{
private:
    SCRSDK::ICrCameraObjectInfo *pCam;
    SCRSDK::CrDeviceHandle hDev;
    CameraGstream gstream;
public:
    // Constructor
    Camera() : pCam(nullptr), hDev(0) {
    }

    SCRSDK::ICrCameraObjectInfo* getCameraObjectInfo() const {
        return pCam;
    }

    void releasePcam(){
        pCam->Release(); // After this point, pobj or any data derived from it might be invalid!
    }

    void disconnect(){
        SCRSDK::Disconnect(hDev);
    }

    bool ConnectCamera(SCRSDK::CrSdkControlMode mode, RemoteSDKController& controller)
    {
        CrInt8u* cam_id = controller.getCameraId();
        if (cam_id == nullptr) {
            return false;
        }
        pCam = nullptr;
        SCRSDK::CrError err = SCRSDK::CreateCameraObjectInfoUSBConnection(&pCam, SCRSDK::CrCameraDeviceModel_ILX_LR1, cam_id);
        if (err != SCRSDK::CrError_None || pCam == nullptr)
        {
            std::cerr << "Failed to create camera object info, error code: " << err << std::endl;
            return false;
        }
        std::cout << "USB Object Created!" << std::endl;

        MyDeviceCallback *cb = new MyDeviceCallback();
        hDev = 0;

        // SCRSDK::CrSdkControlMode_Remote or SCRSDK::CrSdkControlMode_ContentsTransfer
        err = SCRSDK::Connect(pCam, cb, &hDev, mode);
        if (err != SCRSDK::CrError_None || hDev == 0)
        {
            delete cb; // Clean up callback if connection failed
            std::cout << "Error: " << err << std::endl;
            return false; // Return an error code to signify failure
        }
        return true;
    }

    bool setSavePath()
    {
        CrChar path[54] = "/home/rafael2ms/Dev/SonySDK1v11_Examples/app/pictures"; // warnings when declared as CrChar*
        CrChar prefix[8] = "ILXLR1_";
        CrInt32 startNumber = 1;

        SCRSDK::CrError err = SCRSDK::SetSaveInfo(hDev, path, prefix, startNumber);
        if (err != SCRSDK::CrError_None)
        {
            std::cerr << "Failed to set save info, error code: " << err << std::endl;
            return false;
        }
        return true;
    }

    void SingleShot()
    {
        std::cout << "============== Triggering ==============" << std::endl;
        SCRSDK::SendCommand(hDev, SCRSDK::CrCommandId_Release, SCRSDK::CrCommandParam_Down);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        SCRSDK::SendCommand(hDev, SCRSDK::CrCommandId_Release, SCRSDK::CrCommandParam_Up);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }


    void GetCamProperties()
    {
        // Get the Camera Properties (page 59)
        SCRSDK::CrDeviceProperty *pProperties = nullptr;
        CrInt32 numofProperties;
        SCRSDK::CrError err_getdevice = SCRSDK::GetDeviceProperties(hDev, &pProperties, &numofProperties);

        if (err_getdevice != SCRSDK::CrError_None)
        {
            std::cout << "Properties are NOT returned successfully." << std::endl;
            return;
        }
        std::cout << "Properties are returned successfully." << std::endl;

        if (pProperties == nullptr)
        {
            return;
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
            return;
        }
        return;
    }


    void debug_streamSetHande(SCRSDK::CrDeviceHandle ext_hDev){
        gstream.SetDeviceHandle(ext_hDev);
    }

    SCRSDK::CrDeviceHandle debug_streamGetHande(){
        return gstream.GetDeviceHandle();
    }

    void streamInit(bool rtsp){
        gstream.SetDeviceHandle(hDev);
        gstream.InitGstream(rtsp);
    }   

    void streamInitializeMainLoop(){
        gstream.InitializeMainLoop();
    }

    void streamLoopRun(){
        printf("streamLoopRun\n");
        gstream.LoopRun();
        //g_main_loop_run(main_loop);
    }

    void streamLoopQuit(){
        gstream.LoopQuit();
        //g_main_loop_quit(main_loop);
        printf("streamLoopQuit\n");
    }

    void streamPush(int &retFlag){
        //printf("streamPush\n");
        gstream.pushToGstream();
    }

    void streamDisconnect(){
        gstream.DisconnectGstream();
        //gst_element_set_state(pipeline, GST_STATE_NULL);
        //gst_object_unref(GST_OBJECT(pipeline));
    }
};


// From GStream
void toggle_flags(int &count);
void checkForKeyPress();
void liveViewRoutine(Camera sonyCamera, bool rtsp); //CrInt32u handle, 

// Atomic flag to signal all threads to stop
std::atomic<bool> quit(false);
std::atomic<bool> program(true);
std::atomic<bool> shoot(false);
std::atomic<bool> stream(false);

std::condition_variable cv;

void SequentialShootingRoutine(Camera sonyCamera, int numOfPictures, int intervalPerPicture_sec);

int main(int argc, char *argv[])
{
    Camera sonyCamera;
    RemoteSDKController remoteController;
    
    remoteController.init();

    if (!remoteController.enumerate())
    {
        std::cerr << "Invalid camera ID provided." << std::endl;
        return 0;
    }

    
    SCRSDK::CrSdkControlMode mode = SCRSDK::CrSdkControlMode_Remote;
    if (!sonyCamera.ConnectCamera(mode, remoteController))
    {
        std::cerr << "Conenction failed." << std::endl;
        return 0;
    }

    if (!sonyCamera.setSavePath())
    {
        std::cerr << "Failed Path Definition." << std::endl;
        return 0;
    }

    /*  -------------- Pag 51
        After the Connect() function, ICrCameraObjectInfo can be freed. There is no need to wait for
        OnConnected() or the OnError() callback function. It means you can delete the
        ICrEnumCameraObjectInfo object returned from the EnumCameraObjects() function
    */

    //sonyCamera.releasePcam();
    remoteController.releaseEnum();

    std::cout << "Connecting..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "Done!\n\n"
              << std::endl;

    int count = 0;
    bool rtsp = false;
    int numOfPictures = 4;
    int intervalPerPicture_sec = 3;
    // SequentialShootingRoutine(numOfPictures, intervalPerPicture_sec);

    /*------------------ Noteworthy
    You need to ensure that the need_data function only tries to access appsrc after
    it has been initialized, which only happens after a client has connected to the RTSP server.
    */
    std::thread keyPressThread([&]()
                               { checkForKeyPress(); });

    std::thread gstream_thread([&]()
                               { liveViewRoutine(sonyCamera, rtsp); }); // argc, argv

    std::thread shooting_thread([&]()
                                { SequentialShootingRoutine(sonyCamera, numOfPictures, intervalPerPicture_sec); });

    printf("\n* Press 'T' to start the 'SequentialShootingRoutine' with %d shoots in intervals of %d seconds\n", numOfPictures, intervalPerPicture_sec);
    printf("* Or press 'Q' twice to stop the code properly\n\n");

    /*
    std::thread streaming_thread([&]() {
        need_data();
    });
    */

    while (program.load())
    {
        toggle_flags(count);
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Reduce CPU usage
        // printf("Shoot: %d \t Pressed: %d \t Stream: %d \n", shoot.load(), pressed.load(), stream.load());
    }
    std::cout << "Quiting program..." << std::endl;
    // Signal threads to finish
    shoot.store(false);
    stream.store(false);

    // Join threads
    if (keyPressThread.joinable())
        keyPressThread.join();
    printf(" > keyPressThread ended\n");

    if (gstream_thread.joinable())
        gstream_thread.join();
    printf(" > gstream_thread ended\n");

    if (shooting_thread.joinable())
        shooting_thread.join();
    printf(" > shooting_thread ended\n");
    cv.notify_all(); // Notify in case waiting threads are still blocked

    // if (streaming_thread.joinable()) streaming_thread.join();
    // printf(" > streaming_thread ended\n");

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // If the connection is successful and you're done with the camera, clean up:
    // Assuming Disconnect is a method you need to call
    
    //SCRSDK::Disconnect(hDev);
    sonyCamera.disconnect();
    //SCRSDK::Release(); // Clean up the camera info object
    remoteController.releaseSDK();

    std::cout << "Quit Successfully!" << std::endl;
    return 0;
}

// Function that executes a sequence of shots based on the input parameters. It does not belong to any class.
void SequentialShootingRoutine(Camera sonyCamera, int numOfPictures, int intervalPerPicture_sec)
{
    while (program.load())
    { // Keep thread alive as long as the program is running
        if (!shoot.load())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Sleep briefly before checking again
            continue;
        }

        // ==========================================================
        //  Should this be an idependent function? Threading issue
        // ==========================================================
        for (size_t i = 0; i < numOfPictures; i++)
        {
            sonyCamera.SingleShot();
            printf(" > Shot %ld of %d \n", i + 1, numOfPictures);
            std::this_thread::sleep_for(std::chrono::seconds(intervalPerPicture_sec));
            if (!program.load())
                break;
        }
        // ==========================================================
        
        printf("\n* Press 'T' to start the 'SequentialShootingRoutine' with %d shoots in intervals of %d seconds\n", numOfPictures, intervalPerPicture_sec);
        printf("* Or press 'Q' twice to stop the code properly\n\n");
        shoot.store(false);
    }
    printf("Quiting SequentialShootingRoutine\n");
}

// Main function for fetching and displaying live view images. It does not belong to any class.
void liveViewRoutine(Camera sonyCamera, bool rtsp) //CrInt32u handle
{
    // Assume handle and SDK setup done here
    printf("liveView: In!\n");

    sonyCamera.streamInit(rtsp);
    //InitGstream(rtsp);

    //printf("[MAIN] [1] GetDeviceHandle = %ld\n", sonyCamera.debug_streamGetHande());

    std::thread gstreamLoopThread([&, rtsp]()
                                  {
        if (rtsp){
            printf("-----------------------------------\n");
            sonyCamera.streamInitializeMainLoop();
            printf("-----------------------------------\n");
            sonyCamera.streamLoopRun();
            printf("-----------------------------------\n");
            //g_main_loop_run(main_loop);
        } });

    printf("liveView: Init done!\n");

    while (!stream.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    printf("liveView: Started!\n\n");

    //printf("[MAIN] [2] GetDeviceHandle = %ld\n", sonyCamera.debug_streamGetHande());

    /*
    > If the this is declared outside the while, the error 13105 arises
    > CrWarning_Frame_NotUpdated
    auto pLiveViewImage = std::make_unique<SCRSDK::CrImageDataBlock>();
    */

    if (!rtsp)
    {
        int retFlag;
        //auto pInfo = std::make_unique<SCRSDK::CrImageInfo>();

        //guint length;
        //gpointer user_data;

        while (stream.load())
        {
            sonyCamera.streamPush(retFlag); // pInfo, retFlag
            std::this_thread::sleep_for(std::chrono::milliseconds(33));
            // if(!stream.load()) break;
        }

        sonyCamera.streamDisconnect();
        //gst_element_set_state(pipeline, GST_STATE_NULL);
        //gst_object_unref(GST_OBJECT(pipeline));
    }
    else
    {
        while (stream.load())
        {
            //printf("[MAIN] [loop] GetDeviceHandle = %ld\n", sonyCamera.debug_streamGetHande());
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

        sonyCamera.streamLoopQuit();
        //g_main_loop_quit(main_loop);
    }
    if (gstreamLoopThread.joinable())
    {
        gstreamLoopThread.join();
    }
}

// Toggle flags based on user input, network signal, etc. Simulate reading flags (this should actually check some external condition)
void toggle_flags(int &count)
{
    count++;
    // if (count % 100 == 0)
    //     shoot.store(true);
    if (count == 7)
    {
        printf("> stream = true\n");
        stream.store(true);
    }
    // if (quit.load()) program.store(false); // Stop the program after some cycles
}

void checkForKeyPress() // bool rtsp
{
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    std::cout << "Press 'Q' to quit...\n";

    char ch;
    while (!quit.load())
    {
        if (read(STDIN_FILENO, &ch, 1) > 0 && (ch == 'Q' || ch == 'q'))
        {
            quit.store(true);
            program.store(false);
            stream.store(false);

            std::cout << "QUIT!!\n";
        }
        if (read(STDIN_FILENO, &ch, 1) > 0 && (ch == 'T' || ch == 't'))
        {
            shoot.store(true);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Poll every 100ms
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Restore terminal settings
}

