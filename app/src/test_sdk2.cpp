#include <iostream>
#include <memory>
#include "test_sdk.h"
#include <thread>
#include <unistd.h>

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <gst/app/gstappsrc.h>  // Include for appsrc functions
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <fstream>
#include <termios.h>

// New Functions
void InitSDK();
bool Enumerate();
bool ConnectCamera(SCRSDK::CrSdkControlMode mode);
bool setSavePath();
void GetCamProperties();
void SingleShot();
void SequentialShooting(int numOfPictures, int intervalPerPicture_sec);

// New Globals
CrInt8u *cam_id = 0;
SCRSDK::ICrEnumCameraObjectInfo *pEnum = nullptr;
SCRSDK::ICrCameraObjectInfo *pCam = nullptr;
SCRSDK::CrDeviceHandle hDev = 0;

// From GStream
void InitGstream(bool rtsp); //int argc, char *argv[]
void read_flags(int &count);
void checkForKeyPress();
void liveView(CrInt32u handle, bool rtsp);
void pushToGstream(GstElement *src, guint length, gpointer user_data, std::unique_ptr<SCRSDK::CrImageInfo> &pInfo, int &retFlag);
static void need_data(GstElement *src, guint length, gpointer user_data);
static void media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media, gpointer user_data);

static gboolean timeout(GstRTSPServer *server);
static GstElement *appsrc;
GstElement *pipeline, *source, *decoder, *sink;
GMainLoop* global_main_loop = nullptr;

bool appsrc_ready = false;

// Atomic flag to signal all threads to stop
std::atomic<bool> quit(false);
std::atomic<bool> program(true);
std::atomic<bool> shoot(false);
std::atomic<bool> stream(false);

std::mutex mtx;
std::mutex buffer_mutex; // Mutex for synchronizing access to the buffer
std::condition_variable cv;


int main(int argc, char *argv[])
{
    InitSDK();

    if (!Enumerate())
    {
        std::cerr << "Invalid camera ID provided." << std::endl;
        return 0;
    }

    SCRSDK::CrSdkControlMode mode = SCRSDK::CrSdkControlMode_Remote;
    if (!ConnectCamera(mode))
    {
        std::cerr << "Conenction failed." << std::endl;
        return 0;
    }

    if (!setSavePath())
    {
        std::cerr << "Failed Path Definition." << std::endl;
        return 0;
    }

    /*  -------------- Pag 51
        After the Connect() function, ICrCameraObjectInfo can be freed. There is no need to wait for
        OnConnected() or the OnError() callback function. It means you can delete the
        ICrEnumCameraObjectInfo object returned from the EnumCameraObjects() function
    */
    pEnum->Release(); // After this point, pobj or any data derived from it might be invalid!
    pCam->Release();  // Release the camera object info

    std::cout << "Connecting..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "Done!\n\n"
              << std::endl;

    // GetCamProperties();

    int count = 0;
    bool rtsp = true;
    int numOfPictures = 4;
    int intervalPerPicture_sec = 3;
    // SequentialShooting(numOfPictures, intervalPerPicture_sec);

    /*------------------ Noteworthy
    You need to ensure that the need_data function only tries to access appsrc after
    it has been initialized, which only happens after a client has connected to the RTSP server.
    */
    std::thread keyPressThread([&]()
                               { checkForKeyPress(); });

    std::thread gstream_thread([&]()
                               { liveView(hDev, rtsp); }); //argc, argv

    std::thread shooting_thread([&]()
                                { SequentialShooting(numOfPictures, intervalPerPicture_sec); });

    /*
    std::thread streaming_thread([&]() {
        need_data();
    });
    */

    while (program.load())
    {
        read_flags(count);
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
    SCRSDK::Disconnect(hDev);
    SCRSDK::Release(); // Clean up the camera info object

    std::cout << "Quit Successfully!" << std::endl;
    return 0;
}

void InitSDK()
{
    SCRSDK::Release();
    std::cout << "Sony" << std::endl;
    SCRSDK::Init();
}

bool Enumerate()
{
    std::cout << "Enumerate!" << std::endl;
    pEnum = nullptr;
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
    return true;
}

bool ConnectCamera(SCRSDK::CrSdkControlMode mode)
{
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

void SequentialShooting(int numOfPictures, int intervalPerPicture_sec)
{
    while (program.load())
    { // Keep thread alive as long as the program is running
        if (!shoot.load()){
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Sleep briefly before checking again
            continue;
        }
        for (size_t i = 0; i < numOfPictures; i++){
            SingleShot();
            printf(" > Shot %ld of %d \n", i + 1, numOfPictures);
            std::this_thread::sleep_for(std::chrono::seconds(intervalPerPicture_sec));
            if(!program.load()) break;
        }
        shoot.store(false);
    }
    printf("Quiting SequentialShooting\n");
}

void SingleShot()
{
    std::cout << "============== Triggering ==============" << std::endl;
    SCRSDK::SendCommand(hDev, SCRSDK::CrCommandId_Release, SCRSDK::CrCommandParam_Down);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    SCRSDK::SendCommand(hDev, SCRSDK::CrCommandId_Release, SCRSDK::CrCommandParam_Up);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

static void need_data(GstElement *src, guint length, gpointer user_data)
{
    // =================================
    //    THIS APPROACH WORKS SLOWLY:
    // =================================
    int retFlag;
    auto pInfo = std::make_unique<SCRSDK::CrImageInfo>();
    pushToGstream(src, length, user_data, pInfo, retFlag);
     
}

void media_configure_callback(GstRTSPMediaFactory *factory, GstRTSPMedia *media, gpointer user_data)
{
    GstElement *element = gst_rtsp_media_get_element(media);
    appsrc = gst_bin_get_by_name_recurse_up(GST_BIN(element), "mysrc");

    if (!appsrc)
    {
        std::cerr << "Failed to retrieve 'appsrc' element.\n";
        return;
    }

    g_signal_connect(appsrc, "need-data", G_CALLBACK(need_data), NULL);

    appsrc_ready = true;
    
    gst_object_unref(element); // Unref the element, not appsrc
}

void InitGstream(bool rtsp) {
    gst_init(nullptr, nullptr);

    if (rtsp){
        // Create the main pipeline
        //pipeline = gst_pipeline_new("rtsp-pipeline");

        global_main_loop = g_main_loop_new(nullptr, FALSE);
        GstRTSPServer* server = gst_rtsp_server_new();
        gst_rtsp_server_set_service(server, "8554");

        GstRTSPMountPoints* mount_points = gst_rtsp_server_get_mount_points(server);
        GstRTSPMediaFactory* factory = gst_rtsp_media_factory_new();
        
        // RTSP initialization remains mostly the same but correct the caps in the launch string:
        gst_rtsp_media_factory_set_launch(factory,
                "( appsrc name=mysrc is-live=true block=true format=TIME "
                "caps=image/jpeg,width=640,height=480 ! "
                "jpegparse ! rtpjpegpay name=pay0 pt=96 )");

        gst_rtsp_media_factory_set_shared(factory, TRUE);
        gst_rtsp_mount_points_add_factory(mount_points, "/test", factory);
        g_object_unref(mount_points);

        // Before, it was "media_configure", now is "media_configure_callback"
        g_signal_connect(factory, "media-configure", G_CALLBACK(media_configure_callback), NULL);
        gst_rtsp_server_attach(server, nullptr);

        g_timeout_add_seconds(2, (GSourceFunc)timeout, server);

        g_print("Stream ready at rtsp://127.0.0.1:8554/test\n");

    }
    else{
        pipeline = gst_pipeline_new("video-pipeline");
        source = gst_element_factory_make("appsrc", "jpeg-source");
        decoder = gst_element_factory_make("jpegdec", "jpeg-decoder");
        sink = gst_element_factory_make("autovideosink", "video-output");

        if (!pipeline || !source || !decoder || !sink) {
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

// Main function for fetching and displaying live view images
void liveView(CrInt32u handle, bool rtsp) {

    // Assume handle and SDK setup done here
    printf("liveView: In!\n");

    InitGstream(rtsp);
    //if (rtsp){
    std::thread gstreamLoopThread([&]()
                                { g_main_loop_run(global_main_loop); });
    //}
    
    printf("liveView: Init done!\n");
    
    while(!stream.load()){
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    printf("liveView: Started!\n");
    
    /* 
    > If the this is declared outside the while, the error 13105 arises
    > CrWarning_Frame_NotUpdated
    auto pLiveViewImage = std::make_unique<SCRSDK::CrImageDataBlock>();
    */ 
    
    if (!rtsp){
        int retFlag;
        auto pInfo = std::make_unique<SCRSDK::CrImageInfo>();
        GstElement *src;
        guint length;
        gpointer user_data;

        while (stream.load()) {
            pushToGstream(src, length,user_data,pInfo, retFlag); //pInfo, retFlag
            std::this_thread::sleep_for(std::chrono::milliseconds(33));
            //if(!stream.load()) break;
        }
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(pipeline));
    }
    else{
        while (stream.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        g_main_loop_quit(global_main_loop);
        
    }
    if (gstreamLoopThread.joinable())
    {
        gstreamLoopThread.join();
    }
    
}

void pushToGstream(GstElement *src, guint length, gpointer user_data, std::unique_ptr<SCRSDK::CrImageInfo> &pInfo, int &retFlag) //
{
    retFlag = 0;
    //std::unique_ptr<SCRSDK::CrImageInfo> pInfo(new SCRSDK::CrImageInfo());
    SCRSDK::CrError err1 = SCRSDK::GetLiveViewImageInfo(hDev, pInfo.get());
    if (err1 != SCRSDK::CrError_None)
    {
        printf("Error 1: %d \n", err1);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        {
            retFlag = 1;
            return;
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
            return;
        };
    }
    // ============================================================
    //  void pushFrameToGstreamer(SCRSDK::CrImageDataBlock* pLiveViewImage) {
    // ============================================================

    /* THIS COMES FROM THE FAST VERSION */
    if (!GST_IS_ELEMENT(src))
    {
        std::cerr << "Invalid source element." << std::endl;
        {
            retFlag = 3;
            return;
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
    g_signal_emit_by_name(src, "push-buffer", buffer, &ret);
    gst_buffer_unref(buffer);

    if (ret != GST_FLOW_OK)
    {
        std::cerr << "Error pushing buffer to GStreamer pipeline." << std::endl;
    }
}

void read_flags(int &count)
{
    // Simulate reading flags (this should actually check some external condition)
    // Toggle flags based on user input, network signal, etc.
    count++;
    //if (count % 100 == 0)
    //    shoot.store(true);
    if (count == 10)
    {
        printf("> stream = true\n");
        stream.store(true);
    }
    // if (quit.load()) program.store(false); // Stop the program after some cycles
}

void checkForKeyPress() //bool rtsp
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
        if (read(STDIN_FILENO, &ch, 1) > 0 && (ch == 'T' || ch == 't')){
            shoot.store(true);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Poll every 100ms
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Restore terminal settings
}


static gboolean timeout(GstRTSPServer *server)
{
    GstRTSPSessionPool *pool = gst_rtsp_server_get_session_pool(server);
    gst_rtsp_session_pool_cleanup(pool);
    g_object_unref(pool);
    return TRUE;
}